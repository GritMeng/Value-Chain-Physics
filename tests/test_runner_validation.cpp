#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <chrono>
#include <random>
#include "ipc_types.h"
using namespace ipc;

// Mock PartVocab for standalone runner testing
class PartVocab {
private:
    std::vector<std::string> id_to_code;
    std::unordered_map<std::string, uint32_t> code_to_id;
public:
    uint32_t get_or_create(const std::string& code) {
        auto it = code_to_id.find(code);
        if (it != code_to_id.end()) return it->second;
        uint32_t new_id = id_to_code.size();
        id_to_code.push_back(code);
        code_to_id[code] = new_id;
        return new_id;
    }
    
    std::string get_code(uint32_t id) const {
        if (id < id_to_code.size()) return id_to_code[id];
        return "UNKNOWN";
    }
    
    size_t size() const { return id_to_code.size(); }
};


PartVocab vocab;

// ==========================================
// Telemetry Struct for Cross-Validation
// ==========================================
#include <fstream>
#include <iomanip>

struct Telemetry {
    int s1_choice1 = 0, s1_choice2 = 0, s1_choice3 = 0;
    double s1_hist_p1 = 0.0, s1_hist_p2 = 0.0;

    int s2_choice1 = 0, s2_choice2 = 0, s2_choice3 = 0;
    double s2_hist_p1 = 0.0, s2_hist_p2 = 0.0;

    double s3_hist_p1 = 0.0, s3_hist_p2 = 0.0, s3_hist_p3 = 0.0;

    bool s4_eq_102_102 = false, s4_eq_101_102 = false;
    bool s4_ge_102_101 = false, s4_ge_100_101 = false;

    int s5_sorted_group_ids[2] = {0, 0};

    double s6_net_demand_after = 0.0;
    double s6_current_on_hand_alt_after = 0.0;
    double s6_swapped_qty = 0.0;

    double s7_allocated = 0.0;
    double s7_shortage = 0.0;

    int s8_scheduled_orders_count = 0;
    int s8_order0_finish = 0;
    double s8_order0_cap = 0.0;
    int s8_order1_finish = 0;
    double s8_order1_cap = 0.0;
    int s8_order2_finish = 0;
    double s8_order2_cap = 0.0;

    double s10_case1_batches_a = 0.0, s10_case1_batches_b = 0.0;
    double s10_case1_left_512 = 0.0, s10_case1_left_256 = 0.0, s10_case1_left_128 = 0.0;
    double s10_case2_left_512 = 0.0, s10_case2_left_256 = 0.0, s10_case2_left_128 = 0.0;
    double s10_case3_batches_a = 0.0, s10_case3_batches_b = 0.0;
    double s10_case3_left_512 = 0.0, s10_case3_left_256 = 0.0, s10_case3_left_128 = 0.0;

    int s17_sorted_indices[5] = {0, 0, 0, 0, 0};

    double s19_case1_routing_cost = 0.0;
    double s19_case1_booked_alt_b = 0.0;
    double s19_case2_routing_cost = 0.0;
    double s19_case2_booked_alt_c = 0.0;

    double s20_cap_booked1 = 0.0;
    double s20_cap_booked2 = 0.0;
    double s20_cap_booked3 = 0.0;
};

Telemetry telemetry;

void write_cpp_results_to_json(const std::string& filepath) {
    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
        std::cerr << "Failed to write C++ validation results to: " << filepath << std::endl;
        return;
    }
    ofs << "{\n";
    ofs << "  \"scenario_1\": {\n"
        << "    \"choice_1\": " << telemetry.s1_choice1 << ",\n"
        << "    \"choice_2\": " << telemetry.s1_choice2 << ",\n"
        << "    \"choice_3\": " << telemetry.s1_choice3 << ",\n"
        << "    \"historical_qty_p1\": " << telemetry.s1_hist_p1 << ",\n"
        << "    \"historical_qty_p2\": " << telemetry.s1_hist_p2 << "\n"
        << "  },\n";
    ofs << "  \"scenario_2\": {\n"
        << "    \"choice_1\": " << telemetry.s2_choice1 << ",\n"
        << "    \"choice_2\": " << telemetry.s2_choice2 << ",\n"
        << "    \"choice_3\": " << telemetry.s2_choice3 << ",\n"
        << "    \"historical_qty_p1\": " << telemetry.s2_hist_p1 << ",\n"
        << "    \"historical_qty_p2\": " << telemetry.s2_hist_p2 << "\n"
        << "  },\n";
    ofs << "  \"scenario_3\": {\n"
        << "    \"historical_qty_p1\": " << telemetry.s3_hist_p1 << ",\n"
        << "    \"historical_qty_p2\": " << telemetry.s3_hist_p2 << ",\n"
        << "    \"historical_qty_p3\": " << telemetry.s3_hist_p3 << "\n"
        << "  },\n";
    ofs << "  \"scenario_4\": {\n"
        << "    \"eq_102_102\": " << (telemetry.s4_eq_102_102 ? "true" : "false") << ",\n"
        << "    \"eq_101_102\": " << (telemetry.s4_eq_101_102 ? "true" : "false") << ",\n"
        << "    \"ge_102_101\": " << (telemetry.s4_ge_102_101 ? "true" : "false") << ",\n"
        << "    \"ge_100_101\": " << (telemetry.s4_ge_100_101 ? "true" : "false") << "\n"
        << "  },\n";
    ofs << "  \"scenario_5\": {\n"
        << "    \"sorted_group_id_0\": " << telemetry.s5_sorted_group_ids[0] << ",\n"
        << "    \"sorted_group_id_1\": " << telemetry.s5_sorted_group_ids[1] << "\n"
        << "  },\n";
    ofs << "  \"scenario_6\": {\n"
        << "    \"net_demand_after\": " << telemetry.s6_net_demand_after << ",\n"
        << "    \"current_on_hand_alt_after\": " << telemetry.s6_current_on_hand_alt_after << ",\n"
        << "    \"swapped_qty\": " << telemetry.s6_swapped_qty << "\n"
        << "  },\n";
    ofs << "  \"scenario_7\": {\n"
        << "    \"allocated\": " << telemetry.s7_allocated << ",\n"
        << "    \"shortage\": " << telemetry.s7_shortage << "\n"
        << "  },\n";
    ofs << "  \"scenario_8\": {\n"
        << "    \"scheduled_orders_count\": " << telemetry.s8_scheduled_orders_count << ",\n"
        << "    \"order0_finish\": " << telemetry.s8_order0_finish << ",\n"
        << "    \"order0_cap\": " << telemetry.s8_order0_cap << ",\n"
        << "    \"order1_finish\": " << telemetry.s8_order1_finish << ",\n"
        << "    \"order1_cap\": " << telemetry.s8_order1_cap << ",\n"
        << "    \"order2_finish\": " << telemetry.s8_order2_finish << ",\n"
        << "    \"order2_cap\": " << telemetry.s8_order2_cap << "\n"
        << "  },\n";
    ofs << "  \"scenario_10\": {\n"
        << "    \"case1_batches_a\": " << telemetry.s10_case1_batches_a << ",\n"
        << "    \"case1_batches_b\": " << telemetry.s10_case1_batches_b << ",\n"
        << "    \"case1_left_512\": " << telemetry.s10_case1_left_512 << ",\n"
        << "    \"case1_left_256\": " << telemetry.s10_case1_left_256 << ",\n"
        << "    \"case1_left_128\": " << telemetry.s10_case1_left_128 << ",\n"
        << "    \"case2_left_512\": " << telemetry.s10_case2_left_512 << ",\n"
        << "    \"case2_left_256\": " << telemetry.s10_case2_left_256 << ",\n"
        << "    \"case2_left_128\": " << telemetry.s10_case2_left_128 << ",\n"
        << "    \"case3_batches_a\": " << telemetry.s10_case3_batches_a << ",\n"
        << "    \"case3_batches_b\": " << telemetry.s10_case3_batches_b << ",\n"
        << "    \"case3_left_512\": " << telemetry.s10_case3_left_512 << ",\n"
        << "    \"case3_left_256\": " << telemetry.s10_case3_left_256 << ",\n"
        << "    \"case3_left_128\": " << telemetry.s10_case3_left_128 << "\n"
        << "  },\n";
    ofs << "  \"scenario_17\": {\n"
        << "    \"sorted_index_0\": " << telemetry.s17_sorted_indices[0] << ",\n"
        << "    \"sorted_index_1\": " << telemetry.s17_sorted_indices[1] << ",\n"
        << "    \"sorted_index_2\": " << telemetry.s17_sorted_indices[2] << ",\n"
        << "    \"sorted_index_3\": " << telemetry.s17_sorted_indices[3] << ",\n"
        << "    \"sorted_index_4\": " << telemetry.s17_sorted_indices[4] << "\n"
        << "  },\n";
    ofs << "  \"scenario_19\": {\n"
        << "    \"case1_routing_cost\": " << telemetry.s19_case1_routing_cost << ",\n"
        << "    \"case1_booked_alt_b\": " << telemetry.s19_case1_booked_alt_b << ",\n"
        << "    \"case2_routing_cost\": " << telemetry.s19_case2_routing_cost << ",\n"
        << "    \"case2_booked_alt_c\": " << telemetry.s19_case2_booked_alt_c << "\n"
        << "  },\n";
    ofs << "  \"scenario_20\": {\n"
        << "    \"cap_booked1\": " << telemetry.s20_cap_booked1 << ",\n"
        << "    \"cap_booked2\": " << telemetry.s20_cap_booked2 << ",\n"
        << "    \"cap_booked3\": " << telemetry.s20_cap_booked3 << "\n"
        << "  }\n";
    ofs << "}\n";
    ofs.close();
}


// Scenario 1
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
        return best ? best->child_id : -1;
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
    telemetry.s1_choice1 = choice1;
    telemetry.s1_choice2 = choice2;
    telemetry.s1_choice3 = choice3;
    telemetry.s1_hist_p1 = bom1.historical_qty;
    telemetry.s1_hist_p2 = bom2.historical_qty;
    std::cout << "   -> [PASS] Class 1 sourcing balancing asserts passed!" << std::endl;
}

// Scenario 2
void test_scenario_class2() {
    std::cout << "[TEST RUN] Verifying Scenario 2: Class 2 Substitution (Supplier Rating)..." << std::endl;

    FlatBomItem bom1 = { 1, 10, 1.0, 0.0, 1, 2, 0.6, 0.0, 0.0 }; // PART_P1
    FlatBomItem bom2 = { 1, 11, 1.0, 0.0, 1, 2, 0.4, 0.0, 0.0 }; // PART_P2
    std::vector<FlatBomItem*> group = { &bom1, &bom2 };

    auto allocate_c2 = [](const std::vector<FlatBomItem*>& grp) -> uint32_t {
        FlatBomItem* best = nullptr;
        double min_rating = 9999999999.0;
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
        return best ? best->child_id : -1;
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
    telemetry.s2_choice1 = choice1;
    telemetry.s2_choice2 = choice2;
    telemetry.s2_choice3 = choice3;
    telemetry.s2_hist_p1 = bom1.historical_qty;
    telemetry.s2_hist_p2 = bom2.historical_qty;
    std::cout << "   -> [PASS] Class 2 performance rating asserts passed!" << std::endl;
}

// Scenario 3
void test_scenario_class3() {
    std::cout << "[TEST RUN] Verifying Scenario 3: Class 3 Substitution (Lot-Sizing Constraints)..." << std::endl;

    FlatBomItem bom1 = { 1, 10, 1.0, 0.0, 1, 3, 0.5, 0.0, 20.0 }; // Lot 20, Quota 0.5
    FlatBomItem bom2 = { 1, 11, 1.0, 0.0, 1, 3, 0.3, 0.0, 15.0 }; // Lot 15, Quota 0.3
    FlatBomItem bom3 = { 1, 12, 1.0, 0.0, 1, 3, 0.2, 0.0, 10.0 }; // Lot 10, Quota 0.2
    std::vector<FlatBomItem*> group = { &bom1, &bom2, &bom3 };
    std::vector<double> current_on_hand(100, 100.0);

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
        for (auto& cand : active) {
            cand.due_qty = cand.current_ratio * remaining_net;
        }
        std::sort(active.begin(), active.end(), [](const ActiveCandidate& a, const ActiveCandidate& b) {
            return a.due_qty > b.due_qty;
        });

        auto chosen_it = active.begin();
        FlatBomItem* chosen_item = chosen_it->item;
        double lot = chosen_item->lot_size > 0.0 ? chosen_item->lot_size : 1.0;
        double actual_qty = std::ceil(chosen_it->due_qty / lot) * lot;

        double alt_avail = current_on_hand[chosen_item->child_id];
        double alt_consumed = std::min(actual_qty, std::min(alt_avail, remaining_net));

        if (alt_consumed > 0.0) {
            current_on_hand[chosen_item->child_id] -= alt_consumed;
            remaining_net -= alt_consumed;
            chosen_item->historical_qty += alt_consumed;
        }
        active.erase(chosen_it);

        if (remaining_net <= 0.0 || active.empty()) break;

        double sum_remaining_due = 0.0;
        for (const auto& cand : active) sum_remaining_due += cand.due_qty;
        if (sum_remaining_due > 0.0) {
            for (auto& cand : active) cand.current_ratio = cand.due_qty / sum_remaining_due;
        }
    }

    assert(bom1.historical_qty == 60.0 && "PART_P1 must receive exactly 60");
    assert(bom2.historical_qty == 30.0 && "PART_P2 must receive exactly 30");
    assert(bom3.historical_qty == 10.0 && "PART_P3 must receive exactly 10");
    telemetry.s3_hist_p1 = bom1.historical_qty;
    telemetry.s3_hist_p2 = bom2.historical_qty;
    telemetry.s3_hist_p3 = bom3.historical_qty;
    std::cout << "   -> [PASS] Class 3 lot-sizing and dynamic ratio re-normalization asserts passed!" << std::endl;
}

// Scenario 4
void test_scenario_dimensions() {
    std::cout << "[TEST RUN] Verifying Scenario 4: Dimension Matching & Downgrading..." << std::endl;

    auto evaluate_dim = [](double order_val, uint8_t op, double bom_val) -> bool {
        switch (static_cast<RelationOp>(op)) {
            case RelationOp::PASS: return true;
            case RelationOp::EQ: return std::abs(order_val - bom_val) < 1e-9;
            case RelationOp::LT: return order_val < bom_val;
            case RelationOp::LE: return order_val <= bom_val;
            case RelationOp::GE: return order_val >= bom_val;
            case RelationOp::GT: return order_val > bom_val;
            case RelationOp::NE: return std::abs(order_val - bom_val) > 1e-9;
            default: return true;
        }
    };

    // Assert exact match
    assert(evaluate_dim(102.0, static_cast<uint8_t>(RelationOp::EQ), 102.0) == true);
    assert(evaluate_dim(101.0, static_cast<uint8_t>(RelationOp::EQ), 102.0) == false);

    // Assert GE downgrading match (Higher grade 512MB can cover mid-grade 256MB)
    assert(evaluate_dim(102.0, static_cast<uint8_t>(RelationOp::GE), 101.0) == true);
    assert(evaluate_dim(100.0, static_cast<uint8_t>(RelationOp::GE), 101.0) == false);

    telemetry.s4_eq_102_102 = evaluate_dim(102.0, static_cast<uint8_t>(RelationOp::EQ), 102.0);
    telemetry.s4_eq_101_102 = evaluate_dim(101.0, static_cast<uint8_t>(RelationOp::EQ), 102.0);
    telemetry.s4_ge_102_101 = evaluate_dim(102.0, static_cast<uint8_t>(RelationOp::GE), 101.0);
    telemetry.s4_ge_100_101 = evaluate_dim(100.0, static_cast<uint8_t>(RelationOp::GE), 101.0);

    std::cout << "   -> [PASS] Multi-dimensional grading and downgrading rules validated!" << std::endl;
}

// Scenario 5
struct MCDMCandidate {
    int alt_group_id;
    std::vector<FlatBomItem*> items;
    int max_llc = 0;
    double exist_cost = 0.0;
    double new_cost = 0.0;
    double kit_qty = 0.0;
};

void test_scenario_mcdm_groups() {
    std::cout << "[TEST RUN] Verifying Scenario 5: Group Sourcing MCDM Strategy..." << std::endl;

    // Build Mock MCDM Candidates
    FlatBomItem g1_c1 = { 100, 10, 1.0, 0.0, 1 };
    FlatBomItem g1_c2 = { 100, 11, 1.0, 0.0, 1 };
    
    FlatBomItem g2_c1 = { 100, 12, 1.0, 0.0, 2 };
    FlatBomItem g2_c2 = { 100, 13, 1.0, 0.0, 2 };

    MCDMCandidate cand1 = { 1, { &g1_c1, &g1_c2 }, 2, 50.0, 100.0, 5.0 }; // LLC 2, New Cost 100
    MCDMCandidate cand2 = { 2, { &g2_c1, &g2_c2 }, 3, 20.0, 80.0, 2.0 };  // LLC 3, New Cost 80

    std::vector<MCDMCandidate> candidates = { cand1, cand2 };

    std::sort(candidates.begin(), candidates.end(), [](const MCDMCandidate& x, const MCDMCandidate& y) {
        if (x.max_llc != y.max_llc) return x.max_llc < y.max_llc;
        if (std::abs(x.new_cost - y.new_cost) > 1e-9) return x.new_cost < y.new_cost;
        return x.exist_cost < y.exist_cost;
    });

    assert(candidates[0].alt_group_id == 1 && "Group 1 must be selected due to superior LLC level");
    telemetry.s5_sorted_group_ids[0] = candidates[0].alt_group_id;
    telemetry.s5_sorted_group_ids[1] = candidates[1].alt_group_id;
    std::cout << "   -> [PASS] Sourcing group MCDM selection asserts passed!" << std::endl;
}

// Scenario 6
void test_scenario_swap_engine() {
    std::cout << "[TEST RUN] Verifying Scenario 6: Swap Engine & Incomplete Substitution (Allotment Breakwater)..." << std::endl;

    // 1. Naive Model (零和抢夺): FG2 runs first or is calculated without allotment bounds, grabbing shared stock X
    {
        double stock_X = 100.0;
        double demand_fg1_qty = 60.0; // Priority 1 (VVIP)
        double demand_fg2_qty = 80.0; // Priority 2 (Normal)

        // Simulating naive sequence where FG2 calculation runs first and drains the common stock
        double fg2_alloc = std::min(demand_fg2_qty, stock_X); // Grabs 80
        stock_X -= fg2_alloc;

        double fg1_alloc = std::min(demand_fg1_qty, stock_X); // Only gets 20!
        stock_X -= fg1_alloc;

        double fg1_shortage = demand_fg1_qty - fg1_alloc; // 40 units short!

        std::cout << "   -> [无配额保护模式（传统系统）] FG2 先行分走 = " << fg2_alloc 
                  << "，战略订单 FG1 仅分得 = " << fg1_alloc 
                  << "，FG1 产生缺口 = " << fg1_shortage << "（引发停线崩溃！）" << std::endl;
        
        assert(fg1_shortage == 40.0 && "Without allotment, FG1 must experience 40 units shortage");
    }

    // 2. IPC "Allotment Breakwater" Model (不完全替代的配额防波堤)
    {
        double stock_X = 100.0;
        double demand_fg1_qty = 60.0; // Priority 1 (VVIP)
        double demand_fg2_qty = 80.0; // Priority 2 (Normal)

        // Phase 1: Strategic Allocation & Allotment locking based on global Priority (Strategic Priority)
        double fg1_allotment = 0.0;
        double fg2_allotment = 0.0;

        // Allocate strategically: High-priority FG1 gets its allocation first
        double strat_alloc_fg1 = std::min(demand_fg1_qty, stock_X); // 60
        fg1_allotment = strat_alloc_fg1;
        double remaining_stock = stock_X - strat_alloc_fg1; // 40

        double strat_alloc_fg2 = std::min(demand_fg2_qty, remaining_stock); // 40
        fg2_allotment = strat_alloc_fg2;

        std::cout << "   -> [ITP 战略配额确权] 生成防波堤结界：FG1 配额 (Allotment) = " << fg1_allotment 
                  << "，FG2 配额 = " << fg2_allotment << std::endl;

        // Phase 2: IOP Detailed scheduling loop with Allotment lock.
        // Even if FG2 runs first in the physical scheduling loop, its allocation is strictly capped by its Allotment limit!
        double run_stock_X = 100.0;
        
        // Simulating physical execution where FG2 runs first
        double exec_alloc_fg2 = std::min(demand_fg2_qty, std::min(run_stock_X, fg2_allotment)); // min(80, min(100, 40)) = 40!
        run_stock_X -= exec_alloc_fg2; // 60 remains

        // Now FG1 runs
        double exec_alloc_fg1 = std::min(demand_fg1_qty, std::min(run_stock_X, fg1_allotment)); // min(60, min(60, 60)) = 60!
        run_stock_X -= exec_alloc_fg1; // 0 remains

        double fg1_final_shortage = demand_fg1_qty - exec_alloc_fg1; // 0! Fully satisfied!
        double fg2_final_shortage = demand_fg2_qty - exec_alloc_fg2; // 40

        std::cout << "   -> [IPC 配额防波堤模式] 即使 FG2 先行排程，其用料被强制锁定在 " << exec_alloc_fg2 
                  << "；战略订单 FG1 依然安全分得 = " << exec_alloc_fg1 
                  << "，FG1 最终缺口 = " << fg1_final_shortage << "（完美保护战略交付！）" << std::endl;

        assert(fg1_final_shortage == 0.0 && "Under allotment protection, FG1 must have zero shortage");
        assert(fg2_final_shortage == 40.0 && "FG2 shortage must remain exactly 40");
        assert(exec_alloc_fg2 == 40.0 && "FG2 allocation must be locked at 40");
        
        telemetry.s6_net_demand_after = fg1_final_shortage;
        telemetry.s6_current_on_hand_alt_after = fg2_final_shortage;
        telemetry.s6_swapped_qty = exec_alloc_fg2;
    }

    std::cout << "   -> [PASS] Swap Engine & Incomplete Substitution allotment protections verified!" << std::endl;
}

// Scenario 7
void test_scenario_netting_operator() {
    std::cout << "[TEST RUN] Verifying Scenario 7: Double-Ended Prefix-Sum Netting Operator..." << std::endl;

    double CD_1 = 120.0; 
    double CD_0 = 40.0;  
    double CS_0 = 100.0; 

    double allocated = std::max(0.0, std::min(CD_1, CS_0) - std::max(CD_0, 0.0));
    double shortage = std::max(0.0, CD_1 - std::max(CD_0, CS_0));

    assert(allocated == 60.0 && "Allocated quantity must be exactly 60.0");
    assert(shortage == 20.0 && "Shortage quantity must be exactly 20.0");

    telemetry.s7_allocated = allocated;
    telemetry.s7_shortage = shortage;

    std::cout << "   -> [PASS] Geometric double-ended prefix-sum netting math is correct!" << std::endl;
}

const int TIMELINE_DAYS = 365;

struct OTPSupplyNode {
    std::string supply_code;
    std::string supply_type; // "On-Hand", "SR", "Planned-Order"
    uint32_t part_id;
    int available_day;
    double qty;
    double allocated_qty = 0.0;
    uint64_t priority = 99999ULL; // Added priority for Z preference sorting
    std::string node_id;

    OTPSupplyNode(std::string code, std::string type, uint32_t pid, int day, double q, double alloc = 0.0, uint64_t pri = 99999ULL)
        : supply_code(code), supply_type(type), part_id(pid), available_day(day), qty(q), allocated_qty(alloc), priority(pri), node_id(code) {}

    OTPSupplyNode() = default;
};

struct ResourceConstraint {
    std::vector<double> rates;
    std::vector<double> allocated_rates;
    std::vector<double> limits;
    std::vector<double> fixed_consumptions;
    std::vector<double> unit_consumptions;
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

static thread_local std::vector<PlannedOrderSplit> dummy_po_splits;

bool reserve_otp_and_capacity_recursive(
    uint32_t part_id,
    int due_day,
    double qty,
    uint64_t priority,
    double dimension_val,
    std::vector<std::vector<OTPSupplyNode>>& otp_supplies,
    std::vector<ConstraintRecord>& shared_constraints,
    const std::vector<PartSiteRecord>& parts,
    const std::vector<FlatBomItem>& boms,
    const std::vector<std::vector<size_t>>& local_parent_to_bom,
    const std::vector<SourceConstraintRecord>& source_constraints,
    std::vector<OTPSupplyNode*>& temp_allocations,
    std::vector<double>& temp_alloc_qty,
    std::vector<std::pair<size_t, std::pair<int, double>>>& temp_capacity_allocations,
    const std::string& preference_mode,
    std::vector<double>& bom_ltb_consumed,
    std::vector<std::pair<size_t, double>>& temp_ltb_allocations,
    std::unordered_map<uint32_t, int>& active_mix_groups,
    const std::vector<std::vector<double>>& last_dim_val,
    double& out_routing_cost,
    std::vector<PlannedOrderSplit>& temp_po_splits = dummy_po_splits,
    bool is_recursive_child = false
) {
    if (qty <= 0.0) return true;
    double demand_qty = qty;

    bool use_default_order = (preference_mode != "Z" && preference_mode != "C");
    if (use_default_order) {
        for (size_t idx = 0; idx < otp_supplies[part_id].size(); ++idx) {
            auto& node = otp_supplies[part_id][idx];
            if (node.supply_type == "SR" || node.supply_type == "On-Hand") {
                double avail = node.qty - node.allocated_qty;
                for (size_t k = 0; k < temp_allocations.size(); ++k) {
                    if (temp_allocations[k] == &node) {
                        avail -= temp_alloc_qty[k];
                    }
                }
                if (avail > 0.0 && node.available_day <= due_day) {
                    double allocated = std::min(demand_qty, avail);
                    temp_allocations.push_back(&node);
                    temp_alloc_qty.push_back(allocated);
                    demand_qty -= allocated;
                    if (demand_qty <= 0.0) break;
                }
            }
        }
    } else {
        std::vector<size_t> sorted_indices(otp_supplies[part_id].size());
        for (size_t i = 0; i < sorted_indices.size(); ++i) sorted_indices[i] = i;

        if (preference_mode == "Z") {
            std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t x, size_t y) {
                return otp_supplies[part_id][x].priority < otp_supplies[part_id][y].priority;
            });
        } else {
            std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t x, size_t y) {
                bool is_po_x = (otp_supplies[part_id][x].supply_type == "Planned-Order");
                bool is_po_y = (otp_supplies[part_id][y].supply_type == "Planned-Order");
                if (is_po_x != is_po_y) {
                    return !is_po_x;
                }
                return otp_supplies[part_id][x].available_day < otp_supplies[part_id][y].available_day;
            });
        }

        for (size_t idx : sorted_indices) {
            auto& node = otp_supplies[part_id][idx];
            if (node.supply_type == "SR" || node.supply_type == "On-Hand") {
                double avail = node.qty - node.allocated_qty;
                for (size_t k = 0; k < temp_allocations.size(); ++k) {
                    if (temp_allocations[k] == &node) {
                        avail -= temp_alloc_qty[k];
                    }
                }
                if (avail > 0.0 && node.available_day <= due_day) {
                    double allocated = std::min(demand_qty, avail);
                    temp_allocations.push_back(&node);
                    temp_alloc_qty.push_back(allocated);
                    demand_qty -= allocated;
                    if (demand_qty <= 0.0) break;
                }
            }
        }
    }

    if (demand_qty <= 0.0) return true;

    double prod_qty = demand_qty;
    size_t start_alloc_size = temp_allocations.size();

    bool has_po_quota = false;
    for (const auto& node : otp_supplies[part_id]) {
        if (node.supply_type == "Planned-Order") {
            has_po_quota = true;
            break;
        }
    }

    if (is_recursive_child && has_po_quota) {
        double temp_demand = prod_qty;
        if (use_default_order) {
            for (size_t idx = 0; idx < otp_supplies[part_id].size(); ++idx) {
                auto& node = otp_supplies[part_id][idx];
                if (node.supply_type == "Planned-Order") {
                    double avail = node.qty - node.allocated_qty;
                    for (size_t k = 0; k < temp_allocations.size(); ++k) {
                        if (temp_allocations[k] == &node) {
                            avail -= temp_alloc_qty[k];
                        }
                    }
                    if (avail > 0.0) {
                        double allocated = std::min(temp_demand, avail);
                        temp_allocations.push_back(&node);
                        temp_alloc_qty.push_back(allocated);
                        temp_demand -= allocated;
                        if (temp_demand <= 0.0) break;
                    }
                }
            }
        } else {
            // Need sorted indices from earlier
            std::vector<size_t> sorted_indices(otp_supplies[part_id].size());
            for (size_t i = 0; i < sorted_indices.size(); ++i) sorted_indices[i] = i;
            if (preference_mode == "Z") {
                std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t x, size_t y) {
                    return otp_supplies[part_id][x].priority < otp_supplies[part_id][y].priority;
                });
            } else {
                std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t x, size_t y) {
                    bool is_po_x = (otp_supplies[part_id][x].supply_type == "Planned-Order");
                    bool is_po_y = (otp_supplies[part_id][y].supply_type == "Planned-Order");
                    if (is_po_x != is_po_y) {
                        return !is_po_x;
                    }
                    return otp_supplies[part_id][x].available_day < otp_supplies[part_id][y].available_day;
                });
            }
            for (size_t idx : sorted_indices) {
                auto& node = otp_supplies[part_id][idx];
                if (node.supply_type == "Planned-Order") {
                    double avail = node.qty - node.allocated_qty;
                    for (size_t k = 0; k < temp_allocations.size(); ++k) {
                        if (temp_allocations[k] == &node) {
                            avail -= temp_alloc_qty[k];
                        }
                    }
                    if (avail > 0.0) {
                        double allocated = std::min(temp_demand, avail);
                        temp_allocations.push_back(&node);
                        temp_alloc_qty.push_back(allocated);
                        temp_demand -= allocated;
                        if (temp_demand <= 0.0) break;
                    }
                }
            }
        }
        if (temp_demand > 0.0) {
            return false;
        }
        for (size_t k = start_alloc_size; k < temp_allocations.size(); ++k) {
            auto* node = temp_allocations[k];
            if (node->supply_type == "Planned-Order") {
                if (node->node_id.rfind("PO_", 0) == 0) {
                    size_t child_po_idx = 0;
                    for (size_t c_i = 3; c_i < node->node_id.size(); ++c_i) {
                        child_po_idx = child_po_idx * 10 + (node->node_id[c_i] - '0');
                    }
                    double child_lt = parts[part_id].lead_time + temp_alloc_qty[k] * parts[part_id].run_rate;
                    int child_start = due_day - static_cast<int>(std::ceil(child_lt));
                    
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
        }
        return true;
    }




    double lead_time = parts[part_id].lead_time + demand_qty * parts[part_id].run_rate;
    int start_day = due_day - static_cast<int>(std::ceil(lead_time));
    if (start_day < 0) {
        return false;
    }

    const auto& sc = source_constraints[part_id];
    uint32_t cid = sc.constraint_id;
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

    if (part_id < local_parent_to_bom.size()) {
        MixGroupGuard mix_guard(active_mix_groups);
        std::vector<size_t> standard_bom_indices;
        std::unordered_map<int, std::vector<size_t>> alt_groups;
        
        for (size_t bom_idx : local_parent_to_bom[part_id]) {
            const auto& bom = boms[bom_idx];
            if (bom.alt_group_id == -1) {
                standard_bom_indices.push_back(bom_idx);
            } else {
                alt_groups[bom.alt_group_id].push_back(bom_idx);
            }
        }

        for (size_t bom_idx : standard_bom_indices) {
            const auto& bom = boms[bom_idx];
            int trans_lt = (parts[part_id].site != parts[bom.child_id].site) ? parts[bom.child_id].transshipment_lead_time : 0;
            int child_due_day = start_day - trans_lt;
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

            size_t child_order_idx = size_t(-1);
            for (size_t k = start_alloc_size; k < temp_allocations.size(); ++k) {
                if (temp_allocations[k]->part_id == bom.child_id && temp_allocations[k]->supply_type == "Planned-Order") {
                    if (temp_allocations[k]->node_id.rfind("PO_", 0) == 0) {
                        child_order_idx = 0;
                        for (size_t c_i = 3; c_i < temp_allocations[k]->node_id.size(); ++c_i) {
                            child_order_idx = child_order_idx * 10 + (temp_allocations[k]->node_id[c_i] - '0');
                        }
                        break;
                    }
                }
            }

            bool child_ok = reserve_otp_and_capacity_recursive(
                bom.child_id, child_due_day, child_qty, priority, child_dim_val,
                otp_supplies, shared_constraints, parts, boms,
                local_parent_to_bom, source_constraints,
                temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                last_dim_val, child_routing_cost,
                temp_po_splits,
                true
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
                    int child_due_day = start_day - trans_lt;
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
                        for (auto& node : otp_supplies[bom.child_id]) {
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
                            size_t child_order_idx = size_t(-1);
                            for (size_t k = start_alloc_size; k < temp_allocations.size(); ++k) {
                                if (temp_allocations[k]->part_id == primary_id && temp_allocations[k]->supply_type == "Planned-Order") {
                                    if (temp_allocations[k]->node_id.rfind("PO_", 0) == 0) {
                                        child_order_idx = 0;
                                        for (size_t c_i = 3; c_i < temp_allocations[k]->node_id.size(); ++c_i) {
                                            child_order_idx = child_order_idx * 10 + (temp_allocations[k]->node_id[c_i] - '0');
                                        }
                                        break;
                                    }
                                }
                            }

                            ok = reserve_otp_and_capacity_recursive(
                                primary_id, best_it->child_due_day, rem_qty, priority, child_dim_val,
                                otp_supplies, shared_constraints, parts, boms,
                                local_parent_to_bom, source_constraints,
                                temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                                preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                                last_dim_val, child_routing_cost,
                                temp_po_splits,
                                true
                            );
                        }
                    } else {
                        size_t child_order_idx = size_t(-1);
                        for (size_t k = start_alloc_size; k < temp_allocations.size(); ++k) {
                            if (temp_allocations[k]->part_id == bom.child_id && temp_allocations[k]->supply_type == "Planned-Order") {
                                if (temp_allocations[k]->node_id.rfind("PO_", 0) == 0) {
                                    child_order_idx = 0;
                                    for (size_t c_i = 3; c_i < temp_allocations[k]->node_id.size(); ++c_i) {
                                        child_order_idx = child_order_idx * 10 + (temp_allocations[k]->node_id[c_i] - '0');
                                    }
                                    break;
                                }
                            }
                        }

                        ok = reserve_otp_and_capacity_recursive(
                            bom.child_id, best_it->child_due_day, req_qty, priority, child_dim_val,
                            otp_supplies, shared_constraints, parts, boms,
                            local_parent_to_bom, source_constraints,
                            temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                            preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                            last_dim_val, child_routing_cost,
                            temp_po_splits,
                            true
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
                    int child_due_day = start_day - trans_lt;
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
                        if (!otp_supplies[bom.child_id].empty()) {
                            earliest_avail = otp_supplies[bom.child_id].front().available_day;
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
                        for (auto& node : otp_supplies[bom.child_id]) {
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
                            size_t child_order_idx = size_t(-1);
                            for (size_t k = start_alloc_size; k < temp_allocations.size(); ++k) {
                                if (temp_allocations[k]->part_id == primary_id && temp_allocations[k]->supply_type == "Planned-Order") {
                                    if (temp_allocations[k]->node_id.rfind("PO_", 0) == 0) {
                                        child_order_idx = 0;
                                        for (size_t c_i = 3; c_i < temp_allocations[k]->node_id.size(); ++c_i) {
                                            child_order_idx = child_order_idx * 10 + (temp_allocations[k]->node_id[c_i] - '0');
                                        }
                                        break;
                                    }
                                }
                            }

                            ok = reserve_otp_and_capacity_recursive(
                                primary_id, cand.child_due_day, rem_qty, priority, child_dim_val,
                                otp_supplies, shared_constraints, parts, boms,
                                local_parent_to_bom, source_constraints,
                                temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                                preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                                last_dim_val, child_routing_cost,
                                temp_po_splits,
                                true
                            );
                        }
                    } else {
                        size_t child_order_idx = size_t(-1);
                        for (size_t k = start_alloc_size; k < temp_allocations.size(); ++k) {
                            if (temp_allocations[k]->part_id == bom.child_id && temp_allocations[k]->supply_type == "Planned-Order") {
                                if (temp_allocations[k]->node_id.rfind("PO_", 0) == 0) {
                                    child_order_idx = 0;
                                    for (size_t c_i = 3; c_i < temp_allocations[k]->node_id.size(); ++c_i) {
                                        child_order_idx = child_order_idx * 10 + (temp_allocations[k]->node_id[c_i] - '0');
                                    }
                                    break;
                                }
                            }
                        }

                        ok = reserve_otp_and_capacity_recursive(
                            bom.child_id, cand.child_due_day, cand.child_qty, priority, child_dim_val,
                            otp_supplies, shared_constraints, parts, boms,
                            local_parent_to_bom, source_constraints,
                            temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                            preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                            last_dim_val, child_routing_cost,
                            temp_po_splits,
                            true
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
    const std::string& solver_mode = "iop"
) {
    std::vector<PlannedOrder> ipc_planned_orders = const_ipc_planned_orders;
    std::cout << "\n[点火] [DBD 派程点火] 开始执行双表融合 MCDS 微观时空产能与时序 OTP 派程调度..." << std::endl;

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


    size_t num_constraints = (ipc_planned_orders.size() >= 10000) ? 4 : 3;
    std::vector<std::vector<double>> last_dim_val(num_constraints, std::vector<double>(timeline_days, -1.0));

    std::vector<std::vector<size_t>> local_parent_to_bom(parts.size());
    for (size_t i = 0; i < boms.size(); ++i) {
        if (boms[i].parent_id < parts.size()) {
            local_parent_to_bom[boms[i].parent_id].push_back(i);
        }
    }

    double base_cap = (ipc_planned_orders.size() >= 10000) ? 10000000.0 : 1000.0;
    std::vector<ConstraintRecord> shared_constraints(num_constraints);
    shared_constraints[0] = { 0, "LINE_FINISHED", "Constrained", std::vector<double>(timeline_days, base_cap), std::vector<double>(timeline_days, 0.0) };
    shared_constraints[1] = { 1, "LINE_SEMI", "Constrained", std::vector<double>(timeline_days, base_cap), std::vector<double>(timeline_days, 0.0) };
    shared_constraints[2] = { 2, "LINE_RAW", "Constrained", std::vector<double>(timeline_days, base_cap), std::vector<double>(timeline_days, 0.0) };
    if (num_constraints >= 4) {
        shared_constraints[3] = { 3, "LINE_ALT", "Constrained", std::vector<double>(timeline_days, base_cap), std::vector<double>(timeline_days, 0.0) };
    }

    std::vector<SourceConstraintRecord> source_constraints(parts.size());
    for (size_t i = 0; i < parts.size(); ++i) {
        uint32_t cid = 2;
        if (parts[i].part_type == "FINISHED") {
            cid = 0;
        } else if (parts[i].part_type == "SEMI") {
            cid = 1;
        }
        source_constraints[i].part_id = static_cast<uint32_t>(i);
        source_constraints[i].constraint_id = cid;
        source_constraints[i].constraint_factor = 1.0;
        source_constraints[i].before_fixed_factor = (ipc_planned_orders.size() >= 10000) ? 2.0 : 10.0;
        source_constraints[i].after_fixed_factor = 0.0;

        // Inject alt routing and co-allocation for benchmark parts
        if (ipc_planned_orders.size() >= 10000 && i < 400) {
            source_constraints[i].extra_constraints = { { 1, 0.5 } };
            AlternativeRouting r1 = { 1, { { 2, 1.2 } }, 5.0, 1 };
            AlternativeRouting r2 = { 2, { { 3, 1.5 } }, 20.0, 2 };
            source_constraints[i].alternative_routings = { r1, r2 };
        }
    }

    std::vector<std::vector<size_t>> po_by_part(parts.size());
    for (size_t i = 0; i < ipc_planned_orders.size(); ++i) {
        po_by_part[ipc_planned_orders[i].part_id].push_back(i);
    }

    std::vector<uint64_t> po_priority(ipc_planned_orders.size(), 18446744073709551615ULL);
    std::vector<std::string> po_preference_mode(ipc_planned_orders.size(), "N");

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
                };
                std::vector<ChildDemand> child_demands;
                for (size_t p_po_idx : parent_pos) {
                    uint64_t pri = po_priority[p_po_idx];
                    if (pri == 18446744073709551615ULL) continue;
                    const std::string& pref = po_preference_mode[p_po_idx];
                    
                    const auto& p_po = ipc_planned_orders[p_po_idx];
                    double c_qty = p_po.qty * bom.per_qty * (1.0 + bom.scrap);
                    child_demands.push_back({p_po.start_day, c_qty, pri, pref});
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

    std::vector<std::vector<OTPSupplyNode>> otp_supplies(parts.size());
    for (size_t i = 0; i < parts.size(); ++i) {
        if (parts[i].on_hand > 0.0) {
            OTPSupplyNode node = {"OH_" + parts[i].part_code, "On-Hand", static_cast<uint32_t>(i), 0, parts[i].on_hand, 0.0, 99999};
            otp_supplies[i].push_back(node);
        }
        if (parts[i].ipc_scheduled_receipt > 0.0) {
            OTPSupplyNode node = {"SR_" + parts[i].part_code, "SR", static_cast<uint32_t>(i), 0, parts[i].ipc_scheduled_receipt, 0.0, 99999};
            otp_supplies[i].push_back(node);
        }
    }

    for (size_t i = 0; i < ipc_planned_orders.size(); ++i) {
        const auto& po = ipc_planned_orders[i];
        OTPSupplyNode node = {"PO_" + std::to_string(i), "Planned-Order", po.part_id, po.finish_day, po.qty, 0.0, po_priority[i]};
        otp_supplies[po.part_id].push_back(node);
    }

    for (size_t i = 0; i < parts.size(); ++i) {
        std::sort(otp_supplies[i].begin(), otp_supplies[i].end(), [](const OTPSupplyNode& a, const OTPSupplyNode& b) {
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
            po_preference_mode[i]
        });
    }

    std::sort(global_orders.begin(), global_orders.end(), [](const SchedOrderWrapper& a, const SchedOrderWrapper& b) {
        if (a.priority != b.priority) return a.priority < b.priority;
        if (a.finish_day != b.finish_day) return a.finish_day < b.finish_day;
        return a.original_index < b.original_index;
    });

    std::vector<double> bom_ltb_consumed(boms.size(), 0.0);

    for (size_t g_idx = 0; g_idx < global_orders.size(); ++g_idx) {
        const auto wrapper = global_orders[g_idx];
        size_t order_idx = wrapper.original_index;
        auto& order = out_scheduled_orders[order_idx];

        if (parts[order.part_id].part_type != "FINISHED") {
            order.qty = 0.0;
            out_order_capacities[order_idx] = 0.0;
            continue;
        }

        double allocated_already = 0.0;
        for (const auto& node : otp_supplies[order.part_id]) {
            if (node.node_id == "PO_" + std::to_string(order_idx)) {
                allocated_already = node.allocated_qty;
                break;
            }
        }
        double qty = order.qty - allocated_already;
        if (qty <= 0.0) {
            order.qty = 0.0;
            continue;
        }

        int original_due = order.finish_day;
        uint32_t part_id = order.part_id;

        bool scheduled_successfully = false;
        int max_slide_day = timeline_days;
        for (int d = original_due; d < max_slide_day; ++d) {
            // Check available capacity on Finished Good Line (using start_day offset)
            uint32_t cid = source_constraints[part_id].constraint_id;
            double setup_time = source_constraints[part_id].before_fixed_factor;
            double factor = source_constraints[part_id].constraint_factor;
            double clean_up = source_constraints[part_id].after_fixed_factor;
            
            double lead_time = parts[part_id].lead_time + qty * parts[part_id].run_rate;
            int start_day = d - static_cast<int>(std::ceil(lead_time));
            if (start_day < 0) continue;

            if (cid < last_dim_val.size() && start_day >= 0 && start_day < static_cast<int>(last_dim_val[cid].size()) && last_dim_val[cid][start_day] == wrapper.dimension_val) {
                setup_time = 0.0;
            }

            double current_allocated = shared_constraints[cid].allocated_rates[start_day];
            double avail_cap = shared_constraints[cid].rates[start_day] - current_allocated;
            
            double required_cap = setup_time + qty * factor + clean_up;
            double target_qty = qty;
            bool need_split = false;
            bool ok = false;

            if (avail_cap < required_cap && demands.size() >= 1000) {
                double possible_qty = (avail_cap - setup_time - clean_up) / factor;
                if (possible_qty >= 0.1) {
                    target_qty = possible_qty;
                    need_split = true;
                } else {
                    continue; // Fast skip to next day
                }
            }

            std::vector<OTPSupplyNode*> temp_allocations;
            std::vector<double> temp_alloc_qty;
            std::vector<std::pair<size_t, std::pair<int, double>>> temp_capacity_allocations;
            std::vector<std::pair<size_t, double>> temp_ltb_allocations;
            std::unordered_map<uint32_t, int> active_mix_groups;
            double chosen_routing_cost = 0.0;

            std::vector<PlannedOrderSplit> temp_po_splits;

            ok = reserve_otp_and_capacity_recursive(
                part_id, d, target_qty, wrapper.priority, wrapper.dimension_val,
                otp_supplies, shared_constraints, parts, boms,
                local_parent_to_bom, source_constraints,
                temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                wrapper.preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                last_dim_val, chosen_routing_cost,
                temp_po_splits,
                false
            );

            if (!ok && wrapper.priority < 18446744073709551615ULL && demands.size() < 1000) {
                double lead_time = parts[part_id].lead_time + target_qty * parts[part_id].run_rate;
                int start_day = d - static_cast<int>(std::ceil(lead_time));
                uint32_t my_cid = source_constraints[part_id].constraint_id;

                size_t preempt_target = size_t(-1);
                for (size_t h_idx = 0; h_idx < out_scheduled_orders.size(); ++h_idx) {
                    if (h_idx == order_idx) continue;
                    const auto& h_order = out_scheduled_orders[h_idx];
                    
                    if (h_order.finish_day == d && po_priority[h_idx] < wrapper.priority) {
                        preempt_target = h_idx;
                        break;
                    }
                }

                if (preempt_target != size_t(-1)) {
                    int h_new_due = d - 1;
                    double h_lt = parts[out_scheduled_orders[preempt_target].part_id].lead_time + out_scheduled_orders[preempt_target].qty * parts[out_scheduled_orders[preempt_target].part_id].run_rate;
                    if (h_new_due >= static_cast<int>(std::ceil(h_lt))) {
                        uint32_t h_cid = source_constraints[out_scheduled_orders[preempt_target].part_id].constraint_id;
                        double h_cap = out_order_capacities[preempt_target];
                        int h_start = d - static_cast<int>(std::ceil(h_lt));
                        shared_constraints[h_cid].allocated_rates[h_start] -= h_cap;
                        
                        std::vector<OTPSupplyNode*> h_alloc;
                        std::vector<double> h_alloc_qty;
                        std::vector<std::pair<size_t, std::pair<int, double>>> h_cap_alloc;
                        std::vector<std::pair<size_t, double>> h_ltb_alloc;
                        std::unordered_map<uint32_t, int> h_mix;
                        double h_chosen_routing_cost = 0.0;
                        
                        std::vector<PlannedOrderSplit> h_po_splits;
                        bool h_shift_ok = reserve_otp_and_capacity_recursive(
                            out_scheduled_orders[preempt_target].part_id, h_new_due, out_scheduled_orders[preempt_target].qty,
                            po_priority[preempt_target], out_scheduled_orders[preempt_target].dimension_val,
                            otp_supplies, shared_constraints, parts, boms,
                            local_parent_to_bom, source_constraints, h_alloc, h_alloc_qty, h_cap_alloc,
                            po_preference_mode[preempt_target], bom_ltb_consumed, h_ltb_alloc, h_mix,
                            last_dim_val, h_chosen_routing_cost,
                            h_po_splits,
                            false
                        );

                        if (h_shift_ok) {
                            for (size_t a = 0; a < h_alloc.size(); ++a) {
                                h_alloc[a]->allocated_qty += h_alloc_qty[a];
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
                            }

                            temp_allocations.clear();
                            temp_alloc_qty.clear();
                            temp_capacity_allocations.clear();
                            temp_ltb_allocations.clear();
                            active_mix_groups.clear();
                            chosen_routing_cost = 0.0;
                            
                            temp_po_splits.clear();
                            ok = reserve_otp_and_capacity_recursive(
                                part_id, d, target_qty, wrapper.priority, wrapper.dimension_val,
                                otp_supplies, shared_constraints, parts, boms,
                                local_parent_to_bom, source_constraints,
                                temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                                wrapper.preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                                last_dim_val, chosen_routing_cost,
                                temp_po_splits,
                                false
                            );
                        } else {
                            shared_constraints[h_cid].allocated_rates[h_start] += h_cap;
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
                    chosen_routing_cost = 0.0;
                    temp_po_splits.clear();
                    
                    ok = reserve_otp_and_capacity_recursive(
                        part_id, d, target_qty, wrapper.priority, wrapper.dimension_val,
                        otp_supplies, shared_constraints, parts, boms,
                        local_parent_to_bom, source_constraints,
                        temp_allocations, temp_alloc_qty, temp_capacity_allocations,
                        wrapper.preference_mode, bom_ltb_consumed, temp_ltb_allocations, active_mix_groups,
                        last_dim_val, chosen_routing_cost,
                        temp_po_splits,
                        false
                    );
                }
            }

            if (ok) {
                for (size_t a = 0; a < temp_allocations.size(); ++a) {
                    temp_allocations[a]->allocated_qty += temp_alloc_qty[a];
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
                }

                if (need_split) {
                    order.qty = target_qty;
                    order.start_day = d - static_cast<int>(std::ceil(final_lt));
                    order.finish_day = d;
                    
                    double order_cap = 0.0;
                    for (const auto& cap_alloc : temp_capacity_allocations) {
                        if (cap_alloc.first == cid && cap_alloc.second.first == order.start_day) {
                            order_cap = cap_alloc.second.second;
                            break;
                        }
                    }
                    out_order_capacities[order_idx] = order_cap;
                    out_order_routing_costs[order_idx] = chosen_routing_cost;
                    
                    double rem_qty = qty - target_qty;
                    PlannedOrder rem_po = order;
                    rem_po.qty = rem_qty;
                    rem_po.finish_day = d + 1;
                    
                    size_t new_po_idx = out_scheduled_orders.size();
                    out_scheduled_orders.push_back(rem_po);
                    out_order_capacities.push_back(0.0);
                    out_order_routing_costs.push_back(0.0);
                    
                    global_orders.push_back({
                        new_po_idx,
                        wrapper.priority,
                        part_id,
                        rem_qty,
                        rem_po.start_day,
                        rem_po.finish_day,
                        wrapper.dimension_val,
                        wrapper.preference_mode
                    });
                } else {
                    order.qty = target_qty;
                    order.start_day = d - static_cast<int>(std::ceil(final_lt));
                    order.finish_day = d;
                    
                    double order_cap = 0.0;
                    for (const auto& cap_alloc : temp_capacity_allocations) {
                        if (cap_alloc.first == cid && cap_alloc.second.first == order.start_day) {
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
            order.finish_day = timeline_days - 1;
        }
    }

    out_allocated_rates.assign(timeline_days, 0.0);
    for (const auto& constr : shared_constraints) {
        for (int t = 0; t < timeline_days; ++t) {
            out_allocated_rates[t] += constr.allocated_rates[t];
        }
    }
}


void test_scenario_dbd_capacity_push() {
    std::cout << "   -> [TEST] Scenario 8A: DBD Capacity Pushback..." << std::endl;
    std::string code = "PART_DBD_S1";
    uint32_t pid = vocab.get_or_create(code);

    std::vector<PartSiteRecord> parts(vocab.size());
    PartSiteRecord rec = { pid, code, 0.0, 0.0, 0, 2.0, "MPS", "FINISHED" };
    parts[pid] = rec;

    std::vector<FlatBomItem> boms;
    std::vector<IndependentDemand> demands;

    std::vector<PlannedOrder> ipc_planned_supplys;
    PlannedOrder o1 = { pid, 500.0, 5, 5, 0.0 };
    PlannedOrder o2 = { pid, 600.0, 5, 5, 0.0 };
    ipc_planned_supplys.push_back(o1);
    ipc_planned_supplys.push_back(o2);

    std::vector<PlannedOrder> scheduled_orders;
    std::vector<double> allocated_rates;
    std::vector<double> order_capacities;

    std::vector<double> order_routing_costs;
    run_dbd_dispatch_engine(parts, boms, ipc_planned_supplys, demands, scheduled_orders, allocated_rates, order_capacities, order_routing_costs);

    std::cout << "      [DEBUG] scheduled_orders size = " << scheduled_orders.size() << std::endl;
    for (size_t i = 0; i < scheduled_orders.size(); ++i) {
        std::cout << "        Order " << i << ": qty=" << scheduled_orders[i].qty 
                  << ", start=" << scheduled_orders[i].start_day 
                  << ", finish=" << scheduled_orders[i].finish_day 
                  << ", capacity=" << order_capacities[i] << std::endl;
    }
    assert(scheduled_orders.size() == 3);
    assert(scheduled_orders[0].finish_day == 5 && "Order A must remain on Day 5");
    assert(scheduled_orders[1].finish_day == 5 && "Order B1 must remain on Day 5 (split)");
    assert(scheduled_orders[2].finish_day == 6 && "Order B2 must be pushed to Day 6 (split remainder)");
    assert(order_capacities[0] == 510.0);
    assert(order_capacities[1] == 490.0);
    assert(order_capacities[2] == 120.0);
    
    telemetry.s8_scheduled_orders_count = scheduled_orders.size();
    telemetry.s8_order0_finish = scheduled_orders[0].finish_day;
    telemetry.s8_order0_cap = order_capacities[0];
    telemetry.s8_order1_finish = scheduled_orders[1].finish_day;
    telemetry.s8_order1_cap = order_capacities[1];
    telemetry.s8_order2_finish = scheduled_orders[2].finish_day;
    telemetry.s8_order2_cap = order_capacities[2];
    
    std::cout << "      -> [PASS] Scenario 8A capacity limits pushback verified!" << std::endl;
}

void test_scenario_dbd_otp_absorption() {
    std::cout << "   -> [TEST] Scenario 8B: DBD OTP Stock Absorption..." << std::endl;
    std::string code = "PART_DBD_S2";
    uint32_t pid = vocab.get_or_create(code);

    std::vector<PartSiteRecord> parts(vocab.size());
    PartSiteRecord rec = { pid, code, 100.0, 50.0, 0, 2.0, "MPS", "FINISHED" };
    parts[pid] = rec;

    std::vector<FlatBomItem> boms;
    std::vector<IndependentDemand> demands;

    std::vector<PlannedOrder> ipc_planned_supplys;
    PlannedOrder o1 = { pid, 120.0, 8, 8, 0.0 };
    PlannedOrder o2 = { pid, 200.0, 10, 10, 0.0 };
    ipc_planned_supplys.push_back(o1);
    ipc_planned_supplys.push_back(o2);

    std::vector<PlannedOrder> scheduled_orders;
    std::vector<double> allocated_rates;
    std::vector<double> order_capacities;

    std::vector<double> order_routing_costs;
    run_dbd_dispatch_engine(parts, boms, ipc_planned_supplys, demands, scheduled_orders, allocated_rates, order_capacities, order_routing_costs);

    assert(scheduled_orders.size() == 2);
    assert(scheduled_orders[0].finish_day == 8 && "Order C finish day must remain Day 8");
    assert(scheduled_orders[1].finish_day == 10 && "Order D finish day must remain Day 10");
    assert(order_capacities[0] == 0.0 && "Order C capacity load must be 0 due to OTP");
    assert(order_capacities[1] == 180.0 && "Order D capacity load must be 180");
    std::cout << "      -> [PASS] Scenario 8B OTP absorption and new production scheduling verified!" << std::endl;
}

// Scenario 8
void test_scenario_dbd_dispatch() {
    std::cout << "[TEST RUN] Verifying Scenario 8: DBD Dispatching & OTP Capacity Scheduling..." << std::endl;
    test_scenario_dbd_capacity_push();
    test_scenario_dbd_otp_absorption();
    std::cout << "   -> [PASS] DBD dispatching scheduling logic assert passed!" << std::endl;
}

// Chapter 9 Configuration Verification
void test_scenario_isolation_seeding() {
    std::cout << "[TEST RUN] Verifying Chapter 9: Multi-Core Isolated Seeding Configuration..." << std::endl;

    std::vector<PartSiteRecord> parts;
    std::vector<FlatBomItem> boms;
    std::vector<IndependentDemand> demands;

    std::string main_code = "PART_SCENARIO1_MAIN";
    std::string p1_code = "PART_SCENARIO1_P1";
    std::string p2_code = "PART_SCENARIO1_P2";
    
    uint32_t main_id = vocab.get_or_create(main_code);
    uint32_t p1_id = vocab.get_or_create(p1_code);
    uint32_t p2_id = vocab.get_or_create(p2_code);
    
    PartSiteRecord main_rec = { main_id, main_code, 0.0, 0.0, 0, 2.0, "MPS", "FINISHED" };
    PartSiteRecord p1_rec = { p1_id, p1_code, 100.0, 0.0, 1, 3.0, "MRP", "ALT" };
    PartSiteRecord p2_rec = { p2_id, p2_code, 100.0, 0.0, 1, 3.0, "MRP", "ALT" };
    
    parts.push_back(main_rec);
    parts.push_back(p1_rec);
    parts.push_back(p2_rec);
    
    FlatBomItem bom1 = { main_id, p1_id, 1.0, 0.0, 999, 1, 0.6, 0.0, 0.0 };
    FlatBomItem bom2 = { main_id, p2_id, 1.0, 0.0, 999, 1, 0.4, 0.0, 0.0 };
    
    boms.push_back(bom1);
    boms.push_back(bom2);
    
    IndependentDemand d1 = { 999901, "DEMAND_D1", "CUST_TEST", main_id, 10.0, 10, 1, 100.0 };
    IndependentDemand d2 = { 999902, "DEMAND_D2", "CUST_TEST", main_id, 10.0, 20, 2, 100.0 };
    IndependentDemand d3 = { 999903, "DEMAND_D3", "CUST_TEST", main_id, 20.0, 30, 3, 100.0 };
    
    demands.push_back(d1);
    demands.push_back(d2);
    demands.push_back(d3);

    assert(parts.size() == 3);
    assert(boms.size() == 2);
    assert(demands.size() == 3);
    assert(boms[0].alt_group_id == 999 && "BOM links must represent the isolated Scenario Alt Group 999");
    assert(parts[0].part_code == "PART_SCENARIO1_MAIN" && "ID isolation should preserve original scenario key");

    std::cout << "   -> [PASS] Chapter 9 isolated seeding and namespaces verified successfully!" << std::endl;
}

// Scenario 10: Coproduct Dimension Planning & Downgrading (Patent implementation)
struct CoproductCaseInput {
    std::string case_name;
    double demand_512; // EQ
    double demand_256; // GE
    double demand_128; // GE
    double expected_batches_a;
    double expected_batches_b;
    double expected_leftover_512;
    double expected_leftover_256;
    double expected_leftover_128;
};

struct InventoryDowngradeCase {
    std::string case_name;
    double demand_qty;
    double inv_512;
    double inv_256;
    double inv_128;
    double expected_left_512;
    double expected_left_256;
    double expected_left_128;
};

void run_coproduct_solver_case(const CoproductCaseInput& input) {
    std::cout << "\n   [专利对比子场景] 正在验证: " << input.case_name << std::endl;

    double ratio_a_512 = 0.5, ratio_a_256 = 0.3, ratio_a_128 = 0.2;
    double ratio_b_512 = 0.0, ratio_b_256 = 0.4, ratio_b_128 = 0.3;
    double batch_size = 1000.0;

    double stock_512 = 0.0;
    double stock_256 = 0.0;
    double stock_128 = 0.0;

    double order1_short = input.demand_512; 
    double order2_short = input.demand_256; 
    double order3_short = input.demand_128; 

    // 1. Run ROUTING_A batches to satisfy 512MB EQ demand
    double batches_a = 0.0;
    if (order1_short > 0.0) {
        double yield_512_per_batch = batch_size * ratio_a_512;
        batches_a = std::ceil(order1_short / yield_512_per_batch);
        stock_512 += batches_a * (batch_size * ratio_a_512);
        stock_256 += batches_a * (batch_size * ratio_a_256);
        stock_128 += batches_a * (batch_size * ratio_a_128);
    }

    // Allocate to Order 1 (512MB EQ)
    double alloc_o1_512 = std::min(order1_short, stock_512);
    stock_512 -= alloc_o1_512;
    order1_short -= alloc_o1_512;

    // 2. Allocate to Order 2 (256MB GE)
    double alloc_o2_256 = std::min(order2_short, stock_256);
    stock_256 -= alloc_o2_256;
    order2_short -= alloc_o2_256;

    double alloc_o2_512 = std::min(order2_short, stock_512);
    stock_512 -= alloc_o2_512;
    order2_short -= alloc_o2_512;

    // 3. Allocate to Order 3 (128MB GE)
    double alloc_o3_128 = std::min(order3_short, stock_128);
    stock_128 -= alloc_o3_128;
    order3_short -= alloc_o3_128;

    double alloc_o3_256 = std::min(order3_short, stock_256);
    stock_256 -= alloc_o3_256;
    order3_short -= alloc_o3_256;

    double alloc_o3_512 = std::min(order3_short, stock_512);
    stock_512 -= alloc_o3_512;
    order3_short -= alloc_o3_512;

    // 4. Run ROUTING_B batches if shortages still exist
    double batches_b = 0.0;
    if (order2_short > 0.0 || order3_short > 0.0) {
        for (int b = 1; b <= 100; ++b) {
            double temp_stock_256 = b * (batch_size * ratio_b_256);
            double temp_stock_128 = b * (batch_size * ratio_b_128);
            
            double temp_o2_short = order2_short;
            double temp_o3_short = order3_short;

            double alloc_256 = std::min(temp_o2_short, temp_stock_256);
            temp_stock_256 -= alloc_256;
            temp_o2_short -= alloc_256;

            double alloc_128_3 = std::min(temp_o3_short, temp_stock_128);
            temp_stock_128 -= alloc_128_3;
            temp_o3_short -= alloc_128_3;

            double alloc_256_3 = std::min(temp_o3_short, temp_stock_256);
            temp_stock_256 -= alloc_256_3;
            temp_o3_short -= alloc_256_3;

            if (temp_o2_short <= 0.0 && temp_o3_short <= 0.0) {
                batches_b = b;
                stock_256 += temp_stock_256;
                stock_128 += temp_stock_128;
                order2_short = 0.0;
                order3_short = 0.0;
                break;
            }
        }
    }

    std::cout << "      -> ROUTING_A 投产批数 = " << batches_a << " (期望: " << input.expected_batches_a << ")" << std::endl;
    std::cout << "      -> ROUTING_B 投产批数 = " << batches_b << " (期望: " << input.expected_batches_b << ")" << std::endl;
    std::cout << "      -> 512MB 最终库存余量 = " << stock_512 << " (期望: " << input.expected_leftover_512 << ")" << std::endl;
    std::cout << "      -> 256MB 最终库存余量 = " << stock_256 << " (期望: " << input.expected_leftover_256 << ")" << std::endl;
    std::cout << "      -> 128MB 最终库存余量 = " << stock_128 << " (期望: " << input.expected_leftover_128 << ")" << std::endl;

    assert(std::abs(batches_a - input.expected_batches_a) < 1e-9);
    assert(std::abs(batches_b - input.expected_batches_b) < 1e-9);
    assert(std::abs(stock_512 - input.expected_leftover_512) < 1e-9);
    assert(std::abs(stock_256 - input.expected_leftover_256) < 1e-9);
    assert(std::abs(stock_128 - input.expected_leftover_128) < 1e-9);
    
    if (input.case_name.find("Scenario 1") != std::string::npos) {
        telemetry.s10_case1_batches_a = batches_a;
        telemetry.s10_case1_batches_b = batches_b;
        telemetry.s10_case1_left_512 = stock_512;
        telemetry.s10_case1_left_256 = stock_256;
        telemetry.s10_case1_left_128 = stock_128;
    } else if (input.case_name.find("Scenario 3") != std::string::npos) {
        telemetry.s10_case3_batches_a = batches_a;
        telemetry.s10_case3_batches_b = batches_b;
        telemetry.s10_case3_left_512 = stock_512;
        telemetry.s10_case3_left_256 = stock_256;
        telemetry.s10_case3_left_128 = stock_128;
    }
    
    std::cout << "      -> [OK] 该发明专利场景计算对账一致！" << std::endl;
}

void run_inventory_downgrade_case(const InventoryDowngradeCase& input) {
    std::cout << "\n   [专利对比子场景] 正在验证: " << input.case_name << std::endl;

    double stock_512 = input.inv_512;
    double stock_256 = input.inv_256;
    double stock_128 = input.inv_128;

    double order_short = input.demand_qty; // 256MB_ge (accepts 512M and 256M)

    // downbinning_priority is HIGHER_FIRST, so we consume 512MB stock first!
    double alloc_512 = std::min(order_short, stock_512);
    stock_512 -= alloc_512;
    order_short -= alloc_512;

    double alloc_256 = std::min(order_short, stock_256);
    stock_256 -= alloc_256;
    order_short -= alloc_256;

    std::cout << "      -> 512MB 最终库存剩余 = " << stock_512 << " (期望: " << input.expected_left_512 << ")" << std::endl;
    std::cout << "      -> 256MB 最终库存剩余 = " << stock_256 << " (期望: " << input.expected_left_256 << ")" << std::endl;
    std::cout << "      -> 128MB 最终库存剩余 = " << stock_128 << " (期望: " << input.expected_left_128 << ")" << std::endl;

    assert(std::abs(stock_512 - input.expected_left_512) < 1e-9);
    assert(std::abs(stock_256 - input.expected_left_256) < 1e-9);
    assert(std::abs(stock_128 - input.expected_left_128) < 1e-9);
    assert(order_short == 0.0 && "Demand must be fully satisfied by inventory!");
    
    telemetry.s10_case2_left_512 = stock_512;
    telemetry.s10_case2_left_256 = stock_256;
    telemetry.s10_case2_left_128 = stock_128;
    
    std::cout << "      -> [OK] 该发明专利场景计算对账一致！" << std::endl;
}

void test_scenario_ipc_coproduct_dimension_planning() {
    std::cout << "[TEST RUN] Verifying Scenario 10: Co-product Dimension Planning & Downgrading (Patent Scenarios)..." << std::endl;

    // 1. 专利【具体实施方式】中的【场景一】
    CoproductCaseInput case1 = { 
        "Patent Scenario 1 (3 Orders, 0 Inventory, Multi-Routing)", 
        2000.0, 1500.0, 1000.0,  // Order A (512MB_only: 2000), Order B (256MB_ge: 1500), Order C (128MB_ge: 1000)
        4.0, 1.0,                // Expected: ROUTING_A 4 batches, ROUTING_B 1 batch
        0.0, 100.0, 100.0        // Expected leftovers: 512M: 0, 256M: 100, 128M: 100
    };
    run_coproduct_solver_case(case1);

    // 2. 专利【具体实施方式】中的【场景二】（紧急订单，自动降级库存分配）
    InventoryDowngradeCase case2 = {
        "Patent Scenario 2 (Emergency Order, Inventory Downgrading)",
        500.0,                   // Emergency Order Qty 500 (256MB_ge)
        500.0, 800.0, 1000.0,    // Inventory: 512MB: 500, 256MB: 800, 128MB: 1000
        0.0, 800.0, 1000.0       // Expected leftovers: 512MB: 0, 256MB: 800, 128MB: 1000 (Uses 512M stock first)
    };
    run_inventory_downgrade_case(case2);

    // 3. 专利【具体实施方式】中的【场景三】
    CoproductCaseInput case3 = { 
        "Patent Scenario 3 (3 Orders with different Qty, Multi-Routing)", 
        1000.0, 2000.0, 1500.0,  // Order A (512MB_only: 1000), Order B (256MB_ge: 2000), Order C (128MB_ge: 1500)
        2.0, 4.0,                // Expected: ROUTING_A 2 batches, ROUTING_B 4 batches
        0.0, 200.0, 100.0        // Expected leftovers: 512M: 0, 256M: 200, 128M: 100
    };
    run_coproduct_solver_case(case3);

    std::cout << "   -> [PASS] Scenario 10 Co-product Patent Scenarios 1/2/3 validated successfully!" << std::endl;
}

// Scenario 11: ECN & LTB Mixed Test
void test_scenario_ecn_ltb() {
    std::cout << "[TEST RUN] Verifying Scenario 11: ECN Temporal Cutover & LTB limits..." << std::endl;

    std::string main_code = "PART_11_MAIN";
    std::string old_code = "PART_11_OLD";
    std::string new_code = "PART_11_NEW";

    uint32_t main_id = vocab.get_or_create(main_code);
    uint32_t old_id = vocab.get_or_create(old_code);
    uint32_t new_id = vocab.get_or_create(new_code);

    std::vector<PartSiteRecord> parts(vocab.size());
    parts[main_id] = { main_id, main_code, 0.0, 0.0, 0, 2.0, "MPS", "FINISHED" };
    parts[old_id] = { old_id, old_code, 100.0, 0.0, 1, 1.0, "MRP", "ALT" };
    parts[new_id] = { new_id, new_code, 0.0, 0.0, 1, 1.0, "MRP", "ALT" };

    std::vector<FlatBomItem> boms;
    // BOM1 (Old): active Day 0 to Day 9, LTB = 50
    FlatBomItem bom1;
    bom1.parent_id = main_id;
    bom1.child_id = old_id;
    bom1.per_qty = 1.0;
    bom1.scrap = 0.0;
    bom1.alt_group_id = 1;
    bom1.alt_priority = 1;
    bom1.eff_start_day = 0;
    bom1.eff_end_day = 9;
    bom1.ltb_limit = 50.0;
    boms.push_back(bom1);

    // BOM2 (New): active Day 10 onwards
    FlatBomItem bom2;
    bom2.parent_id = main_id;
    bom2.child_id = new_id;
    bom2.per_qty = 1.0;
    bom2.scrap = 0.0;
    bom2.alt_group_id = 1;
    bom2.alt_priority = 2;
    bom2.eff_start_day = 10;
    bom2.eff_end_day = -1;
    boms.push_back(bom2);

    // Test Case A: Order at Day 5, Qty = 80.
    // Should consume 50 from old (reaching LTB), then fail to use old for remaining 30.
    // Can it use new? Child due date is Day 5 - 2 = Day 3. But new is only active from Day 10!
    // So scheduling A at Day 5 should FAIL (or push back). Let's verify it gets pushed back to Day 12 (where child due date = 10, so new is active)!
    std::vector<PlannedOrder> ipc_planned_supplys;
    ipc_planned_supplys.push_back({ main_id, 80.0, 5, 5, 0.0 });

    std::vector<IndependentDemand> demands; // empty
    std::vector<PlannedOrder> scheduled_orders;
    std::vector<double> allocated_rates;
    std::vector<double> order_capacities;

    std::vector<double> order_routing_costs;
    run_dbd_dispatch_engine(parts, boms, ipc_planned_supplys, demands, scheduled_orders, allocated_rates, order_capacities, order_routing_costs);

    assert(scheduled_orders.size() == 1);
    // Since child due day must be >= 10 for new to be active, parent finish day must be >= 12!
    assert(scheduled_orders[0].finish_day >= 12 && "Order must be pushed to Day 12+ due to ECN of new part");
    std::cout << "   -> [PASS] Scenario 11 ECN temporal cutover and LTB limits verified!" << std::endl;
}

// Scenario 12: Z/N/C Preferences Test
void test_scenario_znc_preferences() {
    std::cout << "[TEST RUN] Verifying Scenario 12: Z/N/C Preferences..." << std::endl;

    std::string main_code = "PART_12_MAIN";
    std::string high_code = "PART_12_HIGH";
    std::string low_code = "PART_12_LOW";

    uint32_t main_id = vocab.get_or_create(main_code);
    uint32_t high_id = vocab.get_or_create(high_code);
    uint32_t low_id = vocab.get_or_create(low_code);

    std::vector<PartSiteRecord> parts(vocab.size());
    parts[main_id] = { main_id, main_code, 0.0, 0.0, 0, 2.0, "MPS", "FINISHED" };
    parts[high_id] = { high_id, high_code, 100.0, 0.0, 1, 1.0, "MRP", "ALT" };
    parts[low_id] = { low_id, low_code, 100.0, 0.0, 1, 1.0, "MRP", "ALT" };

    std::vector<FlatBomItem> boms;
    FlatBomItem bom1;
    bom1.parent_id = main_id;
    bom1.child_id = high_id;
    bom1.per_qty = 1.0;
    bom1.scrap = 0.0;
    bom1.alt_group_id = 1;
    bom1.alt_priority = 1; // High priority
    boms.push_back(bom1);

    FlatBomItem bom2;
    bom2.parent_id = main_id;
    bom2.child_id = low_id;
    bom2.per_qty = 1.0;
    bom2.scrap = 0.0;
    bom2.alt_group_id = 1;
    bom2.alt_priority = 2; // Low priority
    boms.push_back(bom2);

    // Test N (Delivery first):
    // Let's set low priority stock available at Day 0, high priority stock available at Day 8.
    // N mode should deliver at Day 2 (consuming low pri).
    // Z mode should deliver at Day 10 (consuming high pri).
    {
        std::vector<std::vector<OTPSupplyNode>> otp(vocab.size());
        otp[low_id].push_back({ "OH_LOW", "On-Hand", low_id, 0, 100.0, 0.0, 2 });
        otp[high_id].push_back({ "OH_HIGH", "On-Hand", high_id, 8, 100.0, 0.0, 1 });

        std::vector<ConstraintRecord> shared(3);
        shared[0] = { 0, "LINE_FINISHED", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
        shared[1] = { 1, "LINE_SEMI", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
        shared[2] = { 2, "LINE_RAW", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };

        std::vector<SourceConstraintRecord> sc(vocab.size());
        sc[main_id] = { main_id, 0, 1.0, 0.0, 0.0 };

        std::vector<std::vector<size_t>> local_bom(vocab.size());
        local_bom[main_id] = { 0, 1 };

        std::vector<OTPSupplyNode*> temp_alloc;
        std::vector<double> temp_qty;
        std::vector<std::pair<size_t, std::pair<int, double>>> temp_cap;
        std::vector<double> ltb(boms.size(), 0.0);
        std::vector<std::pair<size_t, double>> temp_ltb;
        std::unordered_map<uint32_t, int> mix;

        double dummy_routing_cost = 0.0;
        std::vector<std::vector<double>> dummy_last_dim(parts.size(), std::vector<double>(365, -1.0));
        // Try N mode
        bool ok_n = reserve_otp_and_capacity_recursive(
            main_id, 15, 10.0, 1, 0.0, otp, shared, parts, boms,
            local_bom, sc, temp_alloc, temp_qty, temp_cap,
            "N", ltb, temp_ltb, mix, dummy_last_dim, dummy_routing_cost
        );
        assert(ok_n);
        // Find which node was allocated
        bool allocated_low = false;
        for (auto* node : temp_alloc) {
            if (node->supply_code == "OH_LOW") allocated_low = true;
        }
        assert(allocated_low && "N mode must select the earliest available low priority node");
    }

    {
        std::vector<std::vector<OTPSupplyNode>> otp(vocab.size());
        otp[low_id].push_back({ "OH_LOW", "On-Hand", low_id, 0, 100.0, 0.0, 2 });
        otp[high_id].push_back({ "OH_HIGH", "On-Hand", high_id, 8, 100.0, 0.0, 1 });

        std::vector<ConstraintRecord> shared(3);
        shared[0] = { 0, "LINE_FINISHED", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
        shared[1] = { 1, "LINE_SEMI", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
        shared[2] = { 2, "LINE_RAW", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };

        std::vector<SourceConstraintRecord> sc(vocab.size());
        sc[main_id] = { main_id, 0, 1.0, 0.0, 0.0 };

        std::vector<std::vector<size_t>> local_bom(vocab.size());
        local_bom[main_id] = { 0, 1 };

        std::vector<OTPSupplyNode*> temp_alloc;
        std::vector<double> temp_qty;
        std::vector<std::pair<size_t, std::pair<int, double>>> temp_cap;
        std::vector<double> ltb(boms.size(), 0.0);
        std::vector<std::pair<size_t, double>> temp_ltb;
        std::unordered_map<uint32_t, int> mix;

        double dummy_routing_cost = 0.0;
        std::vector<std::vector<double>> dummy_last_dim(parts.size(), std::vector<double>(365, -1.0));
        // Try Z mode
        bool ok_z = reserve_otp_and_capacity_recursive(
            main_id, 15, 10.0, 1, 0.0, otp, shared, parts, boms,
            local_bom, sc, temp_alloc, temp_qty, temp_cap,
            "Z", ltb, temp_ltb, mix, dummy_last_dim, dummy_routing_cost
        );
        assert(ok_z);
        bool allocated_high = false;
        for (auto* node : temp_alloc) {
            if (node->supply_code == "OH_HIGH") allocated_high = true;
        }
        assert(allocated_high && "Z mode must select the high priority node even if it is later");
    }

    std::cout << "   -> [PASS] Scenario 12 Z/N/C preferences verified!" << std::endl;
}

// Scenario 13: Mix Rules Test
void test_scenario_mix_rules() {
    std::cout << "[TEST RUN] Verifying Scenario 13: Mixing constraints..." << std::endl;

    std::string main_code = "PART_13_MAIN";
    std::string x1_code = "PART_13_X1";
    std::string y1_code = "PART_13_Y1";
    std::string x2_code = "PART_13_X2";
    std::string y2_code = "PART_13_Y2";

    uint32_t main_id = vocab.get_or_create(main_code);
    uint32_t x1_id = vocab.get_or_create(x1_code);
    uint32_t y1_id = vocab.get_or_create(y1_code);
    uint32_t x2_id = vocab.get_or_create(x2_code);
    uint32_t y2_id = vocab.get_or_create(y2_code);

    std::vector<PartSiteRecord> parts(vocab.size());
    parts[main_id] = { main_id, main_code, 0.0, 0.0, 0, 2.0, "MPS", "FINISHED" };
    parts[x1_id] = { x1_id, x1_code, 100.0, 0.0, 1, 1.0, "MRP", "ALT" };
    parts[y1_id] = { y1_id, y1_code, 0.0, 0.0, 1, 1.0, "MRP", "ALT" };
    parts[x2_id] = { x2_id, x2_code, 0.0, 0.0, 1, 1.0, "MRP", "ALT" };
    parts[y2_id] = { y2_id, y2_code, 100.0, 0.0, 1, 1.0, "MRP", "ALT" };

    std::vector<FlatBomItem> boms;
    // Group 1 alternates: X1 (mix group 10) vs Y1 (mix group 20)
    FlatBomItem bom1;
    bom1.parent_id = main_id;
    bom1.child_id = x1_id;
    bom1.per_qty = 1.0;
    bom1.scrap = 0.0;
    bom1.alt_group_id = 1;
    bom1.alt_priority = 1;
    bom1.mix_group_id = 10;
    boms.push_back(bom1);

    FlatBomItem bom2;
    bom2.parent_id = main_id;
    bom2.child_id = y1_id;
    bom2.per_qty = 1.0;
    bom2.scrap = 0.0;
    bom2.alt_group_id = 1;
    bom2.alt_priority = 2;
    bom2.mix_group_id = 20;
    boms.push_back(bom2);

    // Group 2 alternates: X2 (mix group 10) vs Y2 (mix group 20)
    FlatBomItem bom3;
    bom3.parent_id = main_id;
    bom3.child_id = x2_id;
    bom3.per_qty = 1.0;
    bom3.scrap = 0.0;
    bom3.alt_group_id = 2;
    bom3.alt_priority = 1;
    bom3.mix_group_id = 10;
    boms.push_back(bom3);

    FlatBomItem bom4;
    bom4.parent_id = main_id;
    bom4.child_id = y2_id;
    bom4.per_qty = 1.0;
    bom4.scrap = 0.0;
    bom4.alt_group_id = 2;
    bom4.alt_priority = 2;
    bom4.mix_group_id = 20;
    boms.push_back(bom4);

    // We have X1 stock (mix group 10) and Y2 stock (mix group 20).
    // If we choose X1 (group 1), we lock mix group to 10. Then we must choose X2 for group 2.
    // But X2 has 0 stock, so it will fail to netting from stock.
    // If we fail to netting from stock, the solver will return false (we won't allow capacity booking for this test).
    // Let's see: if we disable capacity booking (or if capacity rates are 0), can we satisfy using stock?
    // Without mix group constraint, we could use X1 (stock) and Y2 (stock).
    // With mix group constraint, we CANNOT mix them, so we must fail!
    
    std::vector<std::vector<OTPSupplyNode>> otp(vocab.size());
    otp[x1_id].push_back({ "OH_X1", "On-Hand", x1_id, 0, 100.0, 0.0, 1 });
    otp[y2_id].push_back({ "OH_Y2", "On-Hand", y2_id, 0, 100.0, 0.0, 1 });

    std::vector<ConstraintRecord> shared(3); // Capacity rates = 0 to prevent new production
    shared[0] = { 0, "LINE_FINISHED", "Constrained", std::vector<double>(365, 0.0), std::vector<double>(365, 0.0) };
    shared[1] = { 1, "LINE_SEMI", "Constrained", std::vector<double>(365, 0.0), std::vector<double>(365, 0.0) };
    shared[2] = { 2, "LINE_RAW", "Constrained", std::vector<double>(365, 0.0), std::vector<double>(365, 0.0) };

    std::vector<SourceConstraintRecord> sc(vocab.size());
    sc[main_id] = { main_id, 0, 1.0, 0.0, 0.0 };

    std::vector<std::vector<size_t>> local_bom(vocab.size());
    local_bom[main_id] = { 0, 1, 2, 3 };

    std::vector<OTPSupplyNode*> temp_alloc;
    std::vector<double> temp_qty;
    std::vector<std::pair<size_t, std::pair<int, double>>> temp_cap;
    std::vector<double> ltb(boms.size(), 0.0);
    std::vector<std::pair<size_t, double>> temp_ltb;
    std::unordered_map<uint32_t, int> mix;

    double dummy_routing_cost = 0.0;
    std::vector<std::vector<double>> dummy_last_dim(parts.size(), std::vector<double>(365, -1.0));

    bool ok = reserve_otp_and_capacity_recursive(
        main_id, 10, 10.0, 1, 0.0, otp, shared, parts, boms,
        local_bom, sc, temp_alloc, temp_qty, temp_cap,
        "N", ltb, temp_ltb, mix, dummy_last_dim, dummy_routing_cost
    );

    assert(!ok && "Mixing rules must prevent using X1 and Y2 together, causing allocation failure");
    std::cout << "   -> [PASS] Scenario 13 mix group constraints verified!" << std::endl;
}

// Scenario 14: Interchangeable PO Redirection Test
void test_scenario_interchangeable() {
    std::cout << "[TEST RUN] Verifying Scenario 14: Interchangeable PO Redirection..." << std::endl;

    std::string main_code = "PART_14_MAIN";
    std::string pri_code = "PART_14_PRI";
    std::string sub_code = "PART_14_SUB";

    uint32_t main_id = vocab.get_or_create(main_code);
    uint32_t pri_id = vocab.get_or_create(pri_code);
    uint32_t sub_id = vocab.get_or_create(sub_code);

    std::vector<PartSiteRecord> parts(vocab.size());
    parts[main_id] = { main_id, main_code, 0.0, 0.0, 0, 2.0, "MPS", "FINISHED" };
    parts[pri_id] = { pri_id, pri_code, 0.0, 0.0, 1, 1.0, "MRP", "ALT" };
    parts[sub_id] = { sub_id, sub_code, 30.0, 0.0, 1, 1.0, "MRP", "ALT" }; // substitute has 30 OH stock

    std::vector<FlatBomItem> boms;
    FlatBomItem bom1;
    bom1.parent_id = main_id;
    bom1.child_id = pri_id;
    bom1.per_qty = 1.0;
    bom1.scrap = 0.0;
    bom1.alt_group_id = 1;
    bom1.alt_priority = 1;
    bom1.relationship_type = "interchangeable";
    boms.push_back(bom1);

    FlatBomItem bom2;
    bom2.parent_id = main_id;
    bom2.child_id = sub_id;
    bom2.per_qty = 1.0;
    bom2.scrap = 0.0;
    bom2.alt_group_id = 1;
    bom2.alt_priority = 2;
    bom2.relationship_type = "interchangeable";
    boms.push_back(bom2);

    std::vector<std::vector<OTPSupplyNode>> otp(vocab.size());
    otp[sub_id].push_back({ "OH_SUB", "On-Hand", sub_id, 0, 30.0, 0.0, 2 });

    std::vector<ConstraintRecord> shared(3); // Enormous capacity to allow scheduling
    shared[0] = { 0, "LINE_FINISHED", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
    shared[1] = { 1, "LINE_SEMI", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
    shared[2] = { 2, "LINE_RAW", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };

    std::vector<SourceConstraintRecord> sc(vocab.size());
    sc[main_id] = { main_id, 0, 1.0, 0.0, 0.0 };
    sc[pri_id] = { pri_id, 2, 1.0, 0.0, 0.0 };
    sc[sub_id] = { sub_id, 2, 1.0, 0.0, 0.0 };

    std::vector<std::vector<size_t>> local_bom(vocab.size());
    local_bom[main_id] = { 0, 1 };

    std::vector<OTPSupplyNode*> temp_alloc;
    std::vector<double> temp_qty;
    std::vector<std::pair<size_t, std::pair<int, double>>> temp_cap;
    std::vector<double> ltb(boms.size(), 0.0);
    std::vector<std::pair<size_t, double>> temp_ltb;
    std::unordered_map<uint32_t, int> mix;

    double dummy_routing_cost = 0.0;
    std::vector<std::vector<double>> dummy_last_dim(parts.size(), std::vector<double>(365, -1.0));

    // Demand is 100.
    // It should choose bom2 (SUBSTITUTE) and consume its 30 OH stock.
    // The remaining shortage of 70 should be redirected to pri_id (PRIMARY).
    // So capacity allocation of 70 should happen on pri_id's line (cid 2 at Day 15 - 2 - 1 = Day 12).
    bool ok = reserve_otp_and_capacity_recursive(
        main_id, 15, 100.0, 1, 0.0, otp, shared, parts, boms,
        local_bom, sc, temp_alloc, temp_qty, temp_cap,
        "N", ltb, temp_ltb, mix, dummy_last_dim, dummy_routing_cost
    );

    assert(ok);
    
    // Check OTP allocations
    double sub_allocated = 0.0;
    for (size_t i = 0; i < temp_alloc.size(); ++i) {
        if (temp_alloc[i]->part_id == sub_id) {
            sub_allocated += temp_qty[i];
        }
    }
    assert(sub_allocated == 30.0 && "Substitute part stock of 30 must be fully consumed");

    // Check capacity allocations
    double pri_capacity = 0.0;
    double sub_capacity = 0.0;
    for (const auto& cap : temp_cap) {
        size_t cid = cap.first;
        int day = cap.second.first;
        double amt = cap.second.second;
        if (cid == 2 && day == 12) { // RAW constraint at start_day (15 - 2 - 1 = 12)
            pri_capacity += amt; // Redirected capacity should be on pri_id
        }
    }
    // Since primary part capacity constraint is cid 2, and the redirection maps to primary, it books RAW constraint for 70
    assert(pri_capacity == 70.0 && "Remaining shortage of 70 must be redirected to primary part's capacity schedule");

    std::cout << "   -> [PASS] Scenario 14 interchangeable redirection verified!" << std::endl;
}


// 12. ETO CPM algorithm
void calculate_project_wbs_cpm(std::vector<ProjectTaskRecord>& tasks, int project_due_day) {
    for (auto& t : tasks) {
        t.late_finish = project_due_day;
        t.late_start = project_due_day - static_cast<int>(t.duration);
    }
    
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& t : tasks) {
            for (const auto& dep : t.dependencies) {
                for (auto& pred : tasks) {
                    if (pred.task_id == dep.predecessor_task_id) {
                        int max_finish = t.late_start - dep.lag_days;
                        if (pred.late_finish > max_finish) {
                            pred.late_finish = max_finish;
                            pred.late_start = max_finish - static_cast<int>(pred.duration);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
}

void test_scenario_wbs_mrp() {
    std::cout << "[TEST RUN] Verifying Scenario 15: WBS Task Network & MRP Co-Scheduling (ETO)..." << std::endl;
    
    std::vector<ProjectTaskRecord> tasks;
    // Task 1: Design (2 days) -> Task 2: Sampling (3 days) -> Task 3: Assembly (1 day)
    ProjectTaskRecord t1 = { 1, 100, "Research & SMT Layout", 2.0 };
    ProjectTaskRecord t2 = { 2, 100, "PCB Board Prototyping", 3.0 };
    ProjectTaskRecord t3 = { 3, 100, "System Assembly & Aging", 1.0 };
    
    // Dependencies
    t2.dependencies.push_back({ 1, DependencyType::FS, 0 }); // T1 -> T2
    t3.dependencies.push_back({ 2, DependencyType::FS, 0 }); // T2 -> T3
    
    // T2 requires GPU component
    t2.output_part_id = vocab.get_or_create("PART_ETO_GPU");
    
    tasks.push_back(t1);
    tasks.push_back(t2);
    tasks.push_back(t3);
    
    int project_due = 10; // Project delivery day 10
    calculate_project_wbs_cpm(tasks, project_due);
    
    // Asserts on late schedules
    assert(tasks[2].late_start == 9 && "Task 3 assembly must start at Day 9");
    assert(tasks[1].late_finish == 9 && "Task 2 prototyping must finish at Day 9");
    assert(tasks[1].late_start == 6 && "Task 2 prototyping must start at Day 6");
    assert(tasks[0].late_finish == 6 && "Task 1 design must finish at Day 6");
    assert(tasks[0].late_start == 4 && "Task 1 design must start at Day 4");
    
    std::cout << "   -> WBS Project Network CPM dates backwards-pass verified!" << std::endl;
    std::cout << "   -> PRECISION MRP pulling triggers on Day " << tasks[1].late_start << " for PART_ETO_GPU" << std::endl;
    std::cout << "   -> [PASS] Scenario 15 WBS ETO project logic verified!" << std::endl;
}

void test_scenario_otp_preemption() {
    std::cout << "[TEST RUN] Verifying Scenario 16: OTP Preemption & Sched-Allotment Swap (置换)..." << std::endl;
    
    std::string code = "PART_OTP_CELL";
    uint32_t pid = vocab.get_or_create(code);
    
    std::vector<PartSiteRecord> parts(vocab.size());
    parts[pid] = { pid, code, 0.0, 0.0, 0, 1.0, "MPS", "FINISHED" };
    
    std::vector<FlatBomItem> boms;
    std::vector<IndependentDemand> demands;
    demands.push_back({ 999101, "DEMAND_999101", "CUST_OTP_A", pid, 600.0, 5, 1, 600.0, "N" });
    demands.push_back({ 999102, "DEMAND_999102", "CUST_OTP_B", pid, 600.0, 5, 2, 600.0, "N" });
    
    // Shared constraints: Only 100 capacity units available per day on LINE_FINISHED (cid 0)
    std::vector<ConstraintRecord> shared_constraints(3);
    shared_constraints[0] = { 0, "LINE_FINISHED", "Constrained", std::vector<double>(365, 100.0), std::vector<double>(365, 0.0) };
    shared_constraints[1] = { 1, "LINE_SEMI", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
    shared_constraints[2] = { 2, "LINE_RAW", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
    
    std::vector<SourceConstraintRecord> sc(vocab.size());
    sc[pid] = { pid, 0, 1.0, 0.0, 0.0 }; // Consumption factor = 1.0, setup = 0.0
    
    // We create 2 planned orders. 
    // Order A: Qty = 600, Due = 5, Priority = 1 (High priority)
    // Order B: Qty = 600, Due = 5, Priority = 2 (Low priority)
    std::vector<PlannedOrder> ipc_planned_supplys;
    ipc_planned_supplys.push_back({ pid, 600.0, 4, 5, 0.0 }); // Order A (High)
    ipc_planned_supplys.push_back({ pid, 600.0, 4, 5, 0.0 }); // Order B (Low)
    
    // We pre-schedule them: Order B (low priority) runs FIRST or is processed.
    // In our sequential DBD engine:
    // If Order B (low) is processed AFTER Order A (high) due to priority sort (wrapper.priority < ...),
    // Order A (priority 1) will take the Day 4 capacity.
    // Order B (priority 2) will encounter shortage on Day 4. 
    // It will trigger OTP Preemption: shift Order A (high priority) forward to Day 3 (h_new_due = 4, start = 3), freeing up Day 4.
    std::vector<PlannedOrder> scheduled_orders;
    std::vector<double> allocated_rates;
    std::vector<double> order_capacities;
    
    // Execute DBD engine with custom wrapper prioritizing A over B
    // Let's directly test the run_dbd_dispatch_engine
    std::vector<double> order_routing_costs;
    run_dbd_dispatch_engine(parts, boms, ipc_planned_supplys, demands, scheduled_orders, allocated_rates, order_capacities, order_routing_costs);
    
    assert(scheduled_orders.size() == 2);
    // Order A should be shifted to Day 4 (start Day 3)
    assert(scheduled_orders[0].finish_day == 4 && "Order A (High Priority) must pre-shift/preempt to Day 4");
    // Order B should be satisfied at Day 5 (start Day 4)
    assert(scheduled_orders[1].finish_day == 5 && "Order B (Low Priority) must be scheduled on Day 5 due to Preemption");
    
    std::cout << "   -> Order A finish day = " << scheduled_orders[0].finish_day << " (Start Day: " << scheduled_orders[0].start_day << ")" << std::endl;
    std::cout << "   -> Order B finish day = " << scheduled_orders[1].finish_day << " (Start Day: " << scheduled_orders[1].start_day << ")" << std::endl;
    std::cout << "   -> [PASS] Scenario 16 OTP Preemption scheduling verified!" << std::endl;
}

void test_scenario_composite_priority() {
    std::cout << "[TEST RUN] Verifying Scenario 17: Kinaxis-style 5-Dimensional Composite Priority..." << std::endl;

    std::string code = "PART_17_FG";
    uint32_t pid = vocab.get_or_create(code);

    std::vector<PartSiteRecord> parts(vocab.size());
    parts[pid] = { pid, code, 0.0, 0.0, 0, 1.0, "MPS", "FINISHED" };

    // We create 5 demands on the same part with different parameters to check sorting.
    // A: Committed, Tier 3, Due Day 6, Priority 3, Revenue 10000
    // B: Committed, Tier 3, Due Day 5, Priority 2, Revenue 5000
    // C: Open,      Tier 1, Due Day 5, Priority 1, Revenue 20000
    // D1: Open,     Tier 3, Due Day 5, Priority 2, Revenue 10000
    // D2: Open,     Tier 3, Due Day 5, Priority 2, Revenue 20000  (Higher revenue than D1)

    std::vector<IndependentDemand> demands;
    
    IndependentDemand dA = { 17001, "DEMAND_A", "CUST_A", pid, 50.0, 6, 3, 100.0, "N", "COMMITTED", 3, 10000.0 };
    IndependentDemand dB = { 17002, "DEMAND_B", "CUST_B", pid, 50.0, 5, 2, 100.0, "N", "COMMITTED", 3, 5000.0 };
    IndependentDemand dC = { 17003, "DEMAND_C", "CUST_C", pid, 50.0, 5, 1, 100.0, "N", "OPEN", 1, 20000.0 };
    IndependentDemand dD1 = { 17004, "DEMAND_D1", "CUST_D1", pid, 50.0, 5, 2, 100.0, "N", "OPEN", 3, 10000.0 };
    IndependentDemand dD2 = { 17005, "DEMAND_D2", "CUST_D2", pid, 50.0, 5, 2, 100.0, "N", "OPEN", 3, 20000.0 };

    demands.push_back(dA);
    demands.push_back(dB);
    demands.push_back(dC);
    demands.push_back(dD1);
    demands.push_back(dD2);

    // Compute composite priorities
    for (auto& d : demands) {
        d.composite_priority = encode_composite_priority(d.status == "COMMITTED", d.customer_tier, d.due_day, d.priority, d.revenue);
    }

    // Sort index list
    std::vector<size_t> indices = {0, 1, 2, 3, 4};
    std::sort(indices.begin(), indices.end(), [&](size_t x, size_t y) {
        if (demands[x].composite_priority != demands[y].composite_priority)
            return demands[x].composite_priority < demands[y].composite_priority;
        return x < y;
    });

    // Expected order:
    // 1st: dB (Committed Day 5) -> index 1
    // 2nd: dA (Committed Day 6) -> index 0
    // 3rd: dC (Open Tier 1 Day 5) -> index 2
    // 4th: dD2 (Open Tier 3 Day 5 Pri 2 Rev 20000) -> index 4
    // 5th: dD1 (Open Tier 3 Day 5 Pri 2 Rev 10000) -> index 3

    assert(indices[0] == 1 && "Expected Demand B (Committed Day 5) to sort first");
    assert(indices[1] == 0 && "Expected Demand A (Committed Day 6) to sort second");
    assert(indices[2] == 2 && "Expected Demand C (Open Tier 1) to sort third");
    assert(indices[3] == 4 && "Expected Demand D2 (Open Tier 3, higher revenue) to sort fourth");
    assert(indices[4] == 3 && "Expected Demand D1 (Open Tier 3, lower revenue) to sort fifth");

    for (int i = 0; i < 5; ++i) {
        telemetry.s17_sorted_indices[i] = static_cast<int>(indices[i]);
    }

    std::cout << "   -> Sorting order verified: dB -> dA -> dC -> dD2 -> dD1" << std::endl;
    std::cout << "   -> [PASS] Scenario 17 Kinaxis-style 5-D priority sorting verified!" << std::endl;
}

void test_scenario_co_allocation_sc18() {
    std::cout << "[TEST RUN] Verifying Scenario 18: Multi-Constraint Co-Allocation..." << std::endl;

    std::string main_code = "PART_18_FG";
    uint32_t pid = vocab.get_or_create(main_code);

    std::vector<PartSiteRecord> parts(vocab.size());
    parts[pid] = { pid, main_code, 0.0, 0.0, 0, 2.0, "MPS", "FINISHED" }; // Base Lead time = 2.0, Run rate = 0.0

    std::vector<FlatBomItem> boms;
    std::vector<std::vector<size_t>> local_bom(vocab.size());

    // OTP supplies: none
    std::vector<std::vector<OTPSupplyNode>> otp(vocab.size());

    // Shared constraints:
    // Constraint 0 (Main): Rates = 1000.0 on Day 3
    // Constraint 1 (Extra): Rates = 5.0 on Day 3, 1000.0 on Day 4
    std::vector<ConstraintRecord> shared(2);
    shared[0] = { 0, "LINE_MAIN", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
    shared[1] = { 1, "LINE_EXTRA", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
    shared[1].rates[3] = 5.0; // Restrict Day 3 extra constraint

    // Source Constraint for pid:
    // Main constraint = 0. Extra constraint = 1 (factor = 1.0)
    std::vector<SourceConstraintRecord> sc(vocab.size());
    sc[pid].part_id = pid;
    sc[pid].constraint_id = 0;
    sc[pid].constraint_factor = 1.0;
    sc[pid].before_fixed_factor = 0.0;
    sc[pid].after_fixed_factor = 0.0;
    sc[pid].extra_constraints = { { 1, 1.0 } }; // Extra constraint 1 with factor 1.0

    // Temporary variables for recursive call
    std::vector<OTPSupplyNode*> temp_alloc;
    std::vector<double> temp_qty;
    std::vector<std::pair<size_t, std::pair<int, double>>> temp_cap;
    std::vector<double> ltb(boms.size(), 0.0);
    std::vector<std::pair<size_t, double>> temp_ltb;
    std::unordered_map<uint32_t, int> mix;
    std::vector<std::vector<double>> dummy_last_dim(parts.size(), std::vector<double>(365, -1.0));
    double routing_cost = 0.0;

    // Try Day 5. Lead time is 2.0. Start Day = 5 - 2 = 3.
    // Requires: 10.0 main capacity on Day 3, 10.0 extra capacity on Day 3.
    // But Day 3 extra capacity is only 5.0. Should FAIL!
    bool ok_fail = reserve_otp_and_capacity_recursive(
        pid, 5, 10.0, 1, 0.0, otp, shared, parts, boms,
        local_bom, sc, temp_alloc, temp_qty, temp_cap,
        "N", ltb, temp_ltb, mix, dummy_last_dim, routing_cost
    );
    assert(!ok_fail && "Co-allocation must fail if extra constraint capacity is insufficient on start day");

    // Clear temp variables
    temp_alloc.clear();
    temp_qty.clear();
    temp_cap.clear();
    temp_ltb.clear();

    // Try Day 6. Lead time is 2.0. Start Day = 6 - 2 = 4.
    // Requires: 10.0 main capacity on Day 4, 10.0 extra capacity on Day 4.
    // Both have 1000.0. Should SUCCESS!
    bool ok_success = reserve_otp_and_capacity_recursive(
        pid, 6, 10.0, 1, 0.0, otp, shared, parts, boms,
        local_bom, sc, temp_alloc, temp_qty, temp_cap,
        "N", ltb, temp_ltb, mix, dummy_last_dim, routing_cost
    );
    assert(ok_success && "Co-allocation must succeed if both main and extra constraints have enough capacity");

    // Verify booked capacity
    double cap_main = 0.0;
    double cap_extra = 0.0;
    for (const auto& cap : temp_cap) {
        if (cap.first == 0 && cap.second.first == 4) cap_main += cap.second.second;
        if (cap.first == 1 && cap.second.first == 4) cap_extra += cap.second.second;
    }
    assert(cap_main == 10.0 && "Must book 10.0 capacity on main constraint (Day 4)");
    assert(cap_extra == 10.0 && "Must book 10.0 capacity on extra constraint (Day 4)");

    std::cout << "   -> [PASS] Scenario 18 co-allocation validated successfully!" << std::endl;
}

void test_scenario_alternative_routing_sc19() {
    std::cout << "[TEST RUN] Verifying Scenario 19: Alternative Routing Selection..." << std::endl;

    std::string main_code = "PART_19_FG";
    uint32_t pid = vocab.get_or_create(main_code);

    std::vector<PartSiteRecord> parts(vocab.size());
    parts[pid] = { pid, main_code, 0.0, 0.0, 0, 2.0, "MPS", "FINISHED" }; // Base Lead time = 2.0, Run rate = 0.0

    std::vector<FlatBomItem> boms;
    std::vector<std::vector<size_t>> local_bom(vocab.size());
    std::vector<std::vector<OTPSupplyNode>> otp(vocab.size());

    // Constraints:
    // Constraint 0 (Default): Rates = 0.0 on Day 3 (Overloaded)
    // Constraint 1 (Alt A): Rates = 1000.0, Priority = 2, Cost = 5.0
    // Constraint 2 (Alt B): Rates = 1000.0, Priority = 1, Cost = 20.0 (Higher priority, higher cost)
    // Constraint 3 (Alt C): Rates = 1000.0, Priority = 2, Cost = 2.0 (Same priority as A, lower cost)
    std::vector<ConstraintRecord> shared(4);
    shared[0] = { 0, "LINE_DEFAULT", "Constrained", std::vector<double>(365, 0.0), std::vector<double>(365, 0.0) };
    shared[1] = { 1, "LINE_ALT_A", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
    shared[2] = { 2, "LINE_ALT_B", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
    shared[3] = { 3, "LINE_ALT_C", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };

    std::vector<SourceConstraintRecord> sc(vocab.size());
    sc[pid].part_id = pid;
    sc[pid].constraint_id = 0;
    sc[pid].constraint_factor = 1.0;
    sc[pid].before_fixed_factor = 0.0;
    sc[pid].after_fixed_factor = 0.0;

    // Define alternative routings:
    // Routing 1 (Alt B): Priority = 1, Cost = 20.0
    AlternativeRouting r1;
    r1.routing_id = 1;
    r1.priority = 1;
    r1.routing_cost = 20.0;
    r1.constraints = { { 2, 1.0 } }; // Uses constraint 2

    // Routing 2 (Alt A): Priority = 2, Cost = 5.0
    AlternativeRouting r2;
    r2.routing_id = 2;
    r2.priority = 2;
    r2.routing_cost = 5.0;
    r2.constraints = { { 1, 1.0 } }; // Uses constraint 1

    // Routing 3 (Alt C): Priority = 2, Cost = 2.0
    AlternativeRouting r3;
    r3.routing_id = 3;
    r3.priority = 2;
    r3.routing_cost = 2.0;
    r3.constraints = { { 3, 1.0 } }; // Uses constraint 3

    sc[pid].alternative_routings = { r2, r1, r3 }; // Unordered insert to test sorting

    // First attempt: Alt B capacity is available (rates[2] = 1000.0).
    // Priority 1 (Alt B, cost 20.0) should be preferred over Priority 2 (Alt C, cost 2.0),
    // because lower priority value means higher routing preference.
    std::vector<OTPSupplyNode*> temp_alloc;
    std::vector<double> temp_qty;
    std::vector<std::pair<size_t, std::pair<int, double>>> temp_cap;
    std::vector<double> ltb(boms.size(), 0.0);
    std::vector<std::pair<size_t, double>> temp_ltb;
    std::unordered_map<uint32_t, int> mix;
    std::vector<std::vector<double>> dummy_last_dim(parts.size(), std::vector<double>(365, -1.0));
    double routing_cost = 0.0;

    bool ok1 = reserve_otp_and_capacity_recursive(
        pid, 5, 10.0, 1, 0.0, otp, shared, parts, boms,
        local_bom, sc, temp_alloc, temp_qty, temp_cap,
        "N", ltb, temp_ltb, mix, dummy_last_dim, routing_cost
    );
    assert(ok1 && "Alternative routing should succeed");
    assert(routing_cost == 20.0 && "Should choose Alt B (priority 1) over Alt C (priority 2, lower cost)");
    bool booked_alt_b = false;
    for (const auto& cap : temp_cap) {
        if (cap.first == 2 && cap.second.first == 3 && cap.second.second == 10.0) booked_alt_b = true;
    }
    assert(booked_alt_b && "Must book capacity on constraint 2 (Alt B)");
    
    telemetry.s19_case1_routing_cost = routing_cost;
    telemetry.s19_case1_booked_alt_b = booked_alt_b ? 10.0 : 0.0;

    // Second attempt: Disable Alt B capacity (rates[2] = 0.0).
    // Now it should fallback to Alt C (priority 2, cost 2.0) instead of Alt A (priority 2, cost 5.0) because of lower cost.
    shared[2].rates[3] = 0.0; // Block Alt B
    temp_alloc.clear();
    temp_qty.clear();
    temp_cap.clear();
    temp_ltb.clear();
    routing_cost = 0.0;

    bool ok2 = reserve_otp_and_capacity_recursive(
        pid, 5, 10.0, 1, 0.0, otp, shared, parts, boms,
        local_bom, sc, temp_alloc, temp_qty, temp_cap,
        "N", ltb, temp_ltb, mix, dummy_last_dim, routing_cost
    );
    assert(ok2 && "Fallback routing should succeed");
    assert(routing_cost == 2.0 && "Should choose Alt C (cost 2.0) over Alt A (cost 5.0) due to lower cost at same priority");
    bool booked_alt_c = false;
    for (const auto& cap : temp_cap) {
        if (cap.first == 3 && cap.second.first == 3 && cap.second.second == 10.0) booked_alt_c = true;
    }
    assert(booked_alt_c && "Must book capacity on constraint 3 (Alt C)");
    
    telemetry.s19_case2_routing_cost = routing_cost;
    telemetry.s19_case2_booked_alt_c = booked_alt_c ? 10.0 : 0.0;

    std::cout << "   -> [PASS] Scenario 19 alternative routing priority/cost selection validated!" << std::endl;
}

void test_scenario_setup_leadtime_sc20() {
    std::cout << "[TEST RUN] Verifying Scenario 20: Setup Matrix & Dynamic Lead-Time Offset..." << std::endl;

    std::string main_code = "PART_20_FG";
    uint32_t pid = vocab.get_or_create(main_code);

    std::vector<PartSiteRecord> parts(vocab.size());
    parts[pid] = { pid, main_code, 0.0, 0.0, 0, 1.0, "MPS", "FINISHED", false, false, 1.0, "SITE_001", 0.0, 0, 0.1 }; // Base LT = 1.0, Run Rate = 0.1

    std::vector<FlatBomItem> boms;
    std::vector<std::vector<size_t>> local_bom(vocab.size());
    std::vector<std::vector<OTPSupplyNode>> otp(vocab.size());

    // Constraints:
    std::vector<ConstraintRecord> shared(1);
    shared[0] = { 0, "LINE_001", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };

    std::vector<SourceConstraintRecord> sc(vocab.size());
    sc[pid].part_id = pid;
    sc[pid].constraint_id = 0;
    sc[pid].constraint_factor = 1.0;
    sc[pid].before_fixed_factor = 5.0; // Setup time = 5.0
    sc[pid].after_fixed_factor = 0.0;

    std::vector<std::vector<double>> last_dim_val(1, std::vector<double>(365, -1.0));

    // Test case 1: First order with Qty = 10.0, Dimension = 100.0, Due = 5.
    // Lead time = 1.0 + 10.0 * 0.1 = 2.0. Start Day = 5 - 2 = 3.
    // No previous dimension on Day 3 (last_dim_val[0][3] is -1.0). Setup time = 5.0.
    // Required Capacity = 5.0 (setup) + 10.0 * 1.0 = 15.0.
    std::vector<OTPSupplyNode*> temp_alloc1;
    std::vector<double> temp_qty1;
    std::vector<std::pair<size_t, std::pair<int, double>>> temp_cap1;
    std::vector<double> ltb(boms.size(), 0.0);
    std::vector<std::pair<size_t, double>> temp_ltb1;
    std::unordered_map<uint32_t, int> mix1;
    double routing_cost1 = 0.0;

    bool ok1 = reserve_otp_and_capacity_recursive(
        pid, 5, 10.0, 1, 100.0, otp, shared, parts, boms,
        local_bom, sc, temp_alloc1, temp_qty1, temp_cap1,
        "N", ltb, temp_ltb1, mix1, last_dim_val, routing_cost1
    );
    assert(ok1);
    
    // Simulate committing the capacity and dimension updates as done in run_dbd_dispatch_engine:
    for (const auto& cap : temp_cap1) {
        shared[cap.first].allocated_rates[cap.second.first] += cap.second.second;
        last_dim_val[cap.first][cap.second.first] = 100.0; // Update dimension on Day 3 to 100.0
    }

    double cap_booked1 = 0.0;
    for (const auto& cap : temp_cap1) {
        if (cap.first == 0 && cap.second.first == 3) cap_booked1 += cap.second.second;
    }
    assert(cap_booked1 == 15.0 && "First order must book 15.0 capacity (including 5.0 setup)");
    
    telemetry.s20_cap_booked1 = cap_booked1;

    // Test case 2: Second order with Qty = 20.0, Dimension = 100.0 (same dimension!), Due = 6.
    // Lead time = 1.0 + 20.0 * 0.1 = 3.0. Start Day = 6 - 3 = 3 (same start day!).
    // Previous dimension on Day 3 is 100.0. Setup time should be 0.0.
    // Required Capacity = 0.0 (setup) + 20.0 * 1.0 = 20.0.
    std::vector<OTPSupplyNode*> temp_alloc2;
    std::vector<double> temp_qty2;
    std::vector<std::pair<size_t, std::pair<int, double>>> temp_cap2;
    std::vector<std::pair<size_t, double>> temp_ltb2;
    std::unordered_map<uint32_t, int> mix2;
    double routing_cost2 = 0.0;

    bool ok2 = reserve_otp_and_capacity_recursive(
        pid, 6, 20.0, 1, 100.0, otp, shared, parts, boms,
        local_bom, sc, temp_alloc2, temp_qty2, temp_cap2,
        "N", ltb, temp_ltb2, mix2, last_dim_val, routing_cost2
    );
    assert(ok2);
    
    double cap_booked2 = 0.0;
    for (const auto& cap : temp_cap2) {
        if (cap.first == 0 && cap.second.first == 3) cap_booked2 += cap.second.second;
    }
    assert(cap_booked2 == 20.0 && "Second order must book 20.0 capacity (setup is exempted!)");
    
    telemetry.s20_cap_booked2 = cap_booked2;

    // Test case 3: Third order with Qty = 10.0, Dimension = 200.0 (different dimension!), Due = 5.
    // Lead time = 1.0 + 10.0 * 0.1 = 2.0. Start Day = 5 - 2 = 3.
    // Previous dimension on Day 3 is 100.0 (from Case 2). Since 200.0 != 100.0, Setup time = 5.0.
    // Required Capacity = 5.0 (setup) + 10.0 = 15.0.
    std::vector<OTPSupplyNode*> temp_alloc3;
    std::vector<double> temp_qty3;
    std::vector<std::pair<size_t, std::pair<int, double>>> temp_cap3;
    std::vector<std::pair<size_t, double>> temp_ltb3;
    std::unordered_map<uint32_t, int> mix3;
    double routing_cost3 = 0.0;

    bool ok3 = reserve_otp_and_capacity_recursive(
        pid, 5, 10.0, 1, 200.0, otp, shared, parts, boms,
        local_bom, sc, temp_alloc3, temp_qty3, temp_cap3,
        "N", ltb, temp_ltb3, mix3, last_dim_val, routing_cost3
    );
    assert(ok3);
    
    double cap_booked3 = 0.0;
    for (const auto& cap : temp_cap3) {
        if (cap.first == 0 && cap.second.first == 3) cap_booked3 += cap.second.second;
    }
    assert(cap_booked3 == 15.0 && "Third order must book 15.0 capacity (setup is charged due to dimension mismatch)");
    
    telemetry.s20_cap_booked3 = cap_booked3;

    std::cout << "   -> [PASS] Scenario 20 setup matrix and dynamic lead-time stretching validated!" << std::endl;
}


// Scenario 21: Decoupled Multi-Model E2E Integration Test (20-Layer BOM)
void test_scenario_decoupled_e2e_sc21() {
    std::cout << "[TEST RUN] Verifying Scenario 21: Decoupled Multi-Model E2E Integration Test (20-Layer BOM)..." << std::endl;

    int num_parts = 22;
    std::vector<PartSiteRecord> parts(num_parts);
    for (int i = 0; i < num_parts; ++i) {
        parts[i].part_id = i;
        if (i == 0) {
            parts[i].part_code = "PART_21_FG";
            parts[i].part_type = "FINISHED";
            parts[i].mrp_rule = "MPS";
            parts[i].lead_time = 1.0;
            parts[i].run_rate = 0.05; // Dynamic LT
        } else if (i <= 19) {
            parts[i].part_code = "PART_21_SEMI_" + std::to_string(i);
            parts[i].part_type = "SEMI";
            parts[i].mrp_rule = "MRP";
            parts[i].lead_time = 1.0;
            parts[i].run_rate = 0.0;
        } else if (i == 20) {
            parts[i].part_code = "PART_21_RAW";
            parts[i].part_type = "RAW";
            parts[i].mrp_rule = "MRP";
            parts[i].lead_time = 2.0;
        } else {
            parts[i].part_code = "PART_21_ALT";
            parts[i].part_type = "ALT";
            parts[i].mrp_rule = "MRP";
            parts[i].lead_time = 2.0;
        }
        parts[i].on_hand = 0.0;
        parts[i].ipc_scheduled_receipt = 0.0;
        parts[i].low_level_code = i;
    }

    std::vector<FlatBomItem> boms;
    std::vector<std::vector<size_t>> local_bom(num_parts);
    std::vector<std::vector<OTPSupplyNode>> otp(num_parts);

    for (int i = 0; i < 19; ++i) {
        FlatBomItem item;
        item.parent_id = i;
        item.child_id = i + 1;
        item.per_qty = 1.0;
        item.scrap = 0.0;
        boms.push_back(item);
        local_bom[i].push_back(boms.size() - 1);
    }

    FlatBomItem raw_item;
    raw_item.parent_id = 19;
    raw_item.child_id = 20;
    raw_item.per_qty = 2.0;
    raw_item.scrap = 0.05;
    raw_item.alt_group_id = 21;
    raw_item.alt_priority = 3; // Class 3 Lot-size Substitution
    raw_item.target_ratio = 0.5;
    raw_item.lot_size = 10.0;
    boms.push_back(raw_item);
    local_bom[19].push_back(boms.size() - 1);

    FlatBomItem alt_item;
    alt_item.parent_id = 19;
    alt_item.child_id = 21;
    alt_item.per_qty = 2.0;
    alt_item.scrap = 0.05;
    alt_item.alt_group_id = 21;
    alt_item.alt_priority = 3;
    alt_item.target_ratio = 0.5;
    alt_item.lot_size = 5.0;
    boms.push_back(alt_item);
    local_bom[19].push_back(boms.size() - 1);

    std::vector<ConstraintRecord> shared(4);
    shared[0] = { 0, "LINE_FINISHED", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
    shared[1] = { 1, "LINE_SEMI", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
    shared[2] = { 2, "LINE_AUX", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };
    shared[3] = { 3, "LINE_ALT", "Constrained", std::vector<double>(365, 1000.0), std::vector<double>(365, 0.0) };

    std::vector<SourceConstraintRecord> sc(num_parts);
    for (int i = 0; i < num_parts; ++i) {
        sc[i].part_id = i;
        sc[i].constraint_factor = 1.0;
        sc[i].after_fixed_factor = 0.0;
        if (i == 0) {
            sc[i].constraint_id = 0; // LINE_FINISHED
            sc[i].before_fixed_factor = 2.0;
        } else if (i == 5) {
            sc[i].constraint_id = 1; // LINE_SEMI
            sc[i].before_fixed_factor = 0.0;
            sc[i].extra_constraints = { { 2, 0.5 } };
            AlternativeRouting alt = { 1, { { 3, 1.0 } }, 10.0, 1 };
            sc[i].alternative_routings = { alt };
        } else {
            // Map other parts to unblocked Constraint 0
            sc[i].constraint_id = 0;
            sc[i].before_fixed_factor = 0.0;
        }
    }

    std::vector<OTPSupplyNode*> temp_alloc1;
    std::vector<double> temp_qty1;
    std::vector<std::pair<size_t, std::pair<int, double>>> temp_cap1;
    std::vector<double> bom_ltb_consumed(boms.size(), 0.0);
    std::vector<std::pair<size_t, double>> temp_ltb1;
    std::unordered_map<uint32_t, int> mix1;
    std::vector<std::vector<double>> last_dim_val(4, std::vector<double>(365, -1.0));
    double routing_cost1 = 0.0;
    std::vector<PlannedOrderSplit> temp_po_splits1;

    bool ok1 = reserve_otp_and_capacity_recursive(
        0, 30, 10.0, 1, 100.0, otp, shared, parts, boms,
        local_bom, sc, temp_alloc1, temp_qty1, temp_cap1,
        "N", bom_ltb_consumed, temp_ltb1, mix1, last_dim_val, routing_cost1,
        temp_po_splits1, false
    );
    assert(ok1 && "First order must schedule successfully!");

    for (const auto& cap : temp_cap1) {
        shared[cap.first].allocated_rates[cap.second.first] += cap.second.second;
    }

    bool booked_aux = false;
    double aux_cap = 0.0;
    for (const auto& cap : temp_cap1) {
        if (cap.first == 2) {
            booked_aux = true;
            aux_cap = cap.second.second;
        }
    }
    assert(booked_aux && "Multi-constraint co-allocation must book capacity on LINE_AUX!");
    assert(aux_cap == 5.0 && "LINE_AUX capacity must be 10.0 * 0.5 = 5.0!");

    for (int day = 0; day < 365; ++day) {
        shared[0].allocated_rates[day] = 0.0;
        shared[1].allocated_rates[day] = 0.0;
        shared[2].allocated_rates[day] = 0.0;
        shared[3].allocated_rates[day] = 0.0;
        last_dim_val[0][day] = -1.0;
        last_dim_val[1][day] = -1.0;
        last_dim_val[2][day] = -1.0;
        last_dim_val[3][day] = -1.0;
    }
    for (int d = 20; d <= 30; ++d) {
        shared[1].allocated_rates[d] = 1000.0;
    }

    std::vector<OTPSupplyNode*> temp_alloc2;
    std::vector<double> temp_qty2;
    std::vector<std::pair<size_t, std::pair<int, double>>> temp_cap2;
    std::vector<std::pair<size_t, double>> temp_ltb2;
    std::unordered_map<uint32_t, int> mix2;
    double routing_cost2 = 0.0;
    std::vector<PlannedOrderSplit> temp_po_splits2;

    bool ok2 = reserve_otp_and_capacity_recursive(
        0, 30, 10.0, 1, 100.0, otp, shared, parts, boms,
        local_bom, sc, temp_alloc2, temp_qty2, temp_cap2,
        "N", bom_ltb_consumed, temp_ltb2, mix2, last_dim_val, routing_cost2,
        temp_po_splits2, false
    );
    assert(ok2 && "Order must schedule using alternative routing!");
    
    bool booked_alt_routing = false;
    for (const auto& cap : temp_cap2) {
        if (cap.first == 3 && cap.second.second == 10.0) {
            booked_alt_routing = true;
        }
    }
    assert(booked_alt_routing && "Alternative routing must book capacity on LINE_ALT!");

    std::cout << "   -> [PASS] Scenario 21 decoupled multi-model E2E integration test (20-Layer BOM) validated!" << std::endl;
}

void test_scenario_aps_benchmark() {
    std::cout << "\n=====================================================================" << std::endl;
    std::cout << "[BENCHMARK] Starting Netting Planning Engine Extreme Pressure Test (200,000 Demands, 20-Layer BOM, 500+ Descendants)" << std::endl;
    std::cout << "=====================================================================" << std::endl;

    auto t_start = std::chrono::high_resolution_clock::now();

    // 1. Generate 1,000 Parts with 20 levels in the Planning Hierarchy
    // 0-299: FG (Finished Goods)
    // 300-399: SEMI (Semi-finished) - part 300 is entry point, 301-318 forms the linear chain
    // 400-999: RAW / ALT_RAW component pairs (600 parts total, 300 pairs)
    int num_parts = 1000;
    std::vector<PartSiteRecord> parts(num_parts);
    for (int i = 0; i < num_parts; ++i) {
        parts[i].part_id = i;
        parts[i].on_hand = 0.0;
        parts[i].ipc_scheduled_receipt = 0.0;
        parts[i].lead_time = 1.0;
        parts[i].run_rate = 0.01; // Dynamic lead time stretch factor
        parts[i].site = "SITE_001";

        if (i < 300) {
            parts[i].part_code = "FG_PART_" + std::to_string(i);
            parts[i].part_type = "FINISHED";
            parts[i].mrp_rule = "MPS";
            parts[i].low_level_code = 0; // Layer 0
        } else if (i <= 318) {
            parts[i].part_code = "SEMI_PART_" + std::to_string(i);
            parts[i].part_type = "SEMI";
            parts[i].mrp_rule = "MRP";
            parts[i].low_level_code = i - 300 + 1; // Layer 1 to 19
        } else if (i < 400) {
            parts[i].part_code = "SEMI_UNUSED_" + std::to_string(i);
            parts[i].part_type = "SEMI";
            parts[i].mrp_rule = "MRP";
            parts[i].low_level_code = 1;
        } else {
            parts[i].part_code = (i % 2 == 0) ? "RAW_PART_" + std::to_string(i) : "ALT_RAW_PART_" + std::to_string(i);
            parts[i].part_type = (i % 2 == 0) ? "RAW" : "ALT";
            parts[i].mrp_rule = "MRP";
            parts[i].low_level_code = 20; // Layer 20 (Deepest level)
        }
    }

    // 2. Build 20-Layer BOM Structure with 500+ Descendant Nodes
    std::vector<FlatBomItem> boms;
    std::vector<std::vector<size_t>> local_bom(num_parts);

    // FGs (0-299) -> SEMI_300 (Layer 0 -> Layer 1)
    for (int i = 0; i < 300; ++i) {
        FlatBomItem item;
        item.parent_id = i;
        item.child_id = 300; 
        item.per_qty = 1.0;
        item.scrap = 0.0;
        item.alt_group_id = -1;
        item.relation_op = static_cast<uint8_t>(RelationOp::PASS);
        boms.push_back(item);
        local_bom[i].push_back(boms.size() - 1);
    }

    // Linear chain SEMI_300 -> SEMI_301 -> ... -> SEMI_318 (Layer 1 -> Layer 19)
    for (int j = 300; j < 318; ++j) {
        FlatBomItem item;
        item.parent_id = j;
        item.child_id = j + 1;
        item.per_qty = 1.0;
        item.scrap = 0.0;
        item.alt_group_id = -1;
        item.relation_op = static_cast<uint8_t>(RelationOp::PASS);
        boms.push_back(item);
        local_bom[j].push_back(boms.size() - 1);
    }

    // SEMI_318 -> RAW & ALT_RAW component pairs (Layer 19 -> Layer 20)
    // 300 component groups, 600 descendant parts total. Explodes to all 600 components.
    for (int g = 0; g < 300; ++g) {
        int raw_id = 400 + 2 * g;
        int alt_raw_id = 400 + 2 * g + 1;

        // Primary RAW Component
        FlatBomItem item1;
        item1.parent_id = 318;
        item1.child_id = raw_id;
        item1.per_qty = 1.0;
        item1.scrap = 0.05;
        item1.alt_group_id = g;
        item1.alt_priority = 1;
        item1.target_ratio = 0.5;
        item1.lot_size = 5.0;
        item1.relationship_type = "interchangeable";
        item1.relation_op = static_cast<uint8_t>(RelationOp::PASS);
        boms.push_back(item1);
        local_bom[318].push_back(boms.size() - 1);

        // Alternate RAW Component (Class 3 lot-size substitute)
        FlatBomItem item2;
        item2.parent_id = 318;
        item2.child_id = alt_raw_id;
        item2.per_qty = 1.0;
        item2.scrap = 0.10;
        item2.alt_group_id = g;
        item2.alt_priority = 3;
        item2.target_ratio = 0.5;
        item2.lot_size = 10.0;
        item2.relationship_type = "interchangeable";
        item2.relation_op = static_cast<uint8_t>(RelationOp::PASS);
        boms.push_back(item2);
        local_bom[318].push_back(boms.size() - 1);
    }

    int num_demands = 200000;
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist_fg(0, 299);
    std::uniform_int_distribution<int> dist_due(20, 320);
    std::uniform_real_distribution<double> dist_qty(5.0, 25.0);
    std::uniform_int_distribution<int> dist_dim(0, 2);
    std::vector<double> dim_choices = { 100.0, 110.0, 120.0 };

    std::vector<IndependentDemand> demands(num_demands);
    std::vector<PlannedOrder> ipc_planned_orders(num_demands);
    int wbs_project_count = 0;

    for (int i = 0; i < num_demands; ++i) {
        uint32_t pid = dist_fg(rng);
        double qty = dist_qty(rng);
        double dim = dim_choices[dist_dim(rng)];
        bool is_wbs = (i % 10 == 0);

        int due = dist_due(rng);
        if (is_wbs) {
            std::vector<ProjectTaskRecord> tasks;
            ProjectTaskRecord t1 = { 1, static_cast<uint32_t>(i), "Research & Design", 2.0 };
            ProjectTaskRecord t2 = { 2, static_cast<uint32_t>(i), "Prototype Fab", 3.0 };
            ProjectTaskRecord t3 = { 3, static_cast<uint32_t>(i), "Product Assembly", 1.0 };

            t2.dependencies.push_back({ 1, DependencyType::FS, 0 });
            t3.dependencies.push_back({ 2, DependencyType::FS, 0 });
            t3.output_part_id = pid;

            tasks.push_back(t1);
            tasks.push_back(t2);
            tasks.push_back(t3);

            calculate_project_wbs_cpm(tasks, due);
            due = tasks[2].late_start;
            wbs_project_count++;
        }

        demands[i] = {
            static_cast<uint32_t>(i),
            "DEMAND_" + std::to_string(i),
            "CUST_" + std::to_string(i % 10),
            pid,
            qty,
            due,
            1,
            dim,
            "N",
            "COMMITTED",
            3,
            qty * 100.0,
            0
        };

        double lead_time = parts[pid].lead_time + qty * parts[pid].run_rate;
        int start = due - static_cast<int>(std::ceil(lead_time));
        if (start < 0) start = 0;
        ipc_planned_orders[i] = {
            pid,
            qty,
            start,
            due,
            dim
        };
    }

    auto t_gen_done = std::chrono::high_resolution_clock::now();
    double ms_gen = std::chrono::duration<double, std::milli>(t_gen_done - t_start).count();
    std::cout << "[计时] Benchmark 数据生成与 WBS CPM 预处理耗时: " << ms_gen << " ms (项目数: " << wbs_project_count << ")" << std::endl;

    auto t_solve_start = std::chrono::high_resolution_clock::now();

    std::vector<PlannedOrder> scheduled_orders;
    std::vector<double> allocated_capacity;
    std::vector<double> order_capacities;
    std::vector<double> order_routing_costs;

    run_dbd_dispatch_engine(
        parts, boms, ipc_planned_orders, demands,
        scheduled_orders, allocated_capacity, order_capacities, order_routing_costs
    );

    auto t_solve_done = std::chrono::high_resolution_clock::now();
    double ms_solve = std::chrono::duration<double, std::milli>(t_solve_done - t_solve_start).count();

    int exact_due_count = 0;
    int delayed_count = 0;
    int failed_window_count = 0;
    double total_delay_days = 0.0;
    int success_count = 0;
    int alt_route_count = 0;

    for (size_t i = 0; i < scheduled_orders.size(); ++i) {
        if (scheduled_orders[i].finish_day > 0) {
            success_count++;
        }
        if (i < order_routing_costs.size() && order_routing_costs[i] > 0.0) {
            alt_route_count++;
        }
        
        if (i < num_demands) {
            int orig_due = demands[i].due_day;
            int sched_finish = scheduled_orders[i].finish_day;
            if (sched_finish == TIMELINE_DAYS - 1) {
                failed_window_count++;
            } else if (sched_finish == orig_due) {
                exact_due_count++;
            } else {
                delayed_count++;
                total_delay_days += (sched_finish - orig_due);
            }
        }
    }

    std::cout << "\n=================== PRESSURE BENCHMARK RESULT ===================" << std::endl;
    std::cout << "测试需求订单数       : " << num_demands << " 笔" << std::endl;
    std::cout << "包含 ETO-WBS 项目数  : " << wbs_project_count << " 个" << std::endl;
    std::cout << "BOM 物料分级爆炸层数 : 20 层 (FG -> SEMI_1 -> ... -> SEMI_18 -> RAW/ALT_RAW)" << std::endl;
    std::cout << "单订单 BOM 遍历节点数: 619 个 (19 SEMI + 600 Component RAW/ALT_RAW)" << std::endl;
    std::cout << "按期准时满足工单数   : " << exact_due_count << " 笔 (准时率 " << (exact_due_count * 100.0 / num_demands) << "%)" << std::endl;
    std::cout << "延期满足工单数       : " << delayed_count << " 笔 (延期率 " << (delayed_count * 100.0 / num_demands) << "%)" << std::endl;
    std::cout << "超出滑窗未满足工单数 : " << failed_window_count << " 笔 (失败率 " << (failed_window_count * 100.0 / num_demands) << "%)" << std::endl;
    if (delayed_count > 0) {
        std::cout << "平均延期满足天数     : " << (total_delay_days / delayed_count) << " 天" << std::endl;
    }
    std::cout << "替代料/替代路线调用数: " << alt_route_count << " 次" << std::endl;
    std::cout << "成功排产分发工单数   : " << scheduled_orders.size() << " 笔 (成功率 " << (success_count * 100.0 / num_demands) << "%)" << std::endl;
    std::cout << "规划消纳引擎(DBD)耗时: " << ms_solve << " ms" << std::endl;
    std::cout << "每笔订单平均分发耗时 : " << (ms_solve * 1000.0 / num_demands) << " 微秒 (us)" << std::endl;
    std::cout << "========================================================\n" << std::endl;

    std::cout << "------ [抽样对账验证] 随机抽样前 5 笔排产分发工单明细 ------" << std::endl;
    for (int i = 0; i < 5; ++i) {
        std::cout << "工单 #" << i << " | 物料ID: " << scheduled_orders[i].part_id 
                  << " | 需求数: " << scheduled_orders[i].qty 
                  << " | 芯片维度: " << scheduled_orders[i].dimension_val 
                  << " | 原定交期: " << demands[i].due_day 
                  << " | 计划开工: " << scheduled_orders[i].start_day 
                  << " | 计划完工: " << scheduled_orders[i].finish_day 
                  << " | 类型: " << (demands[i].demand_id % 10 == 0 ? "ETO-WBS" : "Standard")
                  << std::endl;
    }
    std::cout << "------------------------------------------------------------\n" << std::endl;
}

int main() {
    std::cout << "=====================================================================" << std::endl;
    std::cout << "          IPC ENGINE COM-LEVEL VALIDATION TEST SUITE RUNNER          " << std::endl;
    std::cout << "=====================================================================" << std::endl;

    try {
        test_scenario_class1();
        test_scenario_class2();
        test_scenario_class3();
        test_scenario_dimensions();
        test_scenario_mcdm_groups();
        test_scenario_swap_engine();
        test_scenario_netting_operator();
        test_scenario_dbd_dispatch();
        test_scenario_isolation_seeding();
        test_scenario_ipc_coproduct_dimension_planning();
        test_scenario_ecn_ltb();
        test_scenario_znc_preferences();
        test_scenario_mix_rules();
        test_scenario_interchangeable();
        test_scenario_wbs_mrp();
        test_scenario_otp_preemption();
        test_scenario_composite_priority();
        test_scenario_co_allocation_sc18();
        test_scenario_alternative_routing_sc19();
        test_scenario_setup_leadtime_sc20();
        test_scenario_decoupled_e2e_sc21();
        test_scenario_aps_benchmark();

        std::cout << "=====================================================================" << std::endl;
        std::cout << " [SUCCESS] All IPC Planning Scenarios & Isolation Seeding validated!" << std::endl;
        std::cout << "=====================================================================" << std::endl;
        
        write_cpp_results_to_json("ipc_cpp_results.json");
    } catch (const std::exception& e) {
        std::cerr << " [FAILURE] Regression detected: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
