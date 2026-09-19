// IPC Engine 极限性能基准测试 (数据由 CSV 加载)
// 数据来源: data/*.csv  (由 data/generate_data.py 确定性生成)
#include "ipc_core/atp_ctp_engine.h"
#include "ipc_core/itp_iop_alignment.h"
#include "ipc_core/data_loader.h"
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace ipc_core;

int main(int argc, char** argv) {
    std::string data_dir = (argc > 1) ? argv[1] : "data";

    std::cout << "========================================================\n";
    std::cout << "  IPC Engine 核心求解器 极限性能基准测试\n";
    std::cout << "  数据源: " << data_dir << "/*.csv\n";
    std::cout << "========================================================\n";

    // ---------- Phase 1: 从 CSV 加载数据 ----------
    std::cout << "\n[Phase 1] 正在从 CSV 加载测试数据...\n";
    auto t0 = std::chrono::high_resolution_clock::now();

    BenchDataset ds;
    try {
        ds = load_bench_dataset(data_dir);
    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] 数据加载失败: " << e.what() << "\n";
        std::cerr << "提示: 请先运行  python3 data/generate_data.py\n";
        return 1;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double load_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    const size_t NUM_SKUS    = ds.parts.size();
    const size_t NUM_MASTER  = ds.master_demands.size();
    const size_t NUM_EXEC    = ds.execution_demands.size();

    std::cout << "  - 物料 SKU       : " << NUM_SKUS   << "\n";
    std::cout << "  - ITP 主计划需求 : " << NUM_MASTER << "\n";
    std::cout << "  - IOP 执行需求   : " << NUM_EXEC   << "\n";
    std::cout << "  - BOM 条目       : " << ds.boms.size() << "\n";
    std::cout << "  - 产能记录       : " << ds.capacity_records.size() << "\n";
    std::cout << "  - 加载耗时       : " << std::fixed << std::setprecision(2)
              << load_ms << " ms\n";

    // ---------- Phase 2: ITP 主计划防波堤 ----------
    std::cout << "\n[Phase 2] 基准 1: ITP 主计划软约束防波堤求解 (" << NUM_MASTER << " 条)...\n";
    auto t2 = std::chrono::high_resolution_clock::now();

    auto allotments = generate_itp_master_allotments(
        ds.master_demands, ds.parts, 1.15);

    auto t3 = std::chrono::high_resolution_clock::now();
    double itp_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();

    std::cout << "  - 防波堤约束生成数: " << allotments.size() << "\n";
    std::cout << "  - ITP 耗时: " << std::fixed << std::setprecision(2) << itp_ms
              << " ms  (" << std::setprecision(0)
              << (NUM_MASTER / (itp_ms / 1000.0)) << " Demands/Sec)\n";

    // ---------- Phase 3: IOP 执行计划刚性阻断 ----------
    std::cout << "\n[Phase 3] 基准 2: IOP 执行计划刚性阻断协同 (" << NUM_EXEC << " 条)...\n";
    auto t4 = std::chrono::high_resolution_clock::now();

    IOPExecutionResult iop = run_iop_execution_alignment(
        ds.execution_demands, ds.parts, allotments);

    auto t5 = std::chrono::high_resolution_clock::now();
    double iop_ms = std::chrono::duration<double, std::milli>(t5 - t4).count();

    std::cout << "  - 总请求单数     : " << iop.total_orders << "\n";
    std::cout << "  - 成功下派单数   : " << iop.scheduled_orders << "\n";
    std::cout << "  - 配额阻断单数   : " << iop.blocked_orders << "\n";
    std::cout << "  - 配额消耗率     : " << std::fixed << std::setprecision(2)
              << (iop.quota_utilization * 100.0) << " %\n";
    std::cout << "  - IOP 耗时: " << iop_ms << " ms  (" << std::setprecision(0)
              << (NUM_EXEC / (iop_ms / 1000.0)) << " Orders/Sec)\n";

    // 正确性校验：执行需求含 1.3x 超量插单，必须触发阻断
    if (iop.blocked_orders == 0) {
        std::cout << "\n  [WARN] 未触发任何配额阻断，请检查 demands_execution.csv\n";
    } else {
        std::cout << "\n  [CHECK] 刚性阻断路径已被覆盖 (blocked=" << iop.blocked_orders << ")\n";
    }

    // ---------- 汇总 ----------
    std::cout << "\n========================================================\n";
    std::cout << "  极限性能测试最终汇总\n";
    std::cout << "========================================================\n";
    std::cout << "  - 全链路总延迟     : " << std::fixed << std::setprecision(2)
              << (itp_ms + iop_ms) << " ms\n";
    std::cout << "  - 单次求解吞吐量   : " << std::setprecision(0)
              << ((NUM_MASTER + NUM_EXEC) / ((itp_ms + iop_ms) / 1000.0))
              << " 需求/秒\n";
    std::cout << "  - 数据加载开销     : " << std::setprecision(2) << load_ms << " ms (不计入求解)\n";
    std::cout << "========================================================\n";
    return 0;
}
