#pragma once
#include "types.h"
#include <vector>
#include <string>

namespace ipc_core {

struct ATPQueryResult {
    bool is_fulfillable = false;
    int promised_day = -1;
    double promised_qty = 0.0;
    double total_capacity_used = 0.0;
    size_t rollback_steps_count = 0;
};

bool reserve_atp_and_capacity_recursive(
    uint32_t part_id,
    int due_day,
    double qty,
    uint64_t priority,
    std::vector<std::vector<ATPSupplyNode>>& atp_supplies,
    std::vector<CapacityRecord>& capacity_records,
    const std::vector<SkuNodeRecord>& parts,
    const std::vector<BomEdgeRecord>& boms,
    std::vector<ATPSupplyNode*>& temp_allocations,
    std::vector<double>& temp_alloc_qty,
    std::vector<std::pair<size_t, double>>& temp_capacity_allocations
);

ATPQueryResult promise_delivery_date(
    uint32_t part_id,
    int requested_due_day,
    double qty,
    uint64_t priority,
    std::vector<std::vector<ATPSupplyNode>>& atp_supplies,
    std::vector<CapacityRecord>& capacity_records,
    const std::vector<SkuNodeRecord>& parts,
    const std::vector<BomEdgeRecord>& boms
);

} // namespace ipc_core
