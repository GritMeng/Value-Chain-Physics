// 一条命令启动演示：依赖检查 -> 构建 C++ CLI -> 建库 -> 发现场景并导入输入 -> 构建前端 -> 启动前后端。
// 重复启动：既存 DuckDB 与已构建 CLI 直接复用，不清空数据。
import { spawnSync, spawn } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { loadConfig, ensureDirs, APP_ROOT, REPO_ROOT } from '../src/config.js';

const step = (n, msg) => console.log(`\n[${n}] ${msg}`);
function fail(msg) { console.error(`\n[demo] 失败：${msg}`); process.exit(1); }

function which(cmd) {
  const r = spawnSync('which', [cmd], { encoding: 'utf8' });
  return r.status === 0 ? r.stdout.trim() : null;
}

// 1) 依赖检查
step(1, '检查依赖');
const nodeMajor = Number(process.versions.node.split('.')[0]);
if (nodeMajor < 20) fail(`需要 Node.js >= 20，当前 ${process.versions.node}`);
console.log(`  Node ${process.versions.node}`);
if (!fs.existsSync(path.join(APP_ROOT, 'node_modules'))) {
  fail('未安装依赖，请先在 ipc-demo 下执行 npm install');
}
console.log('  node_modules 就绪');
const cxx = which('clang++') || which('g++');
if (!cxx) fail('未找到 C++ 编译器（clang++ / g++），无法构建 CLI');
console.log(`  C++ 编译器: ${cxx}`);

const cfg = loadConfig();
ensureDirs(cfg);

// 2) 构建 C++ CLI（缺失时）
step(2, '检查 / 构建 C++ CLI');
if (fs.existsSync(cfg.engineBin)) {
  console.log(`  已存在可执行文件：${cfg.engineBin}（复用）`);
} else {
  const script = path.join(REPO_ROOT, 'ipc-core-benchmark', 'build_and_run.sh');
  console.log(`  未找到，执行 ${script}`);
  const r = spawnSync('bash', [script], { cwd: path.join(REPO_ROOT, 'ipc-core-benchmark'), stdio: 'inherit' });
  if (r.status !== 0 || !fs.existsSync(cfg.engineBin)) fail('C++ CLI 构建失败');
}

// 3) 建库 + 发现场景并导入输入数据 + 构建前端
step(3, '初始化 DuckDB 并导入场景输入数据');
const { openDb, closeDb } = await import('../src/db/duckdb.js');
const { discoverScenarios } = await import('../src/scenario/manager.js');
const { importScenarioInputs } = await import('../src/db/importer.js');
const { query } = await import('../src/db/duckdb.js');
await openDb(cfg);
const scenarios = discoverScenarios(cfg);
let importedScenarios = 0, totalRows = 0;
for (const s of scenarios) {
  const existing = await query(`SELECT count(*) AS c FROM dataset_imports WHERE scenario_id = '${s.scenarioId.replace(/'/g, "''")}'`);
  if (Number(existing[0].c) > 0) {
    console.log(`  复用已导入场景：${s.scenarioId}`);
    continue;
  }
  const results = await importScenarioInputs(s.scenarioId, s.rootPath);
  const rows = results.reduce((a, x) => a + x.rowCount, 0);
  totalRows += rows;
  importedScenarios++;
  console.log(`  导入 ${s.scenarioId}: ${results.length} 个数据集 / ${rows} 行`);
}
console.log(`  共 ${scenarios.length} 个场景（本次新导入 ${importedScenarios} 个，合计 ${totalRows} 行）`);
await closeDb();

step(4, '构建前端 SPA');
const build = spawnSync(process.execPath, [path.join(APP_ROOT, 'scripts', 'build-web.mjs')], { cwd: APP_ROOT, stdio: 'inherit' });
if (build.status !== 0) fail('前端构建失败');

// 4) 启动服务
step(5, `启动服务 http://${cfg.host}:${cfg.port}`);
const child = spawn(process.execPath, [path.join(APP_ROOT, 'src', 'server', 'index.js')], {
  cwd: APP_ROOT, stdio: 'inherit', env: process.env,
});
const stop = () => { child.kill('SIGTERM'); };
process.on('SIGINT', stop);
process.on('SIGTERM', stop);
child.on('exit', (code) => process.exit(code ?? 0));
