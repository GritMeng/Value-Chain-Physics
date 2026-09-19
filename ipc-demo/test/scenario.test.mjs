import { test } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { tempConfig } from './helpers.mjs';
import { ensureDirs } from '../src/config.js';
import { discoverScenarios, getScenario, createScenario, putDataset } from '../src/scenario/manager.js';

test('发现场景根下的 sample 子目录与 demo 场景', () => {
  const { cfg } = tempConfig();
  ensureDirs(cfg);
  const ids = discoverScenarios(cfg).map((s) => s.scenarioId);
  assert.ok(ids.includes('sample'), '应识别 sample 子目录');
  assert.ok(ids.includes('delivery-benchmark'), '应识别 demo 下的场景');
});

test('场景根目录本身含数据集时也作为一个场景', () => {
  const { cfg } = tempConfig();
  // 把 sample 的数据集提到场景根顶层，模拟既有 data/ 根
  const sample = path.join(cfg.scenarioRoot, 'sample');
  for (const f of fs.readdirSync(sample)) fs.copyFileSync(path.join(sample, f), path.join(cfg.scenarioRoot, f));
  ensureDirs(cfg);
  const ids = discoverScenarios(cfg).map((s) => s.scenarioId);
  assert.ok(ids.includes(path.basename(cfg.scenarioRoot)), '应识别场景根目录本身');
});

test('四个引擎的 demo 场景均可运行并包含七类数据集', () => {
  const { cfg } = tempConfig();
  ensureDirs(cfg);
  const list = discoverScenarios(cfg);
  for (const id of ['delivery-benchmark', 'itp-iop-benchmark', 'substitution-benchmark']) {
    const s = list.find((x) => x.scenarioId === id);
    assert.ok(s, `缺少场景 ${id}`);
    assert.equal(Object.keys(s.datasets).length, 7);
    for (const engine of ['delivery', 'itp', 'iop', 'substitution']) {
      assert.equal(s.engines[engine].runnable, true, `${id}.${engine} 应可运行`);
    }
  }
});

test('缺失数据集按引擎标注不可运行', () => {
  const { cfg } = tempConfig();
  const dir = path.join(cfg.scenarioRoot, 'partial');
  fs.mkdirSync(dir, { recursive: true });
  fs.writeFileSync(path.join(dir, 'parts.csv'),
    'part_id,part_code,site,safety_stock,initial_on_hand,lot_size,lead_time\n1,P1,S1,0,10,1,1\n');
  ensureDirs(cfg);
  const s = getScenario(cfg, 'partial');
  assert.equal(s.engines.delivery.runnable, false);
  assert.deepEqual(s.engines.delivery.missing.sort(), ['atp_supply', 'bom', 'capacity'].sort());
});

test('列契约不符的数据集标注为不可读', () => {
  const { cfg } = tempConfig();
  const dir = path.join(cfg.scenarioRoot, 'badcol');
  fs.mkdirSync(dir, { recursive: true });
  fs.writeFileSync(path.join(dir, 'parts.csv'), 'wrong,col\n1,2\n');
  ensureDirs(cfg);
  const s = getScenario(cfg, 'badcol');
  assert.equal(s.datasets.parts.readable, false);
  assert.match(s.datasets.parts.error, /缺少列/);
});

test('创建场景并拒绝重名冲突', () => {
  const { cfg } = tempConfig();
  ensureDirs(cfg);
  const c = createScenario(cfg, 'my-scenario');
  assert.ok(fs.existsSync(c.rootPath));
  // 未提供数据集时也应出现在列表中（待填充）
  assert.ok(discoverScenarios(cfg).find((s) => s.scenarioId === 'my-scenario'));
  assert.throws(() => createScenario(cfg, 'my-scenario'), (e) => e.status === 409);
  assert.throws(() => createScenario(cfg, '../evil'), (e) => e.status === 400);
});

test('为场景提供数据集并触发重新校验', () => {
  const { cfg } = tempConfig();
  ensureDirs(cfg);
  createScenario(cfg, 'fill-me');
  putDataset(cfg, 'fill-me', 'parts',
    'part_id,part_code,site,safety_stock,initial_on_hand,lot_size,lead_time\n7,P7,S1,1,2,3,4\n');
  const s = getScenario(cfg, 'fill-me');
  assert.equal(s.rowCounts.parts, 1);
  assert.equal(s.engines.itp.runnable, false); // 仍缺 demands_master
});

test('内置场景只读，不允许替换数据集', () => {
  const { cfg } = tempConfig();
  ensureDirs(cfg);
  assert.throws(() => putDataset(cfg, 'sample', 'parts', 'x'), (e) => e.status === 403);
});

test('空场景根返回空列表而不报错', () => {
  const { cfg: cfg2 } = tempConfig({ scenarioRoot: path.join(tempConfig().tmp, 'no-such-root') });
  ensureDirs(cfg2);
  assert.deepEqual(discoverScenarios(cfg2), []);
});
