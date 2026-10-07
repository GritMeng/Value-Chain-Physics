#include "ipc_core/atp_ctp_engine.h"
#include <algorithm>
#include <iostream>

namespace ipc_core {

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
) {
    if (qty <= 0.0) return true;
    if (part_id >= parts.size()) return false;

    double needed = qty;

    // 1. 尝试从现有库存/计划产出 (On-Hand & Scheduled Receipts) 消纳
    if (part_id < atp_supplies.size()) {
        for (auto& node : atp_supplies[part_id]) {
            if (node.available_day <= due_day) {
                double avail = node.qty - node.allocated_qty;
                if (avail > 0.0) {
                    double take = std::min(needed, avail);
                    node.allocated_qty += take;
                    needed -= take;

                    temp_allocations.push_back(&node);
                    temp_alloc_qty.push_back(take);

                    if (needed <= 0.0) return true;
                }
            }
        }
    }

    // 2. 若库存不足，且无下级 BOM 依赖，说明无法全量满足
    bool has_children = false;
    for (const auto& bom : boms) {
        if (bom.parent_id == part_id) {
            has_children = true;
            break;
        }
    }

    if (!has_children) {
        return needed <= 0.0;
    }

    // 3. 递归向下扣减子件 BOM 与产能
    size_t saved_alloc_size = temp_allocations.size();
    size_t saved_cap_size = temp_capacity_allocations.size();

    const auto& part_info = parts[part_id];
    int start_day = due_day - part_info.lead_time;
    if (start_day < 0) return false; // 超出时域

    // 扣减本级产能
    double hours_needed = needed * 0.1; // 示例：单件 0.1 工时
    bool cap_reserved = false;
    for (size_t i = 0; i < capacity_records.size(); ++i) {
        if (capacity_records[i].day == start_day) {
            double cap_avail = capacity_records[i].capacity_hours - capacity_records[i].allocated_hours;
            if (cap_avail >= hours_needed) {
                capacity_records[i].allocated_hours += hours_needed;
                temp_capacity_allocations.push_back({i, hours_needed});
                cap_reserved = true;
                break;
            }
        }
    }

    if (!cap_reserved) return false;

    // 递归扣减子件物料
    for (const auto& bom : boms) {
        if (bom.parent_id == part_id) {
            double child_needed = needed * bom.usage_qty;
            bool ok = reserve_atp_and_capacity_recursive(
                bom.child_id, start_day, child_needed, priority,
                atp_supplies, capacity_records, parts, boms,
                temp_allocations, temp_alloc_qty, temp_capacity_allocations
            );

            if (!ok) {
                // 零堆内存回滚 (Zero-Heap Rollback)
                while (temp_allocations.size() > saved_alloc_size) {
                    temp_allocations.back()->allocated_qty -= temp_alloc_qty.back();
                    temp_allocations.pop_back();
                    temp_alloc_qty.pop_back();
                }
                while (temp_capacity_allocations.size() > saved_cap_size) {
                    auto& cap_pair = temp_capacity_allocations.back();
                    capacity_records[cap_pair.first].allocated_hours -= cap_pair.second;
                    temp_capacity_allocations.pop_back();
                }
                return false;
            }
        }
    }

    return true;
}

ATPQueryResult promise_delivery_date(
    uint32_t part_id,
    int requested_due_day,
    double qty,
    uint64_t priority,
    std::vector<std::vector<ATPSupplyNode>>& atp_supplies,
    std::vector<CapacityRecord>& capacity_records,
    const std::vector<PartSiteRecord>& parts,
    const std::vector<FlatBomItem>& boms
) {
    ATPQueryResult result;
    
    // 尝试在请求交期 (requested_due_day) 及其后 10 天内计算最快交期
    for (int day = requested_due_day; day <= requested_due_day + 10; ++day) {
        std::vector<ATPSupplyNode*> temp_allocs;
        std::vector<double> temp_qtys;
        std::vector<std::pair<size_t, double>> temp_caps;

        bool success = reserve_atp_and_capacity_recursive(
            part_id, day, qty, priority,
            atp_supplies, capacity_records, parts, boms,
            temp_allocs, temp_qtys, temp_caps
        );

        if (success) {
            result.is_fulfillable = true;
            result.promised_day = day;
            result.promised_qty = qty;
            for (const auto& cap : temp_caps) {
                result.total_capacity_used += cap.second;
            }
            return result;
        } else {
            result.rollback_steps_count++;
        }
    }

    return result;
}

} // namespace ipc_core
