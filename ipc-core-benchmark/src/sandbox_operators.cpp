#include "ipc_core/sandbox_operators.h"
#include <algorithm>
#include <cmath>

namespace ipc_core {

// =========================================================================
// 1. 排序耦合定律最小完备反例 (2 Demands x 3 Materials x 4 Periods)
// =========================================================================
SequencingCouplingResult
evaluate_sequencing_coupling_2x3(const SequencingCouplingInput &in) {
  SequencingCouplingResult r;

  // 传统静态优先级排他锁库：D1 (高优) 先锁走最早通用件 M2@oh_day，却必须干等到
  // M1 到货
  int trad_d1_m2 = std::min(in.m2_oh_day, in.m2_po_day);
  int trad_d2_m2 = std::max(in.m2_oh_day, in.m2_po_day);
  r.trad_d1_finish_day = std::max(in.m1_avail_day, trad_d1_m2);
  r.trad_d2_finish_day = std::max(in.m3_avail_day, trad_d2_m2);
  r.trad_d2_delay_days = std::max(0, r.trad_d2_finish_day - in.d2_req_day);
  r.trad_m2_idle_lock_days = std::max(0, r.trad_d1_finish_day - trad_d1_m2);

  // IPC 两阶段正交解耦：
  // Stage 1: 计算各单物理齐套下界 T_min(D1)=max(m1, m2_oh), T_min(D2)=max(m3,
  // m2_oh) Stage 2: 按物理齐套下界/晚绑定分配，不损害 D1
  // 交期的前提下优先把早期现货给早齐套单
  int d1_earliest = std::max(in.m1_avail_day, trad_d1_m2);
  int d2_earliest = std::max(in.m3_avail_day, trad_d1_m2);

  if (d2_earliest < d1_earliest && trad_d2_m2 <= d1_earliest) {
    // D2 绑定早期现货 M2@oh_day，D1 晚绑定在途 M2@po_day（D1 完工日完全不受损）
    r.ipc_d2_finish_day = std::max(in.m3_avail_day, trad_d1_m2);
    r.ipc_d1_finish_day = std::max(in.m1_avail_day, trad_d2_m2);
    r.ipc_m2_idle_lock_days = std::max(0, r.ipc_d2_finish_day - trad_d1_m2);
  } else {
    r.ipc_d1_finish_day = r.trad_d1_finish_day;
    r.ipc_d2_finish_day = r.trad_d2_finish_day;
    r.ipc_m2_idle_lock_days = r.trad_m2_idle_lock_days;
  }
  r.ipc_d2_delay_days = std::max(0, r.ipc_d2_finish_day - in.d2_req_day);
  return r;
}

// =========================================================================
// 2. 成套组替代沉没长板激活模型 (11,000 vs 1,000)
// =========================================================================
GroupSubResult evaluate_group_substitution_sunk_cost(const GroupSubInput &in) {
  GroupSubResult r;
  // 传统按静态单件齐套跳过短板组 G1，直接采购全新替代组 G2，并将 G1
  // 长板闲置报废
  r.trad_new_cash_out = in.g2_full_new_cost;
  r.trad_scrap_loss = in.g1_main_sunk_value;
  r.trad_total_economic_cost = r.trad_new_cash_out + r.trad_scrap_loss;

  // IPC 增量新购现金流裁决（CFO 严格账面守恒口径）：
  // 正常消耗在库长板进入 COGS（非报废，但需计入总账面成本以防重复扣除沉没存货）
  if (in.g1_minor_expedite_cost < in.g2_full_new_cost) {
    r.ipc_new_cash_out = in.g1_minor_expedite_cost;
    r.ipc_scrap_loss = 0.0;
    r.ipc_total_economic_cost = in.g1_minor_expedite_cost + in.g1_main_sunk_value;
    r.decision_code = "ACTIVATE_SUNK_G1_BY_EXPEDITING_MINOR_PART";
  } else {
    r.ipc_new_cash_out = in.g2_full_new_cost;
    r.ipc_scrap_loss = in.g1_main_sunk_value;
    r.ipc_total_economic_cost = r.ipc_new_cash_out + r.ipc_scrap_loss;
    r.decision_code = "SWITCH_TO_G2_NEW_SET";
  }
  r.net_economic_savings =
      r.trad_total_economic_cost - r.ipc_total_economic_cost;
  return r;
}

// =========================================================================
// 2B. 不完全替代（单向属性门槛 + 主料锚定比例红线）
// =========================================================================
IncompleteSubResult
evaluate_incomplete_substitution_rule(const IncompleteSubInput &in) {
  IncompleteSubResult r;
  if (in.dim_op == IncompleteSubOp::GE) {
    r.dim_passed = (in.candidate_dim + 1e-9 >= in.required_dim);
  } else {
    r.dim_passed = (std::abs(in.candidate_dim - in.required_dim) <= 1e-9);
  }

  double theta = std::max(0.0, std::min(1.0, in.max_sub_ratio));
  double gap = std::max(0.0, in.gross_demand - in.main_kitted_qty);

  // 现有 SCP / APS：常见粗糙做法按总需求毛量乘比例上限，忽略主料实际缺料时的分母坍缩
  if (!r.dim_passed || in.gross_demand <= 0.0 || in.sub_avail_qty <= 0.0 ||
      theta <= 0.0) {
    r.legacy_sub_used = 0.0;
  } else {
    double cap_gross = std::floor(theta * in.gross_demand + 1e-9);
    r.legacy_sub_used = std::min({gap, in.sub_avail_qty, cap_gross});
  }
  r.legacy_total_output = in.main_kitted_qty + r.legacy_sub_used;
  r.legacy_actual_ratio = r.legacy_total_output > 0.0
                              ? (r.legacy_sub_used / r.legacy_total_output)
                              : 0.0;
  r.legacy_breached_cap = (r.legacy_actual_ratio > theta + 1e-9);

  // IPC 确定性主料锚定截断：Q_sub / (Q_main + Q_sub) <= theta  <=>  Q_sub <= (theta/(1-theta)) * Q_main
  if (!r.dim_passed) {
    r.ipc_sub_used = 0.0;
    r.decision_code = "HARD_BLOCKED_BY_DIMENSION_GATE";
  } else if (in.gross_demand <= 0.0 || in.sub_avail_qty <= 0.0 ||
             theta <= 0.0) {
    r.ipc_sub_used = 0.0;
    r.decision_code = "ZERO_ELIGIBLE_SUB";
  } else {
    double cap_gross = std::floor(theta * in.gross_demand + 1e-9);
    double cap_main_anchor = 1e18;
    if (theta < 1.0 - 1e-9) {
      cap_main_anchor =
          std::floor((theta / (1.0 - theta)) * in.main_kitted_qty + 1e-9);
    }
    r.ipc_sub_used =
        std::min({gap, in.sub_avail_qty, cap_gross, cap_main_anchor});
    r.decision_code = (r.legacy_breached_cap)
                          ? "ANCHORED_BY_MAIN_KITTED_PREVENTED_BREACH"
                          : "WITHIN_MIX_RATIO_BOUND";
  }
  r.ipc_total_output = in.main_kitted_qty + r.ipc_sub_used;
  r.ipc_actual_ratio = r.ipc_total_output > 0.0
                           ? (r.ipc_sub_used / r.ipc_total_output)
                           : 0.0;
  r.ipc_within_cap = (r.ipc_actual_ratio <= theta + 1e-9);
  return r;
}

// =========================================================================
// 3. 推拉解耦点 (CODP) 两层非补偿定位与二阶 Φ 影子收益模型
// =========================================================================
CodpPositioningResult
evaluate_codp_two_layer_positioning(const CodpPositioningInput &in) {
  CodpPositioningResult r;

  // 门控一（时间因果硬界）：L_down(v) <= D_tol
  int l_down_chip = in.bare_mfg_lt + in.pack_lt;
  int l_down_bare = in.pack_lt;
  r.chip_passes_gate1 = (l_down_chip <= in.customer_dtol);
  r.bare_passes_gate1 = (l_down_bare <= in.customer_dtol);

  // 门控二（物理可存与未固化不可逆差异化）
  r.bare_passes_gate2 = r.bare_passes_gate1 && (!in.legacy_early_engraving);

  // 门控三（同交付 SLA 下，升级版 GSM 净补货期安全库存对比）
  double tau_fg_plan_a = static_cast<double>(
      std::max(0, in.bare_mfg_lt + in.pack_lt - in.committed_sla));
  double single_fg_ss =
      in.z_score * in.country_daily_sigma * std::sqrt(tau_fg_plan_a);
  r.plan_a_fg_ss_units = in.num_countries * single_fg_ss;
  r.plan_a_fg_capital = r.plan_a_fg_ss_units * in.fg_unit_cost;

  double tau_bare_plan_b = static_cast<double>(std::max(0, in.bare_mfg_lt));
  double pooled_sigma =
      std::sqrt(static_cast<double>(in.num_countries)) * in.country_daily_sigma;
  r.plan_b_bare_ss_units =
      in.z_score * pooled_sigma * std::sqrt(tau_bare_plan_b);
  r.plan_b_bare_capital = r.plan_b_bare_ss_units * in.bare_unit_cost;

  r.unit_reduction_pct = r.plan_a_fg_ss_units > 0.0
                             ? (r.plan_a_fg_ss_units - r.plan_b_bare_ss_units) /
                                   r.plan_a_fg_ss_units * 100.0
                             : 0.0;
  r.capital_reduction_pct =
      r.plan_a_fg_capital > 0.0
          ? (r.plan_a_fg_capital - r.plan_b_bare_capital) /
                r.plan_a_fg_capital * 100.0
          : 0.0;

  // 二阶修宪 Φ：移点影子收益
  r.phi_shadow_value_capital =
      std::max(0.0, r.plan_a_fg_capital - r.plan_b_bare_capital);

  if (r.bare_passes_gate2 && r.plan_b_bare_capital < r.plan_a_fg_capital) {
    r.chosen_codp_node = "SEMI_BARE (Sandglass Waist · codp_decoupling_flag=1)";
  } else {
    r.chosen_codp_node =
        "FG_COUNTRY_POOL (MTS Fallback · Trigger Phi Amendment #122)";
  }
  return r;
}

// =========================================================================
// 4. 零预测冲减：推式防独占日切割 + 拉式超额 Mode A/B/C 三态路由
// =========================================================================
PushSliceResult
evaluate_push_anti_monopoly_slice(double w1_demand_a, double w1_demand_b,
                                  double w1_supply_cap,
                                  const std::vector<double> &daily_caps) {
  PushSliceResult r;
  double total_dem = w1_demand_a + w1_demand_b;
  double share_a = total_dem > 0.0 ? (w1_demand_a / total_dem) : 0.5;
  double share_b = 1.0 - share_a;

  double w1_effective = std::min(total_dem, w1_supply_cap);
  r.w1_alloc_a = w1_effective * share_a;
  r.w1_alloc_b = w1_effective * share_b;
  r.w1_rollover_a = std::max(0.0, w1_demand_a - r.w1_alloc_a);
  r.w1_rollover_b = std::max(0.0, w1_demand_b - r.w1_alloc_b);

  double total_daily_cap = 0.0;
  for (double c : daily_caps)
    total_daily_cap += c;

  for (double c : daily_caps) {
    double weight = total_daily_cap > 0.0
                        ? (c / total_daily_cap)
                        : (1.0 / std::max<size_t>(1, daily_caps.size()));
    r.daily_a.push_back(r.w1_alloc_a * weight);
    r.daily_b.push_back(r.w1_alloc_b * weight);
  }
  return r;
}

PullRouterResult evaluate_pull_overflow_router(
    double w1_actual_order, double w1_alloc_bucket, double monthly_cum_orders,
    double monthly_envelope_cap, int days_to_due, int procure_lt_days) {
  PullRouterResult r;
  r.overflow_qty = std::max(0.0, w1_actual_order - w1_alloc_bucket);

  if (r.overflow_qty <= 1e-9) {
    r.mode = PullOverflowMode::IN_BUCKET_SILENT;
    r.mode_name = "IN_BUCKET_SILENT_CONSUME";
    r.w2_quota_deducted = 0.0;
    r.incremental_mrp_qty = 0.0;
    return r;
  }

  // 规则 1：全月累计实单仍在月度包络线内 -> 节奏前移 (Mode
  // A)，吃下周配额，零新增 MRP
  if (monthly_cum_orders <= monthly_envelope_cap + 1e-9) {
    r.mode = PullOverflowMode::MODE_A_ADVANCE_PULL_ZERO_MRP;
    r.mode_name = "MODE_A_CONSUME_NEXT_BUCKET_ONLY";
    r.w2_quota_deducted = r.overflow_qty;
    r.incremental_mrp_qty = 0.0;
    return r;
  }

  // 规则 2：突破月度总包络线 -> 真需求增量，对比剩余交期与采购提前期
  if (days_to_due >= procure_lt_days) {
    r.mode = PullOverflowMode::MODE_B_TRUE_INCREMENT_DIRECT_MRP;
    r.mode_name = "MODE_B_TRIGGER_MRP_DIRECT";
    r.w2_quota_deducted = 0.0;
    r.incremental_mrp_qty = r.overflow_qty;
  } else {
    r.mode = PullOverflowMode::MODE_C_URGENT_BORROW_AND_BACKFILL;
    r.mode_name = "MODE_C_BORROW_W2_AND_MRP_BACKFILL";
    r.w2_quota_deducted = r.overflow_qty;
    r.incremental_mrp_qty = r.overflow_qty;
  }
  return r;
}

// =========================================================================
// 5. 跨解耦点两阶段推拉排产 (Two-Stage Constrained Push-IOP + Pull-IOP)
// =========================================================================
TwoStageIopResult evaluate_two_stage_push_pull_iop(const TwoStageIopInput &in) {
  TwoStageIopResult r;
  int H = std::max(1, in.horizon_days);
  r.daily_bare_built.assign(H, 0.0);

  // Pass 1 (Push-IOP): 预测驱动在 CODP 前受日有限产能 + 上游关键芯片齐套双重约束预投通用裸机
  double rem_forecast = std::max(0.0, in.forecast_bare_qty);
  double avail_chip = std::max(0.0, in.chip_oh_day1);
  for (int d = 1; d <= H; ++d) {
    if (d == in.chip_po_day) {
      avail_chip += std::max(0.0, in.chip_po_qty);
    }
    if (rem_forecast > 1e-9 && avail_chip <= 1e-9 && in.daily_wc_cap > 0.0) {
      r.idle_wait_chip_days += 1;
    }
    double build = std::min({rem_forecast, std::max(0.0, in.daily_wc_cap), avail_chip});
    r.daily_bare_built[d - 1] = build;
    avail_chip -= build;
    rem_forecast -= build;
    r.total_bare_built += build;
  }

  // 构建通用裸机池按日可用桶副本供 Pass 2 扣减
  std::vector<double> pool_rem = r.daily_bare_built;
  double bare_cum_for_d4 = 0.0;
  int latest_bare_day_for_d4 = 4 - in.post_pack_lt_days;
  for (int d = 1; d <= std::min(H, latest_bare_day_for_d4); ++d) {
    bare_cum_for_d4 += r.daily_bare_built[d - 1];
  }

  // Pass 2 (Pull-IOP): 按优先级排序，先过配额防波堤，再按真实前道裸机池水位承诺交期
  std::vector<PullOrderSlice> sorted_orders = in.pull_orders;
  std::stable_sort(sorted_orders.begin(), sorted_orders.end(),
                   [](const PullOrderSlice &a, const PullOrderSlice &b) {
                     return a.priority < b.priority;
                   });

  double legacy_cum_d4 = 0.0;
  for (const auto &ord : sorted_orders) {
    PullOrderPromise p;
    p.order_id = ord.order_id;
    p.req_qty = ord.req_qty;
    p.admitted_qty = std::min(ord.req_qty, std::max(0.0, ord.allotment_cap));
    p.quota_blocked_qty = std::max(0.0, ord.req_qty - p.admitted_qty);
    r.total_quota_blocked_qty += p.quota_blocked_qty;

    // 传统 MRP/APS 仅看后道包装 1 天静态提前期：盲目承诺在 req_due_day 交货
    p.legacy_promised_day = std::max(1 + in.post_pack_lt_days, ord.req_due_day);
    if (p.legacy_promised_day <= 4) {
      double prev_cum = legacy_cum_d4;
      legacy_cum_d4 += ord.req_qty;
      double avail_before = std::max(0.0, bare_cum_for_d4 - prev_cum);
      p.legacy_shortfall_on_due = std::max(0.0, ord.req_qty - avail_before);
    }

    // IPC 动态齐套相位门控：从真实通用裸机池按时间桶扣减，取最后一颗齐套日 + L_post
    double need = p.admitted_qty;
    int last_bare_day = 1;
    for (int d = 1; d <= H && need > 1e-9; ++d) {
      if (pool_rem[d - 1] > 1e-9) {
        double take = std::min(need, pool_rem[d - 1]);
        pool_rem[d - 1] -= take;
        need -= take;
        last_bare_day = d;
      }
    }
    if (need > 1e-9) {
      last_bare_day = H + 1;
    }
    p.ipc_bare_ready_day = last_bare_day;
    p.ipc_promised_day = std::max(ord.req_due_day, last_bare_day + in.post_pack_lt_days);
    r.order_promises.push_back(p);
  }

  r.legacy_promised_on_d4 = legacy_cum_d4;
  r.legacy_broken_promise_qty = std::max(0.0, legacy_cum_d4 - bare_cum_for_d4);
  r.ipc_broken_promise_qty = 0.0;
  return r;
}

// =========================================================================
// 6. 价值链网络先序区间代数 + 维度规划与国别认证掩码 + 菱形 DAG 正逆向展开
// =========================================================================
namespace {
bool check_dim_op(NetworkDimOp op, double cand, double req) {
  switch (op) {
  case NetworkDimOp::PASS:
    return true;
  case NetworkDimOp::EQ:
    return std::abs(cand - req) <= 1e-9;
  case NetworkDimOp::GE:
    return (cand + 1e-9) >= req;
  case NetworkDimOp::LE:
    return (cand - 1e-9) <= req;
  }
  return true;
}
} // namespace

NetworkExplosionResult explode_value_chain_network(
    const std::vector<NetworkNodeDef> &nodes,
    const std::vector<NetworkEdgeDef> &edges, int root_node_id,
    double root_demand_qty, double required_spec,
    unsigned int required_cert_mask, NetworkDimOp override_leaf_op) {
  NetworkExplosionResult res;
  int max_id = root_node_id;
  for (const auto &n : nodes)
    max_id = std::max(max_id, n.node_id);
  for (const auto &e : edges)
    max_id = std::max({max_id, e.parent_id, e.child_id});

  std::vector<NetworkNodeDef> node_lut(max_id + 1);
  for (const auto &n : nodes) {
    if (n.node_id >= 0 && n.node_id <= max_id)
      node_lut[n.node_id] = n;
  }

  std::vector<std::vector<NetworkEdgeDef>> adj(max_id + 1);
  for (const auto &e : edges) {
    if (e.parent_id >= 0 && e.parent_id <= max_id)
      adj[e.parent_id].push_back(e);
  }

  // 1) 构建生成树先序区间索引 [rank .. rank + tree_size - 1]
  std::vector<bool> tree_vis(max_id + 1, false);
  int cur_rank = 0;
  auto dfs_span = [&](auto &self, int u, int lvl) -> int {
    tree_vis[u] = true;
    int my_idx = static_cast<int>(res.spans.size());
    int my_rank = ++cur_rank;
    res.spans.push_back({u, my_rank, 1, lvl});
    int sz = 1;
    for (const auto &ed : adj[u]) {
      if (!tree_vis[ed.child_id]) {
        sz += self(self, ed.child_id, lvl + 1);
      }
    }
    res.spans[my_idx].tree_size = sz;
    return sz;
  };
  if (root_node_id >= 0 && root_node_id <= max_id) {
    dfs_span(dfs_span, root_node_id, 0);
  }

  // 2) 带维度规划 (EQ/GE/LE) 与 64/32-bit 国别认证位掩码的正向展开
  std::vector<double> distinct_flux(max_id + 1, 0.0);
  std::vector<int> distinct_min_lvl(max_id + 1, 999999);
  std::vector<int> distinct_order;

  auto dfs_explode = [&](auto &self, int u, int lvl, double qty) -> void {
    res.path_instances.push_back({u, node_lut[u].code, lvl, qty});
    if (distinct_flux[u] <= 0.0 && distinct_min_lvl[u] == 999999) {
      distinct_order.push_back(u);
    }
    distinct_flux[u] += qty;
    distinct_min_lvl[u] = std::min(distinct_min_lvl[u], lvl);

    for (const auto &ed : adj[u]) {
      const auto &child = node_lut[ed.child_id];
      NetworkDimOp eff_op = ed.dim_op;
      if (eff_op != NetworkDimOp::PASS && override_leaf_op != NetworkDimOp::PASS) {
        eff_op = override_leaf_op;
      }
      bool spec_ok = (eff_op == NetworkDimOp::PASS) ||
                     check_dim_op(eff_op, child.spec_val, required_spec);
      bool cert_ok = ((child.cert_mask & required_cert_mask) == required_cert_mask);
      if (!spec_ok || !cert_ok) {
        res.pruned_by_dim_or_cert += 1;
        continue;
      }
      self(self, ed.child_id, lvl + 1, qty * ed.usage_qty);
    }
  };

  if (root_node_id >= 0 && root_node_id <= max_id) {
    dfs_explode(dfs_explode, root_node_id, 0, root_demand_qty);
  }

  for (int u : distinct_order) {
    res.distinct_nodes.push_back(
        {u, node_lut[u].code, distinct_min_lvl[u], distinct_flux[u]});
  }
  return res;
}

std::vector<int> trace_network_where_used(
    const std::vector<NetworkEdgeDef> &edges, int target_child_id) {
  int max_id = target_child_id;
  for (const auto &e : edges)
    max_id = std::max({max_id, e.parent_id, e.child_id});
  if (max_id < 0)
    return {};

  std::vector<std::vector<int>> rev_adj(max_id + 1);
  std::vector<bool> has_out_parent(max_id + 1, false);
  for (const auto &e : edges) {
    rev_adj[e.child_id].push_back(e.parent_id);
    has_out_parent[e.child_id] = true;
  }

  std::vector<bool> vis(max_id + 1, false);
  std::vector<int> q = {target_child_id};
  vis[target_child_id] = true;
  std::vector<int> roots;
  for (size_t head = 0; head < q.size(); ++head) {
    int cur = q[head];
    if (cur != target_child_id && rev_adj[cur].empty()) {
      roots.push_back(cur);
    }
    for (int p : rev_adj[cur]) {
      if (!vis[p]) {
        vis[p] = true;
        q.push_back(p);
      }
    }
  }
  std::sort(roots.begin(), roots.end());
  return roots;
}

} // namespace ipc_core
