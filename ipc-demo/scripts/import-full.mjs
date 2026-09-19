// 全量 / 大数据集导入入口：导入指定场景（默认全部场景）并报告耗时与总行数。
// 用法：npm run import:full [-- <scenarioId>]
import { loadConfig, ensureDirs } from '../src/config.js';
import { openDb, closeDb, query } from '../src/db/duckdb.js';
import { discoverScenarios } from '../src/scenario/manager.js';
import { importScenarioInputs } from '../src/db/importer.js';

const cfg = loadConfig();
ensureDirs(cfg);
await openDb(cfg);

const argId = process.argv[2];
const all = discoverScenarios(cfg);
const targets = argId ? all.filter((s) => s.scenarioId === argId) : all;
if (targets.length === 0) {
  console.error(`[import:full] 未找到场景: ${argId ?? '(全部)'}`);
  console.error(`可用场景: ${all.map((s) => s.scenarioId).join(', ')}`);
  process.exit(1);
}

console.log(`[import:full] DuckDB: ${cfg.duckdbPath}`);
console.log(`[import:full] 待导入场景 ${targets.length} 个${argId ? '' : '（全部）'}`);
const startedAll = Date.now();
let grandRows = 0, grandDatasets = 0;
for (const s of targets) {
  const started = Date.now();
  const results = await importScenarioInputs(s.scenarioId, s.rootPath);
  const rows = results.reduce((a, x) => a + x.rowCount, 0);
  const ms = Date.now() - started;
  grandRows += rows; grandDatasets += results.length;
  console.log(`  - ${s.scenarioId}: ${results.length} 数据集 / ${rows} 行 / ${ms} ms`);
}
const totalMs = Date.now() - startedAll;
const totalInDb = await query(`SELECT count(*) AS c FROM dataset_imports`);
console.log(`\n[import:full] 完成：本次导入 ${grandDatasets} 个数据集 / ${grandRows} 行，耗时 ${totalMs} ms`);
console.log(`[import:full] 数据库累计导入记录：${totalInDb[0].c} 条`);
await closeDb();
