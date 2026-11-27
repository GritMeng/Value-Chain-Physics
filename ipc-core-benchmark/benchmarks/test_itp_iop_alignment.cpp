#include "ipc_core/itp_iop_alignment.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace ipc_core;

int main() {
    std::cout << "================================================================\n";
    std::cout << "[Test 5] 主计划 (ITP) 齐套瓶颈配额切分、逆向重展与执行计划 (IOP) 协同测试\n";
    std::cout << "================================================================\n";

    std::vector<SkuNodeRecord> parts(100);

    std::vector<DemandOrderRecord> master_demands = {
        {1, 10, 5, 1000.0, 1, "STRAT_GROUP_A", "NORTH_AMERICA"},
        {2, 20, 5, 500.0,  2, "REG_GROUP_B",   "EAST_ASIA"}
    };

    std::cout << "\n[Part A] 四维配额防波堤生成与执行期越界拦截...\n";
    auto firewalls = generate_itp_master_allotments(master_demands, parts, 1.10);
    std::cout << "  - 生成防波堤约束桶数: " << firewalls.size() << "\n";

    std::vector<DemandOrderRecord> execution_demands = {
        {101, 10, 5, 600.0, 1, "STRAT_GROUP_A", "NORTH_AMERICA"},
        {102, 10, 5, 450.0, 2, "STRAT_GROUP_A", "NORTH_AMERICA"},
        {103, 10, 5, 300.0, 3, "STRAT_GROUP_A", "NORTH_AMERICA"},
        {104, 20, 5, 500.0, 1, "REG_GROUP_B",   "EAST_ASIA"}
    };

    IOPExecutionResult result = run_iop_execution_alignment(execution_demands, parts, firewalls);
    std::cout << "  - 总请求单数: " << result.total_orders
              << " | 放行下派: " << result.scheduled_orders
              << " | 超额阻断: " << result.blocked_orders
              << " | 配额消耗率: " << (result.quota_utilization * 100.0) << "%\n";

    assert(result.scheduled_orders == 3);
    assert(result.blocked_orders == 1);

    std::cout << "\n[Part B] 无菌四步闭环: 齐套先行 -> 最紧瓶颈战略+公平切分 -> 逆向重展防超买 -> 独立桶执行...\n";
    RegionalAllotmentInput closed_in;
    ClosedLoopAllotmentResult cl = evaluate_itp_kitting_most_gating_closed_loop(closed_in);

    std::cout << "  - [Step 1 齐套先行] 区域A预测 6000 (专用件 5000) -> 齐套上限=" << cl.kitted_a
              << " | 区域D预测 3000 -> 齐套上限=" << cl.kitted_d << "\n";
    std::cout << "  - [Step 2 瓶颈切分] 共用芯片 6000 (最紧瓶颈=" << (cl.is_most_gating ? "YES" : "NO")
              << ") -> 战略保底 2000 + 剩余公平对半 -> 配额 A=" << cl.allotment_a
              << ", D=" << cl.allotment_d << "\n";
    std::cout << "  - [Step 3 逆向重展] 非瓶颈外购件: 现有系统直展=" << cl.legacy_non_bottleneck_buy
              << " vs IPC配额重展=" << cl.ipc_renetted_buy
              << " (消除超买呆滞=" << cl.excess_buy_eliminated << ")\n";
    std::cout << "  - [Step 4 独立桶IOP] 区域A实单 4500 放行=" << cl.ipc_fulfilled_a
              << " (拦截=" << cl.ipc_blocked_a << ") | 区域D实单 2000 放行=" << cl.ipc_fulfilled_d
              << " (现有无桶被抢缺口=" << cl.legacy_shortage_d << " vs IPC缺口=" << cl.ipc_shortage_d << ")\n";

    assert(std::abs(cl.kitted_a - 5000.0) < 1e-6 && std::abs(cl.kitted_d - 3000.0) < 1e-6);
    assert(cl.is_most_gating);
    assert(std::abs(cl.allotment_a - 4000.0) < 1e-6 && std::abs(cl.allotment_d - 2000.0) < 1e-6);
    assert(std::abs(cl.excess_buy_eliminated - 3000.0) < 1e-6);
    assert(std::abs(cl.legacy_shortage_d - 500.0) < 1e-6);
    assert(std::abs(cl.ipc_fulfilled_a - 4000.0) < 1e-6 && std::abs(cl.ipc_blocked_a - 500.0) < 1e-6);
    assert(std::abs(cl.ipc_fulfilled_d - 2000.0) < 1e-6 && std::abs(cl.ipc_shortage_d - 0.0) < 1e-6);

    std::cout << "\n[Part C] 通用 4 区域 x 3 共用芯片池: 自动识别最紧瓶颈 + 触顶循环注水公平切分...\n";
    // 4 区域: EU(预测4000, 专用件3200, 保底1000), NA(预测3000, 专用件3000, 保底1000),
    //         APAC(预测2500, 专用件1300 -> 触顶重分测试), LATAM(预测2000, 专用件2000)
    // 共用池: PMIC(12000颗, 单耗1), RF_PA(15000颗, 单耗2 -> 折合7500台, 最紧瓶颈!), PCB_SUB(9500颗, 单耗1)
    std::vector<RegionalDemandStream> multi_streams = {
        {"EU",    1, 4000.0, 1000.0, {3200.0, 3500.0}, 2.0, 3100.0},
        {"NA",    2, 3000.0, 1000.0, {3000.0},         2.0, 2600.0},
        {"APAC",  3, 2500.0, 0.0,    {1300.0},         2.0, 1300.0},
        {"LATAM", 4, 2000.0, 0.0,    {2000.0},         2.0, 1500.0},
    };
    std::vector<SharedComponentPool> shared_pools = {
        {"PMIC_MAIN", 12000.0, 1.0},
        {"RF_PA_2X",  15000.0, 2.0}, // 等效上限 7,500 < 总齐套 9,500 -> Most-Gating!
        {"WIFI_CHIP", 10000.0, 1.0},
    };
    MultiRegionAllotmentResult mr = evaluate_multi_region_most_gating_allotment(multi_streams, shared_pools);
    std::cout << "  - 自动识别最紧瓶颈件: " << mr.most_gating_part_code
              << " | 总齐套需求=" << mr.total_kitted_demand
              << " | 总获批配额=" << mr.total_allotment
              << " | 消除非瓶颈超买=" << mr.total_excess_buy_eliminated << "\n";
    assert(mr.has_bottleneck && mr.most_gating_part_code == "RF_PA_2X");
    assert(std::abs(mr.total_kitted_demand - 9500.0) < 1e-6);
    assert(std::abs(mr.total_allotment - 7500.0) < 1e-6);
    // 非瓶颈件单耗 2.0: 原始预测买 11500*2=23000, 逆向重展买 7500*2=15000 -> 消除 8000 超买
    assert(std::abs(mr.total_excess_buy_eliminated - 8000.0) < 1e-6);

    std::cout << "\n>>> [PASSED] Test 5: ITP/IOP 通用多区域瓶颈注水切分、逆向重展与执行防波堤校验通过.\n";
    return 0;
}
