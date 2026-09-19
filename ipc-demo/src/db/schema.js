// DuckDB schema 定义与初始化。
//
// 分层：
//   1) 场景与基础数据集表（7 类输入数据集，均带 scenario_id）
//   2) 导入元数据（dataset_imports）
//   3) 运行元数据（engine_runs）与原始产物（run_artifacts）
//   4) 结果明细表（7 张，均带 run_id）
//
// 说明：每张表都用 IF NOT EXISTS 创建，可安全重复执行（幂等迁移）。

export const INPUT_DATASETS = [
  {
    key: 'parts',
    label: '物料主数据',
    file: 'parts.csv',
    category: 'master',
    columns: ['part_id', 'part_code', 'site', 'safety_stock', 'initial_on_hand', 'lot_size', 'lead_time'],
    ddl: `CREATE TABLE IF NOT EXISTS parts (
      scenario_id VARCHAR NOT NULL,
      part_id BIGINT,
      part_code VARCHAR,
      site VARCHAR,
      safety_stock DOUBLE,
      initial_on_hand DOUBLE,
      lot_size DOUBLE,
      lead_time BIGINT
    )`,
  },
  {
    key: 'bom',
    label: '产品结构',
    file: 'bom.csv',
    category: 'master',
    columns: ['parent_id', 'child_id', 'usage_qty', 'alt_class', 'alt_group', 'target_ratio', 'historical_qty', 'lot_size'],
    ddl: `CREATE TABLE IF NOT EXISTS bom (
      scenario_id VARCHAR NOT NULL,
      parent_id BIGINT,
      child_id BIGINT,
      usage_qty DOUBLE,
      alt_class BIGINT,
      alt_group BIGINT,
      target_ratio DOUBLE,
      historical_qty DOUBLE,
      lot_size DOUBLE
    )`,
  },
  {
    key: 'capacity',
    label: '产能/资源',
    file: 'capacity.csv',
    category: 'resource',
    columns: ['work_center', 'day', 'capacity_hours'],
    ddl: `CREATE TABLE IF NOT EXISTS capacity (
      scenario_id VARCHAR NOT NULL,
      work_center VARCHAR,
      day BIGINT,
      capacity_hours DOUBLE
    )`,
  },
  {
    key: 'atp_supply',
    label: 'ATP 供给',
    file: 'atp_supply.csv',
    category: 'supply',
    columns: ['supply_code', 'supply_type', 'part_id', 'available_day', 'qty', 'priority'],
    ddl: `CREATE TABLE IF NOT EXISTS atp_supply (
      scenario_id VARCHAR NOT NULL,
      supply_code VARCHAR,
      supply_type VARCHAR,
      part_id BIGINT,
      available_day BIGINT,
      qty DOUBLE,
      priority BIGINT
    )`,
  },
  {
    key: 'demands_master',
    label: '主计划需求',
    file: 'demands_master.csv',
    category: 'demand',
    columns: ['demand_id', 'part_id', 'due_day', 'qty', 'priority', 'customer_group', 'region'],
    ddl: `CREATE TABLE IF NOT EXISTS demands_master (
      scenario_id VARCHAR NOT NULL,
      demand_id BIGINT,
      part_id BIGINT,
      due_day BIGINT,
      qty DOUBLE,
      priority BIGINT,
      customer_group VARCHAR,
      region VARCHAR
    )`,
  },
  {
    key: 'demands_execution',
    label: '执行计划需求',
    file: 'demands_execution.csv',
    category: 'demand',
    columns: ['demand_id', 'part_id', 'due_day', 'qty', 'priority', 'customer_group', 'region'],
    ddl: `CREATE TABLE IF NOT EXISTS demands_execution (
      scenario_id VARCHAR NOT NULL,
      demand_id BIGINT,
      part_id BIGINT,
      due_day BIGINT,
      qty DOUBLE,
      priority BIGINT,
      customer_group VARCHAR,
      region VARCHAR
    )`,
  },
  {
    key: 'substitution_group',
    label: '替代料组',
    file: 'substitution_group.csv',
    category: 'master',
    columns: ['alt_group', 'alt_class', 'member_part_id', 'target_ratio', 'historical_qty'],
    ddl: `CREATE TABLE IF NOT EXISTS substitution_group (
      scenario_id VARCHAR NOT NULL,
      alt_group BIGINT,
      alt_class BIGINT,
      member_part_id BIGINT,
      target_ratio DOUBLE,
      historical_qty DOUBLE
    )`,
  },
];

// 结果明细表：均带 run_id，可 JOIN engine_runs 取元数据。
export const OUTPUT_DATASETS = [
  {
    key: 'delivery_result',
    label: '交付承诺结论',
    file: 'delivery_result.csv',
    engine: 'delivery',
    ddl: `CREATE TABLE IF NOT EXISTS delivery_result (
      run_id VARCHAR NOT NULL,
      part_id BIGINT, due_day BIGINT, qty DOUBLE, priority BIGINT,
      is_fulfillable BOOLEAN, promised_day BIGINT, promised_qty DOUBLE,
      total_capacity_used DOUBLE, rollback_steps_count BIGINT
    )`,
  },
  {
    key: 'delivery_steps',
    label: '交付试算步骤',
    file: 'delivery_steps.csv',
    engine: 'delivery',
    ddl: `CREATE TABLE IF NOT EXISTS delivery_steps (
      run_id VARCHAR NOT NULL,
      attempt_day BIGINT, success BOOLEAN, capacity_used DOUBLE,
      rollback_steps BIGINT, note VARCHAR
    )`,
  },
  {
    key: 'delivery_bom',
    label: '交付 BOM 展开',
    file: 'delivery_bom.csv',
    engine: 'delivery',
    ddl: `CREATE TABLE IF NOT EXISTS delivery_bom (
      run_id VARCHAR NOT NULL,
      level_parent_id BIGINT, level_child_id BIGINT, usage_qty DOUBLE,
      child_required_qty DOUBLE, alt_class BIGINT, alt_group BIGINT
    )`,
  },
  {
    key: 'itp_allotments',
    label: 'ITP 配额',
    file: 'itp_allotments.csv',
    engine: 'itp',
    ddl: `CREATE TABLE IF NOT EXISTS itp_allotments (
      run_id VARCHAR NOT NULL,
      day BIGINT, family_id BIGINT, cust_group_id BIGINT, region_id BIGINT,
      total_quota DOUBLE, consumed_qty DOUBLE
    )`,
  },
  {
    key: 'iop_orders',
    label: 'IOP 逐单',
    file: 'iop_orders.csv',
    engine: 'iop',
    ddl: `CREATE TABLE IF NOT EXISTS iop_orders (
      run_id VARCHAR NOT NULL,
      demand_id BIGINT, part_id BIGINT, due_day BIGINT, qty DOUBLE, priority BIGINT,
      family_id BIGINT, cust_group_id BIGINT, region_id BIGINT,
      status VARCHAR, reason VARCHAR
    )`,
  },
  {
    key: 'iop_summary',
    label: 'IOP 聚合',
    file: 'iop_summary.csv',
    engine: 'iop',
    ddl: `CREATE TABLE IF NOT EXISTS iop_summary (
      run_id VARCHAR NOT NULL,
      total_orders BIGINT, scheduled_orders BIGINT, blocked_orders BIGINT,
      total_fulfilled_qty DOUBLE, quota_utilization DOUBLE
    )`,
  },
  {
    key: 'substitution_allocations',
    label: '替代料分配',
    file: 'substitution_allocations.csv',
    engine: 'substitution',
    ddl: `CREATE TABLE IF NOT EXISTS substitution_allocations (
      run_id VARCHAR NOT NULL,
      day BIGINT, parent_part_id BIGINT, alt_part_id BIGINT,
      allocated_qty DOUBLE, day_allocated BIGINT, alt_class BIGINT
    )`,
  },
  {
    key: 'substitution_decisions',
    label: '替代料决策',
    file: 'substitution_decisions.csv',
    engine: 'substitution',
    ddl: `CREATE TABLE IF NOT EXISTS substitution_decisions (
      run_id VARCHAR NOT NULL,
      category VARCHAR, chosen_part_id BIGINT, basis VARCHAR,
      net_demand DOUBLE, remaining_on_hand_after DOUBLE
    )`,
  },
  {
    key: 'substitution_water',
    label: '替代料水位',
    file: 'substitution_water.csv',
    engine: 'substitution',
    ddl: `CREATE TABLE IF NOT EXISTS substitution_water (
      run_id VARCHAR NOT NULL,
      member_part_id BIGINT, target_ratio DOUBLE, historical_qty DOUBLE,
      on_hand_before DOUBLE, safety_stock DOUBLE,
      allocatable_before DOUBLE, on_hand_after DOUBLE
    )`,
  },
];

// 引擎 → 输出数据集映射
export const ENGINE_OUTPUTS = {
  delivery: ['delivery_result', 'delivery_steps', 'delivery_bom'],
  itp: ['itp_allotments'],
  iop: ['iop_orders', 'iop_summary'],
  substitution: ['substitution_allocations', 'substitution_decisions', 'substitution_water'],
};

// 引擎 → 所需输入数据集（用于场景完整性校验）
export const ENGINE_INPUTS = {
  delivery: ['parts', 'bom', 'atp_supply', 'capacity'],
  itp: ['parts', 'demands_master'],
  iop: ['parts', 'demands_master', 'demands_execution'],
  substitution: ['parts', 'bom', 'substitution_group'],
};

export const META_DDL = [
  `CREATE TABLE IF NOT EXISTS scenarios (
    scenario_id VARCHAR PRIMARY KEY,
    display_name VARCHAR,
    root_path VARCHAR,
    source VARCHAR,
    is_complete BOOLEAN,
    created_at TIMESTAMP
  )`,
  `CREATE TABLE IF NOT EXISTS dataset_imports (
    import_id VARCHAR NOT NULL,
    scenario_id VARCHAR NOT NULL,
    dataset_key VARCHAR NOT NULL,
    row_count BIGINT,
    source_path VARCHAR,
    imported_at TIMESTAMP,
    duration_ms BIGINT
  )`,
  `CREATE TABLE IF NOT EXISTS engine_runs (
    run_id VARCHAR PRIMARY KEY,
    scenario_id VARCHAR,
    engine_type VARCHAR,
    status VARCHAR,
    created_at TIMESTAMP,
    params_json VARCHAR,
    exit_code BIGINT,
    duration_ms BIGINT,
    engine_duration_ms BIGINT,
    error_message VARCHAR,
    executable_path VARCHAR,
    argv_json VARCHAR,
    in_dir VARCHAR,
    out_dir VARCHAR,
    dataset_source_id VARCHAR
  )`,
  `CREATE TABLE IF NOT EXISTS run_artifacts (
    run_id VARCHAR NOT NULL,
    direction VARCHAR NOT NULL,
    filename VARCHAR NOT NULL,
    content VARCHAR,
    archive_path VARCHAR,
    byte_size BIGINT
  )`,
];

// 所有 DDL：元数据表 + 基础数据集表 + 结果明细表
export function allDdl() {
  return [
    ...META_DDL,
    ...INPUT_DATASETS.map((d) => d.ddl),
    ...OUTPUT_DATASETS.map((d) => d.ddl),
  ];
}
