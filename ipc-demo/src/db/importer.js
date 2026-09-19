// 输入/输出 CSV 导入 DuckDB。
//
// 输入数据集：按 scenario_id 先删后插，保证幂等。
// 输出数据集：按 run_id 导入（每次运行独立）。
import fs from 'node:fs';
import path from 'node:path';
import { randomUUID } from 'node:crypto';
import { exec, query } from './duckdb.js';
import { INPUT_DATASETS, OUTPUT_DATASETS, ENGINE_OUTPUTS } from './schema.js';

function lit(v) {
  if (v === null || v === undefined) return 'NULL';
  if (typeof v === 'number') return String(v);
  if (typeof v === 'boolean') return v ? 'true' : 'false';
  return `'${String(v).replace(/'/g, "''")}'`;
}

// 用 read_csv_auto 把 CSV 读入临时视图，选出期望列并补 scenario_id，插入目标表。
// 先删后插保证幂等。返回写入行数。
export async function importDataset(scenarioId, dataset, csvPath) {
  const started = Date.now();
  const cols = dataset.columns.join(', ');
  // read_csv_auto 需按列名选择，保证列顺序与契约一致；缺失列会抛错。
  await exec(`
    DELETE FROM ${dataset.key} WHERE scenario_id = ${lit(scenarioId)};
  `);
  await exec(`
    INSERT INTO ${dataset.key} (scenario_id, ${cols})
    SELECT ${lit(scenarioId)}, ${cols}
    FROM read_csv_auto(${lit(csvPath)}, header = true, all_varchar = false)
  `);
  const rows = await query(
    `SELECT count(*) AS c FROM ${dataset.key} WHERE scenario_id = ${lit(scenarioId)}`
  );
  const rowCount = Number(rows[0].c);
  const durationMs = Date.now() - started;
  const importId = randomUUID();
  await exec(`
    INSERT INTO dataset_imports (import_id, scenario_id, dataset_key, row_count, source_path, imported_at, duration_ms)
    VALUES (${lit(importId)}, ${lit(scenarioId)}, ${lit(dataset.key)}, ${rowCount}, ${lit(csvPath)}, now(), ${durationMs})
  `);
  return { datasetKey: dataset.key, rowCount, durationMs, sourcePath: csvPath };
}

// 导入一个场景的全部可用输入数据集（缺失的跳过，由完整性校验单独报告）。
export async function importScenarioInputs(scenarioId, scenarioDir, datasetKeys = null) {
  const results = [];
  for (const ds of INPUT_DATASETS) {
    if (datasetKeys && !datasetKeys.includes(ds.key)) continue;
    const csvPath = path.join(scenarioDir, ds.file);
    if (!fs.existsSync(csvPath)) continue;
    results.push(await importDataset(scenarioId, ds, csvPath));
  }
  return results;
}

// 导入一次运行的输出 CSV（按 run_id）。
export async function importRunOutputs(runId, engineType, outDir) {
  const keys = ENGINE_OUTPUTS[engineType] || [];
  const results = [];
  for (const key of keys) {
    const ds = OUTPUT_DATASETS.find((d) => d.key === key);
    if (!ds) continue;
    const csvPath = path.join(outDir, ds.file);
    if (!fs.existsSync(csvPath)) continue;
    await exec(`DELETE FROM ${ds.key} WHERE run_id = ${lit(runId)};`);
    const started = Date.now();
    // 显式按表列名选择，避免 CSV 列顺序与表定义不一致时错位。
    const cols = await columnsOf(ds.key);
    const selectCols = cols.filter((c) => c !== 'run_id').join(', ');
    try {
      await exec(`
        INSERT INTO ${ds.key} (run_id, ${selectCols})
        SELECT ${lit(runId)}, ${selectCols}
        FROM read_csv_auto(${lit(csvPath)}, header = true)
      `);
    } catch (err) {
      throw new Error(`导入输出 ${ds.file} 失败: ${err.message}`);
    }
    const rows = await query(`SELECT count(*) AS c FROM ${ds.key} WHERE run_id = ${lit(runId)}`);
    results.push({ datasetKey: ds.key, rowCount: Number(rows[0].c), durationMs: Date.now() - started });
  }
  return results;
}

async function columnsOf(table) {
  const rows = await query(
    `SELECT column_name FROM information_schema.columns
     WHERE table_name = ${lit(table)} ORDER BY ordinal_position`
  );
  return rows.map((r) => r.column_name);
}

// 归档一次运行的输入/输出 CSV 原文到 run_artifacts（任务 5.10）
export async function archiveArtifacts(runId, inDir, outDir) {
  let count = 0;
  for (const [direction, dir] of [['in', inDir], ['out', outDir]]) {
    if (!dir || !fs.existsSync(dir)) continue;
    for (const f of fs.readdirSync(dir)) {
      if (!f.endsWith('.csv')) continue;
      const full = path.join(dir, f);
      const content = fs.readFileSync(full, 'utf8');
      await exec(`
        INSERT INTO run_artifacts (run_id, direction, filename, content, archive_path, byte_size)
        VALUES (${lit(runId)}, ${lit(direction)}, ${lit(f)}, ${lit(content)}, ${lit(full)}, ${Buffer.byteLength(content)})
      `);
      count++;
    }
  }
  return count;
}
