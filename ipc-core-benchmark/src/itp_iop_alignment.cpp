#include "ipc_core/itp_iop_alignment.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace ipc_core {

namespace {
inline uint32_t fast_fnv1a_str(const std::string &s) {
  uint32_t h = 2166136261u;
  for (unsigned char c : s) {
    h ^= c;
    h *= 16777619u;
  }
  return h;
}

inline uint64_t pack_quota_key(const DemandOrderRecord &d) {
  const uint64_t day_u = static_cast<uint64_t>(static_cast<uint32_t>(d.due_day) & 0xFFFFu);
  const uint64_t fam_u = static_cast<uint64_t>((d.part_id / 10u) & 0xFFFFFFu);
  const uint64_t cg_u =
      (d.cust_group_code != 0)
          ? static_cast<uint64_t>(d.cust_group_code & 0x3FFu)
          : static_cast<uint64_t>(fast_fnv1a_str(d.customer_group) & 0x3FFu);
  const uint64_t rg_u =
      (d.region_code != 0)
          ? static_cast<uint64_t>(d.region_code & 0x3Fu)
          : static_cast<uint64_t>(fast_fnv1a_str(d.region) & 0x3Fu);
  return (day_u << 40) | (fam_u << 16) | (cg_u << 6) | rg_u;
}

inline size_t mix_u64(uint64_t k) {
  k ^= k >> 33;
  k *= 0xff51afd7ed558ccdULL;
  k ^= k >> 33;
  k *= 0xc4ceb9fe1a85ec53ULL;
  k ^= k >> 33;
  return static_cast<size_t>(k);
}
} // namespace

void FlatQuotaFirewallTable::init_pow2(size_t expected_keys) {
  size_t cap = 64;
  while (cap < expected_keys * 2) {
    cap <<= 1;
  }
  slots.assign(cap, Slot{});
  active_buckets = 0;
  total_quota_sum = 0.0;
}

FlatQuotaFirewallTable::Slot &FlatQuotaFirewallTable::upsert(uint64_t key) {
  if (slots.empty()) init_pow2(64);
  const size_t mask = slots.size() - 1;
  size_t pos = mix_u64(key) & mask;
  while (true) {
    Slot &s = slots[pos];
    if (s.packed_key == key) return s;
    if (s.packed_key == UINT64_MAX) {
      s.packed_key = key;
      ++active_buckets;
      return s;
    }
    pos = (pos + 1) & mask;
  }
}

FlatQuotaFirewallTable::Slot *FlatQuotaFirewallTable::find(uint64_t key) {
  if (slots.empty()) return nullptr;
  const size_t mask = slots.size() - 1;
  size_t pos = mix_u64(key) & mask;
  while (true) {
    Slot &s = slots[pos];
    if (s.packed_key == key) return &s;
    if (s.packed_key == UINT64_MAX) return nullptr;
    pos = (pos + 1) & mask;
  }
}

FlatQuotaFirewallTable generate_itp_flat_allotments(
    const std::vector<DemandOrderRecord> &master_demands,
    double capacity_buffer_factor) {
  FlatQuotaFirewallTable table;
  table.init_pow2(std::max<size_t>(64, master_demands.size()));
  double sum_q = 0.0;
  for (const auto &d : master_demands) {
    const double q = d.qty * capacity_buffer_factor;
    table.upsert(pack_quota_key(d)).total_quota += q;
    sum_q += q;
  }
  table.total_quota_sum = sum_q;
  return table;
}

IOPExecutionResult run_iop_flat_alignment(
    const std::vector<DemandOrderRecord> &execution_demands,
    FlatQuotaFirewallTable &flat_firewalls) {
  IOPExecutionResult result;
  const size_t n = execution_demands.size();
  result.total_orders = static_cast<uint32_t>(n);
  if (n == 0) return result;

  // 零字符串拷贝分桶/索引排序：若优先级位于常见离散档位 [0..63] 则 O(N) 计数分桶，否则排轻量索引对
  uint64_t max_p = 0;
  for (const auto &d : execution_demands) {
    if (d.priority > max_p) max_p = d.priority;
  }

  std::vector<uint32_t> order_idx(n);
  if (max_p <= 63) {
    size_t counts[65] = {0};
    for (const auto &d : execution_demands) {
      counts[d.priority + 1] += 1;
    }
    for (size_t i = 1; i <= 64; ++i) counts[i] += counts[i - 1];
    for (uint32_t i = 0; i < static_cast<uint32_t>(n); ++i) {
      order_idx[counts[execution_demands[i].priority]++] = i;
    }
  } else {
    std::iota(order_idx.begin(), order_idx.end(), 0u);
    std::sort(order_idx.begin(), order_idx.end(), [&](uint32_t a, uint32_t b) {
      return execution_demands[a].priority < execution_demands[b].priority;
    });
  }

  double total_quota_allocated = flat_firewalls.total_quota_sum;
  if (total_quota_allocated <= 0.0) {
    for (const auto &s : flat_firewalls.slots) {
      if (s.packed_key != UINT64_MAX) total_quota_allocated += s.total_quota;
    }
  }

  double total_quota_consumed = 0.0;
  for (uint32_t idx : order_idx) {
    const auto &d = execution_demands[idx];
    auto *slot = flat_firewalls.find(pack_quota_key(d));
    if (slot != nullptr && (slot->total_quota - slot->consumed_qty) >= d.qty) {
      slot->consumed_qty += d.qty;
      total_quota_consumed += d.qty;
      result.scheduled_orders++;
      result.total_fulfilled_qty += d.qty;
    } else {
      result.blocked_orders++;
    }
  }

  result.quota_utilization =
      total_quota_allocated > 0.0 ? (total_quota_consumed / total_quota_allocated) : 0.0;
  return result;
}

std::unordered_map<QuotaFirewallKey, QuotaFirewallState, QuotaFirewallKeyHash>
generate_itp_master_allotments(
    const std::vector<DemandOrderRecord> &master_demands,
    const std::vector<SkuNodeRecord> & /*parts*/,
    double capacity_buffer_factor) {
  std::unordered_map<QuotaFirewallKey, QuotaFirewallState, QuotaFirewallKeyHash>
      firewalls;
  firewalls.reserve(master_demands.size());

  for (const auto &demand : master_demands) {
    QuotaFirewallKey key;
    key.day = demand.due_day;
    key.family_id = demand.part_id / 10;
    key.cust_group_id = fast_fnv1a_str(demand.customer_group) % 100u;
    key.region_id = fast_fnv1a_str(demand.region) % 10u;

    firewalls[key].total_quota += demand.qty * capacity_buffer_factor;
  }

  return firewalls;
}

IOPExecutionResult run_iop_execution_alignment(
    const std::vector<DemandOrderRecord> &execution_demands,
    const std::vector<SkuNodeRecord> & /*parts*/,
    std::unordered_map<QuotaFirewallKey, QuotaFirewallState,
                       QuotaFirewallKeyHash> &quota_firewalls) {
  IOPExecutionResult result;
  const size_t n = execution_demands.size();
  result.total_orders = static_cast<uint32_t>(n);

  // 对轻量索引排序，避免 50 万条记录下的 std::string 堆拷贝
  std::vector<uint32_t> order_idx(n);
  std::iota(order_idx.begin(), order_idx.end(), 0u);
  std::sort(order_idx.begin(), order_idx.end(), [&](uint32_t a, uint32_t b) {
    return execution_demands[a].priority < execution_demands[b].priority;
  });

  double total_quota_allocated = 0.0;
  double total_quota_consumed = 0.0;
  for (const auto &kv : quota_firewalls) {
    total_quota_allocated += kv.second.total_quota;
  }

  for (uint32_t idx : order_idx) {
    const auto &demand = execution_demands[idx];
    QuotaFirewallKey key;
    key.day = demand.due_day;
    key.family_id = demand.part_id / 10;
    key.cust_group_id = fast_fnv1a_str(demand.customer_group) % 100u;
    key.region_id = fast_fnv1a_str(demand.region) % 10u;

    auto it = quota_firewalls.find(key);
    if (it != quota_firewalls.end()) {
      double remaining_quota = it->second.total_quota - it->second.consumed_qty;
      if (remaining_quota >= demand.qty) {
        it->second.consumed_qty += demand.qty;
        result.scheduled_orders++;
        result.total_fulfilled_qty += demand.qty;
      } else {
        result.blocked_orders++;
      }
    } else {
      result.blocked_orders++;
    }
  }

  for (const auto &kv : quota_firewalls) {
    total_quota_consumed += kv.second.consumed_qty;
  }

  result.quota_utilization =
      total_quota_allocated > 0.0
          ? (total_quota_consumed / total_quota_allocated)
          : 0.0;
  return result;
}

MultiRegionAllotmentResult evaluate_multi_region_most_gating_allotment(
    const std::vector<RegionalDemandStream> &streams,
    const std::vector<SharedComponentPool> &shared_pools) {
  MultiRegionAllotmentResult res;
  const size_t R = streams.size();
  res.region_outcomes.resize(R);
  if (R == 0) return res;

  // Step 1: 计算各区域自身专用受限件的齐套木桶包络 K_r
  for (size_t r = 0; r < R; ++r) {
    auto &out = res.region_outcomes[r];
    out.region_code = streams[r].region_code;
    double kit = std::max(0.0, streams[r].forecast_qty);
    for (double cap : streams[r].dedicated_caps) {
      kit = std::min(kit, std::max(0.0, cap));
    }
    out.kitted_ceiling = kit;
    res.total_kitted_demand += kit;
    res.legacy_total_non_bottleneck_buy +=
        std::max(0.0, streams[r].forecast_qty) * std::max(0.0, streams[r].non_bottleneck_usage);
  }

  // Step 2A: 扫描全部跨区共用件池，自动识别最紧瓶颈 (Most-Gating Bottleneck)
  double min_ratio = 1e18;
  int gating_idx = -1;
  double effective_shared_parent_cap = res.total_kitted_demand;

  for (size_t m = 0; m < shared_pools.size(); ++m) {
    const double u = std::max(1e-9, shared_pools[m].unit_usage);
    const double parent_equiv_sup = std::max(0.0, shared_pools[m].available_supply) / u;
    const double ratio = (res.total_kitted_demand > 1e-9)
                             ? (parent_equiv_sup / res.total_kitted_demand)
                             : 1.0;
    if (ratio < min_ratio) {
      min_ratio = ratio;
      gating_idx = static_cast<int>(m);
      effective_shared_parent_cap = parent_equiv_sup;
    }
  }

  if (gating_idx >= 0) {
    res.most_gating_part_code = shared_pools[gating_idx].part_code;
    res.most_gating_scarcity_ratio = min_ratio;
    res.has_bottleneck = (res.total_kitted_demand > effective_shared_parent_cap + 1e-9);
  }

  // 按战略等级排序索引
  std::vector<size_t> rank_order(R);
  std::iota(rank_order.begin(), rank_order.end(), 0);
  std::stable_sort(rank_order.begin(), rank_order.end(), [&](size_t a, size_t b) {
    return streams[a].strategic_rank < streams[b].strategic_rank;
  });

  // Step 2B: 战略保底优先锁定 + 触顶循环注水公平切分 (Iterative Water-Filling Fair-Share)
  if (!res.has_bottleneck) {
    for (size_t r = 0; r < R; ++r) {
      auto &out = res.region_outcomes[r];
      out.granted_strategic_floor =
          std::min(std::max(0.0, streams[r].strategic_floor_qty), out.kitted_ceiling);
      out.granted_fair_share = out.kitted_ceiling - out.granted_strategic_floor;
      out.final_allotment = out.kitted_ceiling;
    }
  } else {
    double rem_sup = effective_shared_parent_cap;
    for (size_t idx : rank_order) {
      auto &out = res.region_outcomes[idx];
      const double req_floor =
          std::min(std::max(0.0, streams[idx].strategic_floor_qty), out.kitted_ceiling);
      const double grant = std::min(req_floor, rem_sup);
      out.granted_strategic_floor = grant;
      out.final_allotment = grant;
      rem_sup = std::max(0.0, rem_sup - grant);
    }

    // 对剩余供给执行循环注水按比例公平切分（任一区域触及自身齐套上限后自动将溢出份额二次重分）
    std::vector<double> flex_weight(R, 0.0);
    std::vector<bool> active(R, false);
    for (size_t r = 0; r < R; ++r) {
      double cap_room = res.region_outcomes[r].kitted_ceiling - res.region_outcomes[r].final_allotment;
      if (cap_room > 1e-9) {
        flex_weight[r] = cap_room;
        active[r] = true;
      }
    }

    while (rem_sup > 1e-9) {
      double w_sum = 0.0;
      for (size_t r = 0; r < R; ++r) {
        if (active[r]) w_sum += flex_weight[r];
      }
      if (w_sum <= 1e-9) break;

      bool any_capped = false;
      double distributed = 0.0;
      for (size_t r = 0; r < R; ++r) {
        if (!active[r]) continue;
        auto &out = res.region_outcomes[r];
        const double room = out.kitted_ceiling - out.final_allotment;
        const double share = rem_sup * (flex_weight[r] / w_sum);
        if (share >= room - 1e-9) {
          out.granted_fair_share += room;
          out.final_allotment = out.kitted_ceiling;
          distributed += room;
          active[r] = false;
          any_capped = true;
        }
      }
      if (any_capped) {
        rem_sup = std::max(0.0, rem_sup - distributed);
        continue;
      }
      for (size_t r = 0; r < R; ++r) {
        if (!active[r]) continue;
        auto &out = res.region_outcomes[r];
        const double share = rem_sup * (flex_weight[r] / w_sum);
        out.granted_fair_share += share;
        out.final_allotment += share;
      }
      rem_sup = 0.0;
    }
  }

  // Step 3 (逆向重展非瓶颈外购件) + Step 4 (独立区域配额桶执行隔离 vs 无桶共池被抢对比)
  double leg_rem_shared = res.has_bottleneck ? effective_shared_parent_cap : 1e18;
  for (size_t idx : rank_order) {
    const auto &st = streams[idx];
    auto &out = res.region_outcomes[idx];
    res.total_allotment += out.final_allotment;

    out.renetted_non_bottleneck_buy =
        out.final_allotment * std::max(0.0, st.non_bottleneck_usage);
    res.ipc_total_renetted_buy += out.renetted_non_bottleneck_buy;

    const double arr = std::max(0.0, st.exec_arriving_qty);
    out.ipc_fulfilled_qty = std::min(arr, out.final_allotment);
    out.ipc_blocked_qty = std::max(0.0, arr - out.ipc_fulfilled_qty);

    double ded_lim = arr;
    for (double cap : st.dedicated_caps) ded_lim = std::min(ded_lim, std::max(0.0, cap));
    const double leg_take = std::min(ded_lim, leg_rem_shared);
    leg_rem_shared = std::max(0.0, leg_rem_shared - leg_take);
    out.legacy_unprotected_fulfilled_qty = leg_take;
    out.legacy_starved_shortage_qty = std::max(0.0, arr - leg_take);
  }

  res.total_excess_buy_eliminated =
      std::max(0.0, res.legacy_total_non_bottleneck_buy - res.ipc_total_renetted_buy);
  return res;
}

ClosedLoopAllotmentResult evaluate_itp_kitting_most_gating_closed_loop(
    const RegionalAllotmentInput &in) {
  std::vector<RegionalDemandStream> streams = {
      {"REG_A", 1, in.forecast_a, in.strategic_floor_a, {in.dedicated_supply_a}, 1.0, in.exec_order_a},
      {"REG_D", 2, in.forecast_d, in.strategic_floor_d, {in.dedicated_supply_d}, 1.0, in.exec_order_d},
  };
  std::vector<SharedComponentPool> pools = {
      {"CHIP_SHARED", in.shared_chip_supply, 1.0},
  };

  MultiRegionAllotmentResult mr = evaluate_multi_region_most_gating_allotment(streams, pools);
  ClosedLoopAllotmentResult out;
  out.kitted_a = mr.region_outcomes[0].kitted_ceiling;
  out.kitted_d = mr.region_outcomes[1].kitted_ceiling;
  out.is_most_gating = mr.has_bottleneck;
  out.allotment_a = mr.region_outcomes[0].final_allotment;
  out.allotment_d = mr.region_outcomes[1].final_allotment;
  out.legacy_non_bottleneck_buy = mr.legacy_total_non_bottleneck_buy;
  out.ipc_renetted_buy = mr.ipc_total_renetted_buy;
  out.excess_buy_eliminated = mr.total_excess_buy_eliminated;
  out.legacy_fulfilled_d = mr.region_outcomes[1].legacy_unprotected_fulfilled_qty;
  out.legacy_shortage_d = mr.region_outcomes[1].legacy_starved_shortage_qty;
  out.ipc_fulfilled_a = mr.region_outcomes[0].ipc_fulfilled_qty;
  out.ipc_blocked_a = mr.region_outcomes[0].ipc_blocked_qty;
  out.ipc_fulfilled_d = mr.region_outcomes[1].ipc_fulfilled_qty;
  out.ipc_shortage_d = std::max(0.0, in.exec_order_d - out.ipc_fulfilled_d);
  return out;
}

} // namespace ipc_core
