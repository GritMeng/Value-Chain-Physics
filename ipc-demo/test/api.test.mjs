import { test, before, after } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { tempConfig, startTestServer } from './helpers.mjs';

let ctx;
before(async () => {
  const { cfg } = tempConfig();
  ctx = await startTestServer(cfg);
});
after(async () => { await ctx.close(); });

test('GET /api/health 分别标明 DuckDB/场景/引擎', async () => {
  const { status, body } = await ctx.json('GET', '/api/health');
  assert.equal(status, 200);
  assert.equal(body.checks.duckdb.ok, true);
  assert.equal(body.checks.engine.ok, true);
  assert.ok(body.checks.scenarios.count >= 1);
});

test('GET /api/scenarios 返回场景列表与概览', async () => {
  const { body } = await ctx.json('GET', '/api/scenarios');
  const ids = body.scenarios.map((s) => s.scenarioId);
  assert.ok(ids.includes('sample'));
  assert.ok(ids.includes('delivery-benchmark'));
  assert.ok(ids.includes('itp-iop-benchmark'));
});

test('交付承诺端点返回结论并在非法输入时不启动子进程', async () => {
  const ok = await ctx.json('POST', '/api/runs/delivery', { scenarioId: 'delivery-benchmark', due_day: 3, qty: 30 });
  assert.equal(ok.status, 201);
  assert.equal(ok.body.conclusion.promisedDay, 3);
  assert.equal(ok.body.conclusion.totalCapacityUsed, 3);
  const bad = await ctx.json('POST', '/api/runs/delivery', { scenarioId: 'delivery-benchmark', qty: -1 });
  assert.equal(bad.status, 400);
  assert.equal(bad.body.error.kind, 'invalid_request');
});

test('ITP/IOP 端点：阻断订单含约束键与原因', async () => {
  const itp = await ctx.json('POST', '/api/runs/itp', { scenarioId: 'itp-iop-benchmark' });
  assert.equal(itp.status, 201);
  const iop = await ctx.json('POST', '/api/runs/iop', { scenarioId: 'itp-iop-benchmark' });
  assert.equal(iop.body.conclusion.blockedOrders, 1);
  const orders = await ctx.json('GET', `/api/runs/${iop.body.runId}/outputs/iop_orders`);
  const blocked = orders.body.rows.find((o) => String(o.status).toLowerCase() === 'blocked');
  assert.ok(blocked);
  assert.ok(blocked.family_id !== null && blocked.cust_group_id !== null && blocked.region_id !== null);
  assert.ok(blocked.reason);
});

test('替代料端点：不可分配仍为业务结论（201）', async () => {
  const r = await ctx.json('POST', '/api/runs/substitution',
    { scenarioId: 'substitution-benchmark', alt_class: 1, alt_group: 1, net_demand: 100000 });
  assert.equal(r.status, 201);
  assert.equal(r.body.conclusion.allocationCount, 0);
  assert.equal(r.body.conclusion.chosenPartId, 1);
});

test('引擎失败与业务结论可区分（数据错误 -> 5xx）', async () => {
  const r = await ctx.json('POST', '/api/runs/substitution',
    { scenarioId: 'substitution-benchmark', alt_class: 1, alt_group: 999, net_demand: 10 });
  assert.equal(r.status, 502);
  assert.match(r.body.error.message, /退出码 4/);
});

test('透视端点与单元格下钻一致', async () => {
  const iop = await ctx.json('POST', '/api/runs/iop', { scenarioId: 'itp-iop-benchmark' });
  const p = await ctx.json('GET', `/api/scenarios/itp-iop-benchmark/pivot?runId=${iop.body.runId}`);
  assert.ok(p.body.days.length >= 8);
  const cell = await ctx.json('GET',
    `/api/scenarios/itp-iop-benchmark/pivot/cell?metric=balance_blocked_qty&day=5&dimValue=10&runId=${iop.body.runId}`);
  assert.equal(cell.body.value, 300);
});

test('运行历史与详情端点可按 run_id 无重算取回', async () => {
  const run = await ctx.json('POST', '/api/runs/delivery', { scenarioId: 'delivery-benchmark' });
  const detail = await ctx.json('GET', `/api/runs/${run.body.runId}`);
  assert.equal(detail.body.status, 'succeeded');
  assert.equal(detail.body.outputs.length, 3);
  assert.ok(detail.body.artifacts.length > 0);
  const nf = await ctx.json('GET', '/api/runs/does-not-exist');
  assert.equal(nf.status, 404);
});

test('场景新建冲突与数据集提供端点', async () => {
  const c = await ctx.json('POST', '/api/scenarios', { scenarioId: 'api-user-scenario' });
  assert.equal(c.status, 201);
  const dup = await ctx.json('POST', '/api/scenarios', { scenarioId: 'api-user-scenario' });
  assert.equal(dup.status, 409);
  const res = await fetch(`${ctx.base}/api/scenarios/api-user-scenario/datasets/parts`, {
    method: 'PUT', headers: { 'content-type': 'text/csv' },
    body: 'part_id,part_code,site,safety_stock,initial_on_hand,lot_size,lead_time\n9,P9,S9,0,9,1,1\n',
  });
  assert.equal(res.status, 200);
});

test('原始 CSV 内容与磁盘文件逐字一致', async () => {
  const det = await ctx.json('GET', '/api/scenarios/delivery-benchmark');
  const raw = await ctx.json('GET', '/api/scenarios/delivery-benchmark/raw/parts.csv');
  const disk = fs.readFileSync(path.join(det.body.rootPath, 'parts.csv'), 'utf8');
  assert.equal(raw.body.content, disk);
});
