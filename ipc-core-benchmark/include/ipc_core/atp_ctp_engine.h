#pragma once
#include "types.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ipc_core {

// 紧凑 CSR (Compressed Sparse Row) BOM 邻接索引：一次构建 O(V + E)，查询子件切片 O(1)
struct CsrBomGraph {
    std::vector<uint32_t> row_ptr;
    std::vector<BomEdgeRecord> edges;

    void build(uint32_t num_parts, const std::vector<BomEdgeRecord>& boms);
    uint32_t begin_idx(uint32_t parent_id) const {
        return (parent_id + 1 < row_ptr.size()) ? row_ptr[parent_id] : 0;
    }
    uint32_t end_idx(uint32_t parent_id) const {
        return (parent_id + 1 < row_ptr.size()) ? row_ptr[parent_id + 1] : 0;
    }
    bool has_children(uint32_t parent_id) const {
        return end_idx(parent_id) > begin_idx(parent_id);
    }
};

// 预分配水位游标回滚区（搜索热路径零堆内存分配，失败时 O(Delta) 原子复原）
struct ArenaWatermark {
    size_t supply_cursor = 0;
    size_t capacity_cursor = 0;
};

struct WatermarkRollbackArena {
    std::vector<std::pair<ATPSupplyNode*, double>> supply_trail;
    std::vector<std::pair<size_t, double>> capacity_trail;

    explicit WatermarkRollbackArena(size_t reserve_entries = 256) {
        supply_trail.reserve(reserve_entries);
        capacity_trail.reserve(reserve_entries);
    }

    ArenaWatermark mark() const {
        return {supply_trail.size(), capacity_trail.size()};
    }

    void record_supply(ATPSupplyNode* node, double qty) {
        supply_trail.emplace_back(node, qty);
    }

    void record_capacity(size_t cap_idx, double hours) {
        capacity_trail.emplace_back(cap_idx, hours);
    }

    void rollback_to(const ArenaWatermark& wm, std::vector<CapacityRecord>& capacity_records) {
        while (supply_trail.size() > wm.supply_cursor) {
            auto& entry = supply_trail.back();
            entry.first->allocated_qty -= entry.second;
            supply_trail.pop_back();
        }
        while (capacity_trail.size() > wm.capacity_cursor) {
            auto& entry = capacity_trail.back();
            capacity_records[entry.first].allocated_hours -= entry.second;
            capacity_trail.pop_back();
        }
    }

    void clear_committed() {
        supply_trail.clear();
        capacity_trail.clear();
    }
};

struct ATPQueryResult {
    bool is_fulfillable = false;
    int promised_day = -1;
    double promised_qty = 0.0;
    double total_capacity_used = 0.0;
    size_t rollback_steps_count = 0;
};

bool reserve_atp_and_capacity_csr(
    uint32_t part_id,
    int due_day,
    double qty,
    uint64_t priority,
    std::vector<std::vector<ATPSupplyNode>>& atp_supplies,
    std::vector<CapacityRecord>& capacity_records,
    const std::vector<SkuNodeRecord>& parts,
    const CsrBomGraph& csr_bom,
    WatermarkRollbackArena& arena
);

ATPQueryResult promise_delivery_date_zero_heap(
    uint32_t part_id,
    int requested_due_day,
    double qty,
    uint64_t priority,
    std::vector<std::vector<ATPSupplyNode>>& atp_supplies,
    std::vector<CapacityRecord>& capacity_records,
    const std::vector<SkuNodeRecord>& parts,
    const CsrBomGraph& csr_bom,
    WatermarkRollbackArena& arena
);

// 兼容便捷入口（内部自动构建 CSR 与复用预分配回滚区）
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

// 通用多需求×多物料两阶段正交解耦（Stage 1 齐套下界探底 + Stage 2 保交期晚绑定）
struct MultiDemandPeggingOrder {
    uint32_t demand_id = 0;
    int requested_day = 1;
    uint64_t priority = 1; // 数值越小优先级越高
    std::vector<uint32_t> required_parts; // 各子件按 1:1 需求（可扩展单耗）
};

struct MultiDemandPeggingSummary {
    std::vector<int> legacy_finish_days;
    std::vector<int> ipc_finish_days;
    int legacy_total_delay_days = 0;
    int ipc_total_delay_days = 0;
    int legacy_idle_lock_days = 0;
    int ipc_idle_lock_days = 0;
};

MultiDemandPeggingSummary run_two_stage_orthogonal_pegging(
    const std::vector<MultiDemandPeggingOrder>& demands,
    const std::vector<std::vector<int>>& part_supply_arrival_days
);

} // namespace ipc_core
