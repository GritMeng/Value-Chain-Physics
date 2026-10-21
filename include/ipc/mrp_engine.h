#pragma once
#include "ipc_types.h"
#include <vector>
#include <cstdint>

namespace ipc {

void run_lbl_mrp_engine(
    std::vector<PartSiteRecord>& parts,
    std::vector<FlatBomItem>& boms,
    const std::vector<IndependentDemand>& demands,
    std::vector<PlannedOrder>& out_ipc_planned_orders,
    std::vector<AlternateAllocationRecord>& out_alt_records,
    std::vector<SwapRecord>& out_swap_records,
    int max_llc_level,
    const std::vector<ScheduledReceiptRecord>& srs = std::vector<ScheduledReceiptRecord>()
);

} // namespace ipc
