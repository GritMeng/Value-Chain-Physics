#include "ipc_core/sandbox_operators.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>

using namespace ipc_core;

int main() {
    std::cout << "================================================================\n";
    std::cout << "[Test 3] 推拉解耦点 (CODP) 两层非补偿定位与二阶 Φ 影子收益验算\n";
    std::cout << "================================================================\n";

    // 标准 3 层 6 节点沙盘: SKU_CHIP(42d) -> SEMI_BARE(4d, ¥600, K=4) -> FG_CN/EU/US/ME(1d, ¥800)
    // 客户交期容忍期 D_tol = 3d, 统一承诺 SLA = 1d, 单国 sigma = 40 台/天, z = 2.0
    CodpPositioningInput normal_in;
    CodpPositioningResult r1 = evaluate_codp_two_layer_positioning(normal_in);

    std::cout << "\n[Case A] 正常通用裸机工艺 (未前置国别镭雕):\n";
    std::cout << "  - Gate 1 时间因果界 (L_down <= 3d): SKU_CHIP=" << (r1.chip_passes_gate1 ? "PASS" : "REJECT (5d>3d)")
              << " | SEMI_BARE=" << (r1.bare_passes_gate1 ? "PASS (1d<=3d)" : "REJECT") << "\n";
    std::cout << "  - 方案 A (CODP 放 4 国成品 MTS): 安全库存=" << r1.plan_a_fg_ss_units
              << " 台 | 资金占用=¥" << std::fixed << std::setprecision(0) << r1.plan_a_fg_capital << "\n";
    std::cout << "  - 方案 B (CODP 放沙漏腰部裸机): 安全库存=" << r1.plan_b_bare_ss_units
              << " 台 | 资金占用=¥" << r1.plan_b_bare_capital << "\n";
    std::cout << "  - 同源解耦共赢收益: 台数直降 " << std::setprecision(1) << r1.unit_reduction_pct
              << "% | 资金直降 " << r1.capital_reduction_pct
              << "% | 选定解耦点: " << r1.chosen_codp_node << "\n";

    assert(!r1.chip_passes_gate1);
    assert(r1.bare_passes_gate1 && r1.bare_passes_gate2);
    assert(std::abs(r1.plan_a_fg_ss_units - 640.0) < 1e-6);
    assert(std::abs(r1.plan_a_fg_capital - 512000.0) < 1e-6);
    assert(std::abs(r1.plan_b_bare_ss_units - 320.0) < 1e-6);
    assert(std::abs(r1.plan_b_bare_capital - 192000.0) < 1e-6);
    assert(std::abs(r1.unit_reduction_pct - 50.0) < 1e-6);
    assert(std::abs(r1.capital_reduction_pct - 62.5) < 1e-6);

    // Case B: 旧工艺将国别镭雕前置，导致裸机丧失 Gate 2 资格 -> 输出二阶修宪 Φ 移点影子收益
    CodpPositioningInput legacy_in = normal_in;
    legacy_in.legacy_early_engraving = true;
    CodpPositioningResult r2 = evaluate_codp_two_layer_positioning(legacy_in);

    std::cout << "\n[Case B] 旧工艺前置国别镭雕 (触发 Gate 2 拦截与二阶修宪 Φ 影子收益):\n";
    std::cout << "  - Gate 2 未固化差异检查: SEMI_BARE=" << (r2.bare_passes_gate2 ? "PASS" : "REJECT (已不可逆固化国别)") << "\n";
    std::cout << "  - 退化选址: " << r2.chosen_codp_node << "\n";
    std::cout << "  - 二阶修宪 Φ 影子收益 (将镭雕后移至包装段释放常态资金): ¥"
              << std::setprecision(0) << r2.phi_shadow_value_capital << "\n";
    assert(!r2.bare_passes_gate2);
    assert(std::abs(r2.phi_shadow_value_capital - 320000.0) < 1e-6);

    std::cout << "\n>>> [PASSED] Test 3: CODP 两层定位与二阶修宪影子收益 100% 校验通过.\n";
    return 0;
}

