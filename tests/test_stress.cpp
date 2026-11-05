#include "ipc_types.h"
#include "csv_export.h"
#include "ipc/vocab.h"
#include "ipc/mrp_engine.h"
#include "ipc/dbd_engine.h"
#include <chrono>
#include <iostream>
#include <random>
#include <algorithm>
#include <unordered_map>

#define DEBUG_CSV_EXPORT false

namespace ipc {

static std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash> test_dummy_allotment_constraints;


// High-fidelity stress data generator supporting 20-layer deep value chains
// where each finished product has exactly 542 descendants.
void generate_stress_mock_data(
    std::vector<PartSiteRecord>& parts,
    std::vector<FlatBomItem>& boms,
    std::vector<IndependentDemand>& demands,
    int num_parts_target,
    int num_demands_target
) {
    std::cout << "[压力测试] 正在构建 20层 BOM 深度价值链模型 (每个 FG 1000+ 子孙)..." << std::endl;
    std::mt19937 rng(42);

    parts.clear();
    boms.clear();
    demands.clear();

    // 1. Define parts
    // Parts 0..999: Finished Goods (1,000 parts)
    // Parts 1000..4999: Semis L1..L4 (4,000 parts)
    // Parts 5000..36999: Semis L5..L20 (32,000 parts)
    // Parts 37000..38999: Raws L21 (2,000 parts)
    // Parts 39000..40999: Alts L22 (2,000 parts)
    // Parts 41000..num_parts_target-1: Unused Semis
    for (int i = 0; i < num_parts_target; ++i) {
        std::string code = "PART_" + std::to_string(i);
        uint32_t pid = vocab.get_or_create(code);

        PartSiteRecord rec;
        rec.part_id = pid;
        rec.part_code = code;
        rec.low_level_code = 0;
        rec.on_hand = 0.0;
        rec.ipc_scheduled_receipt = 0.0;
        rec.round_to_integer = true;

        if (i < 1000) {
            rec.part_type = "FINISHED";
            rec.mrp_rule = "MPS";
            rec.lead_time = 2.0;
            rec.is_phantom = false;
            rec.site = "SITE_001";
        } else if (i < 37000) {
            rec.part_type = "SEMI";
            rec.mrp_rule = "MRP";
            rec.lead_time = 3.0;
            // 30% of Semis are marked as phantom to test phantom assembly logic
            rec.is_phantom = (i % 10 == 0 || i % 10 == 3 || i % 10 == 7);
            rec.site = "SITE_001";
        } else if (i < 39000) {
            rec.part_type = "RAW";
            rec.mrp_rule = "MRP";
            rec.lead_time = 10.0;
            rec.is_phantom = false;
            // Some Raws are sourced from SUPP_A or SUPP_B to test supplier timelines & capacity limits
            rec.site = (i % 2 == 0) ? "SUPP_A" : "SUPP_B";
        } else if (i < 41000) {
            rec.part_type = "ALT";
            rec.mrp_rule = "MRP";
            rec.lead_time = 8.0;
            rec.is_phantom = false;
            rec.site = "SUPP_B";
        } else {
            // Unused reminder parts up to num_parts_target
            rec.part_type = "SEMI";
            rec.mrp_rule = "MRP";
            rec.lead_time = 3.0;
            rec.is_phantom = false;
            rec.site = "SITE_001";
        }
        parts.push_back(rec);
    }

    // 2. Build BOM relations
    // - Finished Goods i (0..999) connects to L1 Semis:
    for (int i = 0; i < 1000; ++i) {
        FlatBomItem item;
        item.parent_id = static_cast<uint32_t>(i);
        item.child_id = static_cast<uint32_t>(1000 + i);
        item.per_qty = 1.0;
        item.scrap = 0.0;
        item.relation_op = static_cast<uint8_t>(RelationOp::PASS);
        boms.push_back(item);
    }

    // - Level 1 (1000..1999) to Level 2 (2000..2999):
    for (int i = 0; i < 1000; ++i) {
        FlatBomItem item;
        item.parent_id = static_cast<uint32_t>(1000 + i);
        item.child_id = static_cast<uint32_t>(2000 + i);
        item.per_qty = 1.0;
        item.scrap = 0.0;
        item.relation_op = static_cast<uint8_t>(RelationOp::PASS);
        boms.push_back(item);
    }

    // - Level 2 (2000..2999) to Level 3 (3000..3999):
    for (int i = 0; i < 1000; ++i) {
        FlatBomItem item;
        item.parent_id = static_cast<uint32_t>(2000 + i);
        item.child_id = static_cast<uint32_t>(3000 + i);
        item.per_qty = 1.0;
        item.scrap = 0.01; // 1% scrap
        item.relation_op = static_cast<uint8_t>(RelationOp::PASS);
        boms.push_back(item);
    }

    // - Level 3 (3000..3999) to Level 4 (4000..4999):
    for (int i = 0; i < 1000; ++i) {
        FlatBomItem item;
        item.parent_id = static_cast<uint32_t>(3000 + i);
        item.child_id = static_cast<uint32_t>(4000 + i);
        item.per_qty = 1.0;
        item.scrap = 0.0;
        item.relation_op = static_cast<uint8_t>(RelationOp::PASS);
        boms.push_back(item);
    }

    // - Level 4 (4000..4999) to Level 5 (5000..6999):
    //   Each L4 part branches to 64 children at Level 5.
    for (int i = 0; i < 1000; ++i) {
        for (int k = 0; k < 64; ++k) {
            FlatBomItem item;
            item.parent_id = static_cast<uint32_t>(4000 + i);
            item.child_id = static_cast<uint32_t>(5000 + (i * 2 + k) % 2000);
            item.per_qty = 1.0;
            item.scrap = 0.0;
            item.relation_op = static_cast<uint8_t>(RelationOp::PASS);
            boms.push_back(item);
        }
    }

    // - Level 5 to 20 (Deep Semi chains, 5000..36999):
    //   This creates 2,000 parallel linear chains of length 15 levels
    for (int lvl = 5; lvl < 20; ++lvl) {
        int p_start = 5000 + (lvl - 5) * 2000;
        int c_start = p_start + 2000;
        for (int offset = 0; offset < 2000; ++offset) {
            FlatBomItem item;
            item.parent_id = static_cast<uint32_t>(p_start + offset);
            item.child_id = static_cast<uint32_t>(c_start + offset);
            item.per_qty = 1.0;
            item.scrap = 0.0;
            if (offset % 100 == 0) {
                item.eff_start_day = 0;
                item.eff_end_day = 180;
            }
            item.relation_op = static_cast<uint8_t>(RelationOp::PASS);
            boms.push_back(item);
        }
    }

    // - Level 20 (35000..36999) to Level 21 Raw Materials (37000..38999):
    for (int offset = 0; offset < 2000; ++offset) {
        FlatBomItem item;
        item.parent_id = static_cast<uint32_t>(35000 + offset);
        item.child_id = static_cast<uint32_t>(37000 + offset);
        item.per_qty = 2.0;
        item.scrap = 0.02;
        item.relation_op = static_cast<uint8_t>(RelationOp::PASS);
        boms.push_back(item);
    }

    // - Substitution relations (Raw Materials 37000..38999 to Alt Parts 39000..40999):
    //   This implements Class 3 substitution with lot sizing
    for (int r = 37000; r < 39000; ++r) {
        int group_id = r - 37000;
        
        FlatBomItem item_prim;
        item_prim.parent_id = static_cast<uint32_t>(r);
        item_prim.child_id = static_cast<uint32_t>(r);
        item_prim.alt_group_id = group_id;
        item_prim.alt_priority = 1;
        item_prim.target_ratio = 0.5;
        item_prim.per_qty = 1.0;
        item_prim.scrap = 0.0;
        item_prim.lot_size = 5.0;
        item_prim.relationship_type = "alt";
        boms.push_back(item_prim);

        FlatBomItem item_sub;
        item_sub.parent_id = static_cast<uint32_t>(r);
        item_sub.child_id = static_cast<uint32_t>(39000 + group_id);
        item_sub.alt_group_id = group_id;
        item_sub.alt_priority = 2;
        item_sub.target_ratio = 0.5;
        item_sub.per_qty = 1.0;
        item_sub.scrap = 0.0;
        item_sub.lot_size = 10.0;
        item_sub.relationship_type = "alt";
        boms.push_back(item_sub);
    }

    // 3. Generate Independent Demands (500,000 orders)
    for (int i = 0; i < num_demands_target; ++i) {
        IndependentDemand d;
        d.demand_id = static_cast<uint32_t>(i);
        d.customer = "CUST_" + std::to_string(i % 100);
        d.part_id = static_cast<uint32_t>(std::uniform_int_distribution<int>(0, 999)(rng));
        d.qty = std::uniform_real_distribution<double>(5.0, 50.0)(rng);
        d.due_day = std::uniform_int_distribution<int>(10, 85)(rng);
        d.priority = std::uniform_int_distribution<int>(1, 5)(rng);
        
        double r_dim = std::uniform_real_distribution<double>(0.0, 1.0)(rng);
        if (r_dim > 0.8) {
            d.dimension_val = 102.0;
        } else if (r_dim > 0.5) {
            d.dimension_val = 101.0;
        } else {
            d.dimension_val = 100.0;
        }
        d.preference_mode = (i % 3 == 0) ? "Z" : ((i % 3 == 1) ? "C" : "N");
        d.status = (i % 10 == 0) ? "COMMITTED" : "OPEN";
        d.customer_tier = (i % 5 == 0) ? 1 : ((i % 5 == 1) ? 2 : 3);
        d.revenue = d.qty * 15.0;
        d.composite_priority = encode_composite_priority(d.status == "COMMITTED", d.customer_tier, d.due_day, d.priority, d.revenue);

        demands.push_back(d);
    }

    std::cout << "[数据] 数据生成完成！" << std::endl;
    std::cout << "   -> 物料总数: " << parts.size() << " 种" << std::endl;
    std::cout << "   -> 关系有向图 BOM 边数: " << boms.size() << " 条" << std::endl;
    std::cout << "   -> 独立需求订单数: " << demands.size() << " 张" << std::endl;
}

void run_stress_test() {
    const int NUM_PARTS = 50000;
    const int NUM_DEMANDS = 500000;

    std::vector<PartSiteRecord> parts;
    std::vector<FlatBomItem> boms;
    std::vector<IndependentDemand> demands;
    std::vector<PlannedOrder> ipc_planned_orders;
    std::vector<AlternateAllocationRecord> alt_records;
    std::vector<SwapRecord> swap_records;

    auto t_gen_start = std::chrono::high_resolution_clock::now();
    generate_stress_mock_data(parts, boms, demands, NUM_PARTS, NUM_DEMANDS);
    auto t_gen_end = std::chrono::high_resolution_clock::now();
    double ms_gen = std::chrono::duration<double, std::milli>(t_gen_end - t_gen_start).count();
    std::cout << "[计时] 大规模数据生成耗时: " << ms_gen << " ms (" << ms_gen / 1000.0 << " s)" << std::endl;

    // 1. Run LBL MRP engine
    std::cout << "\n=====================================================================" << std::endl;
    std::cout << "[点火] 开始执行 20层 价值链级联 MRP 净需求消纳与物料分解运算..." << std::endl;
    std::cout << "=====================================================================" << std::endl;
    auto t_mrp_start = std::chrono::high_resolution_clock::now();
    int max_level = 25; // accommodates 21 levels
    run_lbl_mrp_engine(parts, boms, demands, ipc_planned_orders, alt_records, swap_records, max_level);
    auto t_mrp_end = std::chrono::high_resolution_clock::now();
    double ms_mrp = std::chrono::duration<double, std::milli>(t_mrp_end - t_mrp_start).count();
    std::cout << "[计时] MRP 级联消纳计算耗时: " << ms_mrp << " ms (" << ms_mrp / 1000.0 << " s)" << std::endl;
    std::cout << "   -> 产生底层 MRP 计划订单笔数: " << ipc_planned_orders.size() << std::endl;
    
    // 统计各 LLC 层级的订单分布
    std::vector<size_t> orders_by_level(max_level + 1, 0);
    for (const auto& po : ipc_planned_orders) {
        int llc = parts[po.part_id].low_level_code;
        if (llc >= 0 && llc <= max_level) {
            orders_by_level[llc]++;
        }
    }
    std::cout << "   -> MRP 各层级 (Low Level Code) 计划订单分布:" << std::endl;
    for (int lvl = 0; lvl <= max_level; ++lvl) {
        if (orders_by_level[lvl] > 0) {
            std::cout << "      Level " << lvl << ": " << orders_by_level[lvl] << " 笔" << std::endl;
        }
    }
    
    std::cout << "   -> 替代料分配计算次数: " << alt_records.size() << std::endl;

    // 2. Run DBD Parallel dispatch engine
    std::vector<PlannedOrder> scheduled_orders;
    std::vector<double> allocated_capacity;
    std::vector<double> order_capacities;
    std::vector<double> order_routing_costs;

    std::cout << "\n=====================================================================" << std::endl;
    std::cout << "[点火] 开始执行 MCDS 有限产能微观时空派程调度 (IOP)..." << std::endl;
    std::cout << "=====================================================================" << std::endl;
    auto t_dbd_start = std::chrono::high_resolution_clock::now();
    run_dbd_dispatch_engine(parts, boms, ipc_planned_orders, demands, scheduled_orders, allocated_capacity, order_capacities, order_routing_costs);
    auto t_dbd_end = std::chrono::high_resolution_clock::now();
    double ms_dbd = std::chrono::duration<double, std::milli>(t_dbd_end - t_dbd_start).count();
    std::cout << "[计时] MCDS 派程排产计算耗时: " << ms_dbd << " ms (" << ms_dbd / 1000.0 << " s)" << std::endl;
    std::cout << "   -> 最终生成执行级排产工单数: " << scheduled_orders.size() << std::endl;

    std::cout << "\n=====================================================================" << std::endl;
    std::cout << "[压力测试成功] 所有计划场景在 50万张订单/20层BOM/1156代子孙的大规模环境下运算完毕！" << std::endl;
    std::cout << "=====================================================================" << std::endl;
}

} // namespace ipc

