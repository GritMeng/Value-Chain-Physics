#pragma once
#include <string>
#include <vector>

namespace ipc_core {

// =========================================================================
// 1. 排序耦合定律最小完备模型 (2 Demands x 3 Materials x 4 Periods)
// =========================================================================
struct SequencingCouplingInput {
    int d1_req_day = 4;
    int d2_req_day = 2;
    int m1_avail_day = 4; // D1 专用件
    int m2_oh_day = 1;    // 通用件在手现货日
    int m2_po_day = 3;    // 通用件在途到货日
    int m3_avail_day = 2; // D2 专用件
};

struct SequencingCouplingResult {
    // 传统静态优先级排他锁库
    int trad_d1_finish_day = 0;
    int trad_d2_finish_day = 0;
    int trad_d2_delay_days = 0;
    int trad_m2_idle_lock_days = 0;

    // IPC 两阶段正交解耦（顺推齐套下界 + 逆拉晚绑定）
    int ipc_d1_finish_day = 0;
    int ipc_d2_finish_day = 0;
    int ipc_d2_delay_days = 0;
    int ipc_m2_idle_lock_days = 0;
};

SequencingCouplingResult evaluate_sequencing_coupling_2x3(const SequencingCouplingInput& in);

// =========================================================================
// 2. 成套组替代沉没长板激活模型 (11,000 vs 1,000)
// =========================================================================
struct GroupSubInput {
    double g1_main_sunk_value = 11000.0; // G1 长板主芯片在手沉没价值
    double g1_minor_expedite_cost = 1000.0; // G1 短板小料定向拉快成本
    double g2_full_new_cost = 12000.0;   // G2 新替代组整套采购成本
};

struct GroupSubResult {
    double trad_new_cash_out = 0.0;
    double trad_scrap_loss = 0.0;
    double trad_total_economic_cost = 0.0;

    double ipc_new_cash_out = 0.0;
    double ipc_scrap_loss = 0.0;
    double ipc_total_economic_cost = 0.0;
    double net_economic_savings = 0.0;
    std::string decision_code;
};

GroupSubResult evaluate_group_substitution_sunk_cost(const GroupSubInput& in);

// =========================================================================
// 2B. 不完全替代（局部替代：单向属性门槛 + 主料锚定混用比例上限）
// =========================================================================
enum class IncompleteSubOp {
    GE = 0, // 高配向下兼容低配 (candidate >= required)
    EQ = 1  // 认证/国别严格匹配 (candidate == required)
};

struct IncompleteSubInput {
    double gross_demand = 1000.0;        // 父件总需求
    double main_kitted_qty = 500.0;      // 主料实际齐套量
    double sub_avail_qty = 500.0;        // 副料（替代料）可用量
    double max_sub_ratio = 0.30;         // 副料在总产出中的法定/工艺比例红线 (如 30%)
    IncompleteSubOp dim_op = IncompleteSubOp::GE;
    double candidate_dim = 5000.0;       // 副料属性值（如 5000mAh / 认证码 101）
    double required_dim = 4500.0;        // 目标需求属性值（如 4500mAh / 认证码 101）
};

struct IncompleteSubResult {
    bool dim_passed = false;
    // 现有 SCP / APS 机械按毛需求乘比例 (floor(theta * gross)) 的结果
    double legacy_sub_used = 0.0;
    double legacy_total_output = 0.0;
    double legacy_actual_ratio = 0.0;
    bool legacy_breached_cap = false;

    // IPC 按主料实际齐套量锚定: Q_sub <= floor((theta / (1 - theta)) * Q_main)
    double ipc_sub_used = 0.0;
    double ipc_total_output = 0.0;
    double ipc_actual_ratio = 0.0;
    bool ipc_within_cap = true;
    std::string decision_code;
};

IncompleteSubResult evaluate_incomplete_substitution_rule(const IncompleteSubInput& in);

// =========================================================================
// 3. 推拉解耦点 (CODP) 两层非补偿定位与二阶 Φ 影子收益模型 (3 层 6 节点)
// =========================================================================
struct CodpPositioningInput {
    int chip_proc_lt = 42;
    int bare_mfg_lt = 4;
    int pack_lt = 1;
    int customer_dtol = 3;
    int committed_sla = 1;
    int num_countries = 4;
    double country_daily_sigma = 40.0;
    double z_score = 2.0;
    double bare_unit_cost = 600.0;
    double fg_unit_cost = 800.0;
    bool legacy_early_engraving = false; // 若为 true，则旧工艺迫使裸机失去通用性
};

struct CodpPositioningResult {
    bool chip_passes_gate1 = false;
    bool bare_passes_gate1 = false;
    bool bare_passes_gate2 = false;

    double plan_a_fg_ss_units = 0.0;
    double plan_a_fg_capital = 0.0;

    double plan_b_bare_ss_units = 0.0;
    double plan_b_bare_capital = 0.0;

    double unit_reduction_pct = 0.0;
    double capital_reduction_pct = 0.0;
    double phi_shadow_value_capital = 0.0; // 二阶修宪 Φ 将 CODP 上移释放的常态资金
    std::string chosen_codp_node;
};

CodpPositioningResult evaluate_codp_two_layer_positioning(const CodpPositioningInput& in);

// =========================================================================
// 4. 零预测冲减：推式防独占日切割 + 拉式超额 Mode A/B/C 三态路由
// =========================================================================
struct PushSliceResult {
    double w1_alloc_a = 0.0;
    double w1_alloc_b = 0.0;
    double w1_rollover_a = 0.0;
    double w1_rollover_b = 0.0;
    std::vector<double> daily_a; // D1..D6
    std::vector<double> daily_b; // D1..D6
};

PushSliceResult evaluate_push_anti_monopoly_slice(
    double w1_demand_a,
    double w1_demand_b,
    double w1_supply_cap,
    const std::vector<double>& daily_caps
);

enum class PullOverflowMode {
    IN_BUCKET_SILENT = 0,
    MODE_A_ADVANCE_PULL_ZERO_MRP = 1,
    MODE_B_TRUE_INCREMENT_DIRECT_MRP = 2,
    MODE_C_URGENT_BORROW_AND_BACKFILL = 3
};

struct PullRouterResult {
    PullOverflowMode mode = PullOverflowMode::IN_BUCKET_SILENT;
    std::string mode_name;
    double overflow_qty = 0.0;
    double w2_quota_deducted = 0.0;
    double incremental_mrp_qty = 0.0;
};

PullRouterResult evaluate_pull_overflow_router(
    double w1_actual_order,
    double w1_alloc_bucket,
    double monthly_cum_orders,
    double monthly_envelope_cap,
    int days_to_due,
    int procure_lt_days
);

// =========================================================================
// 5. 跨解耦点两阶段推拉排产 (Two-Stage Constrained Push-IOP + Pull-IOP)
// =========================================================================
struct PullOrderSlice {
    std::string order_id;
    int req_due_day = 4;
    double req_qty = 0.0;
    double allotment_cap = 0.0;
    int priority = 1;
};

struct PullOrderPromise {
    std::string order_id;
    double req_qty = 0.0;
    double admitted_qty = 0.0;
    double quota_blocked_qty = 0.0;
    int legacy_promised_day = 0;   // 传统仅按后道静态 LT 虚报交期
    int ipc_bare_ready_day = 0;    // 绑定前道真实通用半成品齐套日
    int ipc_promised_day = 0;      // 诚实可兑现交期 = max(req_due_day, bare_ready + post_lt)
    double legacy_shortfall_on_due = 0.0;
};

struct TwoStageIopInput {
    double forecast_bare_qty = 1000.0;
    double daily_wc_cap = 300.0;       // 前道组装线日有限产能
    double chip_oh_day1 = 600.0;       // 关键主芯片 D1 在手现货
    int chip_po_day = 4;               // 关键主芯片在途到货日
    double chip_po_qty = 400.0;        // 关键主芯片在途到货量
    int post_pack_lt_days = 1;         // 后道国别贴标包装周期 L_post
    int horizon_days = 8;
    std::vector<PullOrderSlice> pull_orders;
};

struct TwoStageIopResult {
    std::vector<double> daily_bare_built; // D1..DH 真实受限产出的通用裸机量
    double total_bare_built = 0.0;
    int idle_wait_chip_days = 0;          // 因等在途芯片导致的产线空转天数
    double legacy_promised_on_d4 = 0.0;   // 传统静态后道 LT 在 D4 盲目承诺总量
    double legacy_broken_promise_qty = 0.0; // 传统做法在 D4 必然爽约断料缺口
    double ipc_broken_promise_qty = 0.0;    // IPC 诚实承诺爽约量 (严格为 0)
    double total_quota_blocked_qty = 0.0;
    std::vector<PullOrderPromise> order_promises;
};

TwoStageIopResult evaluate_two_stage_push_pull_iop(const TwoStageIopInput& in);

// =========================================================================
// 6. 价值链网络先序区间代数 + 维度规划与国别认证掩码 + 菱形 DAG 正逆向展开
// =========================================================================
enum class NetworkDimOp {
    PASS = 0, // 无规格过滤（直通）
    EQ = 1,   // 精确同规匹配 (cand == req)
    GE = 2,   // 高规向下兼容代低规 (cand >= req, 如 512MB >= 256MB)
    LE = 3    // 上限约束 (cand <= req)
};

struct NetworkNodeDef {
    int node_id = 0;
    std::string code;
    double spec_val = 0.0;       // 规格档位（如 512 / 256 / 128）
    unsigned int cert_mask = 0xFFFFFFFFu; // 国别/安规认证位掩码
};

struct NetworkEdgeDef {
    int parent_id = 0;
    int child_id = 0;
    double usage_qty = 1.0;
    NetworkDimOp dim_op = NetworkDimOp::PASS;
};

struct PreorderNodeSpan {
    int node_id = 0;
    int rank = 0;       // 先序进入序号 [1..N]
    int tree_size = 1;  // 子树覆盖节点数
    int level = 0;
    bool contains_rank(int target_rank) const {
        return target_rank >= rank && target_rank < (rank + tree_size);
    }
};

struct ExplodedRow {
    int node_id = 0;
    std::string code;
    int level = 0;
    double cum_qty = 0.0;
};

struct NetworkExplosionResult {
    std::vector<PreorderNodeSpan> spans;        // O(1) 先序区间索引表
    std::vector<ExplodedRow> path_instances;    // 全路径实例展开 (PATH_INSTANCE)
    std::vector<ExplodedRow> distinct_nodes;    // 菱形共用件去重守恒汇总 (DISTINCT_NODE)
    int pruned_by_dim_or_cert = 0;              // 被维度规格或国别认证剪枝拦截的边数
};

NetworkExplosionResult explode_value_chain_network(
    const std::vector<NetworkNodeDef>& nodes,
    const std::vector<NetworkEdgeDef>& edges,
    int root_node_id,
    double root_demand_qty,
    double required_spec,
    unsigned int required_cert_mask,
    NetworkDimOp override_leaf_op = NetworkDimOp::PASS
);

std::vector<int> trace_network_where_used(
    const std::vector<NetworkEdgeDef>& edges,
    int target_child_id
);

} // namespace ipc_core

