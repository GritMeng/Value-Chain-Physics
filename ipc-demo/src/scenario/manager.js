// 测试场景管理：发现、列表、完整性校验、创建、数据集替换。
//
// 场景 = 目录。扫描 SCENARIO_ROOT 下的子目录，以及根目录本身（若其含数据集）。
// 内置根（仓库 data/）只读；新建场景写入 writableScenarioRoot。
import fs from 'node:fs';
import path from 'node:path';
import { INPUT_DATASETS, ENGINE_INPUTS } from '../db/schema.js';

// 判断目录是否"含数据集"（至少有一类数据集 CSV）
function hasAnyDataset(dir) {
  return INPUT_DATASETS.some((d) => fs.existsSync(path.join(dir, d.file)));
}

// 解析一个目录的每类数据集状态
function inspectDatasets(dir) {
  const datasets = {};
  for (const ds of INPUT_DATASETS) {
    const p = path.join(dir, ds.file);
    const exists = fs.existsSync(p);
    let readable = false;
    let rowCount = 0;
    let error = null;
    if (exists) {
      try {
        const content = fs.readFileSync(p, 'utf8');
        const lines = content.split(/\r?\n/).filter((l) => l.trim() !== '');
        const header = lines.length ? lines[0].split(',').map((s) => s.trim()) : [];
        const missing = ds.columns.filter((c) => !header.includes(c));
        if (missing.length) {
          error = `缺少列: ${missing.join(', ')}`;
        } else {
          readable = true;
          rowCount = Math.max(0, lines.length - 1);
        }
      } catch (e) {
        error = e.message;
      }
    }
    datasets[ds.key] = { key: ds.key, label: ds.label, file: ds.file, exists, readable, rowCount, error, path: p };
  }
  return datasets;
}

// 按引擎标注可否运行
function engineReadiness(datasets) {
  const out = {};
  for (const [engine, keys] of Object.entries(ENGINE_INPUTS)) {
    const missing = keys.filter((k) => !datasets[k]?.exists || !datasets[k]?.readable);
    out[engine] = { runnable: missing.length === 0, missing };
  }
  return out;
}

// 扫描场景根：返回场景描述数组
export function discoverScenarios(cfg) {
  const scenarios = [];
  const seen = new Set();

  // allowEmpty=true 时，尚无任何数据集的可写场景目录也作为「待填充」场景列出，
  // 这样用户新建后即可在列表/详情中看到并继续提供数据集。
  const addDir = (dir, id, source, allowEmpty = false) => {
    if (seen.has(id)) return;
    if (!fs.existsSync(dir) || !fs.statSync(dir).isDirectory()) return;
    const anyDataset = hasAnyDataset(dir);
    if (!anyDataset && !allowEmpty) return;
    seen.add(id);
    const datasets = inspectDatasets(dir);
    const engines = engineReadiness(datasets);
    const isComplete = Object.values(engines).every((e) => e.runnable);
    scenarios.push({
      scenarioId: id,
      displayName: id === 'root' ? '根数据集' : id,
      rootPath: dir,
      source,
      datasets,
      engines,
      isComplete,
      rowCounts: Object.fromEntries(Object.entries(datasets).map(([k, v]) => [k, v.rowCount])),
    });
  };

  if (fs.existsSync(cfg.scenarioRoot)) {
    const rootId = path.basename(cfg.scenarioRoot);
    addDir(cfg.scenarioRoot, rootId, 'builtin-root');

    for (const entry of fs.readdirSync(cfg.scenarioRoot, { withFileTypes: true })) {
      if (!entry.isDirectory()) continue;
      const sub = path.join(cfg.scenarioRoot, entry.name);
      if (hasAnyDataset(sub)) {
        addDir(sub, entry.name, 'builtin-sub');
      } else {
        for (const inner of fs.readdirSync(sub, { withFileTypes: true })) {
          if (!inner.isDirectory()) continue;
          addDir(path.join(sub, inner.name), inner.name, 'builtin-demo');
        }
      }
    }
  }

  if (fs.existsSync(cfg.writableScenarioRoot)) {
    for (const entry of fs.readdirSync(cfg.writableScenarioRoot, { withFileTypes: true })) {
      if (!entry.isDirectory()) continue;
      addDir(path.join(cfg.writableScenarioRoot, entry.name), entry.name, 'user', true);
    }
  }

  return scenarios.sort((a, b) => a.displayName.localeCompare(b.displayName));
}

export function getScenario(cfg, scenarioId) {
  return discoverScenarios(cfg).find((s) => s.scenarioId === scenarioId) || null;
}

// 创建新场景（写入可写场景根）
export function createScenario(cfg, scenarioId) {
  if (!/^[A-Za-z0-9._-]+$/.test(scenarioId)) {
    const err = new Error('场景标识只允许字母、数字、点、下划线与连字符');
    err.status = 400;
    throw err;
  }
  // 冲突检测针对「场景标识」本身：既包括已发现（含数据集）的场景，也包括
  // 已创建但尚未提供数据集的可写场景目录 —— 否则重复创建会静默覆盖指引。
  const existing = discoverScenarios(cfg).find((s) => s.scenarioId === scenarioId);
  const existingDir = fs.existsSync(path.join(cfg.writableScenarioRoot, scenarioId));
  if (existing || existingDir) {
    const err = new Error(`场景已存在: ${scenarioId}`);
    err.status = 409;
    throw err;
  }
  const dir = path.join(cfg.writableScenarioRoot, scenarioId);
  fs.mkdirSync(dir, { recursive: true });
  return { scenarioId, rootPath: dir, source: 'user' };
}

// 为场景提供/替换一类数据集
export function putDataset(cfg, scenarioId, datasetKey, content) {
  const scenario = getScenario(cfg, scenarioId);
  if (!scenario) {
    const err = new Error(`场景不存在: ${scenarioId}`);
    err.status = 404;
    throw err;
  }
  if (scenario.source !== 'user') {
    const err = new Error(`内置场景 ${scenarioId} 为只读，不能替换数据集`);
    err.status = 403;
    throw err;
  }
  const ds = INPUT_DATASETS.find((d) => d.key === datasetKey);
  if (!ds) {
    const err = new Error(`未知数据集类别: ${datasetKey}`);
    err.status = 400;
    throw err;
  }
  fs.writeFileSync(path.join(scenario.rootPath, ds.file), content, 'utf8');
  return getScenario(cfg, scenarioId);
}

export { inspectDatasets, engineReadiness };
