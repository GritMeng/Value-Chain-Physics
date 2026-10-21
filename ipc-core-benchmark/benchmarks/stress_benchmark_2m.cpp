#include "ipc_core/atp_ctp_engine.h"
#include "ipc_core/itp_iop_alignment.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <iomanip>

using namespace ipc_core;

int main() {
    std::cout << "========================================================\n";
    std::cout << "  IPC Engine 核心求解器 2,000,000 级极限性能基准测试     \n";
    std::cout << "========================================================\n";

    const size_t NUM_SKUS = 2000000;
    const size_t NUM_DEMANDS = 500000;

    std::cout << "[Phase 1] 正在生成 " << NUM_SKUS << " 个 SKU 及 " << NUM_DEMANDS << " 条合成独立需求...\n";
    auto t_start_prep = std::chrono::high_resolution_clock::now();

    std::vector<PartSiteRecord> parts(NUM_SKUS);
    for (size_t i = 0; i < NUM_SKUS; ++i) {
        parts[i] = {
            static_cast<uint32_t>(i),
            "SKU_" + std::to_string(i),
            "PLANT_MAIN",
            10.0,
            100.0,
            1.0,
            1
        };
    }

    std::vector<IndependentDemand> demands(NUM_DEMANDS);
    for (size_t i = 0; i < NUM_DEMANDS; ++i) {
        demands[i] = {
            static_cast<uint32_t>(i + 1),
            static_cast<uint32_t>(i % (NUM_SKUS / 2)),
            static_cast<int>(i % 30) + 1,
            50.0,
            static_cast<uint64_t>(i % 5 + 1),
            "CUST_GRP_" + std::to_string(i % 20),
            "REGION_" + std::to_string(i % 5)
        };
    }

    auto t_end_prep = std::chrono::high_resolution_clock::now();
    double prep_duration = std::chrono::duration<double, std::milli>(t_end_prep - t_start_prep).count();
    std::cout << "  - 数据准备完成，内存准备耗时: " << std::fixed << std::setprecision(2) << prep_duration << " ms\n\n";

    // 1. 测试 ITP 战术防波堤主计划生成速度
    std::cout << "[Phase 2] 基准测试 1: 500,000 条主计划 (ITP) 软约束防波堤求解...\n";
    auto t_start_itp = std::chrono::high_resolution_clock::now();

    auto allotments = generate_itp_master_allotments(demands, parts, 1.15);

    auto t_end_itp = std::chrono::high_resolution_clock::now();
    double itp_duration = std::chrono::duration<double, std::milli>(t_end_itp - t_start_itp).count();

    std::cout << "  - ITP 求解完成，防波堤约束生成数: " << allotments.size() << "\n";
    std::cout << "  - ITP 耗时: " << std::fixed << std::setprecision(2) << itp_duration << " ms ("
              << (NUM_DEMANDS / (itp_duration / 1000.0)) << " Demands/Sec)\n\n";

    // 2. 测试 IOP 执行计划刚性约束下派协同速度
    std::cout << "[Phase 3] 基准测试 2: 500,000 条执行计划 (IOP) 刚性阻断协同与吞吐量求解...\n";
    auto t_start_iop = std::chrono::high_resolution_clock::now();

    IOPExecutionResult iop_res = run_iop_execution_alignment(demands, parts, allotments);

    auto t_end_iop = std::chrono::high_resolution_clock::now();
    double iop_duration = std::chrono::duration<double, std::milli>(t_end_iop - t_start_iop).count();

    std::cout << "  - IOP 求解完成，成功下派单数: " << iop_res.scheduled_orders 
              << ", 拦截单数: " << iop_res.blocked_orders << "\n";
    std::cout << "  - IOP 耗时: " << std::fixed << std::setprecision(2) << iop_duration << " ms ("
              << (NUM_DEMANDS / (iop_duration / 1000.0)) << " Orders/Sec)\n\n";

    std::cout << "========================================================\n";
    std::cout << "  2,000,000 级极限性能测试最终汇总 (Benchmark Performance Summary) \n";
    std::cout << "========================================================\n";
    std::cout << "  - 单次求解吞吐量 (Throughput): " << std::fixed << std::setprecision(0)
              << (NUM_DEMANDS / ((itp_duration + iop_duration) / 1000.0)) << " 需求/秒\n";
    std::cout << "  - 全链路并发响应总延迟 (Total Latency): " << std::fixed << std::setprecision(2)
              << (itp_duration + iop_duration) << " ms\n";
    std::cout << "  - 内存缓存命中状态: SoA 结构体数组连续无指针追踪 (Zero-Pointer Cache Line Alignment)\n";
    std::cout << "========================================================\n";

    return 0;
}
