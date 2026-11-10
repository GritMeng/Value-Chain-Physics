#include "ipc/mrp_engine.h"
#include "ipc/globals.h"
#include "ipc/lsc_tree.h"
#include "ipc/substitution.h"
#include "ipc/dimension.h"
#include "ipc/vocab.h"
#include "ipc/math_utils.h"
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <atomic>
#include <omp.h>

namespace ipc {

#ifdef _OPENMP
#endif

void run_lbl_mrp_engine(
    std::vector<PartSiteRecord>& parts,
    std::vector<FlatBomItem>& boms,
    const std::vector<IndependentDemand>& demands,
    std::vector<PlannedOrder>& out_ipc_planned_orders,
    std::vector<AlternateAllocationRecord>& out_alt_records,
    std::vector<SwapRecord>& out_swap_records,
    int max_llc_level,
    const std::vector<ScheduledReceiptRecord>& srs
) {
    std::cout << "\n[LBL] Starting LBL MRP Netting Engine..." << std::endl;

    parent_to_bom_indices.assign(parts.size(), std::vector<size_t>());
    child_to_bom_indices.assign(parts.size(), std::vector<size_t>());
    alt_group_to_bom_indices.clear();

    for (size_t i = 0; i < boms.size(); ++i) {
        parent_to_bom_indices[boms[i].parent_id].push_back(i);
        child_to_bom_indices[boms[i].child_id].push_back(i);
        if (boms[i].alt_group_id != -1) {
            alt_group_to_bom_indices[boms[i].alt_group_id].push_back(i);
        }
    }

    std::cout << "[DEBUG] parent_to_bom_indices size: " << parent_to_bom_indices.size() << std::endl;
    std::cout << "[DEBUG] child_to_bom_indices size: " << child_to_bom_indices.size() << std::endl;
    std::cout << "[DEBUG] boms size: " << boms.size() << std::endl;
    std::cout << "[DEBUG] Calling compile_low_level_codes..." << std::endl;
    max_llc_level = compile_low_level_codes(parts, boms);
    std::cout << "[DEBUG] compile_low_level_codes returned max_llc_level: " << max_llc_level << std::endl;

    int timeline_days = TIMELINE_DAYS;

    // 3D 需求网格 (DOD时空密铺向量)：part_id * day * dim_idx (0:128M, 1:256M, 2:512M)
    std::cout << "[DEBUG] Allocating gross_demand: parts.size() = " << parts.size() << ", timeline_days = " << timeline_days << std::endl;
    std::vector<std::vector<std::vector<double>>> gross_demand(
        parts.size(),
        std::vector<std::vector<double>>(timeline_days, std::vector<double>(3, 0.0))
    );
    std::vector<std::vector<std::vector<uint64_t>>> gross_demand_priority(
        parts.size(),
        std::vector<std::vector<uint64_t>>(timeline_days, std::vector<uint64_t>(3, 18446744073709551615ULL))
    );
    std::cout << "[DEBUG] gross_demand allocated successfully." << std::endl;

    // 1. 将需求注入 Level 0 成品
    for (const auto& d : demands) {
        int dim_idx = (d.dimension_val <= 100.0) ? 0 : ((d.dimension_val <= 101.0) ? 1 : 2);
        gross_demand[d.part_id][d.due_day][dim_idx] += d.qty;
        gross_demand_priority[d.part_id][d.due_day][dim_idx] = std::min(gross_demand_priority[d.part_id][d.due_day][dim_idx], static_cast<uint64_t>(d.priority));
    }

    // 在手库存状态向量
    struct NettingSR {
        std::string sr_id;
        double qty;
        int due_day;
        std::string sr_type;
        double allocated = 0.0;
    };
    std::vector<std::vector<NettingSR>> part_srs(parts.size());
    std::vector<std::vector<NettingSR>> part_srs_all(parts.size());
    for (const auto& sr : srs) {
        NettingSR n_sr;
        n_sr.sr_id = sr.sr_id;
        n_sr.qty = sr.qty;
        n_sr.due_day = sr.due_day;
        n_sr.sr_type = sr.sr_type;
        part_srs_all[sr.part_id].push_back(n_sr);
        if (sr.sr_type != "Ignore" && sr.sr_type != "ExplodedOnly") {
            part_srs[sr.part_id].push_back(n_sr);
        }
    }
    if (srs.empty()) {
        for (size_t i = 0; i < parts.size(); ++i) {
            if (parts[i].ipc_scheduled_receipt > 0.0) {
                NettingSR n_sr;
                n_sr.sr_id = "SR_" + parts[i].part_code;
                n_sr.qty = parts[i].ipc_scheduled_receipt;
                n_sr.due_day = 0;
                n_sr.sr_type = "In-process";
                part_srs[i].push_back(n_sr);
                part_srs_all[i].push_back(n_sr);
            }
        }
    }
    std::vector<double> current_on_hand(parts.size());
    for (size_t i = 0; i < parts.size(); ++i) {
        double base_oh = (parts[i].on_hand_type == "Exclude") ? 0.0 : parts[i].on_hand;
        double sum_sr = 0.0;
        for (const auto& sr : part_srs[i]) {
            sum_sr += sr.qty;
        }
        current_on_hand[i] = base_oh + sum_sr;
    }

    // 按低层码进行层级分组
    std::cout << "[DEBUG] Allocating parts_by_level: max_llc_level = " << max_llc_level << std::endl;
    std::vector<std::vector<uint32_t>> parts_by_level(max_llc_level + 1);
    std::cout << "[DEBUG] parts_by_level allocated successfully." << std::endl;

    for (const auto& p : parts) {
        parts_by_level[p.low_level_code].push_back(p.part_id);
    }

    // 初始化并发相关的线程局部结果缓存
    int num_threads = 1;
#ifdef _OPENMP
    num_threads = omp_get_max_threads();
#endif

    std::vector<std::vector<PlannedOrder>> thread_ipc_planned_orders(num_threads);
    std::vector<std::vector<AlternateAllocationRecord>> thread_alt_records(num_threads);
    std::vector<std::vector<SwapRecord>> thread_swap_records(num_threads);

    // 2. 逐层 level-by-level 并行消纳
    struct DSU {
        std::vector<int> parent;
        DSU(size_t n) {
            parent.resize(n);
            for (size_t i = 0; i < n; ++i) parent[i] = static_cast<int>(i);
        }
        int find(int i) {
            if (parent[i] == i)
                return i;
            return parent[i] = find(parent[i]);
        }
        void unite(int i, int j) {
            int root_i = find(i);
            int root_j = find(j);
            if (root_i != root_j) {
                parent[root_i] = root_j;
            }
        }
    };

    struct NettingJob {
        bool is_group;
        uint32_t single_part_id;
        std::vector<uint32_t> group_part_ids;
    };

    for (int lvl = 0; lvl <= max_llc_level; ++lvl) {
        int level_parts_count = static_cast<int>(parts_by_level[lvl].size());
        if (level_parts_count == 0) continue;

        DSU dsu(parts.size());
        std::unordered_map<int, std::vector<uint32_t>> parent_alt_group_to_parts;
        std::unordered_map<int, std::vector<uint32_t>> child_alt_group_to_parts;

        for (uint32_t part_id : parts_by_level[lvl]) {
            int my_alt_group = -1;
            if (part_id < child_to_bom_indices.size()) {
                for (size_t bom_idx : child_to_bom_indices[part_id]) {
                    if (boms[bom_idx].alt_group_id != -1) {
                        my_alt_group = boms[bom_idx].alt_group_id;
                        break;
                    }
                }
            }
            if (my_alt_group != -1) {
                parent_alt_group_to_parts[my_alt_group].push_back(part_id);
            }

            if (part_id < parent_to_bom_indices.size()) {
                for (size_t bom_idx : parent_to_bom_indices[part_id]) {
                    const auto& bom = boms[bom_idx];
                    if (bom.alt_group_id != -1) {
                        child_alt_group_to_parts[bom.alt_group_id].push_back(part_id);
                    }
                }
            }
        }

        for (const auto& pair : parent_alt_group_to_parts) {
            const auto& p_list = pair.second;
            for (size_t i = 1; i < p_list.size(); ++i) {
                dsu.unite(p_list[0], p_list[i]);
            }
        }

        for (const auto& pair : child_alt_group_to_parts) {
            const auto& p_list = pair.second;
            for (size_t i = 1; i < p_list.size(); ++i) {
                dsu.unite(p_list[0], p_list[i]);
            }
        }

        std::unordered_map<int, std::vector<uint32_t>> groups;
        for (uint32_t part_id : parts_by_level[lvl]) {
            int root = dsu.find(part_id);
            groups[root].push_back(part_id);
        }

        std::vector<uint32_t> independent_parts;
        std::vector<std::vector<uint32_t>> component_groups;

        for (const auto& pair : groups) {
            const auto& p_list = pair.second;
            if (p_list.size() == 1) {
                independent_parts.push_back(p_list[0]);
            } else {
                component_groups.push_back(p_list);
            }
        }

        std::vector<uint64_t> part_min_priority(parts.size(), 18446744073709551615ULL);
        for (uint32_t part_id : parts_by_level[lvl]) {
            uint64_t min_pri = 18446744073709551615ULL;
            for (int day = 0; day < timeline_days; ++day) {
                for (int dim = 0; dim < 3; ++dim) {
                    if (gross_demand[part_id][day][dim] > 0.0 && gross_demand_priority[part_id][day][dim] < min_pri) {
                        min_pri = gross_demand_priority[part_id][day][dim];
                    }
                }
            }
            part_min_priority[part_id] = min_pri;
        }

        for (auto& group : component_groups) {
            std::sort(group.begin(), group.end(), [&](uint32_t a, uint32_t b) {
                if (part_min_priority[a] != part_min_priority[b]) {
                    return part_min_priority[a] < part_min_priority[b];
                }
                return a < b;
            });
        }

        std::vector<NettingJob> jobs;
        for (uint32_t part_id : independent_parts) {
            NettingJob job;
            job.is_group = false;
            job.single_part_id = part_id;
            jobs.push_back(job);
        }
        for (const auto& group : component_groups) {
            NettingJob job;
            job.is_group = true;
            job.group_part_ids = group;
            jobs.push_back(job);
        }

        int jobs_count = static_cast<int>(jobs.size());

        auto process_part = [&](uint32_t part_id, int tid) {
            // Explode ExplodedOnly SRs for this part down the BOM
            if (part_id < parent_to_bom_indices.size()) {
                const auto& child_bom_idxs = parent_to_bom_indices[part_id];
                for (const auto& sr : part_srs_all[part_id]) {
                    if (sr.sr_type == "ExplodedOnly") {
                        std::vector<const FlatBomItem*> standard_children;
                        std::unordered_map<int, std::vector<const FlatBomItem*>> alternative_groups;
                        for (size_t bom_idx : child_bom_idxs) {
                            const FlatBomItem& bom = boms[bom_idx];
                            if (evaluate_dimension(100.0, bom.relation_op, bom.target_dim_val)) {
                                if (bom.alt_group_id == -1) {
                                    standard_children.push_back(&bom);
                                } else {
                                    alternative_groups[bom.alt_group_id].push_back(&bom);
                                }
                            }
                        }

                        for (const auto* bom : standard_children) {
                            double child_gross = sr.qty * bom->per_qty * (1.0 + bom->scrap);
                            if (parts[bom->child_id].round_to_integer) {
                                child_gross = std::ceil(child_gross);
                            }
                            double child_dim_val = (bom->relation_op == static_cast<uint8_t>(RelationOp::PASS)) ? 100.0 : bom->target_dim_val;
                            int child_dim_idx = (child_dim_val <= 100.0) ? 0 : ((child_dim_val <= 101.0) ? 1 : 2);
                            #pragma omp atomic
                            gross_demand[bom->child_id][sr.due_day][child_dim_idx] += child_gross;
                            {
                                std::atomic_ref<uint64_t> ref(gross_demand_priority[bom->child_id][sr.due_day][child_dim_idx]);
                                uint64_t current = ref.load(std::memory_order_relaxed);
                                uint64_t max_pri = 18446744073709551615ULL;
                                while (max_pri < current && !ref.compare_exchange_weak(current, max_pri, std::memory_order_relaxed)) {}
                            }
                        }

                        if (!alternative_groups.empty()) {
                            for (auto& pair : alternative_groups) {
                                int grp_id = pair.first;
                                const auto& grp_items = pair.second;
                                double total_ratio = 0.0;
                                for (const auto* item : grp_items) {
                                    total_ratio += item->target_ratio;
                                }
                                for (const auto* item : grp_items) {
                                    double share = (total_ratio > 0.0) ? (item->target_ratio / total_ratio) : (1.0 / grp_items.size());
                                    double child_gross = sr.qty * item->per_qty * (1.0 + item->scrap) * share;
                                    if (parts[item->child_id].round_to_integer) {
                                        child_gross = std::ceil(child_gross);
                                    }
                                    double child_dim_val = (item->relation_op == static_cast<uint8_t>(RelationOp::PASS)) ? 100.0 : item->target_dim_val;
                                    int child_dim_idx = (child_dim_val <= 100.0) ? 0 : ((child_dim_val <= 101.0) ? 1 : 2);
                                    #pragma omp atomic
                                    gross_demand[item->child_id][sr.due_day][child_dim_idx] += child_gross;
                                    {
                                        std::atomic_ref<uint64_t> ref(gross_demand_priority[item->child_id][sr.due_day][child_dim_idx]);
                                        uint64_t current = ref.load(std::memory_order_relaxed);
                                        uint64_t max_pri = 18446744073709551615ULL;
                                        while (max_pri < current && !ref.compare_exchange_weak(current, max_pri, std::memory_order_relaxed)) {}
                                    }
                                }
                            }
                        }
                    }
                }
            }

            double part_lot_size = 0.0;
            if (part_id < child_to_bom_indices.size()) {
                for (size_t bom_idx : child_to_bom_indices[part_id]) {
                    if (boms[bom_idx].lot_size > part_lot_size) {
                        part_lot_size = boms[bom_idx].lot_size;
                    }
                }
            }

            struct DemandEvent {
                int day;
                int dim_idx;
                double qty;
                double cd_start;
                double cd_end;
            };

            std::vector<DemandEvent> part_demands;
            double total_gross = 0.0;

            for (int day = 0; day < timeline_days; ++day) {
                for (int dim_idx = 0; dim_idx < 3; ++dim_idx) {
                    double gross = gross_demand[part_id][day][dim_idx];
                    if (gross > 0.0) {
                        DemandEvent ev;
                        ev.day = day;
                        ev.dim_idx = dim_idx;
                        ev.qty = gross;
                        ev.cd_start = total_gross;
                        total_gross += gross;
                        ev.cd_end = total_gross;
                        part_demands.push_back(ev);
                    }
                }
            }

            if (part_demands.empty()) return;

            std::vector<NettingSR> local_srs = part_srs[part_id];
            double sum_local_sr = 0.0;
            for (const auto& sr : local_srs) {
                sum_local_sr += sr.qty;
            }
            double base_oh = (parts[part_id].on_hand_type == "Exclude") ? 0.0 : std::max(0.0, current_on_hand[part_id] - sum_local_sr);
            double current_inv = base_oh;
            int last_sr_day = -1;

            for (const auto& ev : part_demands) {
                gross_demand[part_id][ev.day][ev.dim_idx] = 0.0;
            }

            for (size_t ev_idx = 0; ev_idx < part_demands.size(); ++ev_idx) {
                const auto& ev = part_demands[ev_idx];
                uint64_t ev_priority = gross_demand_priority[part_id][ev.day][ev.dim_idx];

                for (int d = last_sr_day + 1; d <= ev.day; ++d) {
                    for (const auto& sr : local_srs) {
                        if (sr.due_day == d && sr.allocated < sr.qty) {
                            current_inv += (sr.qty - sr.allocated);
                        }
                    }
                }
                last_sr_day = ev.day;

                double consumed = std::min(ev.qty, current_inv);
                current_inv -= consumed;
                double net_demand = ev.qty - consumed;

                double ss_target = 0.0;
                if (parts[part_id].dos_policy == "DAYS_OF_SUPPLY") {
                    double intervals = parts[part_id].dos_intervals;
                    if (intervals > 0.0) {
                        int N = static_cast<int>(std::ceil(intervals));
                        for (int d = ev.day + 1; d <= ev.day + N && d < timeline_days; ++d) {
                            ss_target += gross_demand[part_id][d][0] + gross_demand[part_id][d][1] + gross_demand[part_id][d][2];
                        }
                    }
                } else if (parts[part_id].safety_stock > 0.0) {
                    ss_target = parts[part_id].safety_stock;
                }

                if (current_inv < ss_target) {
                    double ss_deficit = ss_target - current_inv;
                    net_demand += ss_deficit;
                    current_inv += ss_deficit;
                }

                if (net_demand > 0.0) {
                    for (auto& sr : local_srs) {
                        if (sr.due_day > ev.day && (sr.sr_type == "RescheduleRecommend" || sr.sr_type == "Reschedulable")) {
                            double avail = sr.qty - sr.allocated;
                            if (avail > 0.0) {
                                double pull_qty = std::min(net_demand, avail);
                                sr.allocated += pull_qty;
                                net_demand -= pull_qty;
                            }
                        }
                        if (net_demand <= 0.0) break;
                    }
                }

                if (net_demand <= 0.0) continue;

                double dimension_val = (ev.dim_idx == 0 ? 100.0 : (ev.dim_idx == 1 ? 101.0 : 102.0));

                int my_alt_group = -1;
                int my_alt_priority = 0;
                if (part_id < child_to_bom_indices.size()) {
                    for (size_t bom_idx : child_to_bom_indices[part_id]) {
                        const auto& bom = boms[bom_idx];
                        if (bom.alt_group_id != -1) {
                            my_alt_group = bom.alt_group_id;
                            my_alt_priority = bom.alt_priority;
                            break;
                        }
                    }
                }

                if (my_alt_group != -1) {
                    std::vector<FlatBomItem*> group_items;
                    auto it = alt_group_to_bom_indices.find(my_alt_group);
                    if (it != alt_group_to_bom_indices.end()) {
                        for (size_t bom_idx : it->second) {
                            const auto& bom = boms[bom_idx];
                            if (bom.eff_start_day >= 0 && ev.day < bom.eff_start_day) continue;
                            if (bom.eff_end_day >= 0 && ev.day > bom.eff_end_day) {
                                if (bom.relationship_type != "soft" && bom.relationship_type != "soft_cut" && bom.relationship_type != "interchangeable") {
                                    continue; // Hard cutover, skip
                                }
                            }
                            group_items.push_back(&boms[bom_idx]);
                        }
                    }

                    if (my_alt_priority == 1) {
                        uint32_t alt_pid = allocate_class1(net_demand, group_items, current_on_hand);
                        if (alt_pid != uint32_t(-1)) {
                            double alt_avail = std::max(0.0, current_on_hand[alt_pid] - parts[alt_pid].safety_stock);
                            double alt_consumed = std::min(net_demand, alt_avail);
                            current_on_hand[alt_pid] -= alt_consumed;
                            net_demand -= alt_consumed;
                            for (auto* item : group_items) {
                                if (item->child_id == alt_pid) {
                                    item->historical_qty += alt_consumed;
                                    break;
                                }
                            }
                            if (alt_consumed > 0.0) {
                                AlternateAllocationRecord rec = {static_cast<uint32_t>(ev.day), part_id, alt_pid, alt_consumed, ev.day, 1};
                                thread_alt_records[tid].push_back(rec);
                            }
                        }
                    } 
                    else if (my_alt_priority == 2) {
                        uint32_t alt_pid = allocate_class2(group_items);
                        if (alt_pid != uint32_t(-1)) {
                            double alt_avail = std::max(0.0, current_on_hand[alt_pid] - parts[alt_pid].safety_stock);
                            double alt_consumed = std::min(net_demand, alt_avail);
                            current_on_hand[alt_pid] -= alt_consumed;
                            net_demand -= alt_consumed;
                            for (auto* item : group_items) {
                                if (item->child_id == alt_pid) {
                                    item->historical_qty += alt_consumed;
                                    break;
                                }
                            }
                            if (alt_consumed > 0.0) {
                                AlternateAllocationRecord rec = {static_cast<uint32_t>(ev.day), part_id, alt_pid, alt_consumed, ev.day, 2};
                                thread_alt_records[tid].push_back(rec);
                            }
                        }
                    } 
                    else if (my_alt_priority == 3) {
                        allocate_class3(net_demand, group_items, current_on_hand, ev.day, part_id, thread_alt_records[tid], parts);
                        net_demand = 0.0;
                    }
                }

                if (net_demand > 0.0 && my_alt_group != -1) {
                    auto it = alt_group_to_bom_indices.find(my_alt_group);
                    if (it != alt_group_to_bom_indices.end()) {
                        for (size_t bom_idx : it->second) {
                            const auto& bom = boms[bom_idx];
                            if (bom.eff_start_day >= 0 && ev.day < bom.eff_start_day) continue;
                            if (bom.eff_end_day >= 0 && ev.day > bom.eff_end_day) {
                                if (bom.relationship_type != "soft" && bom.relationship_type != "soft_cut" && bom.relationship_type != "interchangeable") {
                                    continue; // Hard cutover, skip
                                }
                            }
                            uint32_t alt_pid = bom.child_id;
                            if (alt_pid != part_id && current_on_hand[alt_pid] > parts[alt_pid].safety_stock) {
                                double alt_avail = std::max(0.0, current_on_hand[alt_pid] - parts[alt_pid].safety_stock);
                                double swap_qty = std::min(net_demand, alt_avail);
                                if (swap_qty > 0.0) {
                                    current_on_hand[alt_pid] -= swap_qty;
                                    net_demand -= swap_qty;
                                    SwapRecord s_rec;
                                    char buf[32];
                                    sprintf(buf, "DEMAND_%05d", ev.day + 1);
                                    s_rec.demand_code = std::string(buf);
                                    s_rec.from_part = vocab.get_code(part_id);
                                    s_rec.to_part = vocab.get_code(alt_pid);
                                    s_rec.swapped_qty = swap_qty;
                                    s_rec.day = ev.day;
                                    s_rec.alt_group = "ALT_GRP_" + std::to_string(my_alt_group);
                                    s_rec.swap_reason = "Incomplete Substitution Stagnant Inventory SWAP";
                                    thread_swap_records[tid].push_back(s_rec);
                                }
                                if (net_demand <= 0.0) break;
                            }
                        }
                    }
                }

                if (net_demand > 0.0) {
                    if (parts[part_id].is_phantom) {
                        std::vector<size_t> child_bom_idxs;
                        if (part_id < parent_to_bom_indices.size()) {
                            child_bom_idxs = parent_to_bom_indices[part_id];
                        }
                        for (size_t bom_idx : child_bom_idxs) {
                            const auto& bom = boms[bom_idx];
                            if (evaluate_dimension(dimension_val, bom.relation_op, bom.target_dim_val)) {
                                double child_gross = net_demand * bom.per_qty * (1.0 + bom.scrap);
                                if (parts[bom.child_id].round_to_integer) {
                                    child_gross = std::ceil(child_gross);
                                }
                                double child_dim_val = (bom.relation_op == static_cast<uint8_t>(RelationOp::PASS)) 
                                                       ? dimension_val 
                                                       : bom.target_dim_val;
                                #pragma omp atomic
                                gross_demand[bom.child_id][ev.day][(child_dim_val <= 100.0) ? 0 : ((child_dim_val <= 101.0) ? 1 : 2)] += child_gross;
                            }
                        }
                    } else {
                        double run_rate = parts[part_id].run_rate;
                        double max_qty = (run_rate > 0.0) ? (1.0 / run_rate) : -1.0;

                        if (max_qty > 0.0 && net_demand > max_qty) {
                            double remaining_qty = net_demand;
                            std::vector<double> chunk_qtys;
                            while (remaining_qty > 0.0) {
                                double cq = std::min(remaining_qty, max_qty);
                                chunk_qtys.push_back(cq);
                                remaining_qty -= cq;
                            }

                            int current_finish_day = ev.day;
                            for (int i = static_cast<int>(chunk_qtys.size()) - 1; i >= 0; --i) {
                                double cq = chunk_qtys[i];
                                if (part_lot_size > 0.0) {
                                    cq = std::ceil(cq / part_lot_size) * part_lot_size;
                                }
                                double chunk_lt = parts[part_id].lead_time + cq * run_rate;
                                int chunk_start_day = get_workday_offset_backward(current_finish_day, static_cast<int>(chunk_lt), parts[part_id].planning_calendar, global_wc_daily_capacity);
                                if (chunk_start_day < parts[part_id].time_fence_days) {
                                    chunk_start_day = parts[part_id].time_fence_days;
                                }
                                int actual_finish_day = get_workday_offset_forward(chunk_start_day, static_cast<int>(chunk_lt), parts[part_id].planning_calendar, global_wc_daily_capacity);
                                actual_finish_day = std::max(actual_finish_day, chunk_start_day);

                                PlannedOrder p_ord = {part_id, cq, chunk_start_day, actual_finish_day, dimension_val};
                                thread_ipc_planned_orders[tid].push_back(p_ord);

                                current_inv += (cq - chunk_qtys[i]);

                                explode_planned_order_to_children(part_id, cq, chunk_start_day, ev.day, dimension_val, boms, parts, gross_demand, gross_demand_priority, parent_to_bom_indices, current_on_hand, thread_alt_records, tid, ev_priority);
                                current_finish_day = get_workday_offset_backward(current_finish_day, 1, parts[part_id].planning_calendar, global_wc_daily_capacity);
                            }
                        } else {
                            double p_qty = net_demand;
                            if (part_lot_size > 0.0) {
                                p_qty = std::ceil(net_demand / part_lot_size) * part_lot_size;
                            }
                            double lt = parts[part_id].lead_time + p_qty * run_rate;
                            int start_day = get_workday_offset_backward(ev.day, static_cast<int>(lt), parts[part_id].planning_calendar, global_wc_daily_capacity);
                            if (start_day < parts[part_id].time_fence_days) {
                                start_day = parts[part_id].time_fence_days;
                            }
                            int finish_day = get_workday_offset_forward(start_day, static_cast<int>(lt), parts[part_id].planning_calendar, global_wc_daily_capacity);
                            finish_day = std::max(finish_day, start_day);

                            PlannedOrder p_ord = {part_id, p_qty, start_day, finish_day, dimension_val};
                            thread_ipc_planned_orders[tid].push_back(p_ord);

                            current_inv += (p_qty - net_demand);

                            explode_planned_order_to_children(part_id, p_qty, start_day, ev.day, dimension_val, boms, parts, gross_demand, gross_demand_priority, parent_to_bom_indices, current_on_hand, thread_alt_records, tid, ev_priority);
                        }
                    }
                }
            }
            double final_inv = current_inv;
            for (int d = last_sr_day + 1; d < timeline_days; ++d) {
                for (const auto& sr : local_srs) {
                    if (sr.due_day == d) {
                        final_inv += (sr.qty - sr.allocated);
                    }
                }
            }
            current_on_hand[part_id] = final_inv;
        };

        #ifdef _OPENMP
        #pragma omp parallel for schedule(dynamic, 1)
        #endif
        for (int j_idx = 0; j_idx < jobs_count; ++j_idx) {
            int tid = 0;
#ifdef _OPENMP
            tid = omp_get_thread_num();
#endif
            const auto& job = jobs[j_idx];
            if (!job.is_group) {
                process_part(job.single_part_id, tid);
            } else {
                for (uint32_t pid : job.group_part_ids) {
                    process_part(pid, tid);
                }
            }
        }
    }

    for (int t = 0; t < num_threads; ++t) {
        out_ipc_planned_orders.insert(out_ipc_planned_orders.end(), thread_ipc_planned_orders[t].begin(), thread_ipc_planned_orders[t].end());
        out_alt_records.insert(out_alt_records.end(), thread_alt_records[t].begin(), thread_alt_records[t].end());
        out_swap_records.insert(out_swap_records.end(), thread_swap_records[t].begin(), thread_swap_records[t].end());
    }
}

} // namespace ipc
