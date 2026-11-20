#include "ipc_core/atp_ctp_engine.h"
#include "ipc_core/itp_iop_alignment.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>

using namespace ipc_core;

int main() {
    std::cout << "================================================================\n";
    std::cout << "[Benchmark 6] IPC 微内核 2,000,000 节点 x 500,000 需求极限压测\n";
    std::cout << "================================================================\n";

    const size_t NUM_SKUS = 2000000;
    const size_t NUM_DEMANDS = 500000;

    std::cout << "[Phase 1] 正在生成 " << NUM_SKUS << " 个 SKU 节点及 " << NUM_DEMANDS << " 条合成独立需求...\n";
    auto t_start_prep = std::chrono::high_resolution_clock::now();

    std::vector<SkuNodeRecord> parts(NUM_SKUS);
    for (size_t i = 0; i < NUM_SKUS; ++i) {
        parts[i].part_id = static_cast<uint32_t>(i);
        parts[i].sku_code = "SKU_" + std::to_string(i);
        parts[i].site_code = "PLANT_MAIN";
        parts[i].safety_stock = 10.0;
        parts[i].initial_on_hand = 100.0;
        parts[i].lot_size = 1.0;
        parts[i].lead_time = 1;
    }

    std::vector<DemandOrderRecord> demands(NUM_DEMANDS);
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
    std::cout << "  - 内存数据合成耗时: " << std::fixed << std::setprecision(2) << prep_duration << " ms\n\n";

    std::cout << "[Phase 2] 500,000 条主计划 (ITP) 防波堤配额生成...\n";
    auto t_start_itp = std::chrono::high_resolution_clock::now();
    auto firewalls = generate_itp_master_allotments(demands, parts, 1.15);
    auto t_end_itp = std::chrono::high_resolution_clock::now();
    double itp_duration = std::chrono::duration<double, std::milli>(t_end_itp - t_start_itp).count();
    std::cout << "  - 防波堤桶数: " << firewalls.size() << " | ITP 耗时: " << itp_duration << " ms\n\n";

    std::cout << "[Phase 3] 500,000 条执行计划 (IOP) 刚性阻断协同...\n";
    auto t_start_iop = std::chrono::high_resolution_clock::now();
    IOPExecutionResult iop_res = run_iop_execution_alignment(demands, parts, firewalls);
    auto t_end_iop = std::chrono::high_resolution_clock::now();
    double iop_duration = std::chrono::duration<double, std::milli>(t_end_iop - t_start_iop).count();

    std::cout << "  - 下派成功: " << iop_res.scheduled_orders
              << " | 越界拦截: " << iop_res.blocked_orders
              << " | IOP 耗时: " << iop_duration << " ms\n\n";

    double total_ms = itp_duration + iop_duration;
    std::cout << "================================================================\n";
    std::cout << "  [SUMMARY] 总计算延迟: " << std::fixed << std::setprecision(2) << total_ms
              << " ms | 吞吐量: " << std::setprecision(0) << (NUM_DEMANDS / (total_ms / 1000.0))
              << " 需求/秒\n";
    std::cout << "================================================================\n";
    return 0;
}
