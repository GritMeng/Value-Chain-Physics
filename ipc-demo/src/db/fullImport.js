// 5.12 大数据集导入路径：导入场景中全部可用输入数据集，并报告耗时与总行数。
// 供 `npm run import:full` 使用；小场景不会被强制加载大数据。
import fs from 'node:fs';
import { importScenarioInputs } from './importer.js';
import { INPUT_DATASETS } from './schema.js';
import { getScenario } from '../scenario/manager.js';

export async function importFull(cfg, { scenarioId = null, datasetKeys = null } = {}) {
  const targets = [];
  if (scenarioId) {
    const s = getScenario(cfg, scenarioId);
    if (!s) {
      const err = new Error(`场景不存在: ${scenarioId}`);
      err.status = 404;
      throw err;
    }
    targets.push(s);
  } else {
    // 未指定场景时导入所有可发现的场景（含 data/ 根与 data/sample/）
    const { discoverScenarios } = await import('../scenario/manager.js');
    targets.push(...discoverScenarios(cfg));
  }

  const started = Date.now();
  const results = [];
  let totalRows = 0;
  for (const s of targets) {
    const imported = await importScenarioInputs(s.scenarioId, s.rootPath, datasetKeys);
    const rows = imported.reduce((a, r) => a + r.rowCount, 0);
    totalRows += rows;
    results.push({ scenarioId: s.scenarioId, datasets: imported, rowCount: rows });
  }
  return {
    scenarios: results,
    totalRows,
    datasetCount: results.reduce((a, r) => a + r.datasets.length, 0),
    durationMs: Date.now() - started,
    availableDatasetKeys: INPUT_DATASETS.map((d) => d.key),
  };
}
