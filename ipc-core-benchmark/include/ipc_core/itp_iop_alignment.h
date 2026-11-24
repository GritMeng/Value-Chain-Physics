#pragma once
#include "types.h"
#include <unordered_map>
#include <vector>

namespace ipc_core {

std::unordered_map<QuotaFirewallKey, QuotaFirewallState, QuotaFirewallKeyHash>
generate_itp_master_allotments(
    const std::vector<DemandOrderRecord> &master_demands,
    const std::vector<SkuNodeRecord> &parts,
    double capacity_buffer_factor = 1.1);

struct IOPExecutionResult {
  uint32_t total_orders = 0;
  uint32_t scheduled_orders = 0;
  uint32_t blocked_orders = 0;
  double total_fulfilled_qty = 0.0;
  double quota_utilization = 0.0;
};

IOPExecutionResult run_iop_execution_alignment(
    const std::vector<DemandOrderRecord> &execution_demands,
    const std::vector<SkuNodeRecord> &parts,
    std::unordered_map<QuotaFirewallKey, QuotaFirewallState,
                       QuotaFirewallKeyHash> &quota_firewalls);

} // namespace ipc_core
