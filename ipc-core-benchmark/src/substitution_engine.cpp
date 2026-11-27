#include "ipc_core/substitution_engine.h"
#include <cmath>
#include <algorithm>

namespace ipc_core {

uint32_t allocate_class1(
    double net_demand,
    std::vector<BomEdgeRecord*>& group_items,
    std::vector<double>& /*current_on_hand*/
) {
    double total_hist = 0.0;
    for (auto* item : group_items) total_hist += item->cum_dispatched_qty;

    double current_total_demand = total_hist + net_demand;
    BomEdgeRecord* best_alt = nullptr;
    double max_gap = -1.0;

    for (auto* item : group_items) {
        double due_qty = current_total_demand * item->target_ratio;
        double gap = std::abs(item->cum_dispatched_qty - due_qty);

        if (gap > max_gap) {
            max_gap = gap;
            best_alt = item;
        } else if (std::abs(gap - max_gap) < 1e-9) {
            if (best_alt == nullptr || item->target_ratio > best_alt->target_ratio) {
                best_alt = item;
            }
        }
    }
    return best_alt ? best_alt->child_id : uint32_t(-1);
}

uint32_t allocate_class2(
    std::vector<BomEdgeRecord*>& group_items
) {
    BomEdgeRecord* best_alt = nullptr;
    double min_rating = 9999999999.0;

    for (auto* item : group_items) {
        double ratio = item->target_ratio > 0.0 ? item->target_ratio : 1.0;
        double rating = item->cum_dispatched_qty / ratio;

        if (rating < min_rating) {
            min_rating = rating;
            best_alt = item;
        } else if (std::abs(rating - min_rating) < 1e-9) {
            if (best_alt == nullptr || item->target_ratio > best_alt->target_ratio) {
                best_alt = item;
            }
        }
    }
    return best_alt ? best_alt->child_id : uint32_t(-1);
}

void allocate_class3(
    double net_demand,
    std::vector<BomEdgeRecord*>& group_items,
    std::vector<double>& current_on_hand,
    int day,
    uint32_t part_id,
    std::vector<SubDispatchRecord>& out_alt_records,
    const std::vector<SkuNodeRecord>& parts
) {
    struct ActiveCandidate {
        BomEdgeRecord* item;
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
        for (auto& cand : active_candidates) {
            cand.due_qty = cand.current_ratio * remaining_net;
        }

        std::sort(active_candidates.begin(), active_candidates.end(), [](const ActiveCandidate& a, const ActiveCandidate& b) {
            if (std::abs(a.due_qty - b.due_qty) > 1e-9) return a.due_qty > b.due_qty;
            return a.original_ratio > b.original_ratio;
        });

        auto chosen_it = active_candidates.begin();
        BomEdgeRecord* chosen_item = chosen_it->item;
        uint32_t alt_pid = chosen_item->child_id;
        double lot = chosen_item->lot_size > 0.0 ? chosen_item->lot_size : 1.0;

        double actual_qty = std::ceil(chosen_it->due_qty / lot) * lot;

        double alt_avail = std::max(0.0, current_on_hand[alt_pid] - parts[alt_pid].safety_stock);
        double alt_consumed = std::min(actual_qty, std::min(alt_avail, remaining_net));

        if (alt_consumed > 0.0) {
            current_on_hand[alt_pid] -= alt_consumed;
            remaining_net -= alt_consumed;
            chosen_item->cum_dispatched_qty += alt_consumed;
            SubDispatchRecord rec = {static_cast<uint32_t>(day), part_id, alt_pid, alt_consumed, day, 3};
            out_alt_records.push_back(rec);
        }

        active_candidates.erase(chosen_it);

        if (remaining_net <= 0.0 || active_candidates.empty()) {
            break;
        }

        double sum_remaining_due = 0.0;
        for (const auto& cand : active_candidates) {
            sum_remaining_due += cand.due_qty;
        }

        if (sum_remaining_due > 0.0) {
            for (auto& cand : active_candidates) {
                cand.current_ratio = cand.due_qty / sum_remaining_due;
            }
        } else {
            double sum_original = 0.0;
            for (const auto& cand : active_candidates) sum_original += cand.original_ratio;
            for (auto& cand : active_candidates) {
                cand.current_ratio = sum_original > 0.0 ? (cand.original_ratio / sum_original) : (1.0 / active_candidates.size());
            }
        }
    }
}

} // namespace ipc_core
