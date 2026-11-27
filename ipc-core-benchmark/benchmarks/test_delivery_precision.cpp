#include "ipc_core/atp_ctp_engine.h"
#include "ipc_core/sandbox_operators.h"
#include <iostream>
#include <cassert>

using namespace ipc_core;

int main() {
    std::cout << "================================================================\n";
    std::cout << "[Test 1] 排序耦合定律 (2 需求 x 3 物料) 与 CSR 零堆内存回滚测试\n";
    std::cout << "================================================================\n";

    // Part A: 排序耦合定律最小完备反例 + 通用多需求正交解耦对账 (SSRN 7640758)
    std::cout << "\n[Part A] 最小完备反例: 2 需求 (D1 高优 t=4, D2 常规 t=2) x 3 物料 (M1, M2, M3)\n";
    SequencingCouplingInput sc_in;
    SequencingCouplingResult sc_res = evaluate_sequencing_coupling_2x3(sc_in);

    std::vector<MultiDemandPeggingOrder> gen_demands = {
        {1, 4, 1, {0, 1}}, // D1 (优1): 需 M1(0) + M2(1), 要求 t=4
        {2, 2, 2, {1, 2}}  // D2 (优2): 需 M2(1) + M3(2), 要求 t=2
    };
    std::vector<std::vector<int>> supply_arrivals = {
        {4},    // M1: t=4
        {1, 3}, // M2: t=1 现货, t=3 在途
        {2}     // M3: t=2
    };
    MultiDemandPeggingSummary gen_res = run_two_stage_orthogonal_pegging(gen_demands, supply_arrivals);
    assert(gen_res.legacy_finish_days[0] == 4 && gen_res.legacy_finish_days[1] == 3);
    assert(gen_res.ipc_finish_days[0] == 4 && gen_res.ipc_finish_days[1] == 2);
    assert(gen_res.legacy_total_delay_days == 1 && gen_res.ipc_total_delay_days == 0);
    assert(gen_res.legacy_idle_lock_days == 4 && gen_res.ipc_idle_lock_days == 2);

    std::cout << "  - 传统静态优先级排他锁库: D1=t" << sc_res.trad_d1_finish_day
              << ", D2=t" << sc_res.trad_d2_finish_day
              << " (D2 无故延期 " << sc_res.trad_d2_delay_days
              << " 天, 通用件 M2 闲置锁死 " << sc_res.trad_m2_idle_lock_days << " 天)\n";
    std::cout << "  - IPC 两阶段正交解耦治理: D1=t" << sc_res.ipc_d1_finish_day
              << ", D2=t" << sc_res.ipc_d2_finish_day
              << " (D2 延期 " << sc_res.ipc_d2_delay_days
              << " 天, 通用件 M2 闲置锁死 " << sc_res.ipc_m2_idle_lock_days << " 天 | 通用多需求引擎精确对齐)\n";

    assert(sc_res.trad_d2_finish_day == 3 && sc_res.trad_d2_delay_days == 1);
    assert(sc_res.trad_m2_idle_lock_days == 3);
    assert(sc_res.ipc_d1_finish_day == 4 && sc_res.ipc_d2_finish_day == 2);
    assert(sc_res.ipc_d2_delay_days == 0 && sc_res.ipc_m2_idle_lock_days == 1);

    // Part B: CSR 邻接索引 + 预分配 WatermarkRollbackArena 真零堆内存原子回滚
    std::vector<SkuNodeRecord> parts = {
        {0, "MDL_PRO_FG",   "PLANT_SZ", 0.0, 0.0,  1.0, 2, 800.0, true, false},
        {1, "SEMI_BARE_01", "PLANT_SZ", 0.0, 50.0, 1.0, 1, 600.0, true, true},
        {2, "RAW_CHIP_01",  "PLANT_SZ", 0.0, 20.0, 1.0, 0, 200.0, true, true}
    };

    std::vector<BomEdgeRecord> boms = {
        {0, 1, 1.0, 0, 0, 1.0, 0.0, 1.0},
        {1, 2, 2.0, 0, 0, 1.0, 0.0, 1.0}
    };
    CsrBomGraph csr_bom;
    csr_bom.build(static_cast<uint32_t>(parts.size()), boms);
    WatermarkRollbackArena arena(128);
    const size_t cap_before = arena.supply_trail.capacity();

    std::vector<std::vector<ATPSupplyNode>> atp_supplies(3);
    atp_supplies[1].push_back({"OH_BARE", "On-Hand", 1, 0, 50.0, 0.0, 100});
    atp_supplies[2].push_back({"OH_CHIP", "On-Hand", 2, 0, 20.0, 0.0, 100});

    std::vector<CapacityRecord> capacity;
    for (int d = 0; d <= 15; ++d) {
        capacity.push_back({"WC_ASM_01", d, 100.0, 0.0});
    }

    std::cout << "\n[Part B-1] 正常齐套查询: 成品 MDL_PRO_FG 需求 30 台 (交期 Day 3)...\n";
    ATPQueryResult res_a = promise_delivery_date_zero_heap(0, 3, 30.0, 1, atp_supplies, capacity, parts, csr_bom, arena);
    std::cout << "  - 计算状态: " << (res_a.is_fulfillable ? "SUCCEEDED" : "FAILED")
              << " | 承诺交付日: Day " << res_a.promised_day
              << " | 占用产能: " << res_a.total_capacity_used << " h\n";
    assert(res_a.is_fulfillable && res_a.promised_day == 3);

    double bare_alloc_before = atp_supplies[1][0].allocated_qty;
    double chip_alloc_before = atp_supplies[2][0].allocated_qty;

    std::cout << "\n[Part B-2] 缺料原子回滚测试: 需求 100 台 (底层芯片不足 -> 触发预分配游标全树零残留回滚)...\n";
    ATPQueryResult res_b = promise_delivery_date_zero_heap(0, 3, 100.0, 1, atp_supplies, capacity, parts, csr_bom, arena);
    std::cout << "  - 计算状态: " << (res_b.is_fulfillable ? "SUCCEEDED" : "BLOCKED_CLEANLY")
              << " | 回滚轮次: " << res_b.rollback_steps_count
              << " | 回滚后在手占用差值: " << (atp_supplies[1][0].allocated_qty - bare_alloc_before)
              << " | 热路径扩容次数: " << (arena.supply_trail.capacity() == cap_before ? 0 : 1) << "\n";
    assert(!res_b.is_fulfillable);
    assert(atp_supplies[1][0].allocated_qty == bare_alloc_before);
    assert(atp_supplies[2][0].allocated_qty == chip_alloc_before);
    assert(arena.supply_trail.capacity() == cap_before);

    std::cout << "\n>>> [PASSED] Test 1: 排序耦合正交解耦与 CSR 零堆内存回滚 100% 校验通过.\n";
    return 0;
}
