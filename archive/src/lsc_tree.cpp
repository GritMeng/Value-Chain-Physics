#include "ipc/lsc_tree.h"
#include "ipc/dimension.h"
#include "ipc/globals.h"
#include "ipc/math_utils.h"
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <atomic>

namespace ipc {

void expand_lsc_tree_node(
    uint32_t root_part_id,
    double root_dimension_val,
    uint32_t current_node_id,
    uint32_t parent_node_id,
    double current_per_qty,
    double current_root_per_qty,
    int current_lt,
    uint32_t current_level,
    const std::vector<FlatBomItem>& boms,
    const std::vector<PartSiteRecord>& parts,
    std::vector<LscTreeNode>& out_nodes,
    std::vector<uint32_t> path
) {
    // 循环依赖检查（防死锁与栈溢出）
    if (std::find(path.begin(), path.end(), current_node_id) != path.end()) {
        static bool warning_printed = false;
        if (!warning_printed) {
            std::cerr << "[编译警报] 探测到 LSC 树展开中存在循环依赖！当前物料 ID: " << current_node_id << std::endl;
            warning_printed = true;
        }
        return;
    }
    path.push_back(current_node_id);

    bool is_leaf = true;
    if (current_node_id < parent_to_bom_indices.size()) {
        for (size_t bom_idx : parent_to_bom_indices[current_node_id]) {
            const auto& bom = boms[bom_idx];
            // 维度约束动态判定
            if (evaluate_dimension(root_dimension_val, bom.relation_op, bom.target_dim_val)) {
                is_leaf = false;
                double scrap_multiplier = 1.0 + bom.scrap;
                double next_root_per_qty = current_root_per_qty * bom.per_qty * scrap_multiplier;
                double next_lt = parts[bom.child_id].is_phantom ? 0.0 : parts[bom.child_id].lead_time;
                expand_lsc_tree_node(
                    root_part_id,
                    root_dimension_val,
                    bom.child_id,
                    current_node_id,
                    bom.per_qty,
                    next_root_per_qty,
                    current_lt + static_cast<int>(next_lt),
                    current_level + 1,
                    boms,
                    parts,
                    out_nodes,
                    path
                );
            }
        }
    }

    LscTreeNode node;
    node.root_part_id = root_part_id;
    node.root_dimension_val = root_dimension_val;
    node.node_part_id = current_node_id;
    node.parent_part_id = parent_node_id;
    node.per_qty = current_per_qty;
    node.root_per_qty = current_root_per_qty;
    node.cumulative_lt = current_lt;
    node.node_level = current_level;
    node.is_leaf = is_leaf;
    out_nodes.push_back(node);
}

std::vector<LscTreeNode> compile_all_lsc_trees(
    const std::vector<PartSiteRecord>& parts,
    const std::vector<FlatBomItem>& boms
) {
    std::vector<LscTreeNode> global_lsctree;
    // 针对每个 Finished Product 和所有合法的容量维度，并行/批量预先展开
    std::vector<double> dimensions = {100.0, 101.0, 102.0};
    for (size_t pid = 0; pid < parts.size(); ++pid) {
        if (parts[pid].part_type == "FINISHED") {
            for (double dim : dimensions) {
                expand_lsc_tree_node(
                    static_cast<uint32_t>(pid),
                    dim,
                    static_cast<uint32_t>(pid),
                    static_cast<uint32_t>(pid),
                    1.0, 1.0, 0, 0,
                    boms,
                    parts,
                    global_lsctree
                );
            }
        }
    }
    return global_lsctree;
}

int compile_low_level_codes(
    std::vector<PartSiteRecord>& parts,
    const std::vector<FlatBomItem>& boms
) {
    for (auto& p : parts) {
        p.low_level_code = 0;
    }
    int max_level = 0;
    bool changed = true;
    int iterations = 0;

    while (changed) {
        changed = false;
        iterations++;
        for (size_t p_id = 0; p_id < parts.size(); ++p_id) {
            if (p_id < parent_to_bom_indices.size()) {
                for (size_t bom_idx : parent_to_bom_indices[p_id]) {
                    const auto& bom = boms[bom_idx];
                    uint32_t c_id = bom.child_id;
                    if (static_cast<size_t>(c_id) == p_id) continue;
                    if (parts[c_id].low_level_code < parts[p_id].low_level_code + 1) {
                        parts[c_id].low_level_code = parts[p_id].low_level_code + 1;
                        if (static_cast<int>(parts[c_id].low_level_code) > max_level) {
                            max_level = parts[c_id].low_level_code;
                        }
                        changed = true;
                    }
                }
            }
        }
        if (iterations > 100) {
            std::cerr << "[错误] [编译警报] 探测到 BOM 网络中存在循环依赖环路！" << std::endl;
            break;
        }
    }
    return max_level;
}

void explode_planned_order_to_children(
    uint32_t part_id,
    double p_qty,
    int start_day,
    int demand_day,
    double dimension_val,
    const std::vector<FlatBomItem>& boms,
    const std::vector<PartSiteRecord>& parts,
    std::vector<std::vector<std::vector<double>>>& gross_demand,
    std::vector<std::vector<std::vector<uint64_t>>>& gross_demand_priority,
    const std::vector<std::vector<size_t>>& parent_to_bom_indices,
    std::vector<double>& current_on_hand,
    std::vector<std::vector<AlternateAllocationRecord>>& thread_alt_records,
    int tid,
    uint64_t parent_priority
) {
    std::vector<size_t> child_bom_idxs;
    if (part_id < parent_to_bom_indices.size()) {
        child_bom_idxs = parent_to_bom_indices[part_id];
    }

    std::vector<const FlatBomItem*> standard_children;
    std::unordered_map<int, std::vector<const FlatBomItem*>> alternative_groups;

    for (size_t bom_idx : child_bom_idxs) {
        const FlatBomItem& bom = boms[bom_idx];
        if (evaluate_dimension(dimension_val, bom.relation_op, bom.target_dim_val)) {
            // ECN validity start check
            if (bom.eff_start_day >= 0 && start_day < bom.eff_start_day) continue;
            // ECN validity end check
            if (bom.eff_end_day >= 0 && start_day > bom.eff_end_day) {
                // Check if soft cutover
                if (bom.relationship_type == "soft" || bom.relationship_type == "soft_cut" || bom.relationship_type == "interchangeable") {
                    double avail = std::max(0.0, current_on_hand[bom.child_id] - parts[bom.child_id].safety_stock);
                    if (avail <= 0.0) continue; // No stock, skip
                } else {
                    continue; // Hard cutover, skip
                }
            }

            if (bom.alt_group_id == -1) {
                standard_children.push_back(&bom);
            } else {
                alternative_groups[bom.alt_group_id].push_back(&bom);
            }
        }
    }

    for (const auto* bom : standard_children) {
        double child_gross = p_qty * bom->per_qty * (1.0 + bom->scrap);
        if (parts[bom->child_id].round_to_integer) {
            child_gross = std::ceil(child_gross);
        }

        // If it is expired but soft cutover:
        if (bom->eff_end_day >= 0 && start_day > bom->eff_end_day) {
            double avail = std::max(0.0, current_on_hand[bom->child_id] - parts[bom->child_id].safety_stock);
            double consumed = std::min(child_gross, avail);
            if (consumed > 0.0) {
                #pragma omp atomic
                current_on_hand[bom->child_id] -= consumed;

                int trans_lt = (parts[part_id].site != parts[bom->child_id].site) ? parts[bom->child_id].transshipment_lead_time : 0;
                int child_day = get_workday_offset_backward(start_day, trans_lt, parts[bom->child_id].planning_calendar, global_wc_daily_capacity);
                if (child_day < 0) child_day = 0;

                AlternateAllocationRecord rec = {
                    static_cast<uint32_t>(demand_day),
                    part_id,
                    bom->child_id,
                    consumed,
                    child_day,
                    -2 // -2 represents soft cutover standard stock pre-consumption
                };
                thread_alt_records[tid].push_back(rec);

                child_gross -= consumed;
            }
        }

        if (child_gross > 0.0) {
            // For expired items, we cannot propagate remaining gross demand!
            if (bom->eff_end_day >= 0 && start_day > bom->eff_end_day) {
                continue;
            }
            double child_dim_val = (bom->relation_op == static_cast<uint8_t>(RelationOp::PASS)) 
                                   ? dimension_val 
                                   : bom->target_dim_val;

            // Site transshipment lead time offset
            int trans_lt = (parts[part_id].site != parts[bom->child_id].site) ? parts[bom->child_id].transshipment_lead_time : 0;
            int child_day = get_workday_offset_backward(start_day, trans_lt, parts[bom->child_id].planning_calendar, global_wc_daily_capacity);
            if (child_day < 0) child_day = 0;

            int child_dim_idx = (child_dim_val <= 100.0) ? 0 : ((child_dim_val <= 101.0) ? 1 : 2);
            #pragma omp atomic
            gross_demand[bom->child_id][child_day][child_dim_idx] += child_gross;
            
            {
                std::atomic_ref<uint64_t> ref(gross_demand_priority[bom->child_id][child_day][child_dim_idx]);
                uint64_t current = ref.load(std::memory_order_relaxed);
                while (parent_priority < current && !ref.compare_exchange_weak(current, parent_priority, std::memory_order_relaxed)) {}
            }
        }
    }

    if (!alternative_groups.empty()) {
        struct MCDMCandidate {
            int alt_group_id;
            std::vector<const FlatBomItem*> items;
            int max_llc = 0;
            double exist_cost = 0.0;
            double new_cost = 0.0;
            double kit_qty = 0.0;
        };

        std::vector<MCDMCandidate> candidates;

        for (auto& pair : alternative_groups) {
            MCDMCandidate cand;
            cand.alt_group_id = pair.first;
            
            for (const auto* item : pair.second) {
                // ECN validity start check
                if (item->eff_start_day >= 0 && start_day < item->eff_start_day) continue;
                // ECN validity end check
                if (item->eff_end_day >= 0 && start_day > item->eff_end_day) {
                    if (item->relationship_type == "soft" || item->relationship_type == "soft_cut" || item->relationship_type == "interchangeable") {
                        double avail = std::max(0.0, current_on_hand[item->child_id] - parts[item->child_id].safety_stock);
                        if (avail <= 0.0) continue; // No stock, skip
                    } else {
                        continue; // Hard cutover, skip
                    }
                }
                cand.items.push_back(item);
            }

            if (cand.items.empty()) continue;

            for (const auto* item : cand.items) {
                int child_llc = parts[item->child_id].low_level_code;
                if (child_llc > cand.max_llc) cand.max_llc = child_llc;
            }

            double min_ratio = 9999999999.0;
            for (const auto* item : cand.items) {
                double u_i = item->per_qty * (1.0 + item->scrap);
                double available_stock = std::max(0.0, current_on_hand[item->child_id] - parts[item->child_id].safety_stock);
                double ratio = available_stock / u_i;
                if (ratio < min_ratio) min_ratio = ratio;
            }

            cand.kit_qty = std::min(p_qty, min_ratio);
            if (cand.kit_qty < 0.0) cand.kit_qty = 0.0;

            for (const auto* item : cand.items) {
                double u_i = item->per_qty * (1.0 + item->scrap);
                double required_total = p_qty * u_i;
                double available_stock = std::max(0.0, current_on_hand[item->child_id] - parts[item->child_id].safety_stock);
                double consumed_stock = std::min(required_total, available_stock);
                double shortage = std::max(0.0, required_total - available_stock);
                double part_cost = parts[item->child_id].cost;

                cand.exist_cost += consumed_stock * part_cost;
                if (item->eff_end_day >= 0 && start_day > item->eff_end_day) {
                    // Expired soft cutover cannot build new planned orders
                } else {
                    cand.new_cost += shortage * part_cost;
                }
            }
            candidates.push_back(cand);
        }

        if (candidates.empty()) return;

        std::sort(candidates.begin(), candidates.end(), [](const MCDMCandidate& x, const MCDMCandidate& y) {
            if (x.max_llc != y.max_llc) {
                return x.max_llc < y.max_llc;
            }
            if (std::abs(x.new_cost - y.new_cost) > 1e-9) {
                return x.new_cost < y.new_cost;
            }
            if (std::abs(x.exist_cost - y.exist_cost) > 1e-9) {
                return x.exist_cost < y.exist_cost;
            }
            return x.alt_group_id < y.alt_group_id;
        });

        const auto& best_cand = candidates[0];

        std::vector<const FlatBomItem*> sorted_items = best_cand.items;
        std::sort(sorted_items.begin(), sorted_items.end(), [&](const FlatBomItem* a, const FlatBomItem* b) {
            bool a_exp = (a->eff_end_day >= 0 && start_day > a->eff_end_day);
            bool b_exp = (b->eff_end_day >= 0 && start_day > b->eff_end_day);
            if (a_exp != b_exp) return a_exp > b_exp; // expired first
            return a->alt_priority < b->alt_priority;
        });

        double rem_p_qty = p_qty;

        // 1. Process expired items first (consume stock only)
        for (const auto* item : sorted_items) {
            bool is_expired = (item->eff_end_day >= 0 && start_day > item->eff_end_day);
            if (!is_expired) continue;

            double u_i = item->per_qty * (1.0 + item->scrap);
            double required_qty = rem_p_qty * u_i;
            double available_stock = std::max(0.0, current_on_hand[item->child_id] - parts[item->child_id].safety_stock);
            double consumed_stock = std::min(required_qty, available_stock);

            if (consumed_stock > 0.0) {
                #pragma omp atomic
                current_on_hand[item->child_id] -= consumed_stock;

                int trans_lt = (parts[part_id].site != parts[item->child_id].site) ? parts[item->child_id].transshipment_lead_time : 0;
                int child_day = get_workday_offset_backward(start_day, trans_lt, parts[item->child_id].planning_calendar, global_wc_daily_capacity);
                if (child_day < 0) child_day = 0;

                AlternateAllocationRecord rec = {
                    static_cast<uint32_t>(demand_day),
                    part_id,
                    item->child_id,
                    consumed_stock,
                    child_day,
                    best_cand.alt_group_id
                };
                thread_alt_records[tid].push_back(rec);

                rem_p_qty -= (consumed_stock / u_i);
            }
            if (rem_p_qty <= 0.0) break;
        }

        // 2. Process active items (consume stock and propagate remaining gross demand)
        if (rem_p_qty > 0.0) {
            std::vector<const FlatBomItem*> active_items;
            double sum_active_target = 0.0;
            for (const auto* item : sorted_items) {
                bool is_expired = (item->eff_end_day >= 0 && start_day > item->eff_end_day);
                if (!is_expired) {
                    active_items.push_back(item);
                    sum_active_target += item->target_ratio;
                }
            }

            if (!active_items.empty()) {
                if (sum_active_target <= 0.0) sum_active_target = 1.0;

                for (const auto* item : active_items) {
                    double share = item->target_ratio / sum_active_target;
                    double item_p_qty = rem_p_qty * share;

                    double u_i = item->per_qty * (1.0 + item->scrap);
                    double required_qty = item_p_qty * u_i;
                    double available_stock = std::max(0.0, current_on_hand[item->child_id] - parts[item->child_id].safety_stock);
                    double consumed_stock = std::min(required_qty, available_stock);

                    if (consumed_stock > 0.0) {
                        #pragma omp atomic
                        current_on_hand[item->child_id] -= consumed_stock;

                        int trans_lt = (parts[part_id].site != parts[item->child_id].site) ? parts[item->child_id].transshipment_lead_time : 0;
                        int child_day = get_workday_offset_backward(start_day, trans_lt, parts[item->child_id].planning_calendar, global_wc_daily_capacity);
                        if (child_day < 0) child_day = 0;

                        AlternateAllocationRecord rec = {
                            static_cast<uint32_t>(demand_day),
                            part_id,
                            item->child_id,
                            consumed_stock,
                            child_day,
                            best_cand.alt_group_id
                        };
                        thread_alt_records[tid].push_back(rec);
                    }

                    double child_net = required_qty - consumed_stock;
                    if (parts[item->child_id].round_to_integer) {
                        child_net = std::ceil(child_net);
                    }

                    if (child_net > 0.0) {
                        double child_dim_val = (item->relation_op == static_cast<uint8_t>(RelationOp::PASS)) 
                                               ? dimension_val 
                                               : item->target_dim_val;

                        // Site transshipment lead time offset
                        int trans_lt = (parts[part_id].site != parts[item->child_id].site) ? parts[item->child_id].transshipment_lead_time : 0;
                        int child_day = get_workday_offset_backward(start_day, trans_lt, parts[item->child_id].planning_calendar, global_wc_daily_capacity);
                        if (child_day < 0) child_day = 0;

                        int child_dim_idx = (child_dim_val <= 100.0) ? 0 : ((child_dim_val <= 101.0) ? 1 : 2);
                        #pragma omp atomic
                        gross_demand[item->child_id][child_day][child_dim_idx] += child_net;
                        
                        {
                            std::atomic_ref<uint64_t> ref(gross_demand_priority[item->child_id][child_day][child_dim_idx]);
                            uint64_t current = ref.load(std::memory_order_relaxed);
                            while (parent_priority < current && !ref.compare_exchange_weak(current, parent_priority, std::memory_order_relaxed)) {}
                        }
                    }
                }
            }
        }
    }
}

} // namespace ipc
