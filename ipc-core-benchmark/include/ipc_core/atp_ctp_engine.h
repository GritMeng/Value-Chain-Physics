#pragma once
#include "types.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace ipc_core {

// ATP 引擎推演结果
struct ATPQueryResult {
    bool is_fulfillable = false;
    int promised_day = -1;
    double promised_qty = 0.0;
    double total_capacity_used = 0.0;
    size_t rollback_steps_count = 0; // 回滚次数（用于验证零堆栈快速回滚）
};

// 递归 ATP / CTP 预留与能力派程算子（包含零堆回滚机制）
bool reserve_atp_and_capacity_recursive(
    uint32_t part_id,
    int due_day,
    double qty,
    uint64_t priority,
    std::vector<std::vector<ATPSupplyNode>>& atp_supplies,
    std::vector<CapacityRecord>& capacity_records,
    const std::vector<PartSiteRecord>& parts,
    const std::vector<FlatBomItem>& boms,
    std::vector<ATPSupplyNode*>& temp_allocations,
    std::vector<double>& temp_alloc_qty,
    std::vector<std::pair<size_t, double>>& temp_capacity_allocations
);

// 订单可承诺交付日 (Delivery Promising Engine) 主引擎入口
ATPQueryResult promise_delivery_date(
    uint32_t part_id,
    int requested_due_day,
    double qty,
    uint64_t priority,
    std::vector<std::vector<ATPSupplyNode>>& atp_supplies,
    std::vector<CapacityRecord>& capacity_records,
    const std::vector<PartSiteRecord>& parts,
    const std::vector<FlatBomItem>& boms
);

} // namespace ipc_core
