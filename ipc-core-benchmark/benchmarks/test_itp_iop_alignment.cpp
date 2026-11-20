#include "ipc_core/itp_iop_alignment.h"
#include <iostream>
#include <cassert>

using namespace ipc_core;

int main() {
    std::cout << "================================================================\n";
    std::cout << "[Test 5] 主计划 (ITP) 与执行计划 (IOP) 刚性配额防波堤协同测试\n";
    std::cout << "================================================================\n";

    std::vector<SkuNodeRecord> parts(100);

    std::vector<DemandOrderRecord> master_demands = {
        {1, 10, 5, 1000.0, 1, "VIP_CLIENT_A", "NORTH_AMERICA"},
        {2, 20, 5, 500.0,  2, "RETAIL_CLIENT_B", "EAST_ASIA"}
    };

    std::cout << "\n[Step 1] ITP 主计划生成战术防波堤配额 (10% Buffer)...\n";
    auto firewalls = generate_itp_master_allotments(master_demands, parts, 1.10);
    std::cout << "  - 生成防波堤约束桶数: " << firewalls.size() << "\n";

    std::vector<DemandOrderRecord> execution_demands = {
        {101, 10, 5, 600.0, 1, "VIP_CLIENT_A", "NORTH_AMERICA"},
        {102, 10, 5, 450.0, 2, "VIP_CLIENT_A", "NORTH_AMERICA"},
        {103, 10, 5, 300.0, 3, "VIP_CLIENT_A", "NORTH_AMERICA"},
        {104, 20, 5, 500.0, 1, "RETAIL_CLIENT_B", "EAST_ASIA"}
    };

    std::cout << "\n[Step 2] IOP 执行计划在刚性配额防波堤内派程与越界拦截...\n";
    IOPExecutionResult result = run_iop_execution_alignment(execution_demands, parts, firewalls);

    std::cout << "  - 总请求单数: " << result.total_orders
              << " | 放行下派: " << result.scheduled_orders
              << " | 超额阻断: " << result.blocked_orders
              << " | 配额消耗率: " << (result.quota_utilization * 100.0) << "%\n";

    assert(result.scheduled_orders == 3);
    assert(result.blocked_orders == 1);

    std::cout << "\n>>> [PASSED] Test 5: ITP/IOP 刚性配额防波堤校验通过！\n";
    return 0;
}
