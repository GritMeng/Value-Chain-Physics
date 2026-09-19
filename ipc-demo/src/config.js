// 配置解析：所有关键配置项均可通过环境变量覆盖。
// 默认值面向「仓库内直接运行」，并把 DuckDB 与运行工作目录放到 ipc-demo/.data 下。
import path from 'node:path';
import fs from 'node:fs';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// ipc-demo/ 目录（src 的上一级）
export const APP_ROOT = path.resolve(__dirname, '..');
// 仓库根目录（ipc-demo 的上一级）
export const REPO_ROOT = path.resolve(APP_ROOT, '..');

function envStr(name, fallback) {
  const v = process.env[name];
  return v === undefined || v === '' ? fallback : v;
}
function envInt(name, fallback) {
  const v = process.env[name];
  if (v === undefined || v === '') return fallback;
  const n = Number.parseInt(v, 10);
  if (Number.isNaN(n)) throw new Error(`环境变量 ${name} 不是合法整数: ${v}`);
  return n;
}

export function loadConfig(overrides = {}) {
  const cfg = {
    // 服务监听端口（默认 5173 前端 + 3001 后端由 demo 脚本分别控制）
    port: envInt('PORT', 3001),
    // 默认仅绑定回环地址
    host: envStr('HOST', '127.0.0.1'),
    // DuckDB 单文件数据库
    duckdbPath: envStr('DUCKDB_PATH', path.join(APP_ROOT, '.data', 'ipc_demo.duckdb')),
    // 只读内置场景根（既有 data/ 与 data/sample/ 及 data/demo/ 都在此下）
    scenarioRoot: envStr('SCENARIO_ROOT', path.join(REPO_ROOT, 'ipc-core-benchmark', 'data')),
    // 可写场景根（新建场景落点，避免改动仓库既有数据）
    writableScenarioRoot: envStr('WRITABLE_SCENARIO_ROOT', path.join(APP_ROOT, 'scenarios')),
    // C++ CLI 可执行文件
    engineBin: envStr('ENGINE_BIN', path.join(REPO_ROOT, 'ipc-core-benchmark', 'bin', 'ipc_engine_cli')),
    // 子进程运行时限（毫秒）
    engineTimeoutMs: envInt('ENGINE_TIMEOUT_MS', 120000),
    // 运行工作目录（每次运行独立子目录）
    runsDir: envStr('RUNS_DIR', path.join(APP_ROOT, 'runs')),
    // 并发运行上限
    maxConcurrentRuns: envInt('MAX_CONCURRENT_RUNS', 2),
    // 前端静态资源目录
    publicDir: envStr('PUBLIC_DIR', path.join(APP_ROOT, 'public')),
  };
  Object.assign(cfg, overrides);
  return cfg;
}

// 确保运行所需目录存在
export function ensureDirs(cfg) {
  fs.mkdirSync(path.dirname(cfg.duckdbPath), { recursive: true });
  fs.mkdirSync(cfg.runsDir, { recursive: true });
  fs.mkdirSync(cfg.writableScenarioRoot, { recursive: true });
}

export default loadConfig;
