#include "ipc_core/substitution_engine.h"
#include "ipc_core/sandbox_operators.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace ipc_core;

int main() {
    std::cout << "================================================================\n";
    std::cout << "[Test 2] 成套组替代 (11,000 vs 1,000) 与三类配额圆整动态重归一化\n";
    std::cout << "================================================================\n";

    // Part A: 成套组替代沉没长板激活 (11,000 vs 1,000)
    std::cout << "\n[Part A] 成套组替代沉没长板激活 (G1 长板 11,000 + 短板拉快 1,000 vs G2 整套新购 12,000)\n";
    GroupSubInput gs_in{11000.0, 1000.0, 12000.0};
    GroupSubResult gs_res = evaluate_group_substitution_sunk_cost(gs_in);

    std::cout << "  - 现有 SCP/APS 跳过短板组重买 G2: 新购现金 " << gs_res.trad_new_cash_out
              << " + 长板报废 " << gs_res.trad_scrap_loss
              << " = 总经济损失 " << gs_res.trad_total_economic_cost << "\n";
    std::cout << "  - IPC 增量新购定向拉快短板 B1: 新购现金 " << gs_res.ipc_new_cash_out
              << " + 长板报废 " << gs_res.ipc_scrap_loss
              << " = 总成本 " << gs_res.ipc_total_economic_cost
              << " (净节省 " << gs_res.net_economic_savings << " | 决策: " << gs_res.decision_code << ")\n";
    assert(std::abs(gs_res.ipc_new_cash_out - 1000.0) < 1e-6);
    assert(std::abs(gs_res.ipc_scrap_loss - 0.0) < 1e-6);
    assert(std::abs(gs_res.net_economic_savings - 22000.0) < 1e-6);

    // Part B: 一/二/三类多源配额平衡、最小包装倍数圆整与安全库存保护
    std::vector<SkuNodeRecord> parts = {
        {0, "MAIN_SOC",    "PLANT_SZ", 0.0,  0.0,   1.0, 0},
        {1, "ALT_SUP_A",   "PLANT_SZ", 0.0,  100.0, 1.0, 0},
        {2, "ALT_SUP_B",   "PLANT_SZ", 0.0,  100.0, 1.0, 0},
        {3, "ALT_SUP_C",   "PLANT_SZ", 10.0, 100.0, 5.0, 0}
    };
    std::vector<double> current_on_hand = {0.0, 100.0, 100.0, 100.0};

    std::cout << "\n[Part B-1] 一类替代料按历史配额比例平衡 (Class 1)...\n";
    BomEdgeRecord edge_1a = {0, 1, 1.0, 1, 1, 0.6, 120.0, 1.0};
    BomEdgeRecord edge_1b = {0, 2, 1.0, 1, 1, 0.4, 100.0, 1.0};
    std::vector<BomEdgeRecord*> group1 = {&edge_1a, &edge_1b};
    uint32_t chosen_c1 = allocate_class1(50.0, group1, current_on_hand);
    std::cout << "  - 选优替代料 ID: " << chosen_c1 << " (预期: 1 号供应商，因配额落后缺口更大)\n";
    assert(chosen_c1 == 1);

    std::cout << "\n[Part B-2] 三类多源配额 + 包装倍数 (MPQ=300) 动态重归一化 (1,000 颗需求, 50%:30%:20%)...\n";
    std::vector<SkuNodeRecord> parts_renorm = {
        {0, "MAIN_RES", "PLANT_SZ", 0.0, 0.0,    1.0, 0},
        {1, "SUP_S1",   "PLANT_SZ", 0.0, 2000.0, 300.0, 0},
        {2, "SUP_S2",   "PLANT_SZ", 0.0, 2000.0, 300.0, 0},
        {3, "SUP_S3",   "PLANT_SZ", 0.0, 2000.0, 300.0, 0}
    };
    std::vector<double> oh_renorm = {0.0, 2000.0, 2000.0, 2000.0};
    BomEdgeRecord r_s1 = {0, 1, 1.0, 3, 10, 0.50, 0.0, 300.0};
    BomEdgeRecord r_s2 = {0, 2, 1.0, 3, 10, 0.30, 0.0, 300.0};
    BomEdgeRecord r_s3 = {0, 3, 1.0, 3, 10, 0.20, 0.0, 300.0};
    std::vector<BomEdgeRecord*> group_r = {&r_s1, &r_s2, &r_s3};
    std::vector<SubDispatchRecord> recs;
    allocate_class3(1000.0, group_r, oh_renorm, 1, 0, recs, parts_renorm);

    double total_alloc = 0.0;
    for (const auto& rec : recs) {
        std::cout << "  - 供应商 ID=" << rec.alt_part_id << " 分配量=" << rec.allocated_qty << "\n";
        total_alloc += rec.allocated_qty;
    }
    // Step1: S1 due=500 -> ceil(500/300)*300 = 600, rem=400
    // Step2: S2:S3 renorm 30:20 = 60%:40% -> S2 due=240 -> ceil(240/300)*300 = 300, rem=100
    // Step3: S3 takes remaining 100 -> Total 600 + 300 + 100 = 1000 exact conservation!
    assert(recs.size() == 3);
    assert(std::abs(recs[0].allocated_qty - 600.0) < 1e-6);
    assert(std::abs(recs[1].allocated_qty - 300.0) < 1e-6);
    assert(std::abs(recs[2].allocated_qty - 100.0) < 1e-6);
    assert(std::abs(total_alloc - 1000.0) < 1e-6);

    // Part C: 不完全替代（单向属性门槛 + 主料锚定 30% 混用红线）
    std::cout << "\n[Part C] 不完全替代：单向属性门槛 + 主料实际齐套量锚定 30% 混用红线...\n";
    // Case C-1: 主料齐套 700, 副料 500, 总需 1000, 红线 30%, 5000mAh >= 4500mAh -> 用 300 刚好 30.0% 满产
    IncompleteSubResult ic1 = evaluate_incomplete_substitution_rule(
        {1000.0, 700.0, 500.0, 0.30, IncompleteSubOp::GE, 5000.0, 4500.0});
    assert(ic1.dim_passed && std::abs(ic1.ipc_sub_used - 300.0) < 1e-6 && ic1.ipc_within_cap);
    std::cout << "  - Case C-1 (满产卡线 700+300): 副料用 " << ic1.ipc_sub_used
              << " 颗 | 实际占比=" << (ic1.ipc_actual_ratio * 100.0) << "% (合标)\n";

    // Case C-2: 主料断料仅齐套 500, 副料有 500, 红线 30%
    // 现有系统按总需 1000*30%=300 派副料 -> 300/800=37.5% 越线违约！
    // IPC 按主料锚定 floor((0.3/0.7)*500)=214 -> 214/714=29.97% <= 30% 零违约！
    IncompleteSubResult ic2 = evaluate_incomplete_substitution_rule(
        {1000.0, 500.0, 500.0, 0.30, IncompleteSubOp::GE, 5000.0, 4500.0});
    assert(ic2.legacy_breached_cap && std::abs(ic2.legacy_sub_used - 300.0) < 1e-6);
    assert(std::abs(ic2.ipc_sub_used - 214.0) < 1e-6 && ic2.ipc_within_cap);
    std::cout << "  - Case C-2 (主料缺料陷阱 500): 现有 SCP/APS 派 " << ic2.legacy_sub_used
              << " 颗 -> 占比飙至 " << (ic2.legacy_actual_ratio * 100.0) << "% (越线违约!) | "
              << "IPC 主料锚定截断 " << ic2.ipc_sub_used << " 颗 -> 占比 "
              << (ic2.ipc_actual_ratio * 100.0) << "% (零违约)\n";

    // Case C-3: 国别认证不匹配 (EQ: 101 欧规 != 102 美规) -> 硬拦截 0 颗
    IncompleteSubResult ic3 = evaluate_incomplete_substitution_rule(
        {1000.0, 700.0, 500.0, 0.30, IncompleteSubOp::EQ, 101.0, 102.0});
    assert(!ic3.dim_passed && std::abs(ic3.ipc_sub_used - 0.0) < 1e-6);
    std::cout << "  - Case C-3 (国别认证不匹配 EQ 101!=102): 硬拦截返回 " << ic3.ipc_sub_used << " 颗 ("
              << ic3.decision_code << ")\n";

    std::cout << "\n>>> [PASSED] Test 2: 组替代长板盘活、不完全替代红线与三类配额重归一化 100% 校验通过！\n";
    return 0;
}
