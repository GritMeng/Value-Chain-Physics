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

    std::cout << "\n>>> [PASSED] Test 5: ITP/IOP 齐套瓶颈配额、逆向重展与执行防波堤校验通过.\n";
    return 0;
}
