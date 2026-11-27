#include "ipc_core/atp_ctp_engine.h"
#include <algorithm>
#include <numeric>

namespace ipc_core {

void CsrBomGraph::build(uint32_t num_parts, const std::vector<BomEdgeRecord>& boms) {
    row_ptr.assign(num_parts + 1, 0);
    for (const auto& e : boms) {
        if (e.parent_id < num_parts) {
            row_ptr[e.parent_id + 1] += 1;
        }
    }
    for (uint32_t i = 0; i < num_parts; ++i) {
        row_ptr[i + 1] += row_ptr[i];
    }
    edges.resize(row_ptr[num_parts]);
    std::vector<uint32_t> write_pos = row_ptr;
    for (const auto& e : boms) {
        if (e.parent_id < num_parts) {
            edges[write_pos[e.parent_id]++] = e;
        }
    }
}

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
) {
    if (qty <= 0.0) return true;
    if (part_id >= parts.size()) return false;

    const ArenaWatermark entry_wm = arena.mark();
    double needed = qty;

    // 1. 先扣减当前及更早到货的在手/在途供应水位
    if (part_id < atp_supplies.size()) {
        for (auto& node : atp_supplies[part_id]) {
            if (node.available_day <= due_day) {
                double avail = node.qty - node.allocated_qty;
                if (avail > 0.0) {
                    double take = std::min(needed, avail);
                    node.allocated_qty += take;
                    needed -= take;
                    arena.record_supply(&node, take);
                    if (needed <= 0.0) return true;
                }
            }
        }
    }

    // 2. CSR O(1) 切片判定：叶节点仍有缺口则立即按入口游标零残留回滚
    if (!csr_bom.has_children(part_id)) {
        if (needed > 0.0) {
            arena.rollback_to(entry_wm, capacity_records);
            return false;
        }
        return true;
    }

    // 3. 预留父件工作中心产能并沿 CSR 邻接切片向下递归齐套
    const auto& part_info = parts[part_id];
    int start_day = due_day - part_info.lead_time;
    if (start_day < 0) {
        arena.rollback_to(entry_wm, capacity_records);
        return false;
    }

    double hours_needed = needed * 0.1;
    bool cap_reserved = false;
    for (size_t i = 0; i < capacity_records.size(); ++i) {
        if (capacity_records[i].day == start_day) {
            double cap_avail = capacity_records[i].capacity_hours - capacity_records[i].allocated_hours;
            if (cap_avail >= hours_needed) {
                capacity_records[i].allocated_hours += hours_needed;
                arena.record_capacity(i, hours_needed);
                cap_reserved = true;
                break;
            }
        }
    }

    if (!cap_reserved) {
        arena.rollback_to(entry_wm, capacity_records);
        return false;
    }

    const uint32_t beg = csr_bom.begin_idx(part_id);
    const uint32_t end = csr_bom.end_idx(part_id);
    for (uint32_t ei = beg; ei < end; ++ei) {
        const auto& bom = csr_bom.edges[ei];
        double child_needed = needed * bom.unit_usage;
        bool ok = reserve_atp_and_capacity_csr(
            bom.child_id, start_day, child_needed, priority,
            atp_supplies, capacity_records, parts, csr_bom, arena
        );
        if (!ok) {
            arena.rollback_to(entry_wm, capacity_records);
            return false;
        }
    }

    return true;
}

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
) {
    ATPQueryResult result;
    arena.clear_committed();

    for (int day = requested_due_day; day <= requested_due_day + 10; ++day) {
        const ArenaWatermark day_wm = arena.mark();
        bool success = reserve_atp_and_capacity_csr(
            part_id, day, qty, priority,
            atp_supplies, capacity_records, parts, csr_bom, arena
        );

        if (success) {
            result.is_fulfillable = true;
            result.promised_day = day;
            result.promised_qty = qty;
            for (size_t i = day_wm.capacity_cursor; i < arena.capacity_trail.size(); ++i) {
                result.total_capacity_used += arena.capacity_trail[i].second;
            }
            arena.clear_committed();
            return result;
        }
        arena.rollback_to(day_wm, capacity_records);
        result.rollback_steps_count++;
    }

    return result;
}

ATPQueryResult promise_delivery_date(
    uint32_t part_id,
    int requested_due_day,
    double qty,
    uint64_t priority,
    std::vector<std::vector<ATPSupplyNode>>& atp_supplies,
    std::vector<CapacityRecord>& capacity_records,
    const std::vector<SkuNodeRecord>& parts,
    const std::vector<BomEdgeRecord>& boms
) {
    CsrBomGraph csr_bom;
    csr_bom.build(static_cast<uint32_t>(parts.size()), boms);
    WatermarkRollbackArena arena(256);
    return promise_delivery_date_zero_heap(
        part_id, requested_due_day, qty, priority,
        atp_supplies, capacity_records, parts, csr_bom, arena
    );
}

MultiDemandPeggingSummary run_two_stage_orthogonal_pegging(
    const std::vector<MultiDemandPeggingOrder>& demands,
    const std::vector<std::vector<int>>& part_supply_arrival_days
) {
    MultiDemandPeggingSummary out;
    const size_t n = demands.size();
    out.legacy_finish_days.assign(n, -1);
    out.ipc_finish_days.assign(n, -1);
    if (n == 0) return out;

    // 排序辅助：制备升序供给池副本
    auto make_sorted_pools = [&]() {
        auto pools = part_supply_arrival_days;
        for (auto& v : pools) std::sort(v.begin(), v.end());
        return pools;
    };

    // Baseline: 传统静态优先级排他锁料
    {
        auto pools = make_sorted_pools();
        std::vector<size_t> ptr(pools.size(), 0);
        std::vector<size_t> order(n);
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
            if (demands[a].priority != demands[b].priority)
                return demands[a].priority < demands[b].priority;
            return demands[a].requested_day < demands[b].requested_day;
        });

        for (size_t idx : order) {
            int fin = 0;
            std::vector<int> picked;
            bool ok = true;
            for (uint32_t p : demands[idx].required_parts) {
                if (p >= pools.size() || ptr[p] >= pools[p].size()) {
                    ok = false;
                    break;
                }
                int t_arr = pools[p][ptr[p]++];
                picked.push_back(t_arr);
                fin = std::max(fin, t_arr);
            }
            if (ok) {
                out.legacy_finish_days[idx] = fin;
                out.legacy_total_delay_days += std::max(0, fin - demands[idx].requested_day);
                for (int t_arr : picked) {
                    out.legacy_idle_lock_days += std::max(0, fin - t_arr);
                }
            }
        }
    }

    // IPC Two-Stage Orthogonal Decoupling:
    // Stage 1: 计算每笔需求独立物理齐套下界 T_min(d)
    // Stage 2: 按齐套下界升序尝试，同时施加无损交期护栏（高优单完工日绝不晚于静态锁料基线）
    std::vector<int> kitting_lb(n, 0);
    {
        auto pools = make_sorted_pools();
        for (size_t i = 0; i < n; ++i) {
            int lb = 0;
            for (uint32_t p : demands[i].required_parts) {
                if (p < pools.size() && !pools[p].empty()) {
                    lb = std::max(lb, pools[p][0]);
                } else {
                    lb = 999999;
                }
            }
            kitting_lb[i] = lb;
        }
    }

    auto pools = make_sorted_pools();
    std::vector<std::vector<bool>> used(pools.size());
    for (size_t p = 0; p < pools.size(); ++p) used[p].assign(pools[p].size(), false);

    std::vector<size_t> ipc_order(n);
    std::iota(ipc_order.begin(), ipc_order.end(), 0);
    std::sort(ipc_order.begin(), ipc_order.end(), [&](size_t a, size_t b) {
        if (kitting_lb[a] != kitting_lb[b]) return kitting_lb[a] < kitting_lb[b];
        if (demands[a].priority != demands[b].priority) return demands[a].priority < demands[b].priority;
        return demands[a].requested_day < demands[b].requested_day;
    });

    // 晚绑定匹配：每笔需求在不超过自身可达成完工日 T_target 的前提下，优先选最接近 T_target 的批次，释放更早批次给早齐套需求
    std::vector<bool> scheduled(n, false);
    for (size_t pos = 0; pos < n; ++pos) {
        size_t idx = ipc_order[pos];
        int fin = 0;
        for (uint32_t p : demands[idx].required_parts) {
            int earliest = -1;
            for (size_t k = 0; k < pools[p].size(); ++k) {
                if (!used[p][k]) {
                    earliest = pools[p][k];
                    break;
                }
            }
            fin = std::max(fin, earliest);
        }
        // 校验让渡后是否会损害尚未分配的更高优先级需求的基线完工日
        std::vector<std::pair<uint32_t, size_t>> chosen;
        for (uint32_t p : demands[idx].required_parts) {
            size_t best_k = pools[p].size();
            for (size_t k = 0; k < pools[p].size(); ++k) {
                if (!used[p][k] && pools[p][k] <= fin) {
                    best_k = k; // 取 <= fin 的最晚批次（晚绑定）
                }
            }
            if (best_k < pools[p].size()) {
                used[p][best_k] = true;
                chosen.emplace_back(p, best_k);
            }
        }

        bool safe_for_higher_prio = true;
        for (size_t j = 0; j < n; ++j) {
            if (j == idx || scheduled[j]) continue;
            if (demands[j].priority < demands[idx].priority && out.legacy_finish_days[j] >= 0) {
                int rem_fin = 0;
                for (uint32_t p : demands[j].required_parts) {
                    int ear = 999999;
                    for (size_t k = 0; k < pools[p].size(); ++k) {
                        if (!used[p][k]) {
                            ear = pools[p][k];
                            break;
                        }
                    }
                    rem_fin = std::max(rem_fin, ear);
                }
                if (rem_fin > out.legacy_finish_days[j]) {
                    safe_for_higher_prio = false;
                    break;
                }
            }
        }

        if (!safe_for_higher_prio) {
            // 撤销并退回按静态优先级安全顺序
            for (const auto& pr : chosen) used[pr.first][pr.second] = false;
            out.ipc_finish_days = out.legacy_finish_days;
            out.ipc_total_delay_days = out.legacy_total_delay_days;
            out.ipc_idle_lock_days = out.legacy_idle_lock_days;
            return out;
        }

        scheduled[idx] = true;
        out.ipc_finish_days[idx] = fin;
        out.ipc_total_delay_days += std::max(0, fin - demands[idx].requested_day);
        for (const auto& pr : chosen) {
            out.ipc_idle_lock_days += std::max(0, fin - pools[pr.first][pr.second]);
        }
    }

    return out;
}

} // namespace ipc_core
