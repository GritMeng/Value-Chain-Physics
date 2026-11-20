#include "ipc_core/sandbox_operators.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <numeric>

using namespace ipc_core;

int main() {
    std::cout << "================================================================\n";
    std::cout << "[Test 4] 零预测冲减: 推式防独占日切割 + 拉式超额 Mode A/B/C 三态路由\n";
    std::cout << "================================================================\n";

    // Part A: 推式周受限配额跨周顺延 + D1..D6 防首日独占平准切割
    std::vector<double> daily_caps = {140.0, 140.0, 140.0, 140.0, 140.0, 100.0}; // 总计 800k
    PushSliceResult ps = evaluate_push_anti_monopoly_slice(600.0, 400.0, 800.0, daily_caps);

    double sum_a = std::accumulate(ps.daily_a.begin(), ps.daily_a.end(), 0.0);
    double sum_b = std::accumulate(ps.daily_b.begin(), ps.daily_b.end(), 0.0);

    std::cout << "\n[Part A] W1 需求 1,000k (A:600k / B:400k), 供应上限 800k:\n";
    std::cout << "  - W1 有效配额: A=" << ps.w1_alloc_a << "k, B=" << ps.w1_alloc_b
              << "k | 顺延滚入 W2 欠账: A=" << ps.w1_rollover_a << "k, B=" << ps.w1_rollover_b << "k\n";
    std::cout << "  - 日度防独占切分: D1..D5 每日 A=" << ps.daily_a[0] << "k / B=" << ps.daily_b[0]
              << "k | D6 A=" << ps.daily_a[5] << "k / B=" << ps.daily_b[5] << "k (周守恒: "
              << sum_a << "k + " << sum_b << "k = " << (sum_a + sum_b) << "k)\n";

    assert(std::abs(ps.w1_alloc_a - 480.0) < 1e-6 && std::abs(ps.w1_alloc_b - 320.0) < 1e-6);
    assert(std::abs(ps.w1_rollover_a - 120.0) < 1e-6 && std::abs(ps.w1_rollover_b - 80.0) < 1e-6);
    assert(std::abs(ps.daily_a[0] - 84.0) < 1e-6 && std::abs(ps.daily_b[0] - 56.0) < 1e-6);
    assert(std::abs(ps.daily_a[5] - 60.0) < 1e-6 && std::abs(ps.daily_b[5] - 40.0) < 1e-6);
    assert(std::abs(sum_a - 480.0) < 1e-6 && std::abs(sum_b - 320.0) < 1e-6);

    // Part B: 拉式实单超配额三态动态路由 (零孤立预测冲减)
    std::cout << "\n[Part B] 拉式实单超配额三态动态路由 (W1 配额=480k, 月包络=1,500k, 采购提前期=10d):\n";
    auto m_a = evaluate_pull_overflow_router(600.0, 480.0, 1420.0, 1500.0, 5, 10);
    auto m_b = evaluate_pull_overflow_router(680.0, 480.0, 1700.0, 1500.0, 12, 10);
    auto m_c = evaluate_pull_overflow_router(680.0, 480.0, 1700.0, 1500.0, 3, 10);

    std::cout << "  - Case 1 (节奏前移): " << m_a.mode_name
              << " | 扣 W2=" << m_a.w2_quota_deducted << "k | 增量 MRP=" << m_a.incremental_mrp_qty << "k\n";
    std::cout << "  - Case 2 (真增量交期够): " << m_b.mode_name
              << " | 扣 W2=" << m_b.w2_quota_deducted << "k | 增量 MRP=" << m_b.incremental_mrp_qty << "k\n";
    std::cout << "  - Case 3 (真增量急单): " << m_c.mode_name
              << " | 借 W2=" << m_c.w2_quota_deducted << "k | 回填 MRP=" << m_c.incremental_mrp_qty << "k\n";

    assert(m_a.mode == PullOverflowMode::MODE_A_ADVANCE_PULL_ZERO_MRP && std::abs(m_a.w2_quota_deducted - 120.0) < 1e-6 && std::abs(m_a.incremental_mrp_qty) < 1e-6);
    assert(m_b.mode == PullOverflowMode::MODE_B_TRUE_INCREMENT_DIRECT_MRP && std::abs(m_b.w2_quota_deducted) < 1e-6 && std::abs(m_b.incremental_mrp_qty - 200.0) < 1e-6);
    assert(m_c.mode == PullOverflowMode::MODE_C_URGENT_BORROW_AND_BACKFILL && std::abs(m_c.w2_quota_deducted - 200.0) < 1e-6 && std::abs(m_c.incremental_mrp_qty - 200.0) < 1e-6);

    std::cout << "\n>>> [PASSED] Test 4: 零预测冲减日切割与三态路由 100% 校验通过！\n";
    return 0;
}

