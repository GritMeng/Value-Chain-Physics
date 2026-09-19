// 计划透视查询：把「指标 × 天」规整为行结构，并支持筛选与下钻。
//
// 关键语义：
//   - 时间桶覆盖场景数据实际范围（需求 due_day / 供给 available_day / 产能 day 的并集）
//   - 空值 NULL 原样保留，前端可区分「无计划(空)」与「计划为零(0)」
//   - 指标值来自持久化的输入/运行结果，不在前端推算
import { query, queryParams, sqlLiteral } from '../db/duckdb.js';
import { METRICS, METRIC_BY_KEY, FAMILIES, buildTimeRangeSql } from './metrics.js';

// 6.1 时间范围推导：返回 { lo, hi, days: [...] }
export async function timeRange(scenarioId) {
  const rows = await query(buildTimeRangeSql(scenarioId));
  if (!rows.length) return { lo: null, hi: null, days: [] };
  const lo = Number(rows[0].lo);
  const hi = Number(rows[0].hi);
  const days = [];
  for (let d = lo; d <= hi; d++) days.push(d);
  return { lo, hi, days };
}

// 维度筛选：part / work_center / customer_group / region / family（按值）
// 仅当筛选维度与指标自身维度一致时才应用，避免把工作中心（字符串）与物料 ID（数值）比较。
function filterClause(metric, filters) {
  const clauses = [];
  if (!filters) return clauses;
  const v = filters.dim_value;
  if (v === undefined || v === null || v === '') return clauses;
  const dim = filters.dimension || metric.dimension;
  if (dim && metric.dimension !== dim) return clauses;
  if (metric.dimension === 'work_center') {
    clauses.push(`CAST(dim_value AS VARCHAR) = ${sqlLiteral(String(v))}`);
  } else {
    clauses.push(`dim_value = ${sqlLiteral(Number(v))}`);
  }
  return clauses;
}

// 6.6 透视查询：返回 { days, families, rows }
// filters: { dim_value }（物料/工作中心 ID 通用维度值过滤）
export async function pivot(scenarioId, { metrics = null, runId = null, filters = {} } = {}) {
  const range = await timeRange(scenarioId);
  let selected = metrics && metrics.length
    ? METRICS.filter((m) => metrics.includes(m.key))
    : METRICS;
  // 按维度筛选时，仅保留该维度的指标行（其他维度的行不属于匹配范围）。
  if (filters.dimension) {
    selected = selected.filter((m) => m.dimension === filters.dimension);
  }

  const rows = [];
  for (const metric of selected) {
    const cells = new Map();
    try {
      const sql = `SELECT * FROM (${metric.sql(scenarioId, runId)}) t ${filterClause(metric, filters).map((c) => `WHERE ${c}`).join(' ')}`;
      const recs = await query(sql);
      for (const r of recs) {
        const key = `${r.dim_value}`;
        if (!cells.has(key)) cells.set(key, { dimValue: r.dim_value, values: {} });
        const cur = cells.get(key);
        const v = r.value === null || r.value === undefined ? null : Number(r.value);
        cur.values[Number(r.day)] = (cur.values[Number(r.day)] ?? 0) + (v ?? 0);
        // 显式记录 NULL：如果所有构成记录都是 NULL，则保持 null
        if (cur.values[Number(r.day)] === 0 && (v === null)) cur.values[Number(r.day)] = null;
      }
    } catch (err) {
      // 单个指标在无数据/无运行时不阻断整表
      cells.set('__error__', { dimValue: null, error: err.message, values: {} });
    }

    // 如果没有任何维度行，仍然输出一行（dimValue=null），所有单元格为空；
    // 但当请求带维度筛选时，空结果不应作为行返回（用户已缩小到某个成员）。
    if (cells.size === 0 && !filters.dim_value) {
      cells.set('__none__', { dimValue: null, values: {} });
    }

    for (const { dimValue, values, error } of cells.values()) {
      if (dimValue === null && !error && cells.size > 1) continue;
      const cellsOut = range.days.map((d) => (d in values ? values[d] : null));
      rows.push({
        metric: metric.key,
        label: metric.label,
        family: metric.family,
        unit: metric.unit,
        dimension: metric.dimension,
        negativeIsOverload: !!metric.negativeIsOverload,
        dimValue,
        error: error || null,
        values: cellsOut,
        hasData: cellsOut.some((v) => v !== null),
      });
    }
  }

  return {
    scenarioId,
    runId,
    days: range.days,
    lo: range.lo,
    hi: range.hi,
    families: FAMILIES,
    metrics: selected.map((m) => ({
      key: m.key, label: m.label, family: m.family, unit: m.unit,
      dimension: m.dimension, negativeIsOverload: !!m.negativeIsOverload,
    })),
    rows,
  };
}

// 6.7 单元格下钻：返回构成该值的底层记录
export async function drillDown(scenarioId, { metric: metricKey, day, dimValue = null, runId = null } = {}) {
  const metric = METRIC_BY_KEY[metricKey];
  if (!metric) {
    const err = new Error(`未知指标: ${metricKey}`);
    err.status = 400;
    throw err;
  }
  if (day === undefined || day === null || Number.isNaN(Number(day))) {
    const err = new Error('缺少 day 参数');
    err.status = 400;
    throw err;
  }
  const d = Number(day);
  const table = metric.detailTable;
  const keyCol = metric.detailKey;

  // 下钻取该指标定义所对应的原始明细；运行类指标需带 run 过滤。
  const runScoped = ['delivery_result', 'itp_allotments', 'iop_orders'].includes(table);
  const params = [scenarioId, d];
  let sql;
  if (runScoped) {
    sql = `SELECT t.* FROM ${table} t
           JOIN engine_runs r ON r.run_id = t.run_id
           WHERE r.scenario_id = ? AND t.${keyCol} = ?`;
    if (runId) { sql += ` AND t.run_id = ?`; params.push(runId); }
  } else {
    sql = `SELECT * FROM ${table} WHERE scenario_id = ? AND ${keyCol} = ?`;
  }
  if (metric.detailFilter) sql += ` AND ${metric.detailFilter.replace(/\b(status|is_fulfillable)\b/g, 't.$1')}`;
  if (dimValue !== null && dimValue !== undefined && dimValue !== '') {
    const dimCol = dimColumn(metric, table);
    if (dimCol) {
      // 统一按字符串比较：字符串维度（work_center）不再触发 'WC_01' -> DOUBLE 的转换错误，
      // 数值维度（part_id / family_id）用 '10' 也能正确匹配，保持与聚合单元格口径一致。
      sql += ` AND CAST(${dimCol} AS VARCHAR) = ?`;
      params.push(String(dimValue));
    }
  }
  const rows = await queryParams(sql, params);

  // 值 = 明细求和（与聚合单元格口径一致）
  const value = rows.length ? rows.reduce((a, r) => a + Number(valueOfRow(metric, r)), 0) : null;
  return { metric: metricKey, day: d, dimValue, value, count: rows.length, rows };
}

function dimColumn(metric, table) {
  if (table === 'atp_supply' || table === 'demands_master' || table === 'demands_execution' || table === 'delivery_result') return 'part_id';
  if (table === 'capacity') return 'work_center';
  if (table === 'itp_allotments') return 'family_id';
  if (table === 'iop_orders') return 'part_id';
  return null;
}

function valueOfRow(metric, row) {
  switch (metric.key) {
    case 'supply_on_hand':
    case 'supply_scheduled_receipt':
    case 'supply_planned_order':
    case 'supply_planned_available_cumulative':
      return row.qty;
    case 'demand_master':
    case 'demand_execution':
    case 'demand_gross':
    case 'balance_blocked_qty':
      return row.qty;
    case 'resource_available_capacity': return row.capacity_hours;
    case 'resource_consumed_capacity': return row.total_capacity_used;
    case 'resource_remaining_capacity': return row.capacity_hours;
    case 'balance_committed_qty': return row.promised_qty;
    case 'balance_quota_total': return row.total_quota;
    case 'balance_quota_consumed': return row.consumed_qty;
    case 'balance_shortage': return row.qty;
    default: return 0;
  }
}
