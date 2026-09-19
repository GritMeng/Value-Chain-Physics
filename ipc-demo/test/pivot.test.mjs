import { test } from 'node:test';
import assert from 'node:assert/strict';
import { tempConfig } from './helpers.mjs';
import { ensureDirs } from '../src/config.js';
import { openDb, closeDb, query } from '../src/db/duckdb.js';
import { importScenarioInputs } from '../src/db/importer.js';
import { RunService } from '../src/engine/runService.js';
import { getScenario } from '../src/scenario/manager.js';
import { timeRange, pivot, drillDown } from '../src/pivot/pivot.js';

async function boot(id) {
  const { cfg } = tempConfig();
  ensureDirs(cfg);
  await openDb(cfg);
  await importScenarioInputs(id, getScenario(cfg, id).rootPath);
  return { cfg, svc: new RunService(cfg) };
}

test('时间范围覆盖需求/供给/产能并集区间且连续', async () => {
  const { cfg, svc } = await boot('delivery-benchmark');
  try {
    const tr = await timeRange('delivery-benchmark');
    assert.equal(tr.lo, 0);
    assert.equal(tr.hi, 15);
    for (let i = 1; i < tr.days.length; i++) assert.equal(tr.days[i], tr.days[i - 1] + 1);
  } finally { await closeDb(); }
});

test('产能占用与交付承诺一致，剩余产能超载为负', async () => {
  const { cfg, svc } = await boot('delivery-benchmark');
  try {
    const run = await svc.run({ scenarioId: 'delivery-benchmark', engineType: 'delivery' });
    const p = await pivot('delivery-benchmark', { runId: run.runId });
    const i3 = p.days.indexOf(3);
    const consumed = p.rows.find((r) => r.metric === 'resource_consumed_capacity');
    assert.equal(consumed.values[i3], 3);
    const remaining = p.rows.find((r) => r.metric === 'resource_remaining_capacity');
    assert.equal(remaining.values[i3], 97);
  } finally { await closeDb(); }
});

test('配额/阻断与 ITP/IOP 结果一致', async () => {
  const { cfg, svc } = await boot('itp-iop-benchmark');
  try {
    const itp = await svc.run({ scenarioId: 'itp-iop-benchmark', engineType: 'itp' });
    const iop = await svc.run({ scenarioId: 'itp-iop-benchmark', engineType: 'iop' });
    const p = await pivot('itp-iop-benchmark', {});
    const sumMetric = (k) => p.rows.filter((r) => r.metric === k)
      .reduce((a, r) => a + r.values.reduce((s, v) => s + (v || 0), 0), 0);
    const quota = await query(`SELECT sum(total_quota) tq FROM itp_allotments WHERE run_id='${itp.runId}'`);
    const blocked = await query(`SELECT sum(qty) q FROM iop_orders WHERE run_id='${iop.runId}' AND lower(status)='blocked'`);
    assert.equal(sumMetric('balance_quota_total'), Number(quota[0].tq));
    assert.equal(sumMetric('balance_blocked_qty'), Number(blocked[0].q));
    assert.equal(sumMetric('balance_blocked_qty'), 300);
  } finally { await closeDb(); }
});

test('空值语义：无计划日为 NULL 而非 0', async () => {
  const { cfg, svc } = await boot('itp-iop-benchmark');
  try {
    const p = await pivot('itp-iop-benchmark', {});
    const dm = p.rows.find((r) => r.metric === 'demand_execution' && r.dimValue === 10);
    assert.ok(dm.values.some((v) => v === null), '应有空值单元格');
    assert.equal(dm.values.filter((v) => v === 0).length, 0, '无计划日不应为 0');
  } finally { await closeDb(); }
});

test('下钻明细求和等于单元格值', async () => {
  const { cfg, svc } = await boot('itp-iop-benchmark');
  try {
    const iop = await svc.run({ scenarioId: 'itp-iop-benchmark', engineType: 'iop' });
    const p = await pivot('itp-iop-benchmark', {});
    const cell = p.rows.find((r) => r.metric === 'balance_blocked_qty' && r.dimValue === 10);
    const day = p.days[cell.values.findIndex((v) => v !== null)];
    const dd = await drillDown('itp-iop-benchmark', { metric: 'balance_blocked_qty', day, dimValue: 10, runId: iop.runId });
    assert.equal(dd.value, cell.values[p.days.indexOf(day)]);
  } finally { await closeDb(); }
});

test('字符串维度（work_center）下钻与聚合单元格一致', async () => {
  const { cfg, svc } = await boot('itp-iop-benchmark');
  try {
    await svc.run({ scenarioId: 'itp-iop-benchmark', engineType: 'iop' });
    const p = await pivot('itp-iop-benchmark', {});
    // 资源族按 work_center 字符串维度展开；回归：曾因 Number('WC_01')->NaN 触发 DOUBLE 转换错误
    const cell = p.rows.find((r) => r.metric === 'resource_available_capacity' && typeof r.dimValue === 'string');
    assert.ok(cell, '应存在字符串维度的资源行');
    const idx = cell.values.findIndex((v) => v !== null);
    assert.ok(idx >= 0, '资源行应有可用值');
    const day = p.days[idx];
    const dd = await drillDown('itp-iop-benchmark', { metric: 'resource_available_capacity', day, dimValue: cell.dimValue });
    assert.equal(dd.value, cell.values[idx]);
    assert.ok(dd.count >= 1);
  } finally { await closeDb(); }
});

test('维度筛选仅返回匹配行', async () => {
  const { cfg, svc } = await boot('itp-iop-benchmark');
  try {
    await svc.run({ scenarioId: 'itp-iop-benchmark', engineType: 'iop' });
    const p = await pivot('itp-iop-benchmark', { filters: { dim_value: 10, dimension: 'part' } });
    assert.ok(p.rows.length > 0);
    assert.ok(p.rows.every((r) => r.dimValue === 10));
  } finally { await closeDb(); }
});
