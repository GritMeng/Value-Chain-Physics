// 业务类别查询与运行历史查询。
//
// 分层：
//   - 输入数据：按业务类别（需求/供给/资源/计划供给/物料主数据/产品结构/替代料组）
//     读取基础数据集表，全部带 scenario_id。
//   - 运行历史：读 engine_runs / run_artifacts，按 run_id 取回元数据与产物。
import { INPUT_DATASETS, OUTPUT_DATASETS, ENGINE_OUTPUTS } from './schema.js';
import { query, queryOne, queryParams, sqlLiteral } from './duckdb.js';

// 业务类别 ⊆ 数据集类别，另加「计划供给」等面向用户的聚合类别。
export const BUSINESS_CATEGORIES = [
  { key: 'demand', label: '需求', datasets: ['demands_master', 'demands_execution'] },
  { key: 'supply', label: '供给', datasets: ['atp_supply'] },
  { key: 'resource', label: '资源', datasets: ['capacity'] },
  { key: 'master', label: '物料主数据', datasets: ['parts', 'substitution_group'] },
  { key: 'structure', label: '产品结构', datasets: ['bom'] },
];

export function datasetByKey(key) {
  return INPUT_DATASETS.find((d) => d.key === key) || null;
}

export function outputDatasetByKey(key) {
  return OUTPUT_DATASETS.find((d) => d.key === key) || null;
}

// 某个场景某数据集的入库行数
export async function datasetRowCount(scenarioId, table) {
  const rows = await queryParams(
    `SELECT count(*) AS c FROM ${table} WHERE scenario_id = ?`,
    [scenarioId]
  );
  return Number(rows[0]?.c ?? 0);
}

// 5.8 按业务类别查询输入数据。返回每类的数据集列表与行数，供浏览视图使用。
export async function inputDataOverview(scenarioId) {
  const out = [];
  for (const cat of BUSINESS_CATEGORIES) {
    const items = [];
    for (const key of cat.datasets) {
      const ds = datasetByKey(key);
      const count = await datasetRowCount(scenarioId, key);
      items.push({ key, label: ds.label, file: ds.file, category: ds.category, rowCount: count });
    }
    out.push({ category: cat.key, label: cat.label, datasets: items });
  }
  return out;
}

// 读取某场景某输入数据集的明细（分页，默认限制保护）
export async function inputRows(scenarioId, datasetKey, { limit = 500, offset = 0 } = {}) {
  const ds = datasetByKey(datasetKey);
  if (!ds) {
    const err = new Error(`未知输入数据集: ${datasetKey}`);
    err.status = 400;
    throw err;
  }
  const rows = await queryParams(
    `SELECT * FROM ${datasetKey} WHERE scenario_id = ? LIMIT ? OFFSET ?`,
    [scenarioId, limit, offset]
  );
  return { datasetKey, columns: ds.columns, rows };
}

// 5.11 运行历史：按场景/引擎类型列摘要
export async function listRuns({ scenarioId = null, engineType = null, limit = 100 } = {}) {
  const where = [];
  const params = [];
  if (scenarioId) { where.push('scenario_id = ?'); params.push(scenarioId); }
  if (engineType) { where.push('engine_type = ?'); params.push(engineType); }
  const sql = `SELECT * FROM engine_runs
    ${where.length ? 'WHERE ' + where.join(' AND ') : ''}
    ORDER BY created_at DESC LIMIT ?`;
  params.push(limit);
  return queryParams(sql, params);
}

// 5.11 运行详情：按 run_id 取回元数据 + 输出数据集行数；未知 run_id 抛 404。
export async function getRun(runId) {
  const run = await queryOne(`SELECT * FROM engine_runs WHERE run_id = ${sqlLiteral(runId)}`);
  if (!run) {
    const err = new Error(`未找到运行记录: ${runId}`);
    err.status = 404;
    throw err;
  }
  const outputs = [];
  for (const key of ENGINE_OUTPUTS[run.engine_type] || []) {
    const ds = outputDatasetByKey(key);
    const rows = await queryParams(`SELECT count(*) AS c FROM ${key} WHERE run_id = ?`, [runId]);
    outputs.push({ key, label: ds.label, file: ds.file, rowCount: Number(rows[0]?.c ?? 0) });
  }
  const artifacts = await queryParams(
    `SELECT direction, filename, archive_path, byte_size FROM run_artifacts WHERE run_id = ? ORDER BY direction, filename`,
    [runId]
  );
  return { ...run, outputs, artifacts };
}
