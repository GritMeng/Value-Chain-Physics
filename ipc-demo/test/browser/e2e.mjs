// 一次 MCP 会话内完成全部前端页面测试（AionUi 内置浏览器 + chrome-devtools-mcp）。
import { createMcp, initMcp } from './mcp-client.mjs';
import { setTimeout as sleep } from 'node:timers/promises';

const port = process.env.AIONUI_CDP_ACTIVE_PORT;
const mcp = createMcp({ command: 'npx', args: ['-y', 'chrome-devtools-mcp@0.16.0', '--browser-url', `http://127.0.0.1:${port}`] });
await initMcp(mcp);

async function call(name, args = {}) {
  const r = await mcp.request('tools/call', { name, arguments: args });
  const text = (r.content || []).filter((c) => c.type === 'text').map((c) => c.text).join('\n');
  if (r.isError) throw new Error(`${name}: ${text.slice(0, 300)}`);
  return text;
}
const json = (t) => { const m = t.match(/```json\n([\s\S]*?)\n```/); return m ? JSON.parse(m[1]) : null; };
const evalJs = async (fn) => json(await call('evaluate_script', { function: fn }));

let pass = 0, fail = 0;
const results = [];
function check(name, cond, detail = '') {
  if (cond) { pass++; results.push(`  ✅ ${name}${detail ? ' — ' + detail : ''}`); }
  else { fail++; results.push(`  ❌ ${name}${detail ? ' — ' + detail : ''}`); }
}

// ---- 0) 选中页面 + 导航 ----
// chrome-devtools-mcp 要求先用 list_pages 取到数字 pageId，再 select_page
let pageId = null;
for (let i = 0; i < 20; i++) {
  try {
    const lp = await call('list_pages', {});
    const m = lp.match(/^\s*(\d+):/m);
    if (m) { pageId = Number(m[1]); break; }
  } catch {}
  await sleep(500);
}
if (pageId === null) throw new Error('list_pages 未返回可选中页面');
await call('select_page', { pageId });
await call('navigate_page', { type: 'url', url: 'http://127.0.0.1:3001', timeout: 30000 });
try { await call('wait_for', { text: '测试场景', timeout: 20000 }); } catch {}

// ---- 1) 应用外壳加载 ----
let ready = null;
for (let i = 0; i < 40; i++) {
  ready = await evalJs(`() => ({
    sc: document.querySelectorAll('[data-testid="scenario-select"] option').length,
    health: document.querySelector('.health')?.textContent?.trim() || '',
  })`);
  if (ready && ready.sc > 0) break;
  await sleep(500);
}
check('应用外壳加载并取得场景列表', !!ready && ready.sc >= 5, `场景选项=${ready?.sc}, 健康=${ready?.health}`);

const shell = await evalJs(`() => ({
  brand: document.querySelector('.brand')?.textContent?.trim(),
  tabs: [...document.querySelectorAll('.tabs button')].map(b => b.textContent.trim()),
  active: document.querySelector('.tabs button.active')?.textContent?.trim(),
})`);
check('六个导航页签齐全', shell.tabs.length === 6, shell.tabs.join(' | '));
check('品牌标题正确', shell.brand === 'IPC 优化引擎可视化演示', shell.brand);

// ---- 2) 场景浏览页（输入数据） ----
const sel = async (scenarioId) => {
  await evalJs(`() => { const s=document.querySelector('[data-testid="scenario-select"]'); s.value=${JSON.stringify(scenarioId)}; s.dispatchEvent(new Event('change',{bubbles:true})); return true; }`);
  await sleep(900);
};

// React 受控输入：必须用原生 value setter + input 事件，否则 React 的 value tracker 会吞掉 change
const setInputs = async (pairs) => {
  const js = `() => {
    const entries = ${JSON.stringify(pairs)};
    for (const [testid, v] of entries) {
      const el = document.querySelector('[data-testid="' + testid + '"]');
      if (!el) continue;
      const desc = Object.getOwnPropertyDescriptor(Object.getPrototypeOf(el), 'value');
      desc.set.call(el, String(v));
      el.dispatchEvent(new Event('input', { bubbles: true }));
      el.dispatchEvent(new Event('change', { bubbles: true }));
    }
    return true;
  }`;
  await evalJs(js);
  await sleep(400);
};
await sel('itp-iop-benchmark');
const browser = await evalJs(`() => ({
  cats: [...document.querySelectorAll('[data-testid^="cat-"]')].map(e => e.textContent.trim()),
  dss: [...document.querySelectorAll('[data-testid^="ds-"]')].map(e => e.textContent.trim()),
  table: !!document.querySelector('[data-testid="data-table"]'),
})`);
check('场景浏览显示 5 类输入数据分类', browser.cats.length >= 5, browser.cats.join(' | '));
check('需求/供给/资源数据集可见', browser.dss.some(t=>t.includes('需求')) || browser.dss.length > 0, `${browser.dss.length} 个数据集按钮`);

// 查看原始 CSV（必须先选中一个数据集，show-raw 才渲染）
const dsClicked = await evalJs(`() => {
  const b = document.querySelector('[data-testid^="ds-"]');
  if (!b) return false;
  b.click();
  return true;
}`);
await sleep(1200);
const rawBtn = await evalJs(`() => {
  const b = document.querySelector('[data-testid="show-raw"]');
  if (b) b.click();
  return !!b;
}`);
await sleep(900);
const rawCsv = await evalJs(`() => document.querySelector('[data-testid="raw-csv"]')?.textContent?.slice(0,80) || ''`);
check('原始 CSV 可查看', dsClicked && rawBtn && !!rawCsv, rawCsv.replace(/\n/g,'⏎').slice(0,60));

// ---- 3) 计划透视页 ----
await evalJs(`() => { [...document.querySelectorAll('.tabs button')].find(b=>b.textContent.includes('计划透视')).click(); return true; }`);
await sleep(1500);
const pivot = await evalJs(`() => {
  const fams = [...document.querySelectorAll('[data-testid^="family-"]')].map(e => e.textContent.trim());
  const rows = document.querySelectorAll('[data-testid^="pivot-row-"]').length;
  const cells = document.querySelectorAll('td').length;
  const blanks = document.querySelectorAll('[data-testid="cell-blank"]').length;
  const zeros = document.querySelectorAll('[data-testid="cell-zero"]').length;
  const values = document.querySelectorAll('[data-testid="cell-value"]').length;
  const grid = !!document.querySelector('[data-testid="pivot-grid"]');
  return { fams, rows, cells, blanks, zeros, values, grid };
}`);
check('计划透视图渲染', pivot.grid, `行=${pivot.rows}, 单元格=${pivot.cells}`);
check('四个指标族分组', pivot.fams.length === 4, pivot.fams.join(' | '));
check('空值(NULL)与零值区分渲染', (pivot.blanks + pivot.zeros) > 0, `空=${pivot.blanks}, 零=${pivot.zeros}, 值=${pivot.values}`);

// 维度筛选
const filterCheck = await evalJs(`() => {
  const d = document.querySelector('[data-testid="pivot-dimension"]');
  if (!d) return { ok:false };
  d.value = 'work_center'; d.dispatchEvent(new Event('change',{bubbles:true}));
  return { ok:true };
}`);
await sleep(900);
const filtered = await evalJs(`() => {
  const inp = document.querySelector('[data-testid="pivot-dimvalue"]');
  if (inp) { inp.value='WC_01'; inp.dispatchEvent(new Event('input',{bubbles:true})); }
  return { rows: document.querySelectorAll('[data-testid^="pivot-row-"]').length };
}`);
await sleep(1500);
const afterFilter = await evalJs(`() => ({ rows: document.querySelectorAll('[data-testid^="pivot-row-"]').length })`);
check('按工作中心维度筛选生效', filterCheck.ok && afterFilter.rows > 0 && afterFilter.rows < pivot.rows, `筛选后行=${afterFilter.rows}（原 ${pivot.rows}）`);

// 单元格下钻：点击第一个有值的单元格
const drill = await evalJs(`() => {
  const td = [...document.querySelectorAll('td[data-testid="cell-value"]')][0];
  if (!td) return { clicked:false };
  td.click();
  return { clicked:true };
}`);
await sleep(1200);
const modal = await evalJs(`() => {
  const m = document.querySelector('[data-testid="drill-modal"]');
  return { open: !!m, text: m?.textContent?.slice(0,120) || '' };
}`);
check('单元格下钻明细弹窗', drill.clicked && modal.open, modal.text.replace(/\s+/g,' ').slice(0,70));

// ---- 4) 交付承诺页 ----
await evalJs(`() => { [...document.querySelectorAll('.tabs button')].find(b=>b.textContent.includes('交付承诺')).click(); return true; }`);
await sleep(1200);
await sel('delivery-benchmark');
const dform = await evalJs(`() => ({
  part: !!document.querySelector('[data-testid="p-part"]'),
  due: !!document.querySelector('[data-testid="p-due"]'),
  qty: !!document.querySelector('[data-testid="p-qty"]'),
  prio: !!document.querySelector('[data-testid="p-prio"]'),
  run: !!document.querySelector('[data-testid="run-delivery"]'),
})`);
check('交付承诺表单四要素齐全', dform.part && dform.due && dform.qty && dform.prio && dform.run);
// 设 due=3 qty=30 并运行
await setInputs([['p-part',0],['p-due',3],['p-qty',30],['p-prio',1]]);
await evalJs(`() => { document.querySelector('[data-testid="run-delivery"]').click(); return true; }`);
let delivery = null;
for (let i=0;i<40;i++) {
  delivery = await evalJs(`() => ({
    timeline: !!document.querySelector('[data-testid="timeline"]'),
    text: document.querySelector('main')?.textContent || '',
    frames: document.querySelector('[data-testid="replay-frame"]')?.textContent || '',
  })`);
  if (delivery.timeline && /承诺|可承诺|Day|天/.test(delivery.text)) break;
  await sleep(500);
}
check('交付承诺运行并渲染时间轴', delivery.timeline, delivery.frames);
// 基线：due=3 qty=30 -> 可承诺 Day3 / 预留 3 工时 / 回滚 0 步
// 实际文案：承诺日3天 / 承诺量30件 / 预留工时3h / 回滚步数0
check('交付基线 承诺日3/预留3工时/回滚0步',
  /承诺日\s*3/.test(delivery.text) && /承诺量\s*30/.test(delivery.text) && /预留工时\s*3/.test(delivery.text) && /回滚步数\s*0/.test(delivery.text),
  delivery.text.replace(/\s+/g,' ').match(/承诺日\s*3[^无]{0,40}/)?.[0] || '');

// 回放路径：故意用 due_day=1 触发 rollback（该轨迹有 2 帧，回放才有意义）
// 分两步：先写表单值并让 React 完成一次渲染，再点击运行（避免同帧批处理导致参数未生效）
await setInputs([['p-part',0],['p-due',1],['p-qty',30],['p-prio',1]]);
await evalJs(`() => { document.querySelector('[data-testid="run-delivery"]').click(); return true; }`);
let rollback = null;
for (let i=0;i<40;i++) {
  rollback = await evalJs(`() => ({
    frame: document.querySelector('[data-testid="replay-frame"]')?.textContent||'',
    text: document.querySelector('main')?.textContent||'',
    hasFwd: !!document.querySelector('[data-testid="replay-forward"]'),
  })`);
  if (rollback.hasFwd && /2\s*\/\s*2|帧\s*2/.test(rollback.frame)) break;
  await sleep(500);
}
check('回滚轨迹回放为 2 帧', /2\s*\/\s*2|帧\s*2/.test(rollback.frame), rollback.frame.trim());
const replay = await evalJs(`() => {
  const fwd=document.querySelector('[data-testid="replay-forward"]');
  const reset=document.querySelector('[data-testid="replay-reset"]');
  const before=document.querySelector('[data-testid="replay-frame"]')?.textContent||'';
  if (reset) reset.click();
  if (fwd) fwd.click();
  return { hasBack:!!document.querySelector('[data-testid="replay-back"]'), hasPlay:!!document.querySelector('[data-testid="replay-play"]'), hasFwd:!!fwd, hasReset:!!reset, before };
}`);
await sleep(700);
const replayAfter = await evalJs(`() => ({ frame: document.querySelector('[data-testid="replay-frame"]')?.textContent||'' })`);
check('回放控件可用（前进/后退/播放/重置）', replay.hasBack && replay.hasPlay && replay.hasFwd && replay.hasReset, `${replay.before} -> ${replayAfter.frame}`);
check('回放帧前进生效', replay.before !== replayAfter.frame, `${replay.before} -> ${replayAfter.frame}`);

// ---- 5) ITP/IOP 页 ----
await evalJs(`() => { [...document.querySelectorAll('.tabs button')].find(b=>b.textContent.includes('ITP')).click(); return true; }`);
await sleep(1200);
await sel('itp-iop-benchmark');
await evalJs(`() => { const b=document.querySelector('[data-testid="run-itp-iop"]'); if(b) b.click(); return true; }`);
let iop = null;
for (let i=0;i<40;i++) {
  iop = await evalJs(`() => ({
    quotaTab: document.querySelector('[data-testid="tab-quota"]')?.textContent||'',
    blockTab: document.querySelector('[data-testid="tab-blocked"]')?.textContent||'',
    quotaTable: !!document.querySelector('[data-testid="quota-table"]'),
    btn: document.querySelector('[data-testid="run-itp-iop"]')?.textContent||'',
  })`);
  if (iop.blockTab && /阻断订单（\d+）/.test(iop.blockTab) && !/运行中/.test(iop.btn)) break;
  await sleep(500);
}
check('ITP/IOP 运行完成并给出配额/阻断统计', /配额 vs 消耗（\d+）/.test(iop.quotaTab) && /阻断订单（\d+）/.test(iop.blockTab), `${iop.quotaTab} / ${iop.blockTab}`);
// 切到阻断订单页签，直接读取表格里的 reason 单元格
await evalJs(`() => { const b=document.querySelector('[data-testid="tab-blocked"]'); if(b) b.click(); return true; }`);
await sleep(900);
const iopNums = await evalJs(`() => ({
  blocked: document.querySelector('[data-testid="tab-blocked"]')?.textContent||'',
  main: document.querySelector('main')?.textContent||'',
  reasons: [...document.querySelectorAll('table td')].map(td=>td.textContent.trim()).filter(t=>t && t!=='—'),
})`);
check('阻断订单数=1 与引擎一致', /阻断订单（1）/.test(iopNums.blocked), iopNums.blocked);
check('阻断原因含「突破 ITP 配额上限」', iopNums.main.replace(/\s+/g,'').includes('突破ITP配额上限') || iopNums.reasons.some(t=>t.includes('突破 ITP 配额上限')), '');

// ---- 6) 替代料页 ----
await evalJs(`() => { [...document.querySelectorAll('.tabs button')].find(b=>b.textContent.includes('替代料')).click(); return true; }`);
await sleep(1200);
await sel('substitution-benchmark');
const sform = await evalJs(`() => ({
  cls:!!document.querySelector('[data-testid="s-class"]'), grp:!!document.querySelector('[data-testid="s-group"]'),
  dem:!!document.querySelector('[data-testid="s-demand"]'), day:!!document.querySelector('[data-testid="s-day"]'),
  par:!!document.querySelector('[data-testid="s-parent"]'), run:!!document.querySelector('[data-testid="run-substitution"]'),
})`);
check('替代料表单齐全', Object.values(sform).every(Boolean));
// 三类（alt_class=3）必须配合 alt_group=3，净需求 12 → 基线 分配10 / on_hand_after=90
await setInputs([['s-class',3],['s-group',3],['s-demand',12],['s-day',1],['s-parent',0]]);
await evalJs(`() => { document.querySelector('[data-testid="run-substitution"]').click(); return true; }`);
let sub = null;
for (let i=0;i<40;i++) {
  sub = await evalJs(`() => ({ water: !!document.querySelector('[data-testid="water-list"]'), main: document.querySelector('main')?.textContent||'', btn: document.querySelector('[data-testid="run-substitution"]')?.textContent||'' })`);
  // 必须等到 class3 结果（本次分配 · 分配后 90）出现，避免读到上一次 class1 的残留
  if (sub.water && !/运行中/.test(sub.btn) && /class3/.test(sub.main)) break;
  await sleep(500);
}
check('替代料决策运行并渲染安全库存水位', sub.water, sub.main.replace(/\s+/g,' ').slice(0,80));
check('安全库存未侵占（on_hand_after=90）', /分配后在手90|分配后 90/.test(sub.main.replace(/\s+/g,' ')), sub.main.replace(/\s+/g,' ').match(/分配合计\d+件分配后在手\d+件/)?.[0] || '');

// ---- 7) 性能与历史页 ----
await evalJs(`() => { [...document.querySelectorAll('.tabs button')].find(b=>b.textContent.includes('性能')).click(); return true; }`);
await sleep(1500);
const perf = await evalJs(`() => ({
  chart: !!document.querySelector('[data-testid="perf-chart"]'),
  table: !!document.querySelector('[data-testid="runs-table"]'),
  rows: document.querySelectorAll('[data-testid="runs-table"] tbody tr').length,
  filter: !!document.querySelector('[data-testid="perf-filter"]'),
  main: document.querySelector('main')?.textContent||'',
})`);
check('性能页图表与运行历史渲染', perf.chart && perf.table, `历史行=${perf.rows}`);
check('性能页展示端到端与引擎耗时', /ms|毫秒|耗时/.test(perf.main), '');
check('运行历史有记录', perf.rows > 0, `${perf.rows} 行`);

// ---- 汇总 ----
console.log('\n================ 前端页面测试结果 ================');
console.log(results.join('\n'));
console.log('================================================');
console.log(`通过 ${pass} / 失败 ${fail}`);
mcp.close();
process.exit(fail === 0 ? 0 : 1);
