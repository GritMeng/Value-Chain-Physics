#include "ipc/dbd_engine.h"
#include "ipc/globals.h"
#include "ipc/vocab.h"
#include "ipc/math_utils.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace ipc {


static thread_local std::vector<PlannedOrderSplit> dummy_po_splits;

static thread_local std::vector<WcAllocation> dummy_wc_allocs;
static thread_local std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash> dummy_allotment_constraints;
static thread_local std::vector<std::pair<AllotmentState*, double>> dummy_allotment_allocs;
static thread_local std::vector<std::pair<AllotmentState*, double>> dummy_allotment_blocked;
static thread_local std::vector<bool> allotment_constrained_parts;

struct ActivePartsGuard {
    std::vector<bool>& active_parts;
    uint32_t part_id;
    bool active;
    ActivePartsGuard(std::vector<bool>& ap, uint32_t pid) 
        : active_parts(ap), part_id(pid), active(false) {
        if (part_id < active_parts.size() && !active_parts[part_id]) {
            active_parts[part_id] = true;
            active = true;
        }
    }
    ~ActivePartsGuard() {
        if (active && part_id < active_parts.size()) {
            active_parts[part_id] = false;
        }
    }
};

struct MixGroupGuard {
    std::unordered_map<uint32_t, int>& active_mix_groups;
    std::unordered_map<uint32_t, int> saved_mix_groups;
    bool committed;
    MixGroupGuard(std::unordered_map<uint32_t, int>& amg)
        : active_mix_groups(amg), saved_mix_groups(amg), committed(false) {}
    void commit() {
        committed = true;
    }
    ~MixGroupGuard() {
        if (!committed) {
            active_mix_groups = saved_mix_groups;
        }
    }
};

static bool enable_allotment_debug = false;



bool reserve_atp_and_capacity_recursive(
    uint32_t part_id,
    int due_day,
    double qty,
    uint64_t priority,
    double dimension_val,
    std::vector<std::vector<ATPSupplyNode>>& atp_supplies,
    std::vector<ConstraintRecord>& shared_constraints,
    const std::vector<PartSiteRecord>& parts,
    const std::vector<FlatBomItem>& boms,
    const std::vector<PartBomInfo>& part_bom_info,
    const std::vector<SourceConstraintRecord>& source_constraints,
    std::vector<ATPSupplyNode*>& temp_allocations,
    std::vector<double>& temp_alloc_qty,
    std::vector<std::pair<size_t, std::pair<int, double>>>& temp_capacity_allocations,
    const std::string& preference_mode,
    std::vector<double>& bom_ltb_consumed,
    std::vector<std::pair<size_t, double>>& temp_ltb_allocations,
    std::unordered_map<uint32_t, int>& active_mix_groups,
    const std::vector<std::vector<double>>& last_dim_val,
    double& out_routing_cost,
    std::vector<bool>& active_parts,
    std::vector<PlannedOrderSplit>& temp_po_splits,
    bool is_recursive_child,
    std::vector<WcAllocation>& temp_wc_allocations,
    int root_due_day,
    uint32_t root_family_id,
    uint32_t root_cust_group_id,
    uint32_t root_region_id,
    uint32_t wildcard_id,
    std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash>& allotment_constraints,
    std::vector<std::pair<AllotmentState*, double>>& temp_allotment_allocations,
    std::vector<std::pair<AllotmentState*, double>>& temp_allotment_blocked,
    bool skip_po_quota) {
    if (qty <= 0.0) return true;

    if (enable_allotment_debug) {
        std::cout << "[DEBUG FG1] reserve_atp_and_capacity_recursive: part_id=" << part_id
                  << " (" << vocab.get_code(part_id) << "), due_day=" << due_day
                  << ", qty=" << qty << ", is_recursive_child=" << is_recursive_child << std::endl;
    }

    int local_root_due_day = (root_due_day == -1) ? due_day : root_due_day;

    size_t temp_allot_alloc_start = temp_allotment_allocations.size();
    size_t temp_allot_blocked_start = temp_allotment_blocked.size();
    AllotmentRollbackGuard allotment_guard(temp_allotment_allocations, temp_allotment_blocked, temp_allot_alloc_start, temp_allot_blocked_start);

    bool should_check_allotment = is_recursive_child || (part_id < parts.size() && parts[part_id].part_type == "FINISHED");
    double demand_qty = qty;

    bool use_default_order = (preference_mode != "Z" && preference_mode != "C");
    if (use_default_order) {
        for (size_t idx = 0; idx < atp_supplies[part_id].size(); ++idx) {
            auto& node = atp_supplies[part_id][idx];
            if (node.supply_type == "SR" || node.supply_type == "On-Hand") {
                double avail = node.qty - node.allocated_qty;
                for (size_t k = 0; k < temp_allocations.size(); ++k) {
                    if (temp_allocations[k] == &node) {
                        avail -= temp_alloc_qty[k];
                    }
                }
                if (avail > 0.0 && node.available_day <= due_day) {
                    double target_alloc = std::min(demand_qty, avail);
                    double allowed_alloc = target_alloc;

                    double allot_avail = -1.0;
                    AllotmentState* m_state = nullptr;
                    if (should_check_allotment && root_family_id != uint32_t(-1) && !allotment_constraints.empty()) {
                        if (part_id < allotment_constrained_parts.size() && allotment_constrained_parts[part_id]) {
                            m_state = find_matching_allotment(part_id, local_root_due_day, root_family_id, root_cust_group_id, root_region_id, wildcard_id, allotment_constraints);
                            if (m_state != nullptr) {
                                double temp_consumed = 0.0;
                                for (const auto& pair : temp_allotment_allocations) {
                                    if (pair.first == m_state) {
                                        temp_consumed += pair.second;
                                    }
                                }
                                allot_avail = m_state->limit - (m_state->consumed + temp_consumed);
                                if (allot_avail < 0.0) allot_avail = 0.0;
                            } else {
                                allot_avail = 0.0;
                            }
                        }
                    }

                    if (allot_avail >= 0.0) {
                        if (target_alloc > allot_avail) {
                            allowed_alloc = allot_avail;
                            double deficit = target_alloc - allowed_alloc;
                            if (deficit > 0.0 && m_state != nullptr) {
                                temp_allotment_blocked.push_back({m_state, deficit});
                            }
                        }
                    }

                    if (allowed_alloc > 0.0) {
                        if (m_state != nullptr) {
                            temp_allotment_allocations.push_back({m_state, allowed_alloc});
                        }
                        temp_allocations.push_back(&node);
                        temp_alloc_qty.push_back(allowed_alloc);
                        demand_qty -= allowed_alloc;
                        if (demand_qty <= 0.0) break;
                    }
                }
            }
        }
    } else {
        std::vector<size_t> sorted_indices(atp_supplies[part_id].size());
        for (size_t i = 0; i < sorted_indices.size(); ++i) sorted_indices[i] = i;

        if (preference_mode == "Z") {
            std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t x, size_t y) {
                return atp_supplies[part_id][x].priority < atp_supplies[part_id][y].priority;
            });
        } else {
            std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t x, size_t y) {
                bool is_po_x = (atp_supplies[part_id][x].supply_type == "Planned-Order");
                bool is_po_y = (atp_supplies[part_id][y].supply_type == "Planned-Order");
                if (is_po_x != is_po_y) {
                    return !is_po_x;
                }
                return atp_supplies[part_id][x].available_day < atp_supplies[part_id][y].available_day;
            });
        }

        for (size_t idx : sorted_indices) {
            auto& node = atp_supplies[part_id][idx];
            if (node.supply_type == "SR" || node.supply_type == "On-Hand") {
                double avail = node.qty - node.allocated_qty;
                for (size_t k = 0; k < temp_allocations.size(); ++k) {
                    if (temp_allocations[k] == &node) {
                        avail -= temp_alloc_qty[k];
                    }
                }
                if (avail > 0.0 && node.available_day <= due_day) {
                    double target_alloc = std::min(demand_qty, avail);
                    double allowed_alloc = target_alloc;

                    double allot_avail = -1.0;
                    AllotmentState* m_state = nullptr;
                    if (should_check_allotment && root_family_id != uint32_t(-1) && !allotment_constraints.empty()) {
                        if (part_id < allotment_constrained_parts.size() && allotment_constrained_parts[part_id]) {
                            m_state = find_matching_allotment(part_id, local_root_due_day, root_family_id, root_cust_group_id, root_region_id, wildcard_id, allotment_constraints);
                            if (m_state != nullptr) {
                                double temp_consumed = 0.0;
                                for (const auto& pair : temp_allotment_allocations) {
                                    if (pair.first == m_state) {
                                        temp_consumed += pair.second;
                                    }
                                }
                                allot_avail = m_state->limit - (m_state->consumed + temp_consumed);
                                if (allot_avail < 0.0) allot_avail = 0.0;
                            } else {
                                allot_avail = 0.0;
                            }
                        }
                    }

                    if (allot_avail >= 0.0) {
                        if (target_alloc > allot_avail) {
                            allowed_alloc = allot_avail;
                            double deficit = target_alloc - allowed_alloc;
                            if (deficit > 0.0 && m_state != nullptr) {
                                temp_allotment_blocked.push_back({m_state, deficit});
                            }
                        }
                    }

                    if (allowed_alloc > 0.0) {
                        if (m_state != nullptr) {
                            temp_allotment_allocations.push_back({m_state, allowed_alloc});
                        }
                        temp_allocations.push_back(&node);
                        temp_alloc_qty.push_back(allowed_alloc);
                        demand_qty -= allowed_alloc;
                        if (demand_qty <= 0.0) break;
                    }
                }
            }
        }
    }

    if (demand_qty <= 0.0) {
        allotment_guard.commit();
        return true;
    }

    double prod_qty = demand_qty;
    size_t start_alloc_size = temp_allocations.size();

    bool has_po_quota = false;
    for (const auto& node : atp_supplies[part_id]) {
        if (node.supply_type == "Planned-Order") {
            has_po_quota = true;
            break;
        }
    }

    if (is_recursive_child && has_po_quota && !skip_po_quota) {
        double temp_demand = prod_qty;
        if (use_default_order) {
            for (size_t idx = 0; idx < atp_supplies[part_id].size(); ++idx) {
                auto& node = atp_supplies[part_id][idx];
                if (node.supply_type == "Planned-Order") {
                    double avail = node.qty - node.allocated_qty;
                    for (size_t k = 0; k < temp_allocations.size(); ++k) {
                        if (temp_allocations[k] == &node) {
                            avail -= temp_alloc_qty[k];
                        }
                    }
                    if (avail > 0.0) {
                        double target_alloc = std::min(temp_demand, avail);
                        double allowed_alloc = target_alloc;

                        double allot_avail = -1.0;
                        AllotmentState* m_state = nullptr;
                        if (should_check_allotment && root_family_id != uint32_t(-1) && !allotment_constraints.empty()) {
                            if (part_id < allotment_constrained_parts.size() && allotment_constrained_parts[part_id]) {
                                m_state = find_matching_allotment(part_id, local_root_due_day, root_family_id, root_cust_group_id, root_region_id, wildcard_id, allotment_constraints);
                                if (m_state != nullptr) {
                                    double temp_consumed = 0.0;
                                    for (const auto& pair : temp_allotment_allocations) {
                                        if (pair.first == m_state) {
                                            temp_consumed += pair.second;
                                        }
                                    }
                                    allot_avail = m_state->limit - (m_state->consumed + temp_consumed);
                                    if (allot_avail < 0.0) allot_avail = 0.0;
                                } else {
                                    allot_avail = 0.0;
                                }
                            }
                        }

                        if (allot_avail >= 0.0) {
                            if (target_alloc > allot_avail) {
                                allowed_alloc = allot_avail;
                                double deficit = target_alloc - allowed_alloc;
                                if (deficit > 0.0 && m_state != nullptr) {
                                    temp_allotment_blocked.push_back({m_state, deficit});
                                }
                            }
                        }

                        if (allowed_alloc > 0.0) {
                            if (m_state != nullptr) {
                                temp_allotment_allocations.push_back({m_state, allowed_alloc});
                            }
                            temp_allocations.push_back(&node);
                            temp_alloc_qty.push_back(allowed_alloc);
                            temp_demand -= allowed_alloc;
                            if (temp_demand <= 0.0) break;
                        }
                    }
                }
            }
        } else {
            // Need sorted indices from earlier
            std::vector<size_t> sorted_indices(atp_supplies[part_id].size());
            for (size_t i = 0; i < sorted_indices.size(); ++i) sorted_indices[i] = i;
            if (preference_mode == "Z") {
                std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t x, size_t y) {
                    return atp_supplies[part_id][x].priority < atp_supplies[part_id][y].priority;
                });
            } else {
                std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t x, size_t y) {
                    bool is_po_x = (atp_supplies[part_id][x].supply_type == "Planned-Order");
                    bool is_po_y = (atp_supplies[part_id][y].supply_type == "Planned-Order");
                    if (is_po_x != is_po_y) {
                        return !is_po_x;
                    }
                    return atp_supplies[part_id][x].available_day < atp_supplies[part_id][y].available_day;
                });
            }
            for (size_t idx : sorted_indices) {
                auto& node = atp_supplies[part_id][idx];
                if (node.supply_type == "Planned-Order") {
                    double avail = node.qty - node.allocated_qty;
                    for (size_t k = 0; k < temp_allocations.size(); ++k) {
                        if (temp_allocations[k] == &node) {
                            avail -= temp_alloc_qty[k];
                        }
                    }
                    if (avail > 0.0) {
                        double target_alloc = std::min(temp_demand, avail);
                        double allowed_alloc = target_alloc;

                        double allot_avail = -1.0;
                        AllotmentState* m_state = nullptr;
                        if (should_check_allotment && root_family_id != uint32_t(-1) && !allotment_constraints.empty()) {
                            if (part_id < allotment_constrained_parts.size() && allotment_constrained_parts[part_id]) {
                                m_state = find_matching_allotment(part_id, local_root_due_day, root_family_id, root_cust_group_id, root_region_id, wildcard_id, allotment_constraints);
                                if (m_state != nullptr) {
                                    double temp_consumed = 0.0;
                                    for (const auto& pair : temp_allotment_allocations) {
                                        if (pair.first == m_state) {
                                            temp_consumed += pair.second;
                                        }
                                    }
                                    allot_avail = m_state->limit - (m_state->consumed + temp_consumed);
                                    if (allot_avail < 0.0) allot_avail = 0.0;
                                } else {
                                    allot_avail = 0.0;
                                }
                            }
                        }

                        if (allot_avail >= 0.0) {
                            if (target_alloc > allot_avail) {
                                allowed_alloc = allot_avail;
                                double deficit = target_alloc - allowed_alloc;
                                if (deficit > 0.0 && m_state != nullptr) {
                                    temp_allotment_blocked.push_back({m_state, deficit});
                                }
                            }
                        }

                        if (allowed_alloc > 0.0) {
                            if (m_state != nullptr) {
                                temp_allotment_allocations.push_back({m_state, allowed_alloc});
                            }
                            temp_allocations.push_back(&node);
                            temp_alloc_qty.push_back(allowed_alloc);
                            temp_demand -= allowed_alloc;
                            if (temp_demand <= 0.0) break;
                        }
                    }
                }
            }
        }
        if (temp_demand > 0.0) {
            return false;
        }
        for (size_t k = start_alloc_size; k < temp_allocations.size(); ++k) {
            auto* node = temp_allocations[k];
            if (node->supply_type == "Planned-Order" && node->planned_order_index != -1) {
                size_t child_po_idx = static_cast<size_t>(node->planned_order_index);
                double child_lt = parts[part_id].lead_time + temp_alloc_qty[k] * parts[part_id].run_rate;
                int child_start = get_workday_offset_backward(due_day, static_cast<int>(std::ceil(child_lt)), parts[part_id].planning_calendar, global_wc_daily_capacity);

                const auto& child_sc = source_constraints[part_id];
                double child_cap = temp_alloc_qty[k] * child_sc.constraint_factor;

                temp_po_splits.push_back({
                    child_po_idx,
                    temp_alloc_qty[k],
                    due_day,
                    child_start,
                    child_cap,
                    out_routing_cost
                });
            }
        }
        allotment_guard.commit();
        return true;
    }

    if (prod_qty > 0.0) {
        double allowed_prod = prod_qty;
        AllotmentState* prod_m_state = nullptr;
        double allot_avail = -1.0;
        if (should_check_allotment && root_family_id != uint32_t(-1) && !allotment_constraints.empty()) {
            if (part_id < allotment_constrained_parts.size() && allotment_constrained_parts[part_id]) {
                prod_m_state = find_matching_allotment(part_id, local_root_due_day, root_family_id, root_cust_group_id, root_region_id, wildcard_id, allotment_constraints);
                if (prod_m_state != nullptr) {
                    double temp_consumed = 0.0;
                    for (const auto& pair : temp_allotment_allocations) {
                        if (pair.first == prod_m_state) {
                            temp_consumed += pair.second;
                        }
                    }
                    allot_avail = prod_m_state->limit - (prod_m_state->consumed + temp_consumed);
                    if (allot_avail < 0.0) allot_avail = 0.0;
                } else {
                    allot_avail = 0.0;
                }
            }
        }

        if (allot_avail >= 0.0) {
            if (prod_qty > allot_avail) {
                allowed_prod = allot_avail;
                double deficit = prod_qty - allowed_prod;
                if (deficit > 0.0 && prod_m_state != nullptr) {
                    temp_allotment_blocked.push_back({prod_m_state, deficit});
                }
            }
        }

        if (allowed_prod <= 0.0) {
            if (enable_allotment_debug) {
                std::cout << "[DEBUG FG1] Return false: allowed_prod <= 0.0 for new Planned-Order on due_day=" << due_day << std::endl;
            }
            return false;
        }

        if (prod_m_state != nullptr) {
            temp_allotment_allocations.push_back({prod_m_state, allowed_prod});
        }

        demand_qty = allowed_prod;
    }

    int start_day = due_day;
    bool routing_crp_checked = false;
    if (part_id < global_part_routings.size() && !global_part_routings[part_id].empty() && !global_wc_daily_capacity.empty()) {
        routing_crp_checked = true;
        const auto& ops = global_part_routings[part_id];
        int curr_day = due_day;
        std::vector<WcAllocation> potential_wc_allocs;
        for (int idx = static_cast<int>(ops.size()) - 1; idx >= 0; --idx) {
            const auto& op = ops[idx];
            if (idx < static_cast<int>(ops.size()) - 1) {
                curr_day = get_workday_offset_backward(curr_day, 1, op.work_center, global_wc_daily_capacity);
            }
            if (curr_day < parts[part_id].time_fence_days) {
                return false;
            }
            double required_hours = op.setup_time + demand_qty * op.run_time;
            double allocated = 0.0;
            if (global_wc_allocated_capacity.find(op.work_center) != global_wc_allocated_capacity.end()) {
                allocated = global_wc_allocated_capacity[op.work_center][curr_day];
            }
            for (const auto& alloc : temp_wc_allocations) {
                if (alloc.work_center == op.work_center && alloc.day == curr_day) {
                    allocated += alloc.hours;
                }
            }
            double cap_limit = 24.0;
            if (global_wc_daily_capacity.find(op.work_center) != global_wc_daily_capacity.end()) {
                cap_limit = global_wc_daily_capacity[op.work_center][curr_day];
            }
            if (cap_limit - allocated < required_hours) {
                return false;
            }
            potential_wc_allocs.push_back({op.work_center, curr_day, required_hours});
        }
        for (const auto& alloc : potential_wc_allocs) {
            temp_wc_allocations.push_back(alloc);
        }
        out_routing_cost = 0.0;
        start_day = curr_day;
    } else {
        double lead_time = parts[part_id].lead_time + demand_qty * parts[part_id].run_rate;
        start_day = get_workday_offset_backward(due_day, static_cast<int>(std::ceil(lead_time)), parts[part_id].planning_calendar, global_wc_daily_capacity);
        if (start_day < parts[part_id].time_fence_days) {
            return false;
        }

        const auto& sc = source_constraints[part_id];
        uint32_t cid = sc.constraint_id;
        if (cid >= shared_constraints.size()) {
            return false;
        }
        auto& constr = shared_constraints[cid];

        double setup_time = sc.before_fixed_factor;
        if (cid < last_dim_val.size() && start_day >= 0 && start_day < static_cast<int>(last_dim_val[cid].size()) && last_dim_val[cid][start_day] == dimension_val) {
            setup_time = 0.0;
        }

        double required_cap = setup_time + demand_qty * sc.constraint_factor + sc.after_fixed_factor;

        double current_allocated = constr.allocated_rates[start_day];
        for (const auto& cap_alloc : temp_capacity_allocations) {
            if (cap_alloc.first == cid && cap_alloc.second.first == start_day) {
                current_allocated += cap_alloc.second.second;
            }
        }

        bool default_route_ok = true;
        double avail_cap = constr.rates[start_day] - current_allocated;
        if (avail_cap < required_cap) {
            default_route_ok = false;
        }

        std::vector<std::pair<size_t, double>> temp_extra_allocs;
        if (default_route_ok) {
            for (const auto& extra : sc.extra_constraints) {
                uint32_t ecid = extra.constraint_id;
                if (ecid >= shared_constraints.size()) {
                    default_route_ok = false;
                    break;
                }
                double req_ecap = extra.factor * demand_qty;

                double extra_allocated = shared_constraints[ecid].allocated_rates[start_day];
                for (const auto& cap_alloc : temp_capacity_allocations) {
                    if (cap_alloc.first == ecid && cap_alloc.second.first == start_day) {
                        extra_allocated += cap_alloc.second.second;
                    }
                }
                if (shared_constraints[ecid].rates[start_day] - extra_allocated < req_ecap) {
                    default_route_ok = false;
                    break;
                }
                temp_extra_allocs.push_back({ecid, req_ecap});
            }
        }

        if (default_route_ok) {
            temp_capacity_allocations.push_back({cid, {start_day, required_cap}});
            for (const auto& ea : temp_extra_allocs) {
                temp_capacity_allocations.push_back({ea.first, {start_day, ea.second}});
            }
            out_routing_cost = 0.0;
        } else {
            bool alt_route_success = false;
            auto sorted_alt_routings = sc.alternative_routings;
            std::sort(sorted_alt_routings.begin(), sorted_alt_routings.end(), [](const AlternativeRouting& a, const AlternativeRouting& b) {
                if (a.priority != b.priority) return a.priority < b.priority;
                return a.routing_cost < b.routing_cost;
            });

            for (const auto& alt : sorted_alt_routings) {
                bool current_alt_ok = true;
                std::vector<std::pair<size_t, double>> alt_allocs;
                for (const auto& cc : alt.constraints) {
                    uint32_t acid = cc.constraint_id;
                    if (acid >= shared_constraints.size()) {
                        current_alt_ok = false;
                        break;
                    }
                    double req_acap = cc.factor * demand_qty;

                    double alt_allocated = shared_constraints[acid].allocated_rates[start_day];
                    for (const auto& cap_alloc : temp_capacity_allocations) {
                        if (cap_alloc.first == acid && cap_alloc.second.first == start_day) {
                            alt_allocated += cap_alloc.second.second;
                        }
                    }
                    if (shared_constraints[acid].rates[start_day] - alt_allocated < req_acap) {
                        current_alt_ok = false;
                        break;
                    }
                    alt_allocs.push_back({acid, req_acap});
                }
                if (current_alt_ok) {
                    for (const auto& aa : alt_allocs) {
                        temp_capacity_allocations.push_back({aa.first, {start_day, aa.second}});
                    }
                    alt_route_success = true;
                    out_routing_cost = alt.routing_cost;
                    break;
                }
            }
            if (!alt_route_success) {
                return false;
            }
        }
    }

    if (part_id < part_bom_info.size()) {
        if (part_id < active_parts.size() && active_parts[part_id]) {
            return false;
        }
        ActivePartsGuard guard(active_parts, part_id);
        MixGroupGuard mix_guard(active_mix_groups);
        const auto& standard_bom_indices = part_bom_info[part_id].standard_bom_indices;
        const auto& alt_groups = part_bom_info[part_id].alt_groups;

        for (size_t bom_idx : standard_bom_indices) {
            const auto& bom = boms[bom_idx];
            int trans_lt = (parts[part_id].site != parts[bom.child_id].site) ? parts[bom.child_id].transshipment_lead_time : 0;
            int child_due_day = get_workday_offset_backward(start_day, trans_lt, parts[bom.child_id].planning_calendar, global_wc_daily_capacity);
            if (child_due_day < 0) return false;

            if (bom.eff_start_day >= 0 && child_due_day < bom.eff_start_day) return false;
            if (bom.eff_end_day >= 0 && child_due_day > bom.eff_end_day) return false;

            double child_qty = demand_qty * bom.per_qty * (1.0 + bom.scrap);
            if (bom.ltb_limit >= 0.0) {
                double temp_lt = 0.0;
                for (const auto& alloc : temp_ltb_allocations) {
                    if (alloc.first == bom_idx) temp_lt += alloc.second;
                }
                if (bom_ltb_consumed[bom_idx] + temp_lt + child_qty > bom.ltb_limit) return false;
            }

            int old_mix_group = -1;
            bool mix_group_modified = false;
            if (bom.mix_group_id >= 0) {
                auto it = active_mix_groups.find(part_id);
                if (it != active_mix_groups.end() && it->second != bom.mix_group_id) {
                    return false;
                }
                old_mix_group = (it != active_mix_groups.end()) ? it->second : -1;
                active_mix_groups[part_id] = bom.mix_group_id;
                mix_group_modified = true;
            }

            double child_dim_val = (bom.relation_op == static_cast<uint8_t>(RelationOp::PASS)) 
                                   ? dimension_val 
                                   : bom.target_dim_val;
            double child_routing_cost = 0.0;

            bool child_ok = reserve_atp_and_capacity_recursive(
                bom.child_id, child_due_day, child_qty, priority, child_dim_val,
                atp_supplies, shared_constraints, parts, boms,
                part_bom_info, source_constraints,
                temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                last_dim_val, child_routing_cost,
                active_parts,
                temp_po_splits,
                true,
                temp_wc_allocations,
                local_root_due_day,
                root_family_id,
                root_cust_group_id,
                root_region_id,
                wildcard_id,
                allotment_constraints,
                temp_allotment_allocations,
                temp_allotment_blocked,
                skip_po_quota
            );

            if (!child_ok) {
                if (mix_group_modified) {
                    if (old_mix_group == -1) active_mix_groups.erase(part_id);
                    else active_mix_groups[part_id] = old_mix_group;
                }
                return false;
            }

            if (bom.ltb_limit >= 0.0) {
                temp_ltb_allocations.push_back({bom_idx, child_qty});
            }
        }

        for (const auto& group_pair : alt_groups) {
            int gid = group_pair.first;
            const auto& group_bom_idxs = group_pair.second;

            bool is_class3 = false;
            for (size_t bom_idx : group_bom_idxs) {
                if (boms[bom_idx].alt_priority == 3) {
                    is_class3 = true;
                    break;
                }
            }

            if (is_class3) {
                struct Class3Cand {
                    size_t bom_idx;
                    double target_ratio;
                    double current_ratio;
                    double due_qty;
                    int child_due_day;
                };
                std::vector<Class3Cand> active_cands;
                for (size_t bom_idx : group_bom_idxs) {
                    const auto& bom = boms[bom_idx];
                    int trans_lt = (parts[part_id].site != parts[bom.child_id].site) ? parts[bom.child_id].transshipment_lead_time : 0;
                    int child_due_day = get_workday_offset_backward(start_day, trans_lt, parts[bom.child_id].planning_calendar, global_wc_daily_capacity);
                    if (child_due_day < 0) continue;

                    if (bom.eff_start_day >= 0 && child_due_day < bom.eff_start_day) continue;
                    if (bom.eff_end_day >= 0 && child_due_day > bom.eff_end_day) continue;

                    if (bom.mix_group_id >= 0) {
                        auto it = active_mix_groups.find(part_id);
                        if (it != active_mix_groups.end() && it->second != bom.mix_group_id) continue;
                    }

                    active_cands.push_back({bom_idx, bom.target_ratio, bom.target_ratio, 0.0, child_due_day});
                }

                double remaining_net = demand_qty;
                while (remaining_net > 0.0 && !active_cands.empty()) {
                    for (auto& cand : active_cands) {
                        cand.due_qty = cand.current_ratio * remaining_net;
                    }
                    std::sort(active_cands.begin(), active_cands.end(), [](const Class3Cand& a, const Class3Cand& b) {
                        return a.due_qty > b.due_qty;
                    });

                    auto best_it = active_cands.begin();
                    const auto& bom = boms[best_it->bom_idx];
                    double lot = bom.lot_size > 0.0 ? bom.lot_size : 1.0;
                    double req_qty_raw = best_it->due_qty * bom.per_qty * (1.0 + bom.scrap);
                    double req_qty = std::ceil(req_qty_raw / lot) * lot;

                    if (bom.ltb_limit >= 0.0) {
                        double temp_ltb = 0.0;
                        for (const auto& alloc : temp_ltb_allocations) {
                            if (alloc.first == best_it->bom_idx) temp_ltb += alloc.second;
                        }
                        if (bom_ltb_consumed[best_it->bom_idx] + temp_ltb + req_qty > bom.ltb_limit) {
                            active_cands.erase(best_it);
                            continue;
                        }
                    }

                    int old_mix_group = -1;
                    bool mix_group_modified = false;
                    if (bom.mix_group_id >= 0) {
                        auto it = active_mix_groups.find(part_id);
                        old_mix_group = (it != active_mix_groups.end()) ? it->second : -1;
                        active_mix_groups[part_id] = bom.mix_group_id;
                        mix_group_modified = true;
                    }

                    double child_dim_val = (bom.relation_op == static_cast<uint8_t>(RelationOp::PASS)) 
                                           ? dimension_val 
                                           : bom.target_dim_val;
                    double child_routing_cost = 0.0;

                    bool ok = false;
                    size_t temp_alloc_start = temp_allocations.size();
                    size_t temp_cap_start = temp_capacity_allocations.size();
    size_t temp_wc_start = temp_wc_allocations.size();
                    size_t temp_ltb_start = temp_ltb_allocations.size();
                    size_t temp_po_splits_start = temp_po_splits.size();

                    if (bom.relationship_type == "interchangeable") {
                        uint32_t primary_id = bom.child_id;
                        for (size_t idx : group_bom_idxs) {
                            if (boms[idx].alt_priority == 1) {
                                primary_id = boms[idx].child_id;
                                break;
                            }
                        }
                        double rem_qty = req_qty;
                        for (auto& node : atp_supplies[bom.child_id]) {
                            if (node.supply_type == "SR" || node.supply_type == "On-Hand") {
                                double avail = node.qty - node.allocated_qty;
                                for (size_t k = 0; k < temp_allocations.size(); ++k) {
                                    if (temp_allocations[k] == &node) avail -= temp_alloc_qty[k];
                                }
                                if (avail > 0.0 && node.available_day <= best_it->child_due_day) {
                                    double allocated = std::min(rem_qty, avail);
                                    temp_allocations.push_back(&node);
                                    temp_alloc_qty.push_back(allocated);
                                    rem_qty -= allocated;
                                    if (rem_qty <= 0.0) break;
                                }
                            }
                        }
                        if (rem_qty <= 0.0) {
                            ok = true;
                        } else {

                            ok = reserve_atp_and_capacity_recursive(
                                primary_id, best_it->child_due_day, rem_qty, priority, child_dim_val,
                                atp_supplies, shared_constraints, parts, boms,
                                part_bom_info, source_constraints,
                                temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                                preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                                last_dim_val, child_routing_cost,
                                active_parts,
                                temp_po_splits,
                                true,
                                temp_wc_allocations,
                                local_root_due_day,
                                root_family_id,
                                root_cust_group_id,
                                root_region_id,
                                wildcard_id,
                                allotment_constraints,
                                temp_allotment_allocations,
                                temp_allotment_blocked,
                                skip_po_quota
                            );
                        }
                    } else {

                        ok = reserve_atp_and_capacity_recursive(
                            bom.child_id, best_it->child_due_day, req_qty, priority, child_dim_val,
                            atp_supplies, shared_constraints, parts, boms,
                            part_bom_info, source_constraints,
                            temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                            preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                            last_dim_val, child_routing_cost,
                            active_parts,
                            temp_po_splits,
                            true,
                            temp_wc_allocations,
                            local_root_due_day,
                            root_family_id,
                            root_cust_group_id,
                            root_region_id,
                            wildcard_id,
                            allotment_constraints,
                            temp_allotment_allocations,
                            temp_allotment_blocked,
                            skip_po_quota
                        );
                    }

                    if (ok) {
                        double parent_consumed = req_qty / (bom.per_qty * (1.0 + bom.scrap));
                        if (parent_consumed > remaining_net) parent_consumed = remaining_net;
                        remaining_net -= parent_consumed;
                        if (bom.ltb_limit >= 0.0) {
                            temp_ltb_allocations.push_back({best_it->bom_idx, req_qty});
                        }
                        active_cands.erase(best_it);
                    } else {
                        temp_allocations.resize(temp_alloc_start);
                        temp_alloc_qty.resize(temp_alloc_start);
                        temp_capacity_allocations.resize(temp_cap_start);
                        temp_wc_allocations.resize(temp_wc_start);
                        temp_ltb_allocations.resize(temp_ltb_start);
                        temp_po_splits.resize(temp_po_splits_start);
                        if (mix_group_modified) {
                            if (old_mix_group == -1) active_mix_groups.erase(part_id);
                            else active_mix_groups[part_id] = old_mix_group;
                        }
                        active_cands.erase(best_it);
                    }

                    if (remaining_net <= 0.0 || active_cands.empty()) break;

                    double sum_ratios = 0.0;
                    for (const auto& cand : active_cands) sum_ratios += cand.due_qty;
                    if (sum_ratios > 0.0) {
                        for (auto& cand : active_cands) cand.current_ratio = cand.due_qty / sum_ratios;
                    }
                }

                if (remaining_net > 0.0) return false;
            } else {
                struct SourcingCand {
                    size_t bom_idx;
                    double score;
                    double child_qty;
                    int child_due_day;
                };
                std::vector<SourcingCand> active_cands;

                for (size_t bom_idx : group_bom_idxs) {
                    const auto& bom = boms[bom_idx];
                    int trans_lt = (parts[part_id].site != parts[bom.child_id].site) ? parts[bom.child_id].transshipment_lead_time : 0;
                    int child_due_day = get_workday_offset_backward(start_day, trans_lt, parts[bom.child_id].planning_calendar, global_wc_daily_capacity);
                    if (child_due_day < 0) continue;

                    if (bom.eff_start_day >= 0 && child_due_day < bom.eff_start_day) continue;
                    if (bom.eff_end_day >= 0 && child_due_day > bom.eff_end_day) continue;

                    double child_qty = demand_qty * bom.per_qty * (1.0 + bom.scrap);
                    if (bom.ltb_limit >= 0.0) {
                        double temp_ltb = 0.0;
                        for (const auto& alloc : temp_ltb_allocations) {
                            if (alloc.first == bom_idx) temp_ltb += alloc.second;
                        }
                        if (bom_ltb_consumed[bom_idx] + temp_ltb + child_qty > bom.ltb_limit) continue;
                    }

                    if (bom.mix_group_id >= 0) {
                        auto it = active_mix_groups.find(part_id);
                        if (it != active_mix_groups.end() && it->second != bom.mix_group_id) continue;
                    }

                    double score = 0.0;
                    if (preference_mode == "N") {
                        int earliest_avail = child_due_day;
                        if (!atp_supplies[bom.child_id].empty()) {
                            earliest_avail = atp_supplies[bom.child_id].front().available_day;
                        }
                        score = earliest_avail;
                    } else if (bom.alt_priority == 1) {
                        double total_hist = 0.0;
                        for (size_t idx : group_bom_idxs) total_hist += boms[idx].historical_qty;
                        double cur_demand = total_hist + child_qty;
                        double due = cur_demand * bom.target_ratio;
                        score = std::abs(bom.historical_qty - due);
                    } else if (bom.alt_priority == 2) {
                        double ratio = bom.target_ratio > 0.0 ? bom.target_ratio : 1.0;
                        score = bom.historical_qty / ratio;
                    } else {
                        score = bom.alt_priority;
                    }

                    active_cands.push_back({bom_idx, score, child_qty, child_due_day});
                }

                if (preference_mode == "N" || (group_bom_idxs.size() > 0 && boms[group_bom_idxs[0]].alt_priority != 1)) {
                    std::sort(active_cands.begin(), active_cands.end(), [](const SourcingCand& a, const SourcingCand& b) {
                        return a.score < b.score;
                    });
                } else {
                    std::sort(active_cands.begin(), active_cands.end(), [](const SourcingCand& a, const SourcingCand& b) {
                        return a.score > b.score;
                    });
                }

                bool group_ok = false;
                for (const auto& cand : active_cands) {
                    const auto& bom = boms[cand.bom_idx];

                    int old_mix_group = -1;
                    bool mix_group_modified = false;
                    if (bom.mix_group_id >= 0) {
                        auto it = active_mix_groups.find(part_id);
                        if (it != active_mix_groups.end() && it->second != bom.mix_group_id) continue;
                        old_mix_group = (it != active_mix_groups.end()) ? it->second : -1;
                        active_mix_groups[part_id] = bom.mix_group_id;
                        mix_group_modified = true;
                    }

                    double child_dim_val = (bom.relation_op == static_cast<uint8_t>(RelationOp::PASS)) 
                                           ? dimension_val 
                                           : bom.target_dim_val;
                    double child_routing_cost = 0.0;

                    size_t temp_alloc_start = temp_allocations.size();
                    size_t temp_cap_start = temp_capacity_allocations.size();
    size_t temp_wc_start = temp_wc_allocations.size();
                    size_t temp_ltb_start = temp_ltb_allocations.size();
                    size_t temp_po_splits_start = temp_po_splits.size();

                    bool ok = false;
                    if (bom.relationship_type == "interchangeable") {
                        uint32_t primary_id = bom.child_id;
                        for (size_t idx : group_bom_idxs) {
                            if (boms[idx].alt_priority == 1) {
                                primary_id = boms[idx].child_id;
                                break;
                            }
                        }
                        double rem_qty = cand.child_qty;
                        for (auto& node : atp_supplies[bom.child_id]) {
                            if (node.supply_type == "SR" || node.supply_type == "On-Hand") {
                                double avail = node.qty - node.allocated_qty;
                                for (size_t k = 0; k < temp_allocations.size(); ++k) {
                                    if (temp_allocations[k] == &node) avail -= temp_alloc_qty[k];
                                }
                                if (avail > 0.0 && node.available_day <= cand.child_due_day) {
                                    double allocated = std::min(rem_qty, avail);
                                    temp_allocations.push_back(&node);
                                    temp_alloc_qty.push_back(allocated);
                                    rem_qty -= allocated;
                                    if (rem_qty <= 0.0) break;
                                }
                            }
                        }
                        if (rem_qty <= 0.0) {
                            ok = true;
                        } else {

                            ok = reserve_atp_and_capacity_recursive(
                                primary_id, cand.child_due_day, rem_qty, priority, child_dim_val,
                                atp_supplies, shared_constraints, parts, boms,
                                part_bom_info, source_constraints,
                                temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                                preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                                last_dim_val, child_routing_cost,
                                active_parts,
                                temp_po_splits,
                                true,
                                temp_wc_allocations,
                                local_root_due_day,
                                root_family_id,
                                root_cust_group_id,
                                root_region_id,
                                wildcard_id,
                                allotment_constraints,
                                temp_allotment_allocations,
                                temp_allotment_blocked,
                                skip_po_quota
                            );
                        }
                    } else {

                        ok = reserve_atp_and_capacity_recursive(
                            bom.child_id, cand.child_due_day, cand.child_qty, priority, child_dim_val,
                            atp_supplies, shared_constraints, parts, boms,
                            part_bom_info, source_constraints,
                            temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                            preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                            last_dim_val, child_routing_cost,
                            active_parts,
                            temp_po_splits,
                            true,
                            temp_wc_allocations,
                            local_root_due_day,
                            root_family_id,
                            root_cust_group_id,
                            root_region_id,
                            wildcard_id,
                            allotment_constraints,
                            temp_allotment_allocations,
                            temp_allotment_blocked,
                            skip_po_quota
                        );
                    }

                    if (ok) {
                        if (bom.ltb_limit >= 0.0) {
                            temp_ltb_allocations.push_back({cand.bom_idx, cand.child_qty});
                        }
                        group_ok = true;
                        break;
                    } else {
                        temp_allocations.resize(temp_alloc_start);
                        temp_alloc_qty.resize(temp_alloc_start);
                        temp_capacity_allocations.resize(temp_cap_start);
                        temp_wc_allocations.resize(temp_wc_start);
                        temp_ltb_allocations.resize(temp_ltb_start);
                        temp_po_splits.resize(temp_po_splits_start);
                        if (mix_group_modified) {
                            if (old_mix_group == -1) active_mix_groups.erase(part_id);
                            else active_mix_groups[part_id] = old_mix_group;
                        }
                }
            }

            if (!group_ok) return false;
        }
    }
    mix_guard.commit();
}

    // 成功返回前收集此次分配事务中的所有 Planned-Order 消纳记录 (子节点已在其提早返回中收集，此处无需重复收集)

    allotment_guard.commit();
    return true;
}


void run_dbd_dispatch_engine(
    const std::vector<PartSiteRecord>& parts,
    const std::vector<FlatBomItem>& boms,
    const std::vector<PlannedOrder>& const_ipc_planned_orders,
    const std::vector<IndependentDemand>& demands,
    std::vector<PlannedOrder>& out_scheduled_orders,
    std::vector<double>& out_allocated_rates,
    std::vector<double>& out_order_capacities,
    std::vector<double>& out_order_routing_costs,
    const std::string& solver_mode,
    uint32_t wildcard_id,
    std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash>& allotment_constraints
) {
    std::vector<PlannedOrder> ipc_planned_orders = const_ipc_planned_orders;
    std::vector<bool> active_parts(parts.size(), false);
    
    // Initialize allotment constraints fast lookup cache
    allotment_constrained_parts.assign(parts.size(), false);
    if (&allotment_constraints != nullptr && !allotment_constraints.empty()) {
        for (const auto& pair : allotment_constraints) {
            if (pair.first.part_id < allotment_constrained_parts.size()) {
                allotment_constrained_parts[pair.first.part_id] = true;
            }
        }
    }

    std::cout << "\n[点火] [DBD 派程点火] 开始执行双表融合 MCDS 微观时空产能与时序 ATP 派程调度..." << std::endl;

    int timeline_days = TIMELINE_DAYS;
    out_scheduled_orders = ipc_planned_orders;
    out_order_capacities.assign(ipc_planned_orders.size(), 0.0);
    out_order_routing_costs.assign(ipc_planned_orders.size(), 0.0);

    out_scheduled_orders.reserve(ipc_planned_orders.size() * 3);
    out_order_capacities.reserve(ipc_planned_orders.size() * 3);
    out_order_routing_costs.reserve(ipc_planned_orders.size() * 3);

    for (size_t i = 0; i < ipc_planned_orders.size(); ++i) {
        out_scheduled_orders[i].original_lbl_start = ipc_planned_orders[i].start_day;
        out_scheduled_orders[i].original_lbl_finish = ipc_planned_orders[i].finish_day;
    }

    size_t num_constraints = 3;
    std::vector<std::vector<double>> last_dim_val(num_constraints, std::vector<double>(timeline_days, -1.0));

    std::vector<std::vector<size_t>> local_parent_to_bom(parts.size());
    for (size_t i = 0; i < boms.size(); ++i) {
        if (boms[i].parent_id < parts.size()) {
            local_parent_to_bom[boms[i].parent_id].push_back(i);
        }
    }

    std::vector<PartBomInfo> part_bom_info(parts.size());
    for (size_t p = 0; p < parts.size(); ++p) {
        if (p >= local_parent_to_bom.size()) continue;
        std::unordered_map<int, std::vector<size_t>> alt_groups_map;
        for (size_t bom_idx : local_parent_to_bom[p]) {
            const auto& bom = boms[bom_idx];
            if (bom.alt_group_id == -1) {
                part_bom_info[p].standard_bom_indices.push_back(bom_idx);
            } else {
                alt_groups_map[bom.alt_group_id].push_back(bom_idx);
            }
        }
        for (const auto& pair : alt_groups_map) {
            part_bom_info[p].alt_groups.push_back(pair);
        }
    }

    double base_cap = 1000.0;
    if (demands.size() > 2000) {
        base_cap = demands.size() * 50.0;
    }
    std::vector<ConstraintRecord> shared_constraints(num_constraints);
    shared_constraints[0] = { 0, "LINE_FINISHED", "Constrained", std::vector<double>(timeline_days, base_cap), std::vector<double>(timeline_days, 0.0) };
    shared_constraints[1] = { 1, "LINE_SEMI", "Constrained", std::vector<double>(timeline_days, base_cap * 2.0), std::vector<double>(timeline_days, 0.0) };
    shared_constraints[2] = { 2, "LINE_RAW", "Constrained", std::vector<double>(timeline_days, base_cap * 5.0), std::vector<double>(timeline_days, 0.0) };

    std::vector<SourceConstraintRecord> source_constraints(parts.size());
    for (size_t i = 0; i < parts.size(); ++i) {
        uint32_t cid = 2;
        if (parts[i].part_type == "FINISHED") {
            cid = 0;
        } else if (parts[i].part_type == "SEMI") {
            cid = 1;
        }
        source_constraints[i] = { static_cast<uint32_t>(i), cid, 1.0, 10.0, 0.0 };
    }

    std::vector<std::vector<size_t>> po_by_part(parts.size());
    for (size_t i = 0; i < ipc_planned_orders.size(); ++i) {
        po_by_part[ipc_planned_orders[i].part_id].push_back(i);
    }

    std::vector<uint64_t> po_priority(ipc_planned_orders.size(), 18446744073709551615ULL);
    std::vector<std::string> po_preference_mode(ipc_planned_orders.size(), "N");
    std::vector<uint32_t> po_family_id(ipc_planned_orders.size(), uint32_t(-1));
    std::vector<uint32_t> po_cust_group_id(ipc_planned_orders.size(), uint32_t(-1));
    std::vector<uint32_t> po_region_id(ipc_planned_orders.size(), uint32_t(-1));

    std::vector<std::vector<size_t>> demands_by_part(parts.size());
    for (size_t i = 0; i < demands.size(); ++i) {
        demands_by_part[demands[i].part_id].push_back(i);
    }

    for (size_t p = 0; p < parts.size(); ++p) {
        if (parts[p].part_type != "FINISHED") continue;

        auto& my_demands = demands_by_part[p];
        std::sort(my_demands.begin(), my_demands.end(), [&](size_t x, size_t y) {
            uint64_t pri_x = demands[x].composite_priority;
            if (pri_x == 0) {
                pri_x = encode_composite_priority(demands[x].status == "COMMITTED", demands[x].customer_tier, demands[x].due_day, demands[x].priority, demands[x].revenue);
            }
            uint64_t pri_y = demands[y].composite_priority;
            if (pri_y == 0) {
                pri_y = encode_composite_priority(demands[y].status == "COMMITTED", demands[y].customer_tier, demands[y].due_day, demands[y].priority, demands[y].revenue);
            }
            if (pri_x != pri_y) return pri_x < pri_y;
            return x < y;
        });

        auto& my_pos = po_by_part[p];
        std::sort(my_pos.begin(), my_pos.end(), [&](size_t x, size_t y) {
            return ipc_planned_orders[x].finish_day < ipc_planned_orders[y].finish_day;
        });

        std::vector<double> remaining_po_qty(my_pos.size());
        for (size_t i = 0; i < my_pos.size(); ++i) {
            remaining_po_qty[i] = ipc_planned_orders[my_pos[i]].qty;
        }

        size_t po_idx = 0;
        for (size_t d_idx : my_demands) {
            double d_qty = demands[d_idx].qty;
            uint64_t d_pri = demands[d_idx].composite_priority;
            if (d_pri == 0) {
                d_pri = encode_composite_priority(demands[d_idx].status == "COMMITTED", demands[d_idx].customer_tier, demands[d_idx].due_day, demands[d_idx].priority, demands[d_idx].revenue);
            }
            const std::string& d_pref = demands[d_idx].preference_mode;

            while (d_qty > 0.0 && po_idx < my_pos.size()) {
                size_t po_global_idx = my_pos[po_idx];
                double alloc = std::min(d_qty, remaining_po_qty[po_idx]);
                if (alloc > 0.0) {
                    remaining_po_qty[po_idx] -= alloc;
                    d_qty -= alloc;
                    if (d_pri < po_priority[po_global_idx]) {
                        po_priority[po_global_idx] = d_pri;
                        po_preference_mode[po_global_idx] = d_pref;
                        po_family_id[po_global_idx] = demands[d_idx].family_id;
                        po_cust_group_id[po_global_idx] = demands[d_idx].cust_group_id;
                        po_region_id[po_global_idx] = demands[d_idx].region_id;
                    }
                }
                if (remaining_po_qty[po_idx] <= 0.0) {
                    po_idx++;
                }
            }
        }
    }

    int max_llc = 0;
    for (const auto& p : parts) {
        if (static_cast<int>(p.low_level_code) > max_llc) {
            max_llc = p.low_level_code;
        }
    }

    std::vector<std::vector<uint32_t>> parts_by_level(max_llc + 1);
    for (const auto& p : parts) {
        parts_by_level[p.low_level_code].push_back(p.part_id);
    }

    for (int lvl = 0; lvl < max_llc; ++lvl) {
        for (uint32_t parent_id : parts_by_level[lvl]) {
            auto& parent_pos = po_by_part[parent_id];
            if (parent_pos.empty()) continue;

            std::vector<size_t> child_bom_idxs;
            if (parent_id < local_parent_to_bom.size()) {
                child_bom_idxs = local_parent_to_bom[parent_id];
            }
            if (child_bom_idxs.empty()) continue;

            for (size_t bom_idx : child_bom_idxs) {
                const auto& bom = boms[bom_idx];
                uint32_t child_id = bom.child_id;

                struct ChildDemand {
                    int day;
                    double qty;
                    uint64_t priority;
                    std::string preference_mode;
                    uint32_t family_id;
                    uint32_t cust_group_id;
                    uint32_t region_id;
                };
                std::vector<ChildDemand> child_demands;
                for (size_t p_po_idx : parent_pos) {
                    uint64_t pri = po_priority[p_po_idx];
                    if (pri == 18446744073709551615ULL) continue;
                    const std::string& pref = po_preference_mode[p_po_idx];
                    uint32_t fam = po_family_id[p_po_idx];
                    uint32_t grp = po_cust_group_id[p_po_idx];
                    uint32_t reg = po_region_id[p_po_idx];

                    const auto& p_po = ipc_planned_orders[p_po_idx];
                    double c_qty = p_po.qty * bom.per_qty * (1.0 + bom.scrap);
                    child_demands.push_back({p_po.start_day, c_qty, pri, pref, fam, grp, reg});
                }

                if (child_demands.empty()) continue;

                std::sort(child_demands.begin(), child_demands.end(), [](const ChildDemand& x, const ChildDemand& y) {
                    if (x.day != y.day) return x.day < y.day;
                    return x.priority < y.priority;
                });

                auto& child_pos = po_by_part[child_id];
                std::sort(child_pos.begin(), child_pos.end(), [&](size_t x, size_t y) {
                    return ipc_planned_orders[x].finish_day < ipc_planned_orders[y].finish_day;
                });

                std::vector<double> remaining_child_po_qty(child_pos.size());
                for (size_t i = 0; i < child_pos.size(); ++i) {
                    remaining_child_po_qty[i] = ipc_planned_orders[child_pos[i]].qty;
                }

                size_t c_po_idx = 0;
                for (const auto& cd : child_demands) {
                    double cd_qty = cd.qty;
                    uint64_t cd_pri = cd.priority;
                    const std::string& cd_pref = cd.preference_mode;

                    while (cd_qty > 0.0 && c_po_idx < child_pos.size()) {
                        size_t child_po_global_idx = child_pos[c_po_idx];
                        double alloc = std::min(cd_qty, remaining_child_po_qty[c_po_idx]);
                        if (alloc > 0.0) {
                            remaining_child_po_qty[c_po_idx] -= alloc;
                            cd_qty -= alloc;
                            if (cd_pri < po_priority[child_po_global_idx]) {
                                po_priority[child_po_global_idx] = cd_pri;
                                po_preference_mode[child_po_global_idx] = cd_pref;
                                po_family_id[child_po_global_idx] = cd.family_id;
                                po_cust_group_id[child_po_global_idx] = cd.cust_group_id;
                                po_region_id[child_po_global_idx] = cd.region_id;
                            }
                        }
                        if (remaining_child_po_qty[c_po_idx] <= 0.0) {
                            c_po_idx++;
                        }
                    }
                }
            }
        }
    }

    if (solver_mode == "itp") {
        std::cout << "[ITP 模式] 正在进行 Gating 瓶颈检测与战略优先/公平公正容量配额缩减..." << std::endl;
        std::vector<double> constraint_avail_cap(num_constraints, 0.0);
        for (size_t c = 0; c < num_constraints; ++c) {
            for (int day = 0; day < timeline_days; ++day) {
                constraint_avail_cap[c] += shared_constraints[c].rates[day];
            }
        }

        std::vector<double> total_vvip_load(num_constraints, 0.0);
        std::vector<double> total_non_vvip_load(num_constraints, 0.0);

        for (size_t i = 0; i < ipc_planned_orders.size(); ++i) {
            const auto& po = ipc_planned_orders[i];
            if (po.part_id >= parts.size()) continue;
            uint32_t cid = source_constraints[po.part_id].constraint_id;
            if (cid >= num_constraints) continue;
            double factor = source_constraints[po.part_id].constraint_factor;
            double load = po.qty * factor;

            uint64_t tier_val = (po_priority[i] >> 60) & 3ULL;
            if (tier_val == 0) {
                total_vvip_load[cid] += load;
            } else {
                total_non_vvip_load[cid] += load;
            }
        }

        for (size_t c = 0; c < num_constraints; ++c) {
            double total_load = total_vvip_load[c] + total_non_vvip_load[c];
            double avail_cap = constraint_avail_cap[c];
            if (total_load > avail_cap) {
                std::cout << "  -> 发现 Gating 瓶颈！约束: " << shared_constraints[c].constraint_code 
                          << ", 需求容量: " << total_load << ", 可用容量: " << avail_cap << std::endl;

                double vvip_satisfied_load = std::min(total_vvip_load[c], avail_cap);
                double rem_cap = std::max(0.0, avail_cap - vvip_satisfied_load);
                double scale_factor = total_non_vvip_load[c] > 0.0 ? rem_cap / total_non_vvip_load[c] : 0.0;
                double vvip_scale_factor = total_vvip_load[c] > 0.0 ? avail_cap / total_vvip_load[c] : 0.0;

                std::cout << "     * 战略VVIP容量: " << total_vvip_load[c] << " -> 满足: " << vvip_satisfied_load
                          << " (缩放因子: " << (total_vvip_load[c] > avail_cap ? vvip_scale_factor : 1.0) << ")" << std::endl;
                std::cout << "     * 公平公正非VVIP容量: " << total_non_vvip_load[c] << " -> 满足: " << rem_cap
                          << " (缩放因子: " << scale_factor << ")" << std::endl;

                for (size_t i = 0; i < ipc_planned_orders.size(); ++i) {
                    auto& po = ipc_planned_orders[i];
                    if (po.part_id >= parts.size()) continue;
                    uint32_t cid = source_constraints[po.part_id].constraint_id;
                    if (cid != c) continue;

                    uint64_t tier_val = (po_priority[i] >> 60) & 3ULL;
                    if (tier_val == 0) {
                        if (total_vvip_load[c] > avail_cap) {
                            po.qty *= vvip_scale_factor;
                        }
                    } else {
                        po.qty *= scale_factor;
                    }

                    if (parts[po.part_id].round_to_integer) {
                        po.qty = std::round(po.qty);
                    }
                }
            }
        }
        for (size_t i = 0; i < ipc_planned_orders.size(); ++i) {
            out_scheduled_orders[i].qty = ipc_planned_orders[i].qty;
        }
    }

    std::vector<std::vector<ATPSupplyNode>> atp_supplies(parts.size());
    for (size_t i = 0; i < parts.size(); ++i) {
        if (parts[i].on_hand > 0.0 && parts[i].on_hand_type != "Exclude") {
            ATPSupplyNode node = {"OH_" + parts[i].part_code, "On-Hand", static_cast<uint32_t>(i), 0, parts[i].on_hand, 0.0, 99999ULL, -1};
            atp_supplies[i].push_back(node);
        }
        if (parts[i].ipc_scheduled_receipt > 0.0) {
            ATPSupplyNode node = {"SR_" + parts[i].part_code, "SR", static_cast<uint32_t>(i), 0, parts[i].ipc_scheduled_receipt, 0.0, 99999ULL, -1};
            atp_supplies[i].push_back(node);
        }
    }

    for (size_t i = 0; i < ipc_planned_orders.size(); ++i) {
        const auto& po = ipc_planned_orders[i];
        ATPSupplyNode node = {"PO_" + std::to_string(i), "Planned-Order", po.part_id, po.finish_day, po.qty, 0.0, po_priority[i], static_cast<int>(i)};
        atp_supplies[po.part_id].push_back(node);
    }

    for (size_t i = 0; i < parts.size(); ++i) {
        std::sort(atp_supplies[i].begin(), atp_supplies[i].end(), [](const ATPSupplyNode& a, const ATPSupplyNode& b) {
            return a.available_day < b.available_day;
        });
    }

    struct SchedOrderWrapper {
        size_t original_index;
        uint64_t priority;
        uint32_t part_id;
        double qty;
        int start_day;
        int finish_day;
        double dimension_val;
        std::string preference_mode;
        uint32_t family_id;
        uint32_t cust_group_id;
        uint32_t region_id;
    };

    std::vector<SchedOrderWrapper> global_orders;
    for (size_t i = 0; i < ipc_planned_orders.size(); ++i) {
        global_orders.push_back({
            i,
            po_priority[i],
            ipc_planned_orders[i].part_id,
            ipc_planned_orders[i].qty,
            ipc_planned_orders[i].start_day,
            ipc_planned_orders[i].finish_day,
            ipc_planned_orders[i].dimension_val,
            po_preference_mode[i],
            po_family_id[i],
            po_cust_group_id[i],
            po_region_id[i]
        });
    }

    std::sort(global_orders.begin(), global_orders.end(), [](const SchedOrderWrapper& a, const SchedOrderWrapper& b) {
        if (a.priority != b.priority) return a.priority < b.priority;
        if (a.finish_day != b.finish_day) return a.finish_day < b.finish_day;
        return a.original_index < b.original_index;
    });

    std::vector<double> bom_ltb_consumed(boms.size(), 0.0);

    // Reusable zero-allocation vectors for DBD dispatch engine
    std::vector<ATPSupplyNode*> temp_allocations;
    std::vector<double> temp_alloc_qty;
    std::vector<std::pair<size_t, std::pair<int, double>>> temp_capacity_allocations;
    std::vector<std::pair<size_t, double>> temp_ltb_allocations;
    std::unordered_map<uint32_t, int> active_mix_groups;
    std::vector<PlannedOrderSplit> temp_po_splits;
    std::vector<double> po_allocated_qty(ipc_planned_orders.size() * 5, 0.0);
    std::vector<std::pair<AllotmentState*, double>> temp_allotment_allocations;
    std::vector<std::pair<AllotmentState*, double>> temp_allotment_blocked;

    for (size_t g_idx = 0; g_idx < global_orders.size(); ++g_idx) {
        const auto wrapper = global_orders[g_idx];
        size_t order_idx = wrapper.original_index;

        if (parts[out_scheduled_orders[order_idx].part_id].part_type != "FINISHED") {
            continue;
        }

        double allocated_already = 0.0;
        if (order_idx < po_allocated_qty.size()) {
            allocated_already = po_allocated_qty[order_idx];
        }
        double qty = out_scheduled_orders[order_idx].qty - allocated_already;
        if (qty <= 0.0) {
            out_scheduled_orders[order_idx].qty = 0.0;
            continue;
        }

        int original_due = out_scheduled_orders[order_idx].finish_day;
        uint32_t part_id = out_scheduled_orders[order_idx].part_id;

        bool scheduled_successfully = false;
        int max_slide_day = timeline_days;
        for (int d = original_due; d < max_slide_day; ++d) {
            double target_qty = qty;
            bool need_split = false;

            // Finished Good Allotment Check at root level
            if (wrapper.family_id != uint32_t(-1) && !allotment_constraints.empty()) {
                if (part_id < allotment_constrained_parts.size() && allotment_constrained_parts[part_id]) {
                    AllotmentState* m_state = find_matching_allotment(part_id, d, wrapper.family_id, wrapper.cust_group_id, wrapper.region_id, wildcard_id, allotment_constraints);
                    double limit = (m_state != nullptr) ? m_state->limit : 0.0;
                    double consumed = (m_state != nullptr) ? m_state->consumed : 0.0;
                    double avail = limit - consumed;
                    if (avail < 0.0) avail = 0.0;

                    if (qty > avail) {
                        if (avail <= 0.0) {
                            continue; // No allotment left on this day, slide
                        } else {
                            target_qty = avail;
                            need_split = true;
                        }
                    }
                }
            }

            // Check available capacity on Finished Good Line (using start_day offset)
            uint32_t cid = source_constraints[part_id].constraint_id;
            if (cid >= shared_constraints.size()) continue;
            double setup_time = source_constraints[part_id].before_fixed_factor;
            double factor = source_constraints[part_id].constraint_factor;
            double clean_up = source_constraints[part_id].after_fixed_factor;

            double lead_time = parts[part_id].lead_time + target_qty * parts[part_id].run_rate;
            int start_day = get_workday_offset_backward(d, static_cast<int>(std::ceil(lead_time)), parts[part_id].planning_calendar, global_wc_daily_capacity);
            if (start_day < parts[part_id].time_fence_days) continue;

            if (cid < last_dim_val.size() && start_day >= 0 && start_day < static_cast<int>(last_dim_val[cid].size()) && last_dim_val[cid][start_day] == wrapper.dimension_val) {
                setup_time = 0.0;
            }

            double current_allocated = shared_constraints[cid].allocated_rates[start_day];
            double avail_cap = shared_constraints[cid].rates[start_day] - current_allocated;

            double required_cap = setup_time + target_qty * factor + clean_up;
            bool ok = false;

            if (avail_cap < required_cap && demands.size() >= 1000) {
                double possible_qty = (avail_cap - setup_time - clean_up) / factor;
                if (possible_qty >= 0.1) {
                    target_qty = std::min(target_qty, possible_qty);
                    need_split = true;
                } else {
                    continue; // Fast skip to next day
                }
            }

            temp_allocations.clear();
            temp_alloc_qty.clear();
            temp_capacity_allocations.clear();
            temp_ltb_allocations.clear();
            active_mix_groups.clear();
            temp_po_splits.clear();
            temp_allotment_allocations.clear();
            temp_allotment_blocked.clear();
            double chosen_routing_cost = 0.0;

            ok = reserve_atp_and_capacity_recursive(
                part_id, d, target_qty, wrapper.priority, wrapper.dimension_val,
                atp_supplies, shared_constraints, parts, boms,
                part_bom_info, source_constraints,
                temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                wrapper.preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                last_dim_val, chosen_routing_cost,
                active_parts,
                temp_po_splits,
                false,
                dummy_wc_allocs,
                wrapper.finish_day,
                wrapper.family_id, wrapper.cust_group_id, wrapper.region_id, wildcard_id,
                allotment_constraints,
                temp_allotment_allocations,
                temp_allotment_blocked
            );

            if (!ok && wrapper.priority < 18446744073709551615ULL && demands.size() < 1000) {
                double lead_time = parts[part_id].lead_time + target_qty * parts[part_id].run_rate;
                int start_day = get_workday_offset_backward(d, static_cast<int>(std::ceil(lead_time)), parts[part_id].planning_calendar, global_wc_daily_capacity);
                uint32_t my_cid = source_constraints[part_id].constraint_id;

                size_t preempt_target = size_t(-1);
                for (size_t h_idx = 0; h_idx < out_scheduled_orders.size(); ++h_idx) {
                    if (h_idx == order_idx) continue;
                    const auto& h_order = out_scheduled_orders[h_idx];

                    if (h_order.finish_day == d && po_priority[h_idx] > wrapper.priority) {
                        preempt_target = h_idx;
                        break;
                    }
                }

                if (preempt_target != size_t(-1)) {
                    int h_new_due = d - 1;
                    double h_lt = parts[out_scheduled_orders[preempt_target].part_id].lead_time + out_scheduled_orders[preempt_target].qty * parts[out_scheduled_orders[preempt_target].part_id].run_rate;
                    if (h_new_due >= static_cast<int>(std::ceil(h_lt))) {
                        uint32_t h_cid = source_constraints[out_scheduled_orders[preempt_target].part_id].constraint_id;
                        if (h_cid >= shared_constraints.size()) continue;
                        double h_cap = out_order_capacities[preempt_target];
                        int h_start = d - static_cast<int>(std::ceil(h_lt));
                        shared_constraints[h_cid].allocated_rates[h_start] -= h_cap;

                        std::vector<ATPSupplyNode*> h_alloc;
                        std::vector<double> h_alloc_qty;
                        std::vector<std::pair<size_t, std::pair<int, double>>> h_cap_alloc;
                        std::vector<std::pair<size_t, double>> h_ltb_alloc;
                        std::unordered_map<uint32_t, int> h_mix;
                        double h_chosen_routing_cost = 0.0;

                        std::vector<PlannedOrderSplit> h_po_splits;
                        std::vector<std::pair<AllotmentState*, double>> h_allotment_allocations;
                        std::vector<std::pair<AllotmentState*, double>> h_allotment_blocked;

                        bool h_shift_ok = reserve_atp_and_capacity_recursive(
                            out_scheduled_orders[preempt_target].part_id, h_new_due, out_scheduled_orders[preempt_target].qty,
                            po_priority[preempt_target], out_scheduled_orders[preempt_target].dimension_val,
                            atp_supplies, shared_constraints, parts, boms,
                            part_bom_info, source_constraints, h_alloc, h_alloc_qty, h_cap_alloc,
                            po_preference_mode[preempt_target], bom_ltb_consumed, h_ltb_alloc, h_mix,
                            last_dim_val, h_chosen_routing_cost,
                            active_parts,
                            h_po_splits,
                            false,
                            dummy_wc_allocs,
                            out_scheduled_orders[preempt_target].original_lbl_finish,
                            po_family_id[preempt_target], po_cust_group_id[preempt_target], po_region_id[preempt_target], wildcard_id,
                            allotment_constraints,
                            h_allotment_allocations,
                            h_allotment_blocked
                        );

                        if (h_shift_ok) {
                            for (const auto& alloc : h_allotment_allocations) {
                                alloc.first->consumed += alloc.second;
                            }
                            for (const auto& blk : h_allotment_blocked) {
                                blk.first->blocked += blk.second;
                            }
                            for (size_t a = 0; a < h_alloc.size(); ++a) {
                                h_alloc[a]->allocated_qty += h_alloc_qty[a];
                                if (h_alloc[a]->supply_type == "Planned-Order" && h_alloc[a]->planned_order_index >= 0) {
                                    size_t po_idx = static_cast<size_t>(h_alloc[a]->planned_order_index);
                                    if (po_idx < po_allocated_qty.size()) {
                                        po_allocated_qty[po_idx] += h_alloc_qty[a];
                                    }
                                }
                            }
                            for (const auto& cap_alloc : h_cap_alloc) {
                                shared_constraints[cap_alloc.first].allocated_rates[cap_alloc.second.first] += cap_alloc.second.second;
                                if (cap_alloc.first == h_cid && cap_alloc.second.first == (h_new_due - static_cast<int>(std::ceil(h_lt)))) {
                                    out_order_capacities[preempt_target] = cap_alloc.second.second;
                                }
                            }
                            for (const auto& ltb_alloc : h_ltb_alloc) {
                                bom_ltb_consumed[ltb_alloc.first] += ltb_alloc.second;
                            }
                            out_scheduled_orders[preempt_target].start_day = h_new_due - static_cast<int>(std::ceil(h_lt));
                            out_scheduled_orders[preempt_target].finish_day = h_new_due;
                            out_order_routing_costs[preempt_target] = h_chosen_routing_cost;

                            for (const auto& split : h_po_splits) {
                                out_scheduled_orders[split.original_order_idx].qty -= split.qty;
                                PlannedOrder new_po = out_scheduled_orders[split.original_order_idx];
                                new_po.qty = split.qty;
                                new_po.finish_day = split.finish_day;
                                new_po.start_day = split.start_day;
                                out_scheduled_orders.push_back(new_po);
                                out_order_capacities.push_back(split.capacity);
                                out_order_routing_costs.push_back(split.routing_cost);
                                po_priority.push_back(po_priority[split.original_order_idx]);
                                po_preference_mode.push_back(po_preference_mode[split.original_order_idx]);
                                po_family_id.push_back(po_family_id[split.original_order_idx]);
                                po_cust_group_id.push_back(po_cust_group_id[split.original_order_idx]);
                                po_region_id.push_back(po_region_id[split.original_order_idx]);
                            }

                            temp_allocations.clear();
                            temp_alloc_qty.clear();
                            temp_capacity_allocations.clear();
                            temp_ltb_allocations.clear();
                            active_mix_groups.clear();
                            temp_allotment_allocations.clear();
                            temp_allotment_blocked.clear();
                            chosen_routing_cost = 0.0;

                            temp_po_splits.clear();
                            ok = reserve_atp_and_capacity_recursive(
                                part_id, d, target_qty, wrapper.priority, wrapper.dimension_val,
                                atp_supplies, shared_constraints, parts, boms,
                                part_bom_info, source_constraints,
                                temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                                wrapper.preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                                last_dim_val, chosen_routing_cost,
                                active_parts,
                                temp_po_splits,
                                false,
                                dummy_wc_allocs,
                                wrapper.finish_day,
                                wrapper.family_id, wrapper.cust_group_id, wrapper.region_id, wildcard_id,
                                allotment_constraints,
                                temp_allotment_allocations,
                                temp_allotment_blocked
                            );
                        } else {
                            if (h_cid < shared_constraints.size()) {
                                shared_constraints[h_cid].allocated_rates[h_start] += h_cap;
                            }
                        }
                    }
                }
            }

            if (!ok && demands.size() < 1000) {
                double possible_qty = (avail_cap - setup_time - clean_up) / factor;
                if (possible_qty >= 0.1) {
                    target_qty = possible_qty;
                    need_split = true;
                    temp_allocations.clear();
                    temp_alloc_qty.clear();
                    temp_capacity_allocations.clear();
                    temp_ltb_allocations.clear();
                    active_mix_groups.clear();
                    temp_allotment_allocations.clear();
                    temp_allotment_blocked.clear();
                    chosen_routing_cost = 0.0;
                    temp_po_splits.clear();

                    ok = reserve_atp_and_capacity_recursive(
                        part_id, d, target_qty, wrapper.priority, wrapper.dimension_val,
                        atp_supplies, shared_constraints, parts, boms,
                        part_bom_info, source_constraints,
                        temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                        wrapper.preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                        last_dim_val, chosen_routing_cost,
                        active_parts,
                        temp_po_splits,
                        false,
                        dummy_wc_allocs,
                        wrapper.finish_day,
                        wrapper.family_id, wrapper.cust_group_id, wrapper.region_id, wildcard_id,
                        allotment_constraints,
                        temp_allotment_allocations,
                        temp_allotment_blocked
                    );
                }
            }

            if (ok) {
                for (const auto& alloc : temp_allotment_allocations) {
                    alloc.first->consumed += alloc.second;
                }
                for (const auto& blk : temp_allotment_blocked) {
                    blk.first->blocked += blk.second;
                }
                for (size_t a = 0; a < temp_allocations.size(); ++a) {
                    temp_allocations[a]->allocated_qty += temp_alloc_qty[a];
                    if (temp_allocations[a]->supply_type == "Planned-Order" && temp_allocations[a]->planned_order_index >= 0) {
                        size_t po_idx = static_cast<size_t>(temp_allocations[a]->planned_order_index);
                        if (po_idx < po_allocated_qty.size()) {
                            po_allocated_qty[po_idx] += temp_alloc_qty[a];
                        }
                    }
                }
                for (const auto& cap_alloc : temp_capacity_allocations) {
                    size_t ccid = cap_alloc.first;
                    int day = cap_alloc.second.first;
                    double cap = cap_alloc.second.second;
                    shared_constraints[ccid].allocated_rates[day] += cap;
                    if (ccid < last_dim_val.size()) {
                        last_dim_val[ccid][day] = wrapper.dimension_val;
                    }
                }
                for (const auto& ltb_alloc : temp_ltb_allocations) {
                    bom_ltb_consumed[ltb_alloc.first] += ltb_alloc.second;
                }
                double final_lt = parts[part_id].lead_time + target_qty * parts[part_id].run_rate;

                // Commit child splits
                for (const auto& split : temp_po_splits) {
                    out_scheduled_orders[split.original_order_idx].qty -= split.qty;
                    out_order_capacities[split.original_order_idx] -= split.capacity;
                    PlannedOrder new_po = out_scheduled_orders[split.original_order_idx];
                    new_po.qty = split.qty;
                    new_po.finish_day = split.finish_day;
                    new_po.start_day = split.start_day;
                    out_scheduled_orders.push_back(new_po);
                    out_order_capacities.push_back(split.capacity);
                    out_order_routing_costs.push_back(split.routing_cost);
                    po_priority.push_back(po_priority[split.original_order_idx]);
                    po_preference_mode.push_back(po_preference_mode[split.original_order_idx]);
                    po_family_id.push_back(po_family_id[split.original_order_idx]);
                    po_cust_group_id.push_back(po_cust_group_id[split.original_order_idx]);
                    po_region_id.push_back(po_region_id[split.original_order_idx]);
                }

                if (need_split) {
                    out_scheduled_orders[order_idx].qty = target_qty;
                    out_scheduled_orders[order_idx].start_day = get_workday_offset_backward(d, static_cast<int>(std::ceil(final_lt)), parts[out_scheduled_orders[order_idx].part_id].planning_calendar, global_wc_daily_capacity);
                    out_scheduled_orders[order_idx].finish_day = d;

                    double order_cap = 0.0;
                    for (const auto& cap_alloc : temp_capacity_allocations) {
                        if (cap_alloc.first == cid && cap_alloc.second.first == out_scheduled_orders[order_idx].start_day) {
                            order_cap = cap_alloc.second.second;
                            break;
                        }
                    }
                    out_order_capacities[order_idx] = order_cap;
                    out_order_routing_costs[order_idx] = chosen_routing_cost;

                    double rem_qty = qty - target_qty;
                    PlannedOrder rem_po = out_scheduled_orders[order_idx];
                    rem_po.qty = rem_qty;
                    rem_po.finish_day = d + 1;

                    size_t new_po_idx = out_scheduled_orders.size();
                    out_scheduled_orders.push_back(rem_po);
                    out_order_capacities.push_back(0.0);
                    out_order_routing_costs.push_back(0.0);
                    po_priority.push_back(po_priority[order_idx]);
                    po_preference_mode.push_back(po_preference_mode[order_idx]);
                    po_family_id.push_back(po_family_id[order_idx]);
                    po_cust_group_id.push_back(po_cust_group_id[order_idx]);
                    po_region_id.push_back(po_region_id[order_idx]);

                    global_orders.push_back({
                        new_po_idx,
                        wrapper.priority,
                        part_id,
                        rem_qty,
                        rem_po.start_day,
                        rem_po.finish_day,
                        wrapper.dimension_val,
                        wrapper.preference_mode,
                        wrapper.family_id,
                        wrapper.cust_group_id,
                        wrapper.region_id
                    });
                } else {
                    out_scheduled_orders[order_idx].qty = target_qty;
                    out_scheduled_orders[order_idx].start_day = get_workday_offset_backward(d, static_cast<int>(std::ceil(final_lt)), parts[out_scheduled_orders[order_idx].part_id].planning_calendar, global_wc_daily_capacity);
                    out_scheduled_orders[order_idx].finish_day = d;

                    double order_cap = 0.0;
                    for (const auto& cap_alloc : temp_capacity_allocations) {
                        if (cap_alloc.first == cid && cap_alloc.second.first == out_scheduled_orders[order_idx].start_day) {
                            order_cap = cap_alloc.second.second;
                            break;
                        }
                    }
                    out_order_capacities[order_idx] = order_cap;
                    out_order_routing_costs[order_idx] = chosen_routing_cost;
                }

                scheduled_successfully = true;
                break;
            }
        }
        if (!scheduled_successfully) {
            out_scheduled_orders[order_idx].finish_day = timeline_days - 1;
            if (wrapper.family_id != uint32_t(-1) && !allotment_constraints.empty()) {
                if (part_id < allotment_constrained_parts.size() && allotment_constrained_parts[part_id]) {
                    AllotmentState* m_state = find_matching_allotment(part_id, original_due, wrapper.family_id, wrapper.cust_group_id, wrapper.region_id, wildcard_id, allotment_constraints);
                    if (m_state != nullptr) {
                        m_state->blocked += qty;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < ipc_planned_orders.size(); ++i) {
        if (parts[out_scheduled_orders[i].part_id].part_type != "FINISHED") {
            out_scheduled_orders[i].qty = 0.0;
            out_order_capacities[i] = 0.0;
            out_order_routing_costs[i] = 0.0;
        }
    }

    out_allocated_rates.assign(timeline_days, 0.0);
    for (const auto& constr : shared_constraints) {
        for (int t = 0; t < timeline_days; ++t) {
            out_allocated_rates[t] += constr.allocated_rates[t];
        }
    }
}

} // namespace ipc
