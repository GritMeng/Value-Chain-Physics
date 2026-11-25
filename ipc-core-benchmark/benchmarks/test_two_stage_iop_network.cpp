#include "ipc_core/sandbox_operators.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace ipc_core;

int main() {
    std::cout << "=== [Suite 7] Two-Stage Push/Pull IOP & Value Chain Network Algebra ===\n";

    // 1. 跨 CODP 两阶段受限推拉排产 (Push-IOP 预投通用裸机 + Pull-IOP 配额门控与诚实交期承诺)
    TwoStageIopInput iop_in;
    iop_in.forecast_bare_qty = 1000.0;
    iop_in.daily_wc_cap = 300.0;
    iop_in.chip_oh_day1 = 600.0;
    iop_in.chip_po_day = 4;
    iop_in.chip_po_qty = 400.0;
    iop_in.post_pack_lt_days = 1;
    iop_in.horizon_days = 8;
    iop_in.pull_orders = {
        {"ORD_EU_500", 4, 500.0, 500.0, 1},
        {"ORD_AF_400", 4, 400.0, 400.0, 2},
        {"ORD_LATAM_250", 4, 250.0, 100.0, 3}
    };

    auto iop_res = evaluate_two_stage_push_pull_iop(iop_in);
    assert(std::abs(iop_res.total_bare_built - 1000.0) < 1e-6);
    assert(std::abs(iop_res.daily_bare_built[0] - 300.0) < 1e-6); // D1: 300
    assert(std::abs(iop_res.daily_bare_built[1] - 300.0) < 1e-6); // D2: 300
    assert(std::abs(iop_res.daily_bare_built[2] - 0.0) < 1e-6);   // D3: 0 (等芯片在途 D4)
    assert(std::abs(iop_res.daily_bare_built[3] - 300.0) < 1e-6); // D4: 300
    assert(std::abs(iop_res.daily_bare_built[4] - 100.0) < 1e-6); // D5: 100
    assert(iop_res.idle_wait_chip_days == 1);

    // 传统仅看后道 1d 静态提前期盲目承诺 D4 发 1,150 台 -> 实际 D3 裸机仅 600 台，爽约 550 台
    assert(std::abs(iop_res.legacy_broken_promise_qty - 550.0) < 1e-6);
    assert(std::abs(iop_res.ipc_broken_promise_qty - 0.0) < 1e-6);
    assert(std::abs(iop_res.total_quota_blocked_qty - 150.0) < 1e-6);
    assert(iop_res.order_promises[0].ipc_promised_day == 4); // EU 500: D2裸机齐套+1d=D3<=D4 -> 准时 D4
    assert(iop_res.order_promises[1].ipc_promised_day == 5); // AF 400: D4裸机齐套+1d -> 诚实承诺 D5
    assert(iop_res.order_promises[2].ipc_promised_day == 6); // LATAM 100: D5裸机齐套+1d -> 诚实承诺 D6

    std::cout << "  [PASS 7A] Two-Stage Push/Pull IOP: Legacy Broken Promise = "
              << iop_res.legacy_broken_promise_qty << " units vs IPC Broken Promise = "
              << iop_res.ipc_broken_promise_qty << " (EU=D4, AF=D5, LATAM=D6, Blocked=150)\n";

    // 2. 价值链网络先序区间 [rank..rank+tree_size-1] + 维度规划 GE/EQ + 国别认证位掩码 + 菱形去重
    // 节点定义:
    // 0: FG_PHONE_EU, 1: FG_PHONE_AF, 2: SEMI_MAINBOARD, 3: SEMI_POWER_KIT,
    // 4: PMIC_CHIP_SHARED (菱形汇合件: 主板耗 1 颗 + 电源包耗 2 颗),
    // 5: DIE_512M (spec=512, cert=0b11 EU+AF),
    // 6: DIE_256M (spec=256, cert=0b01 EU only),
    // 7: DIE_128M (spec=128, cert=0b10 AF only)
    std::vector<NetworkNodeDef> nodes = {
        {0, "FG_PHONE_EU", 256.0, 0x01u},
        {1, "FG_PHONE_AF", 128.0, 0x02u},
        {2, "SEMI_MAINBOARD", 0.0, 0x03u},
        {3, "SEMI_POWER_KIT", 0.0, 0x03u},
        {4, "PMIC_CHIP_SHARED", 0.0, 0x03u},
        {5, "DIE_512M", 512.0, 0x03u},
        {6, "DIE_256M", 256.0, 0x01u},
        {7, "DIE_128M", 128.0, 0x02u},
    };
    std::vector<NetworkEdgeDef> edges = {
        {0, 2, 1.0, NetworkDimOp::PASS},
        {0, 3, 1.0, NetworkDimOp::PASS},
        {1, 2, 1.0, NetworkDimOp::PASS},
        {1, 3, 1.0, NetworkDimOp::PASS},
        {2, 4, 1.0, NetworkDimOp::PASS},
        {3, 4, 2.0, NetworkDimOp::PASS},
        {2, 5, 1.0, NetworkDimOp::GE},
        {2, 6, 1.0, NetworkDimOp::GE},
        {2, 7, 1.0, NetworkDimOp::GE},
    };

    // EU 订单 100 台: 要求规格 >= 256MB (GE 向下兼容) 且具备 EU CE 认证 (0x01)
    auto net_ge = explode_value_chain_network(nodes, edges, 0, 100.0, 256.0, 0x01u, NetworkDimOp::GE);
    assert(net_ge.spans[0].rank == 1 && net_ge.spans[0].tree_size == 7);
    assert(net_ge.spans[0].contains_rank(net_ge.spans[2].rank)); // O(1) 先序区间包含判定
    assert(net_ge.pruned_by_dim_or_cert == 1); // DIE_128M 因规格<256且无EU认证被剪枝

    // 校验菱形共用件 PMIC_CHIP_SHARED: PATH_INSTANCE 出现 2 次 (100, 200), DISTINCT_NODE 守恒合并为 300
    int pmic_paths = 0;
    double pmic_path_sum = 0.0;
    for (const auto &row : net_ge.path_instances) {
        if (row.node_id == 4) {
            pmic_paths++;
            pmic_path_sum += row.cum_qty;
        }
    }
    double pmic_distinct_qty = 0.0;
    for (const auto &row : net_ge.distinct_nodes) {
        if (row.node_id == 4) pmic_distinct_qty = row.cum_qty;
    }
    assert(pmic_paths == 2 && std::abs(pmic_path_sum - 300.0) < 1e-6);
    assert(std::abs(pmic_distinct_qty - 300.0) < 1e-6);

    // 切换为 EQ 精确同规匹配: DIE_512M 亦被剪枝，仅剩 DIE_256M 通过
    auto net_eq = explode_value_chain_network(nodes, edges, 0, 100.0, 256.0, 0x01u, NetworkDimOp::EQ);
    assert(net_eq.pruned_by_dim_or_cert == 2);

    // 逆向反查 Where-Used: PMIC_CHIP_SHARED (node 4) 向上穿透反查出两个顶层成品 0 (EU) 与 1 (AF)
    auto roots = trace_network_where_used(edges, 4);
    assert(roots.size() == 2 && roots[0] == 0 && roots[1] == 1);

    std::cout << "  [PASS 7B] Value Chain Network Algebra: Preorder [1.." << net_ge.spans[0].tree_size
              << "] O(1) Subtree + GE/EQ Spec & Cert Mask + Diamond Conservation (300 == 100+200) + Where-Used (2 roots)\n";
    return 0;
}

