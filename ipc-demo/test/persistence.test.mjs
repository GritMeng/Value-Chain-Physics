import { test } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { tempConfig } from './helpers.mjs';
import { ensureDirs } from '../src/config.js';
import { openDb, closeDb, query, queryParams } from '../src/db/duckdb.js';
import { importDataset, importScenarioInputs } from '../src/db/importer.js';
import { INPUT_DATASETS, OUTPUT_DATASETS, META_DDL, allDdl } from '../src/db/schema.js';
import { datasetRowCount, inputDataOverview, listRuns, getRun } from '../src/db/queries.js';
import { RunService } from '../src/engine/runService.js';
import { getScenario } from '../src/scenario/manager.js';

async function setup() {
  const { cfg } = tempConfig();
  ensureDirs(cfg);
  await openDb(cfg);
  return cfg;
}

test('空库启动后表清单与设计一致（20 张表）', async () => {
  const cfg = await setup();
  try {
    const rows = await query(`SELECT table_name FROM information_schema.tables WHERE table_schema='main' ORDER BY table_name`);
    const names = rows.map((r) => r.table_name);
    // 4 元数据 + 7 输入 + 9 输出
    assert.equal(META_DDL.length, 4);
    assert.equal(INPUT_DATASETS.length, 7);
    assert.equal(OUTPUT_DATASETS.length, 9);
    assert.equal(allDdl().length, 20);
    assert.equal(names.length, 20);
  } finally { await closeDb(); }
});

test('导入行数与源 CSV 一致且每行带 scenario_id；重复导入幂等', async () => {
  const cfg = await setup();
  try {
    const dir = getScenario(cfg, 'delivery-benchmark').rootPath;
    const first = await importScenarioInputs('delivery-benchmark', dir);
    const countAfter1 = await datasetRowCount('delivery-benchmark', 'capacity');
    const second = await importScenarioInputs('delivery-benchmark', dir);
    const countAfter2 = await datasetRowCount('delivery-benchmark', 'capacity');
    assert.equal(countAfter1, 16);
    assert.equal(countAfter2, 16, '重复导入不应产生重复行');
    const nulls = await query(`SELECT count(*) c FROM capacity WHERE scenario_id IS NULL`);
    assert.equal(Number(nulls[0].c), 0);
  } finally { await closeDb(); }
});

test('多场景共存且按场景筛选独立一致', async () => {
  const cfg = await setup();
  try {
    for (const id of ['delivery-benchmark', 'itp-iop-benchmark']) {
      await importScenarioInputs(id, getScenario(cfg, id).rootPath);
    }
    const a = await datasetRowCount('delivery-benchmark', 'capacity');
    const b = await datasetRowCount('itp-iop-benchmark', 'capacity');
    assert.equal(a, 16);
    assert.equal(b, 8);
  } finally { await closeDb(); }
});

test('dataset_imports 记录类别/行数/源路径/时间', async () => {
  const cfg = await setup();
  try {
    await importScenarioInputs('delivery-benchmark', getScenario(cfg, 'delivery-benchmark').rootPath);
    const rows = await queryParams(
      `SELECT dataset_key, row_count, source_path, imported_at, duration_ms
       FROM dataset_imports WHERE scenario_id = ? ORDER BY imported_at DESC LIMIT 1`, ['delivery-benchmark']);
    assert.equal(rows.length, 1);
    assert.ok(rows[0].source_path.endsWith('.csv'));
    assert.ok(rows[0].imported_at);
  } finally { await closeDb(); }
});

test('运行元数据与结果明细写入，失败运行也被记录', async () => {
  const cfg = await setup();
  try {
    const dir = getScenario(cfg, 'delivery-benchmark').rootPath;
    await importScenarioInputs('delivery-benchmark', dir);
    const svc = new RunService(cfg);
    const ok = await svc.run({ scenarioId: 'delivery-benchmark', engineType: 'delivery' });
    const run = await getRun(ok.runId);
    assert.equal(run.status, 'succeeded');
    assert.equal(run.scenario_id, 'delivery-benchmark');
    assert.ok(run.artifacts.length > 0);
    // 失败运行：替代料组不存在 -> 非零退出
    await assert.rejects(svc.run({ scenarioId: 'substitution-benchmark', engineType: 'substitution', overrides: { alt_group: 999 } }));
    const failed = await query(`SELECT count(*) c FROM engine_runs WHERE status <> 'succeeded'`);
    assert.ok(Number(failed[0].c) >= 1);
  } finally { await closeDb(); }
});

test('未知 run_id 返回明确未找到错误', async () => {
  const cfg = await setup();
  try {
    await assert.rejects(getRun('no-such-run'), (e) => e.status === 404);
  } finally { await closeDb(); }
});
