#include "ipc_core/substitution_engine.h"
#include <cmath>
#include <algorithm>

namespace ipc_core {

uint32_t allocate_class1(
    double net_demand,
    std::vector<FlatBomItem*>& group_items,
    std::vector<double>& current_on_hand
) {
    double total_hist = 0.0;
    for (auto* item : group_items) total_hist += item->historical_qty;

    double current_total_demand = total_hist + net_demand;
    FlatBomItem* best_alt = nullptr;
    double max_gap = -1.0;

    for (auto* item : group_items) {
        double due_qty = current_total_demand * item->target_ratio;
        double gap = std::abs(item->historical_qty - due_qty);

        if (gap > max_gap) {
            max_gap = gap;
            best_alt = item;
        } else if (std::abs(gap - max_gap) < 1e-9) {
            // 平局决断：配额比例大者优先
            if (best_alt == nullptr || item->target_ratio > best_alt->target_ratio) {
                best_alt = item;
            }
        }
    }
    return best_alt ? best_alt->child_id : uint32_t(-1);
}

uint32_t allocate_class2(
    std::vector<FlatBomItem*>& group_items
) {
    FlatBomItem* best_alt = nullptr;
    double min_rating = 9999999999.0;

    for (auto* item : group_items) {
        double ratio = item->target_ratio > 0.0 ? item->target_ratio : 1.0;
        double rating = item->historical_qty / ratio;

        if (rating < min_rating) {
            min_rating = rating;
            best_alt = item;
        } else if (std::abs(rating - min_rating) < 1e-9) {
            // 平局决断：配额比例大者优先
            if (best_alt == nullptr || item->target_ratio > best_alt->target_ratio) {
                best_alt = item;
            }
        }
    }
    return best_alt ? best_alt->child_id : uint32_t(-1);
}

void allocate_class3(
    double net_demand,
    std::vector<FlatBomItem*>& group_items,
    std::vector<double>& current_on_hand,
    int day,
    uint32_t part_id,
    std::vector<AlternateAllocationRecord>& out_alt_records,
    const std::vector<PartSiteRecord>& parts
) {
    struct ActiveCandidate {
        FlatBomItem* item;
        double original_ratio;
        double current_ratio;
        double due_qty;
    };

    std::vector<ActiveCandidate> active_candidates;
    for (auto* item : group_items) {
        active_candidates.push_back({item, item->target_ratio, item->target_ratio, 0.0});
    }

    double remaining_net = net_demand;

    while (remaining_net > 0.0 && !active_candidates.empty()) {
        // 1. 计算应分配量 due_qty = current_ratio * remaining_net
        for (auto& cand : active_candidates) {
            cand.due_qty = cand.current_ratio * remaining_net;
        }

        // 2. 按应分配量从大到小排序
        std::sort(active_candidates.begin(), active_candidates.end(), [](const ActiveCandidate& a, const ActiveCandidate& b) {
            return a.due_qty > b.due_qty;
        });

        // 3. 选择应分配量最大的候选件并应用批量规则
        auto chosen_it = active_candidates.begin();
        FlatBomItem* chosen_item = chosen_it->item;
        uint32_t alt_pid = chosen_item->child_id;
        double lot = chosen_item->lot_size > 0.0 ? chosen_item->lot_size : 1.0;

        // 实际分配量 = ⌈应分配量/订单倍数⌉ * 订单倍数
        double actual_qty = std::ceil(chosen_it->due_qty / lot) * lot;

        // 扣减消纳 (受限于库存和剩余净需求，并扣除安全库存作为保护)
        double alt_avail = std::max(0.0, current_on_hand[alt_pid] - parts[alt_pid].safety_stock);
        double alt_consumed = std::min(actual_qty, std::min(alt_avail, remaining_net));

        if (alt_consumed > 0.0) {
            current_on_hand[alt_pid] -= alt_consumed;
            remaining_net -= alt_consumed;
            chosen_item->historical_qty += alt_consumed;
            AlternateAllocationRecord rec = {static_cast<uint32_t>(day), part_id, alt_pid, alt_consumed, day, 3};
            out_alt_records.push_back(rec);
        }

        // 4. 从本 NET 环路中剔除已被分配的候选件
        active_candidates.erase(chosen_it);

        if (remaining_net <= 0.0 || active_candidates.empty()) {
            break;
        }

        // 5. 重新计算未被选择的剩余替换料的配额比例 (归一化)
        double sum_remaining_due = 0.0;
        for (const auto& cand : active_candidates) {
            sum_remaining_due += cand.due_qty;
        }

        if (sum_remaining_due > 0.0) {
            for (auto& cand : active_candidates) {
                cand.current_ratio = cand.due_qty / sum_remaining_due;
            }
        } else {
            // 回退到原始比例化
            double sum_original = 0.0;
            for (const auto& cand : active_candidates) {
                sum_original += cand.original_ratio;
            }
            if (sum_original > 0.0) {
                for (auto& cand : active_candidates) {
                    cand.current_ratio = cand.original_ratio / sum_original;
                }
            } else {
                for (auto& cand : active_candidates) {
                    cand.current_ratio = 1.0 / active_candidates.size();
                }
            }
        }
    }
}

} // namespace ipc_core
