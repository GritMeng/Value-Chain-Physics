#include "ipc_core/itp_iop_alignment.h"
#include <algorithm>

namespace ipc_core {

std::unordered_map<QuotaFirewallKey, QuotaFirewallState, QuotaFirewallKeyHash>
generate_itp_master_allotments(
    const std::vector<DemandOrderRecord> &master_demands,
    const std::vector<SkuNodeRecord> & /*parts*/,
    double capacity_buffer_factor) {
  std::unordered_map<QuotaFirewallKey, QuotaFirewallState, QuotaFirewallKeyHash>
      firewalls;

  for (const auto &demand : master_demands) {
    QuotaFirewallKey key;
    key.day = demand.due_day;
    key.family_id = demand.part_id / 10;
    key.cust_group_id = static_cast<uint32_t>(
        std::hash<std::string>()(demand.customer_group) % 100);
    key.region_id =
        static_cast<uint32_t>(std::hash<std::string>()(demand.region) % 10);

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
  result.total_orders = static_cast<uint32_t>(execution_demands.size());

  std::vector<DemandOrderRecord> sorted_demands = execution_demands;
  std::sort(sorted_demands.begin(), sorted_demands.end(),
            [](const DemandOrderRecord &a, const DemandOrderRecord &b) {
              return a.priority < b.priority;
            });

  double total_quota_allocated = 0.0;
  double total_quota_consumed = 0.0;
  for (const auto &kv : quota_firewalls) {
    total_quota_allocated += kv.second.total_quota;
  }

  for (const auto &demand : sorted_demands) {
    QuotaFirewallKey key;
    key.day = demand.due_day;
    key.family_id = demand.part_id / 10;
    key.cust_group_id = static_cast<uint32_t>(
        std::hash<std::string>()(demand.customer_group) % 100);
    key.region_id =
        static_cast<uint32_t>(std::hash<std::string>()(demand.region) % 10);

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

ClosedLoopAllotmentResult evaluate_itp_kitting_most_gating_closed_loop(
    const RegionalAllotmentInput &in) {
  ClosedLoopAllotmentResult out;

  const double fc_a = std::max(0.0, in.forecast_a);
  const double fc_d = std::max(0.0, in.forecast_d);
  const double ded_a = std::max(0.0, in.dedicated_supply_a);
  const double ded_d = std::max(0.0, in.dedicated_supply_d);
  const double sh_sup = std::max(0.0, in.shared_chip_supply);
  const double ord_a = std::max(0.0, in.exec_order_a);
  const double ord_d = std::max(0.0, in.exec_order_d);

  // Step 1: 专用受限件齐套包络先行（未齐套虚高预测不得挤占共用瓶颈件）
  out.kitted_a = std::min(fc_a, ded_a);
  out.kitted_d = std::min(fc_d, ded_d);
  const double total_kitted = out.kitted_a + out.kitted_d;

  // Step 2: 最紧瓶颈 (Most-Gating) 判定 + 战略保底锁定 + 剩余齐套需求公平按比例切分
  out.is_most_gating = (total_kitted > sh_sup + 1e-9);
  if (!out.is_most_gating) {
    out.allotment_a = out.kitted_a;
    out.allotment_d = out.kitted_d;
  } else {
    const double lock_a_req = std::min(std::max(0.0, in.strategic_floor_a), out.kitted_a);
    const double lock_d_req = std::min(std::max(0.0, in.strategic_floor_d), out.kitted_d);
    const double grant_lock_a = std::min(lock_a_req, sh_sup);
    const double grant_lock_d = std::min(lock_d_req, std::max(0.0, sh_sup - grant_lock_a));
    const double rem_sup = std::max(0.0, sh_sup - grant_lock_a - grant_lock_d);

    const double flex_a = std::max(0.0, out.kitted_a - grant_lock_a);
    const double flex_d = std::max(0.0, out.kitted_d - grant_lock_d);
    const double flex_tot = flex_a + flex_d;

    double fair_a = 0.0;
    double fair_d = 0.0;
    if (flex_tot > 1e-9 && rem_sup > 0.0) {
      fair_a = rem_sup * (flex_a / flex_tot);
      fair_d = rem_sup - fair_a;
    }
    out.allotment_a = grant_lock_a + fair_a;
    out.allotment_d = grant_lock_d + fair_d;
  }

  // Step 3: 按获批配额上限逆向重展非瓶颈件净采购（消除超买长板呆滞）
  out.legacy_non_bottleneck_buy = fc_a + fc_d;
  out.ipc_renetted_buy = out.allotment_a + out.allotment_d;
  out.excess_buy_eliminated =
      std::max(0.0, out.legacy_non_bottleneck_buy - out.ipc_renetted_buy);

  // Step 4: 执行期独立区域配额桶刚性隔离 vs 现有系统共池先到/高优抢料
  const double leg_take_a = std::min({ord_a, ded_a, sh_sup});
  const double leg_rem_sh = std::max(0.0, sh_sup - leg_take_a);
  out.legacy_fulfilled_d = std::min({ord_d, ded_d, leg_rem_sh});
  out.legacy_shortage_d = std::max(0.0, ord_d - out.legacy_fulfilled_d);

  out.ipc_fulfilled_a = std::min(ord_a, out.allotment_a);
  out.ipc_blocked_a = std::max(0.0, ord_a - out.ipc_fulfilled_a);
  out.ipc_fulfilled_d = std::min(ord_d, out.allotment_d);
  out.ipc_shortage_d = std::max(0.0, ord_d - out.ipc_fulfilled_d);

  return out;
}

} // namespace ipc_core
