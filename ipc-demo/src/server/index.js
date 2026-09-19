// 后端服务入口：初始化 DuckDB -> 首次导入场景输入 -> 启动 HTTP 服务。
import fs from 'node:fs';
import { loadConfig, ensureDirs } from '../config.js';
import { openDb, closeDb, query } from '../db/duckdb.js';
import { discoverScenarios } from '../scenario/manager.js';
import { importScenarioInputs } from '../db/importer.js';
import { createServer } from './http.js';
import { buildApi } from '../routes/api.js';

// 首次启动（或指定场景尚无导入记录）时导入场景输入数据。
export async function bootstrapData(cfg, { force = false } = {}) {
  const scenarios = discoverScenarios(cfg);
  const imported = [];
  for (const s of scenarios) {
    const existing = await query(
      `SELECT count(*) AS c FROM dataset_imports WHERE scenario_id = '${s.scenarioId.replace(/'/g, "''")}'`
    );
    if (!force && Number(existing[0].c) > 0) continue;
    const results = await importScenarioInputs(s.scenarioId, s.rootPath);
    imported.push({ scenarioId: s.scenarioId, datasets: results, rowCount: results.reduce((a, x) => a + x.rowCount, 0) });
  }
  return { scenarios: scenarios.map((s) => s.scenarioId), imported };
}

export async function startServer(cfg = loadConfig(), { importOnStart = true } = {}) {
  ensureDirs(cfg);
  await openDb(cfg);
  if (importOnStart) await bootstrapData(cfg);

  const api = buildApi(cfg);
  const server = createServer({ routes: api, publicDir: fs.existsSync(cfg.publicDir) ? cfg.publicDir : null });

  await new Promise((resolve) => server.listen(cfg.port, cfg.host, resolve));
  const addr = server.address();
  console.log(`[ipc-demo] 后端已启动: http://${cfg.host}:${addr.port}`);
  console.log(`[ipc-demo] DuckDB: ${cfg.duckdbPath}`);
  console.log(`[ipc-demo] 场景根: ${cfg.scenarioRoot}`);
  const found = discoverScenarios(cfg).map((s) => s.scenarioId);
  console.log(`[ipc-demo] 发现场景(${found.length}): ${found.join(', ') || '无'}`);
  return server;
}

// 直接运行：node src/server/index.js
if (import.meta.url === `file://${process.argv[1]}`) {
  const cfg = loadConfig();
  const server = await startServer(cfg);
  const shutdown = async () => {
    server.close();
    await closeDb();
    process.exit(0);
  };
  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
}
