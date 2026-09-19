// REST API 路由定义。
import fs from 'node:fs';
import path from 'node:path';
import { buildRouter } from '../server/http.js';
import { getScenario, discoverScenarios, createScenario, putDataset } from '../scenario/manager.js';
import { importScenarioInputs } from '../db/importer.js';
import { inputDataOverview, inputRows, listRuns, getRun, datasetByKey } from '../db/queries.js';
import { RunService, ENGINE_TYPES } from '../engine/runService.js';
import { pivot, drillDown, timeRange } from '../pivot/pivot.js';
import { query, queryParams, queryOne, sqlLiteral } from '../db/duckdb.js';
import { OUTPUT_DATASETS } from '../db/schema.js';

function httpError(status, message, kind, extra = {}) {
  const e = new Error(message);
  e.status = status;
  e.kind = kind;
  Object.assign(e, extra);
  return e;
}

// 8.1 健康检查：分别标明 DuckDB、场景、C++ 可执行文件
export async function health(cfg) {
  let dbOk = false;
  let dbError = null;
  let scenariosOk = false;
  let scenarioCount = 0;
  try {
    await query('SELECT 1');
    dbOk = true;
  } catch (e) { dbError = e.message; }
  try {
    scenarioCount = discoverScenarios(cfg).length;
    scenariosOk = scenarioCount > 0;
  } catch { scenariosOk = false; }
  const engineExists = fs.existsSync(cfg.engineBin);
  let engineExecutable = false;
  try { if (engineExists) { fs.accessSync(cfg.engineBin, fs.constants.X_OK); engineExecutable = true; } } catch { /* */ }
  return {
    status: dbOk && engineExecutable ? 'ok' : 'degraded',
    checks: {
      duckdb: { ok: dbOk, path: cfg.duckdbPath, error: dbError },
      scenarios: { ok: scenariosOk, count: scenarioCount, root: cfg.scenarioRoot },
      engine: {
        ok: engineExecutable, exists: engineExists, path: cfg.engineBin,
        hint: engineExecutable ? null : '运行 ipc-core-benchmark/build_and_run.sh 构建 CLI',
      },
    },
  };
}

// 校验一次运行的请求参数
function validateRunRequest(engineType, body) {
  if (!body || typeof body !== 'object') throw httpError(400, '请求体必须为 JSON 对象', 'invalid_request');
  const scenarioId = body.scenarioId ?? body.scenario_id;
  if (!scenarioId) throw httpError(400, '缺少 scenarioId', 'invalid_request');
  if (body.qty !== undefined && (typeof body.qty !== 'number' || body.qty < 0)) {
    throw httpError(400, 'qty 必须为非负数值', 'invalid_request');
  }
  if (body.net_demand !== undefined && (typeof body.net_demand !== 'number' || body.net_demand < 0)) {
    throw httpError(400, 'net_demand 必须为非负数值', 'invalid_request');
  }
  if (body.due_day !== undefined && !Number.isFinite(Number(body.due_day))) {
    throw httpError(400, 'due_day 必须为数值', 'invalid_request');
  }
  if (body.alt_class !== undefined && ![1, 2, 3].includes(Number(body.alt_class))) {
    throw httpError(400, 'alt_class 必须为 1、2 或 3', 'invalid_request');
  }
  return { scenarioId: String(scenarioId), overrides: body };
}

// 结论摘要：从结果表读取，供 API 返回业务结论（成功状态 + 业务失败可区分）
async function summarizeRun(engineType, runId) {
  if (engineType === 'delivery') {
    const r = await queryOne(`SELECT * FROM delivery_result WHERE run_id = ${sqlLiteral(runId)}`);
    if (!r) return null;
    return {
      isFulfillable: !!r.is_fulfillable, promisedDay: r.promised_day, promisedQty: r.promised_qty,
      totalCapacityUsed: r.total_capacity_used, rollbackStepsCount: r.rollback_steps_count,
    };
  }
  if (engineType === 'itp') {
    const r = await queryOne(`SELECT count(*) AS n, sum(total_quota) AS tq FROM itp_allotments WHERE run_id = ${sqlLiteral(runId)}`);
    return { allotmentCount: Number(r?.n ?? 0), totalQuota: Number(r?.tq ?? 0) };
  }
  if (engineType === 'iop') {
    const r = await queryOne(`SELECT * FROM iop_summary WHERE run_id = ${sqlLiteral(runId)}`);
    return r ? {
      totalOrders: r.total_orders, scheduledOrders: r.scheduled_orders, blockedOrders: r.blocked_orders,
      totalFulfilledQty: r.total_fulfilled_qty, quotaUtilization: r.quota_utilization,
    } : null;
  }
  if (engineType === 'substitution') {
    const d = await queryOne(`SELECT * FROM substitution_decisions WHERE run_id = ${sqlLiteral(runId)}`);
    const a = await queryOne(`SELECT count(*) AS n, sum(allocated_qty) AS q FROM substitution_allocations WHERE run_id = ${sqlLiteral(runId)}`);
    return d ? {
      category: d.category, chosenPartId: d.chosen_part_id, basis: d.basis,
      netDemand: d.net_demand, remainingOnHandAfter: d.remaining_on_hand_after,
      allocationCount: Number(a?.n ?? 0), allocatedQty: Number(a?.q ?? 0),
    } : null;
  }
  return null;
}

export function buildApi(cfg, runService = new RunService(cfg)) {
  const r = buildRouter();

  r.add('GET', '/api/health', async () => ({ body: await health(cfg) }));

  // ---- 场景 ----
  r.add('GET', '/api/scenarios', async () => {
    const list = discoverScenarios(cfg).map((s) => ({
      scenarioId: s.scenarioId, displayName: s.displayName, source: s.source,
      rootPath: s.rootPath, isComplete: s.isComplete, engines: s.engines, rowCounts: s.rowCounts,
      datasetCount: Object.values(s.datasets).filter((d) => d.exists).length,
      emptyHint: Object.values(s.datasets).every((d) => !d.exists)
        ? '该场景尚无数据集，请上传七类数据集 CSV' : null,
    }));
    return { body: { scenarios: list, count: list.length, scenarioRoot: cfg.scenarioRoot, hint: list.length ? null : '未发现场景。请通过 POST /api/scenarios 创建场景并上传数据集。' } };
  });

  r.add('GET', '/api/scenarios/:scenarioId', async ({ params }) => {
    const s = getScenario(cfg, params.scenarioId);
    if (!s) throw httpError(404, `场景不存在: ${params.scenarioId}`, 'scenario_not_found');
    const imports = await queryParams(
      `SELECT dataset_key, row_count, source_path, imported_at, duration_ms
       FROM dataset_imports WHERE scenario_id = ? ORDER BY imported_at DESC`, [params.scenarioId]);
    return { body: { ...s, imports } };
  });

  r.add('POST', '/api/scenarios', async ({ body }) => {
    const scenarioId = body?.scenarioId ?? body?.name;
    if (!scenarioId) throw httpError(400, '缺少场景标识', 'invalid_request');
    const created = createScenario(cfg, String(scenarioId));
    return { status: 201, body: created };
  });

  r.add('PUT', '/api/scenarios/:scenarioId/datasets/:datasetKey', async ({ params, body }) => {
    const content = typeof body === 'string' ? body : body?.content;
    if (typeof content !== 'string') throw httpError(400, '缺少 CSV 内容', 'invalid_request');
    const updated = putDataset(cfg, params.scenarioId, params.datasetKey, content);
    // 提供数据集后立即导入该数据集
    const ds = datasetByKey(params.datasetKey);
    const imported = await importScenarioInputs(params.scenarioId, updated.rootPath, [params.datasetKey]);
    return { body: { scenario: updated, imported, dataset: ds?.key } };
  });

  // ---- 输入数据浏览 ----
  r.add('GET', '/api/scenarios/:scenarioId/inputs', async ({ params }) => ({
    body: { scenarioId: params.scenarioId, categories: await inputDataOverview(params.scenarioId) },
  }));

  r.add('GET', '/api/scenarios/:scenarioId/inputs/:datasetKey', async ({ params, query: q }) => {
    const limit = Math.min(Number(q.limit) || 500, 5000);
    const offset = Number(q.offset) || 0;
    return { body: await inputRows(params.scenarioId, params.datasetKey, { limit, offset }) };
  });

  // 原始 CSV 文本（与磁盘文件逐字一致）
  r.add('GET', '/api/scenarios/:scenarioId/raw/:filename', async ({ params }) => {
    const s = getScenario(cfg, params.scenarioId);
    if (!s) throw httpError(404, `场景不存在: ${params.scenarioId}`, 'scenario_not_found');
    if (!/^[A-Za-z0-9._-]+\.csv$/.test(params.filename)) throw httpError(400, '非法文件名', 'invalid_request');
    const full = path.join(s.rootPath, params.filename);
    if (!fs.existsSync(full)) throw httpError(404, `文件不存在: ${params.filename}`, 'not_found');
    return { body: { filename: params.filename, content: fs.readFileSync(full, 'utf8') } };
  });

  // ---- 引擎运行（8.4-8.6）----
  for (const engineType of ENGINE_TYPES) {
    r.add('POST', `/api/runs/${engineType}`, async ({ body }) => {
      const { scenarioId, overrides } = validateRunRequest(engineType, body);
      const result = await runService.run({ scenarioId, engineType, overrides });
      const conclusion = await summarizeRun(engineType, result.runId);
      return { status: 201, body: { ...result, conclusion } };
    });
  }

  r.add('POST', '/api/runs', async ({ body }) => {
    const engineType = body?.engineType ?? body?.engine;
    if (!ENGINE_TYPES.includes(engineType)) throw httpError(400, `未知引擎类型: ${engineType}`, 'invalid_request');
    const { scenarioId, overrides } = validateRunRequest(engineType, body);
    const result = await runService.run({ scenarioId, engineType, overrides });
    return { status: 201, body: { ...result, conclusion: await summarizeRun(engineType, result.runId) } };
  });

  // ---- 运行历史（8.8）----
  r.add('GET', '/api/runs', async ({ query: q }) => ({
    body: { runs: await listRuns({ scenarioId: q.scenarioId || null, engineType: q.engineType || null, limit: Math.min(Number(q.limit) || 100, 1000) }) },
  }));

  r.add('GET', '/api/runs/:runId', async ({ params }) => ({ body: await getRun(params.runId) }));

  // 运行输出明细（按 run_id 无重算取回）
  r.add('GET', '/api/runs/:runId/outputs/:datasetKey', async ({ params, query: q }) => {
    const ds = OUTPUT_DATASETS.find((d) => d.key === params.datasetKey);
    if (!ds) throw httpError(400, `未知输出数据集: ${params.datasetKey}`, 'invalid_request');
    const run = await queryOne(`SELECT engine_type FROM engine_runs WHERE run_id = ${sqlLiteral(params.runId)}`);
    if (!run) throw httpError(404, `未找到运行记录: ${params.runId}`, 'run_not_found');
    const limit = Math.min(Number(q.limit) || 1000, 10000);
    const rows = await queryParams(`SELECT * FROM ${ds.key} WHERE run_id = ? LIMIT ?`, [params.runId, limit]);
    return { body: { runId: params.runId, datasetKey: ds.key, rows } };
  });

  // 运行原始产物（5.10）
  r.add('GET', '/api/runs/:runId/artifacts/:direction/:filename', async ({ params }) => {
    const art = await queryOne(
      `SELECT content FROM run_artifacts WHERE run_id = ${sqlLiteral(params.runId)}
       AND direction = ${sqlLiteral(params.direction)} AND filename = ${sqlLiteral(params.filename)}`);
    if (!art) throw httpError(404, '未找到产物', 'artifact_not_found');
    return { body: { runId: params.runId, direction: params.direction, filename: params.filename, content: art.content } };
  });

  // ---- 计划透视（8.7)----
  r.add('GET', '/api/scenarios/:scenarioId/pivot', async ({ params, query: q }) => {
    const metrics = q.metrics ? String(q.metrics).split(',').filter(Boolean) : null;
    const filters = {
      dim_value: q.dimValue ?? q.dim_value ?? null,
      dimension: q.dimension || null,
    };
    return { body: await pivot(params.scenarioId, { metrics, runId: q.runId || null, filters }) };
  });

  r.add('GET', '/api/scenarios/:scenarioId/pivot/range', async ({ params }) => ({
    body: await timeRange(params.scenarioId),
  }));

  r.add('GET', '/api/scenarios/:scenarioId/pivot/cell', async ({ params, query: q }) => ({
    body: await drillDown(params.scenarioId, {
      metric: q.metric, day: q.day, dimValue: q.dimValue ?? q.dim_value ?? null, runId: q.runId || null,
    }),
  }));

  // ---- 场景输入导入（幂等）----
  r.add('POST', '/api/scenarios/:scenarioId/import', async ({ params }) => {
    const s = getScenario(cfg, params.scenarioId);
    if (!s) throw httpError(404, `场景不存在: ${params.scenarioId}`, 'scenario_not_found');
    const imported = await importScenarioInputs(params.scenarioId, s.rootPath);
    return { body: { scenarioId: params.scenarioId, imported, totalRows: imported.reduce((a, x) => a + x.rowCount, 0) } };
  });

  return r;
}
