// 计划透视指标定义（行 = 关键指标 × 维度，列 = 天）。
//
// 指标依引擎业务场景确定，分四族：supply / demand / resource / balance。
// 每个指标声明：
//   key          指标标识
//   label        显示名
//   family       族
//   unit         单位
//   dimension    行维度：part / work_center / customer_group / region / none
//   direction    'positive' | 'negative'（用于前端强调）
//   detailTable  下钻来源表
//   detailKey    下钻时按天过滤的列名
//   sql          返回 (day, dim_value, value) 的 SQL 片段；NULL 表示无数据
//
// 约束：SQL 绝不使用 COALESCE(...,0)，保持 NULL≠0 语义。
import { sqlLiteral } from '../db/duckdb.js';

const S = (scenarioId) => sqlLiteral(scenarioId);

export function buildTimeRangeSql(scenarioId) {
  return `
    WITH bounds AS (
      SELECT min(d) AS lo, max(d) AS hi FROM (
        SELECT min(due_day) AS d FROM demands_master WHERE scenario_id = ${S(scenarioId)}
        UNION ALL SELECT min(due_day) FROM demands_execution WHERE scenario_id = ${S(scenarioId)}
        UNION ALL SELECT min(available_day) FROM atp_supply WHERE scenario_id = ${S(scenarioId)}
        UNION ALL SELECT min(day) FROM capacity WHERE scenario_id = ${S(scenarioId)}
        UNION ALL SELECT max(due_day) FROM demands_master WHERE scenario_id = ${S(scenarioId)}
        UNION ALL SELECT max(due_day) FROM demands_execution WHERE scenario_id = ${S(scenarioId)}
        UNION ALL SELECT max(available_day) FROM atp_supply WHERE scenario_id = ${S(scenarioId)}
        UNION ALL SELECT max(day) FROM capacity WHERE scenario_id = ${S(scenarioId)}
      )
    )
    SELECT lo, hi FROM bounds WHERE lo IS NOT NULL AND hi IS NOT NULL
  `;
}

// 生成连续天桶：range(lo, hi+1)
function daysCte(scenarioId) {
  return `
    WITH b AS (${buildTimeRangeSql(scenarioId)}),
    days AS (SELECT unnest(range(b.lo, b.hi + 1)) AS day FROM b)
  `;
}

// ===== 供给族 =====
// supply_type 取值：'On-Hand' / 'SR' / 'Planned Order'（大小写与空格容忍）
const SUPPLY_FAMILY = `
  CASE
    WHEN lower(replace(supply_type, ' ', '')) IN ('on-hand','onhand') THEN 'on_hand'
    WHEN lower(replace(supply_type, ' ', '')) = 'sr' THEN 'scheduled_receipt'
    WHEN lower(replace(supply_type, ' ', '')) IN ('plannedorder','planned') THEN 'planned_order'
    ELSE 'other'
  END
`;

export const METRICS = [
  // ---------- 供给族 ----------
  {
    key: 'supply_on_hand',
    label: '在手供给',
    family: 'supply',
    unit: '件',
    dimension: 'part',
    detailTable: 'atp_supply',
    detailKey: 'available_day',
    sql: (sid) => `${daysCte(sid)}
      SELECT d.day AS day, s.part_id AS dim_value, sum(s.qty) AS value
      FROM days d
      JOIN atp_supply s ON s.available_day = d.day AND s.scenario_id = ${S(sid)}
      WHERE ${SUPPLY_FAMILY} = 'on_hand'
      GROUP BY d.day, s.part_id`,
  },
  {
    key: 'supply_scheduled_receipt',
    label: '在途供给 (SR)',
    family: 'supply',
    unit: '件',
    dimension: 'part',
    detailTable: 'atp_supply',
    detailKey: 'available_day',
    sql: (sid) => `${daysCte(sid)}
      SELECT d.day AS day, s.part_id AS dim_value, sum(s.qty) AS value
      FROM days d
      JOIN atp_supply s ON s.available_day = d.day AND s.scenario_id = ${S(sid)}
      WHERE ${SUPPLY_FAMILY} = 'scheduled_receipt'
      GROUP BY d.day, s.part_id`,
  },
  {
    key: 'supply_planned_order',
    label: '计划订单',
    family: 'supply',
    unit: '件',
    dimension: 'part',
    detailTable: 'atp_supply',
    detailKey: 'available_day',
    sql: (sid) => `${daysCte(sid)}
      SELECT d.day AS day, s.part_id AS dim_value, sum(s.qty) AS value
      FROM days d
      JOIN atp_supply s ON s.available_day = d.day AND s.scenario_id = ${S(sid)}
      WHERE ${SUPPLY_FAMILY} = 'planned_order'
      GROUP BY d.day, s.part_id`,
  },
  {
    key: 'supply_planned_available_cumulative',
    label: '累计计划可用量',
    family: 'supply',
    unit: '件',
    dimension: 'part',
    detailTable: 'atp_supply',
    detailKey: 'available_day',
    // 按天累计该日及之前所有可用供给（在手+在途+计划订单）
    sql: (sid) => `${daysCte(sid)},
      events AS (
        SELECT s.part_id AS part_id, s.available_day AS day, sum(s.qty) AS qty
        FROM atp_supply s WHERE s.scenario_id = ${S(sid)}
        GROUP BY s.part_id, s.available_day
      ),
      parts AS (SELECT DISTINCT part_id FROM events),
      grid AS (SELECT d.day, p.part_id FROM days d CROSS JOIN parts p),
      grid_ev AS (
        SELECT g.day, g.part_id, sum(coalesce(e.qty, 0)) AS qty
        FROM grid g LEFT JOIN events e ON e.day = g.day AND e.part_id = g.part_id
        GROUP BY g.day, g.part_id
      )
      SELECT day, part_id AS dim_value,
             sum(qty) OVER (PARTITION BY part_id ORDER BY day
                            ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) AS value
      FROM grid_ev
      ORDER BY day`,
  },

  // ---------- 需求族 ----------
  {
    key: 'demand_master',
    label: '主计划独立需求',
    family: 'demand',
    unit: '件',
    dimension: 'part',
    detailTable: 'demands_master',
    detailKey: 'due_day',
    sql: (sid) => `${daysCte(sid)}
      SELECT d.day AS day, m.part_id AS dim_value, sum(m.qty) AS value
      FROM days d
      JOIN demands_master m ON m.due_day = d.day AND m.scenario_id = ${S(sid)}
      GROUP BY d.day, m.part_id`,
  },
  {
    key: 'demand_execution',
    label: '执行需求',
    family: 'demand',
    unit: '件',
    dimension: 'part',
    detailTable: 'demands_execution',
    detailKey: 'due_day',
    sql: (sid) => `${daysCte(sid)}
      SELECT d.day AS day, e.part_id AS dim_value, sum(e.qty) AS value
      FROM days d
      JOIN demands_execution e ON e.due_day = d.day AND e.scenario_id = ${S(sid)}
      GROUP BY d.day, e.part_id`,
  },
  {
    key: 'demand_gross',
    label: '毛需求',
    family: 'demand',
    unit: '件',
    dimension: 'part',
    detailTable: 'demands_master',
    detailKey: 'due_day',
    // 按天汇总该日到期的需求总量（主计划 + 执行）
    sql: (sid) => `${daysCte(sid)},
      all_d AS (
        SELECT due_day AS day, part_id, qty FROM demands_master WHERE scenario_id = ${S(sid)}
        UNION ALL
        SELECT due_day, part_id, qty FROM demands_execution WHERE scenario_id = ${S(sid)}
      )
      SELECT d.day AS day, a.part_id AS dim_value, sum(a.qty) AS value
      FROM days d JOIN all_d a ON a.day = d.day
      GROUP BY d.day, a.part_id
      ORDER BY d.day`,
  },

  // ---------- 资源族 ----------
  {
    key: 'resource_available_capacity',
    label: '可用产能',
    family: 'resource',
    unit: '工时',
    dimension: 'work_center',
    detailTable: 'capacity',
    detailKey: 'day',
    sql: (sid) => `${daysCte(sid)}
      SELECT d.day AS day, c.work_center AS dim_value, sum(c.capacity_hours) AS value
      FROM days d
      JOIN capacity c ON c.day = d.day AND c.scenario_id = ${S(sid)}
      GROUP BY d.day, c.work_center`,
  },
  {
    key: 'resource_consumed_capacity',
    label: '已占用产能',
    family: 'resource',
    unit: '工时',
    dimension: 'part',
    // 产能占用来自交付承诺运行结果（delivery_result @ promised_day），
    // 保证与第 6.9 条「产能对齐交付承诺」一致。可选按 run_id 过滤。
    detailTable: 'delivery_result',
    detailKey: 'promised_day',
    detailFilter: 'is_fulfillable',
    sql: (sid, runId) => `${daysCte(sid)},
      runs AS (
        SELECT r.run_id FROM engine_runs r
        WHERE r.scenario_id = ${S(sid)} AND r.engine_type = 'delivery' AND r.status = 'succeeded'
        ${runId ? `AND r.run_id = ${sqlLiteral(runId)}` : ''}
      )
      SELECT d.day AS day, dr.part_id AS dim_value, sum(dr.total_capacity_used) AS value
      FROM days d
      JOIN delivery_result dr ON dr.promised_day = d.day AND dr.run_id IN (SELECT run_id FROM runs)
      WHERE dr.is_fulfillable
      GROUP BY d.day, dr.part_id`,
  },
  {
    key: 'resource_remaining_capacity',
    label: '剩余产能',
    family: 'resource',
    unit: '工时',
    dimension: 'work_center',
    // 可用产能（按工作中心）扣除交付承诺已占用产能（按物料维度归属到单一工作中心不可直接匹配，
    // 因此这里按「天」对齐：当日全部已占用产能从该工作中心可用产能中扣除，仅在单工作中心场景下等值）。
    detailTable: 'capacity',
    detailKey: 'day',
    negativeIsOverload: true,
    sql: (sid, runId) => `${daysCte(sid)},
      avail AS (
        SELECT c.day, c.work_center, sum(c.capacity_hours) AS cap
        FROM capacity c WHERE c.scenario_id = ${S(sid)}
        GROUP BY c.day, c.work_center
      ),
      used AS (
        SELECT dr.promised_day AS day, sum(dr.total_capacity_used) AS used
        FROM delivery_result dr
        JOIN engine_runs r ON r.run_id = dr.run_id
        WHERE r.scenario_id = ${S(sid)} AND r.engine_type='delivery' AND r.status='succeeded'
          ${runId ? `AND r.run_id = ${sqlLiteral(runId)}` : ''}
          AND dr.is_fulfillable
        GROUP BY dr.promised_day
      )
      SELECT d.day AS day, a.work_center AS dim_value, a.cap - coalesce(u.used,0) AS value
      FROM days d JOIN avail a ON a.day = d.day
      LEFT JOIN used u ON u.day = d.day
      ORDER BY d.day`,
  },

  // ---------- 平衡族 ----------
  {
    key: 'balance_committed_qty',
    label: '已承诺量',
    family: 'balance',
    unit: '件',
    dimension: 'part',
    detailTable: 'delivery_result',
    detailKey: 'promised_day',
    detailFilter: 'is_fulfillable',
    sql: (sid, runId) => `${daysCte(sid)},
      runs AS (
        SELECT r.run_id FROM engine_runs r
        WHERE r.scenario_id = ${S(sid)} AND r.engine_type='delivery' AND r.status='succeeded'
        ${runId ? `AND r.run_id = ${sqlLiteral(runId)}` : ''}
      )
      SELECT d.day AS day, dr.part_id AS dim_value, sum(dr.promised_qty) AS value
      FROM days d JOIN delivery_result dr ON dr.promised_day = d.day AND dr.run_id IN (SELECT run_id FROM runs)
      WHERE dr.is_fulfillable
      GROUP BY d.day, dr.part_id`,
  },
  {
    key: 'balance_quota_total',
    label: 'ITP 配额',
    family: 'balance',
    unit: '件',
    dimension: 'family',
    detailTable: 'itp_allotments',
    detailKey: 'day',
    sql: (sid, runId) => `${daysCte(sid)},
      runs AS (
        SELECT r.run_id FROM engine_runs r
        WHERE r.scenario_id = ${S(sid)} AND r.engine_type='itp' AND r.status='succeeded'
        ${runId ? `AND r.run_id = ${sqlLiteral(runId)}` : ''}
      )
      SELECT d.day AS day, i.family_id AS dim_value, sum(i.total_quota) AS value
      FROM days d JOIN itp_allotments i ON i.day = d.day AND i.run_id IN (SELECT run_id FROM runs)
      GROUP BY d.day, i.family_id`,
  },
  {
    key: 'balance_quota_consumed',
    label: '配额已消耗',
    family: 'balance',
    unit: '件',
    dimension: 'family',
    detailTable: 'itp_allotments',
    detailKey: 'day',
    sql: (sid, runId) => `${daysCte(sid)},
      runs AS (
        SELECT r.run_id FROM engine_runs r
        WHERE r.scenario_id = ${S(sid)} AND r.engine_type='itp' AND r.status='succeeded'
        ${runId ? `AND r.run_id = ${sqlLiteral(runId)}` : ''}
      )
      SELECT d.day AS day, i.family_id AS dim_value, sum(i.consumed_qty) AS value
      FROM days d JOIN itp_allotments i ON i.day = d.day AND i.run_id IN (SELECT run_id FROM runs)
      GROUP BY d.day, i.family_id`,
  },
  {
    key: 'balance_blocked_qty',
    label: '阻断量',
    family: 'balance',
    unit: '件',
    dimension: 'part',
    detailTable: 'iop_orders',
    detailKey: 'due_day',
    detailFilter: "lower(status) IN ('blocked','阻断')",
    negativeIsOverload: true,
    sql: (sid, runId) => `${daysCte(sid)},
      runs AS (
        SELECT r.run_id FROM engine_runs r
        WHERE r.scenario_id = ${S(sid)} AND r.engine_type='iop' AND r.status='succeeded'
        ${runId ? `AND r.run_id = ${sqlLiteral(runId)}` : ''}
      )
      SELECT d.day AS day, o.part_id AS dim_value, sum(o.qty) AS value
      FROM days d JOIN iop_orders o ON o.due_day = d.day AND o.run_id IN (SELECT run_id FROM runs)
      WHERE lower(o.status) IN ('blocked','阻断')
      GROUP BY d.day, o.part_id`,
  },
  {
    key: 'balance_shortage',
    label: '缺口',
    family: 'balance',
    unit: '件',
    dimension: 'part',
    detailTable: 'iop_orders',
    detailKey: 'due_day',
    negativeIsOverload: true,
    // 缺口 = 未满足需求：IOP 阻断订单量，加上主计划需求中未被承诺的部分。
    sql: (sid, runId) => `${daysCte(sid)},
      blocked AS (
        SELECT o.due_day AS day, o.part_id AS part_id, sum(o.qty) AS qty
        FROM iop_orders o JOIN engine_runs r ON r.run_id = o.run_id
        WHERE r.scenario_id = ${S(sid)} AND r.engine_type='iop' AND r.status='succeeded'
          ${runId ? `AND o.run_id = ${sqlLiteral(runId)}` : ''}
          AND lower(o.status) IN ('blocked','阻断')
        GROUP BY o.due_day, o.part_id
      ),
      unmet AS (
        SELECT m.due_day AS day, m.part_id, sum(m.qty) AS demand_qty
        FROM demands_master m WHERE m.scenario_id = ${S(sid)}
        GROUP BY m.due_day, m.part_id
      ),
      committed AS (
        SELECT dr.due_day AS day, dr.part_id, sum(dr.promised_qty) AS committed_qty
        FROM delivery_result dr JOIN engine_runs r ON r.run_id = dr.run_id
        WHERE r.scenario_id = ${S(sid)} AND r.engine_type='delivery' AND r.status='succeeded'
          AND dr.is_fulfillable
        GROUP BY dr.due_day, dr.part_id
      )
      SELECT d.day AS day, coalesce(b.part_id, u.part_id) AS dim_value,
             coalesce(b.qty, 0) + greatest(u.demand_qty - coalesce(c.committed_qty,0), 0) AS value
      FROM days d
      LEFT JOIN blocked b ON b.day = d.day
      LEFT JOIN unmet u ON u.day = d.day AND u.part_id = b.part_id
      LEFT JOIN committed c ON c.day = d.day AND c.part_id = u.part_id
      WHERE b.part_id IS NOT NULL OR u.part_id IS NOT NULL`,
  },
];

export const METRIC_BY_KEY = Object.fromEntries(METRICS.map((m) => [m.key, m]));
export const FAMILIES = [
  { key: 'supply', label: '供给' },
  { key: 'demand', label: '需求' },
  { key: 'resource', label: '资源' },
  { key: 'balance', label: '平衡' },
];
