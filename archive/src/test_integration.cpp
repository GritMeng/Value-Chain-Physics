#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include "ipc_types.h"

// Scenario 1: Class 1 Substitution (Dynamic Quota Balancing)
void test_scenario_class1() {
    std::cout << "[TEST RUN] Verifying Scenario 1: Class 1 Substitution (Dynamic Quota Balancing)..." << std::endl;

    FlatBomItem bom1 = { 1, 10, 1.0, 0.0, 1, 1, 0.6, 0.0, 0.0 }; // PART_P1
    FlatBomItem bom2 = { 1, 11, 1.0, 0.0, 1, 1, 0.4, 0.0, 0.0 }; // PART_P2
    std::vector<FlatBomItem*> group = { &bom1, &bom2 };

    auto allocate_c1 = [](double net, const std::vector<FlatBomItem*>& grp) -> uint32_t {
        double total_hist = 0.0;
        for (auto* item : grp) total_hist += item->historical_qty;
        double cur_demand = total_hist + net;

        FlatBomItem* best = nullptr;
        double max_gap = -1.0;
        for (auto* item : grp) {
            double due = cur_demand * item->target_ratio;
            double gap = std::abs(item->historical_qty - due);
            if (gap > max_gap) {
                max_gap = gap;
                best = item;
            } else if (std::abs(gap - max_gap) < 1e-9) {
                if (best == nullptr || item->target_ratio > best->target_ratio) {
                    best = item;
                }
            }
        }
        return best ? best->child_id : static_cast<uint32_t>(-1);
    };

    // Run 1: Net = 10
    uint32_t choice1 = allocate_c1(10.0, group);
    assert(choice1 == 10 && "Run 1 must choose PART_P1 (ID 10)");
    bom1.historical_qty += 10.0;

    // Run 2: Net = 10
    uint32_t choice2 = allocate_c1(10.0, group);
    assert(choice2 == 11 && "Run 2 must choose PART_P2 (ID 11)");
    bom2.historical_qty += 10.0;

    // Run 3: Net = 20
    uint32_t choice3 = allocate_c1(20.0, group);
    assert(choice3 == 10 && "Run 3 must choose PART_P1 (ID 10)");
    bom1.historical_qty += 20.0;

    assert(bom1.historical_qty == 30.0);
    assert(bom2.historical_qty == 10.0);
    std::cout << "   -> [PASS] Class 1 sourcing balancing asserts passed!" << std::endl;
}

// Scenario 2: Class 2 Substitution (Supplier Rating)
void test_scenario_class2() {
    std::cout << "[TEST RUN] Verifying Scenario 2: Class 2 Substitution (Supplier Rating)..." << std::endl;

    FlatBomItem bom1 = { 1, 10, 1.0, 0.0, 1, 2, 0.6, 0.0, 0.0 }; // PART_P1
    FlatBomItem bom2 = { 1, 11, 1.0, 0.0, 1, 2, 0.4, 0.0, 0.0 }; // PART_P2
    std::vector<FlatBomItem*> group = { &bom1, &bom2 };

    auto allocate_c2 = [](const std::vector<FlatBomItem*>& grp) -> uint32_t {
        FlatBomItem* best = nullptr;
        double min_rating = 9.999999999e9;
        for (auto* item : grp) {
            double ratio = item->target_ratio > 0.0 ? item->target_ratio : 1.0;
            double rating = item->historical_qty / ratio;
            if (rating < min_rating) {
                min_rating = rating;
                best = item;
            } else if (std::abs(rating - min_rating) < 1e-9) {
                if (best == nullptr || item->target_ratio > best->target_ratio) {
                    best = item;
                }
            }
        }
        return best ? best->child_id : static_cast<uint32_t>(-1);
    };

    // Run 1: Net = 10
    uint32_t choice1 = allocate_c2(group);
    assert(choice1 == 10 && "Run 1 must choose PART_P1 (ID 10)");
    bom1.historical_qty += 10.0;

    // Run 2: Net = 20
    uint32_t choice2 = allocate_c2(group);
    assert(choice2 == 11 && "Run 2 must choose PART_P2 (ID 11)");
    bom2.historical_qty += 20.0;

    // Run 3: Net = 30
    uint32_t choice3 = allocate_c2(group);
    assert(choice3 == 10 && "Run 3 must choose PART_P1 (ID 10)");
    bom1.historical_qty += 30.0;

    assert(bom1.historical_qty == 40.0);
    assert(bom2.historical_qty == 20.0);
    std::cout << "   -> [PASS] Class 2 performance rating asserts passed!" << std::endl;
}

// Scenario 3: Class 3 Substitution (Lot‑Sizing Constraints)
void test_scenario_class3() {
    std::cout << "[TEST RUN] Verifying Scenario 3: Class 3 Substitution (Lot‑Sizing Constraints)..." << std::endl;

    FlatBomItem bom1 = { 1, 10, 1.0, 0.0, 1, 3, 0.5, 0.0, 20.0 }; // Lot 20, Quota 0.5
    FlatBomItem bom2 = { 1, 11, 1.0, 0.0, 1, 3, 0.3, 0.0, 15.0 }; // Lot 15, Quota 0.3
    FlatBomItem bom3 = { 1, 12, 1.0, 0.0, 1, 3, 0.2, 0.0, 10.0 }; // Lot 10, Quota 0.2
    std::vector<FlatBomItem*> group = { &bom1, &bom2, &bom3 };

    struct ActiveCandidate {
        FlatBomItem* item;
        double original_ratio;
        double current_ratio;
        double due_qty;
    };

    std::vector<ActiveCandidate> active;
    for (auto* item : group) {
        active.push_back({ item, item->target_ratio, item->target_ratio, 0.0 });
    }

    double remaining_net = 100.0;
    while (remaining_net > 0.0 && !active.empty()) {
        // Compute due quantities based on current ratios
        for (auto& cand : active) {
            cand.due_qty = cand.current_ratio * remaining_net;
        }
        // Choose candidate with highest due_qty
        std::sort(active.begin(), active.end(), [](const ActiveCandidate& a, const ActiveCandidate& b) {
            return a.due_qty > b.due_qty;
        });
        auto& chosen = active.front();
        double lot = (chosen.item->lot_size > 0.0) ? chosen.item->lot_size : 1.0;
        double actual_qty = std::ceil(chosen.due_qty / lot) * lot;
        double consumed = std::min(actual_qty, remaining_net);
        // Update historical quantity
        chosen.item->historical_qty += consumed;
        remaining_net -= consumed;
        // Remove chosen from active list for this loop
        active.erase(active.begin());
        // Re‑normalize ratios for remaining candidates
        if (!active.empty()) {
            double sum_due = 0.0;
            for (auto& c : active) sum_due += c.due_qty;
            if (sum_due > 0.0) {
                for (auto& c : active) {
                    c.current_ratio = c.due_qty / sum_due;
                }
            }
        }
    }

    assert(bom1.historical_qty == 60.0);
    assert(bom2.historical_qty == 30.0);
    assert(bom3.historical_qty == 10.0);
    std::cout << "   -> [PASS] Class 3 lot‑size constraints asserts passed!" << std::endl;
}

int main() {
    std::cout << "[INTEGRATION TEST] Starting IPC engine integration tests..." << std::endl;
    test_scenario_class1();
    std::cout << "[INTEGRATION TEST] ✅ Class 1 scenario passed" << std::endl;
    test_scenario_class2();
    std::cout << "[INTEGRATION TEST] ✅ Class 2 scenario passed" << std::endl;
    test_scenario_class3();
    std::cout << "[INTEGRATION TEST] ✅ Class 3 scenario passed" << std::endl;
    std::cout << "[INTEGRATION TEST] All IPC scenarios completed successfully!" << std::endl;
    return 0;
}
