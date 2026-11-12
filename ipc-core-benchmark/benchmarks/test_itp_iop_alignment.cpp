#include "ipc_core/itp_iop_alignment.h"
#include <iostream>
#include <cassert>

using namespace ipc_core;

int main() {
    std::cout << "========================================================\n";
    std::cout << "[Test 2] 主计划 (ITP) 与执行计划 (IOP) 协同阻断约束测试\n";
    std::cout << "========================================================\n";

    std::vector<PartSiteRecord> parts(100);

    // 1. 模拟 ITP 阶段：生成主计划与战术防波堤配额 (10% Buffer)
    std::vector<IndependentDemand> master_demands = {
        {1, 10, 5, 1000.0, 1, "VIP_CLIENT_A", "NORTH_AMERICA"},
        {2, 20, 5, 500.0,  2, "RETAIL_CLIENT_B", "EAST_ASIA"}
    };

    std::cout << "\n[Step 1] ITP 主计划计算防波堤防漂移配额...\n";
    auto allotments = generate_itp_master_allotments(master_demands, parts, 1.10);
    std::cout << "  - 生成全域防波堤约束项数: " << allotments.size() << "\n";

    // 2. 模拟 IOP 阶段：车间突发插入额外插单/超量需求 (试图超额抢占资源)
    std::vector<IndependentDemand> execution_demands = {
        {101, 10, 5, 600.0, 1, "VIP_CLIENT_A", "NORTH_AMERICA"},    // 正常需求 -> 应该通过
        {102, 10, 5, 450.0, 2, "VIP_CLIENT_A", "NORTH_AMERICA"},    // 正常需求 -> 累计 1050 <= 1100 -> 应该通过
        {103, 10, 5, 300.0, 3, "VIP_CLIENT_A", "NORTH_AMERICA"},    // 超量插单 -> 累计 1350 > 1100 -> 必须阻断拦截！
        {104, 20, 5, 500.0, 1, "RETAIL_CLIENT_B", "EAST_ASIA"}     // 正常需求 -> 500 <= 550 -> 应该通过
    };

    std::cout << "\n[Step 2] IOP 执行计划在刚性配额下求解与异常拦截...\n";
    IOPExecutionResult result = run_iop_execution_alignment(execution_demands, parts, allotments);

    std::cout << "  - 总请求排产单数: " << result.total_orders << "\n";
    std::cout << "  - 成功协同下派单数: " << result.scheduled_orders << "\n";
    std::cout << "  - 配额违规拦截单数 (Blocked): " << result.blocked_orders << "\n";
    std::cout << "  - 主计划配额消耗率: " << (result.quota_utilization * 100.0) << "%\n";

    assert(result.scheduled_orders == 3);
    assert(result.blocked_orders == 1);

    std::cout << "\n>>> [PASSED] Test 2: 主计划与执行计划协同防漂移与配额阻断校验通过！\n";
    return 0;
}
