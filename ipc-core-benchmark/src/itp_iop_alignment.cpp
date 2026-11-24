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

} // namespace ipc_core
