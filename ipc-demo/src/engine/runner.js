// Node 编排层：调用 C++ CLI 完成一次引擎运行。
//
// 职责边界（严格）：
//   - 准备独立运行工作目录（in/out）
//   - 以子进程 spawn C++ 可执行文件，传参、捕获 stdout/stderr、退出码、超时
//   - 读取 CLI 输出 CSV 作为权威结果，写入 DuckDB
//   - 记录 Node 端到端耗时与 C++ 自报耗时
// Node 不实现任何引擎算法；约束键等由 CLI 侧计算。
import fs from 'node:fs';
import path from 'node:path';
import { spawn } from 'node:child_process';
import { randomUUID } from 'node:crypto';

export const ENGINE_TYPES = ['delivery', 'itp', 'iop', 'substitution'];

// 引擎 → 需要从场景复制到运行 in/ 目录的输入数据集文件
export const ENGINE_INPUT_FILES = {
  delivery: ['parts.csv', 'bom.csv', 'atp_supply.csv', 'capacity.csv'],
  itp: ['parts.csv', 'demands_master.csv'],
  iop: ['parts.csv', 'demands_master.csv', 'demands_execution.csv'],
  substitution: ['parts.csv', 'bom.csv', 'substitution_group.csv'],
};

// 引擎 → 支持的覆盖参数（用于 7.3）
export const ENGINE_OVERRIDE_KEYS = {
  delivery: ['part_id', 'due_day', 'qty', 'priority'],
  itp: ['buffer_factor'],
  iop: [],
  substitution: ['net_demand', 'alt_class', 'alt_group', 'day', 'parent_id'],
};

export class EngineError extends Error {
  constructor(message, { status = 500, kind = 'engine_error', detail = {} } = {}) {
    super(message);
    this.name = 'EngineError';
    this.status = status;
    this.kind = kind;
    Object.assign(this, detail);
  }
}

// 校验引擎类型
export function assertEngineType(engineType) {
  if (!ENGINE_TYPES.includes(engineType)) {
    throw new EngineError(`未知引擎类型: ${engineType}`, { status: 400, kind: 'invalid_request' });
  }
}

// 7.1 创建一次运行的独立工作目录，返回 { runRoot, inDir, outDir }
export function createRunDirs(runsDir, runId = randomUUID()) {
  const runRoot = path.join(runsDir, runId);
  const inDir = path.join(runRoot, 'in');
  const outDir = path.join(runRoot, 'out');
  fs.mkdirSync(inDir, { recursive: true });
  fs.mkdirSync(outDir, { recursive: true });
  return { runId, runRoot, inDir, outDir };
}

// 7.2/7.3 从场景目录复制引擎所需输入文件；如提供覆盖参数则追加/覆盖到输入。
// 复制而非软链，保证运行不修改场景目录。
export function prepareInputs(engineType, scenarioDir, inDir, overrides = {}, requiredFiles = null) {
  const files = requiredFiles || ENGINE_INPUT_FILES[engineType] || [];
  const copied = [];
  for (const f of files) {
    const src = path.join(scenarioDir, f);
    if (fs.existsSync(src)) {
      fs.copyFileSync(src, path.join(inDir, f));
      copied.push(f);
    }
  }
  // 交付承诺的显式参数作为查询参数传给 CLI，不写入 CSV；此处仅记录 JSON。
  return { copied, overrides };
}

// 7.4 子进程调用。返回 { code, stdout, stderr, timedOut, durationMs, engineDurationMs }
export function invokeEngine({ engineBin, engineType, inDir, outDir, args = [], timeoutMs = 120000 }) {
  return new Promise((resolve, reject) => {
    if (!fs.existsSync(engineBin)) {
      reject(new EngineError(
        `C++ 可执行文件不存在: ${engineBin}。请先运行 ipc-core-benchmark/build_and_run.sh 构建 CLI。`,
        { status: 503, kind: 'engine_missing', detail: { executablePath: engineBin } }
      ));
      return;
    }
    try {
      fs.accessSync(engineBin, fs.constants.X_OK);
    } catch {
      reject(new EngineError(
        `C++ 可执行文件不可执行: ${engineBin}。请检查文件权限或重新构建。`,
        { status: 503, kind: 'engine_not_executable', detail: { executablePath: engineBin } }
      ));
      return;
    }

    // 只允许白名单参数，避免把任意字符串透传给 CLI。
    const allowed = ENGINE_OVERRIDE_KEYS[engineType] || [];
    const cliArgs = ['--engine', engineType, '--in', inDir, '--out', outDir];
    for (const [k, v] of Object.entries(args)) {
      if (allowed.includes(k) && v !== undefined && v !== null && v !== '') {
        cliArgs.push(`--${k.replace(/_/g, '-')}`, String(v));
      }
    }

    const started = Date.now();
    const child = spawn(engineBin, cliArgs, { stdio: ['ignore', 'pipe', 'pipe'] });
    let stdout = '';
    let stderr = '';
    let timedOut = false;
    let settled = false;

    const timer = setTimeout(() => {
      timedOut = true;
      child.kill('SIGKILL');
    }, timeoutMs);

    child.stdout.on('data', (d) => { stdout += d.toString(); });
    child.stderr.on('data', (d) => { stderr += d.toString(); });

    child.on('error', (err) => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      reject(new EngineError(`启动 C++ 子进程失败: ${err.message}`, {
        status: 500, kind: 'engine_spawn_failed', detail: { executablePath: engineBin, argv: cliArgs },
      }));
    });

    child.on('close', (code, signal) => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      const durationMs = Date.now() - started;
      // 解析 C++ 自报耗时 duration_ms=...
      const m = /duration_ms=([0-9.]+)/.exec(stdout);
      const engineDurationMs = m ? Number(m[1]) : null; // 保留小数精度
      if (timedOut) {
        reject(new EngineError(
          `C++ 引擎运行超时（时限 ${timeoutMs} ms），已终止子进程。`,
          { status: 504, kind: 'engine_timeout',
            detail: { executablePath: engineBin, argv: cliArgs, exitCode: code, signal, stdout, stderr, durationMs, timeoutMs } }
        ));
        return;
      }
      resolve({ code, signal, stdout, stderr, durationMs, engineDurationMs, argv: cliArgs, executablePath: engineBin });
    });
  });
}
