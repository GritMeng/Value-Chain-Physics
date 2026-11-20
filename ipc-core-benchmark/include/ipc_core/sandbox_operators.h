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

} // namespace ipc_core

