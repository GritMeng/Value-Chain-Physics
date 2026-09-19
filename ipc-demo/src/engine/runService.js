// 运行服务：编排一次完整运行并持久化。
//
// 流程：准备输入目录 -> 调子进程 -> 成功判定 -> 导入输出 CSV -> 归档原始产物
//       -> 写 engine_runs 元数据（成功与失败都写）。
import fs from 'node:fs';
import path from 'node:path';
import { randomUUID } from 'node:crypto';
import { exec, execParams } from '../db/duckdb.js';
import { importRunOutputs, archiveArtifacts } from '../db/importer.js';
import { OUTPUT_DATASETS, ENGINE_OUTPUTS } from '../db/schema.js';
import { getScenario } from '../scenario/manager.js';
import {
  ENGINE_INPUT_FILES, ENGINE_TYPES, EngineError, assertEngineType,
  createRunDirs, prepareInputs, invokeEngine,
} from './runner.js';

export { ENGINE_TYPES, EngineError };

// 7.9 并发上限：简单信号量队列，超过上限的请求排队而非无限制并发。
class Semaphore {
  constructor(limit) { this.limit = Math.max(1, limit); this.active = 0; this.queue = []; }
  async acquire() {
    if (this.active < this.limit) { this.active++; return; }
    await new Promise((resolve) => this.queue.push(resolve));
    this.active++;
  }
  release() {
    this.active--;
    const next = this.queue.shift();
    if (next) next();
  }
}

// 5.9 无论成功失败都写入 engine_runs
async function recordRun(row) {
  await execParams(
    `INSERT INTO engine_runs
      (run_id, scenario_id, engine_type, status, created_at, params_json, exit_code,
       duration_ms, engine_duration_ms, error_message, executable_path, argv_json,
       in_dir, out_dir, dataset_source_id)
     VALUES (?, ?, ?, ?, now(), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
    [
      row.runId, row.scenarioId, row.engineType, row.status, row.paramsJson,
      row.exitCode ?? null, row.durationMs ?? null, row.engineDurationMs ?? null,
      row.errorMessage ?? null, row.executablePath ?? null, row.argvJson ?? null,
      row.inDir ?? null, row.outDir ?? null, row.datasetSourceId ?? null,
    ]
  );
}

// 7.5 成功判定：退出码 0 + 该引擎所有输出 CSV 存在 + 必需列可解析
function verifyOutputs(engineType, outDir) {
  const keys = ENGINE_OUTPUTS[engineType] || [];
  const missingFiles = [];
  const malformed = [];
  const present = [];
  for (const key of keys) {
    const ds = OUTPUT_DATASETS.find((d) => d.key === key);
    const csvPath = path.join(outDir, ds.file);
    if (!fs.existsSync(csvPath)) {
      missingFiles.push(ds.file);
      continue;
    }
    const header = fs.readFileSync(csvPath, 'utf8').split(/\r?\n/, 1)[0] || '';
    const cols = header.split(',').map((s) => s.trim().replace(/^"|"$/g, ''));
    const required = ds.ddl.match(/CREATE TABLE IF NOT EXISTS \w+ \(([\s\S]*?)\)/)[1]
      .split(',').map((c) => c.trim().split(/\s+/)[0]).filter(Boolean)
      .filter((c) => c !== 'run_id'); // run_id 由入库层补齐，不在 CLI 输出 CSV 中
    const missingCols = required.filter((c) => !cols.includes(c));
    if (missingCols.length) malformed.push(`${ds.file} 缺少列 ${missingCols.join(',')}`);
    else present.push({ key, file: ds.file, path: csvPath });
  }
  return { missingFiles, malformed, present };
}

export class RunService {
  constructor(cfg) {
    this.cfg = cfg;
    this.sem = new Semaphore(cfg.maxConcurrentRuns ?? 2);
  }

  // 主入口：运行一个引擎。request 为已验证的参数对象。
  async run({ scenarioId, engineType, overrides = {} }) {
    assertEngineType(engineType);
    const scenario = getScenario(this.cfg, scenarioId);
    if (!scenario) {
      throw new EngineError(`场景不存在: ${scenarioId}`, { status: 404, kind: 'scenario_not_found' });
    }

    const ready = scenario.engines?.[engineType];
    if (ready && !ready.runnable) {
      throw new EngineError(
        `场景 ${scenarioId} 缺少运行 ${engineType} 所需数据集: ${ready.missing.join(', ')}`,
        { status: 422, kind: 'scenario_incomplete', detail: { missing: ready.missing } }
      );
    }

    await this.sem.acquire();
    try {
      return await this.#runInner({ scenario, engineType, overrides });
    } finally {
      this.sem.release();
    }
  }

  async #runInner({ scenario, engineType, overrides }) {
    const { runId, runRoot, inDir, outDir } = createRunDirs(this.cfg.runsDir);
    const paramsJson = JSON.stringify(overrides || {});
    prepareInputs(engineType, scenario.rootPath, inDir, overrides);

    let invocation;
    try {
      invocation = await invokeEngine({
        engineBin: this.cfg.engineBin,
        engineType,
        inDir,
        outDir,
        args: overrides,
        timeoutMs: this.cfg.engineTimeoutMs,
      });
    } catch (err) {
      // 7.6 可执行文件缺失 / 超时 / 启动失败 -> 记录失败运行
      await recordRun({
        runId, scenarioId: scenario.scenarioId, engineType, status: 'failed',
        paramsJson, errorMessage: err.message, executablePath: err.executablePath ?? this.cfg.engineBin,
        argvJson: JSON.stringify(err.argv ?? []), inDir, outDir,
        durationMs: err.durationMs ?? null,
      });
      err.runId = runId;
      throw err;
    }

    // 非零退出码：orphan 记录并抛错，不产生部分结果
    if (invocation.code !== 0) {
      const status = invocation.code === 3 ? 'failed_input' : 'failed';
      const message = `C++ 引擎非零退出（退出码 ${invocation.code}）: ${invocation.stderr.trim().slice(0, 500) || invocation.stdout.trim().slice(0, 500)}`;
      await recordRun({
        runId, scenarioId: scenario.scenarioId, engineType, status,
        paramsJson, exitCode: invocation.code, durationMs: invocation.durationMs,
        engineDurationMs: invocation.engineDurationMs, errorMessage: message,
        executablePath: invocation.executablePath, argvJson: JSON.stringify(invocation.argv),
        inDir, outDir,
      });
      throw new EngineError(message, {
        status: 502, kind: 'engine_nonzero_exit',
        detail: { runId, exitCode: invocation.code, stderr: invocation.stderr, stdout: invocation.stdout },
      });
    }

    // 输出格式校验
    const check = verifyOutputs(engineType, outDir);
    if (check.missingFiles.length || check.malformed.length) {
      const message = `C++ 引擎输出不完整: ${[...check.missingFiles, ...check.malformed].join('; ')}`;
      await recordRun({
        runId, scenarioId: scenario.scenarioId, engineType, status: 'failed_output',
        paramsJson, exitCode: invocation.code, durationMs: invocation.durationMs,
        engineDurationMs: invocation.engineDurationMs, errorMessage: message,
        executablePath: invocation.executablePath, argvJson: JSON.stringify(invocation.argv),
        inDir, outDir,
      });
      throw new EngineError(message, { status: 502, kind: 'malformed_output', detail: { runId } });
    }

    // 7.7 输出入库 + 5.10 归档产物 + 5.9 运行元数据
    const outputs = await importRunOutputs(runId, engineType, outDir);
    await archiveArtifacts(runId, inDir, outDir);
    await recordRun({
      runId, scenarioId: scenario.scenarioId, engineType, status: 'succeeded',
      paramsJson, exitCode: invocation.code, durationMs: invocation.durationMs,
      engineDurationMs: invocation.engineDurationMs, executablePath: invocation.executablePath,
      argvJson: JSON.stringify(invocation.argv), inDir, outDir,
    });

    return {
      runId, scenarioId: scenario.scenarioId, engineType, status: 'succeeded',
      durationMs: invocation.durationMs, engineDurationMs: invocation.engineDurationMs,
      stdout: invocation.stdout.trim(), outputs, conclusion: conclusionOf(engineType, outputs),
    };
  }
}

// 从输出行数/摘要给出人类可读的业务结论（8.4-8.6 使用）
function conclusionOf(engineType, outputs) {
  const byKey = Object.fromEntries(outputs.map((o) => [o.datasetKey, o.rowCount]));
  return { engineType, rowCounts: byKey };
}
