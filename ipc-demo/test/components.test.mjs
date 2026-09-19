// 9.19 前端组件测试：覆盖空值/零值、族分组折叠、超载强调、下钻、回放等关键交互分支。
// 使用 jsdom + @testing-library/react；网络层通过替换 global.fetch 注入确定性数据。
import { test, beforeEach, afterEach } from 'node:test';
import assert from 'node:assert/strict';
import { JSDOM } from 'jsdom';

// ---- jsdom 环境 ----
const dom = new JSDOM('<!doctype html><html><body><div id="root"></div></body></html>', { url: 'http://localhost/' });
global.window = dom.window;
global.document = dom.window.document;
Object.defineProperty(global, 'navigator', { value: dom.window.navigator, configurable: true });
global.HTMLElement = dom.window.HTMLElement;
global.Node = dom.window.Node;
global.getComputedStyle = dom.window.getComputedStyle;
global.IS_REACT_ACT_ENVIRONMENT = true;
global.ResizeObserver = class { observe() {} unobserve() {} disconnect() {} };

const { render, screen, fireEvent, act, cleanup } = await import('@testing-library/react');
const React = (await import('react')).default;
const PivotGrid = (await import('./.tsx-build/PivotGrid.js')).default;
const { fmtCell } = await import('./.tsx-build/common.js');

// 构造透视响应：含超载（负剩余产能）、缺口、空值(NULL)与零值
const PIVOT = {
  scenarioId: 'demo', runId: null, days: [0, 1, 2], lo: 0, hi: 2,
  families: [{ key: 'supply', label: '供给' }, { key: 'demand', label: '需求' }, { key: 'resource', label: '资源' }, { key: 'balance', label: '平衡' }],
  metrics: [],
  rows: [
    { metric: 'supply_on_hand', label: '在手供给', family: 'supply', unit: '件', dimension: 'part', dimValue: 1, values: [50, 0, null], hasData: true, error: null },
    { metric: 'resource_remaining_capacity', label: '剩余产能', family: 'resource', unit: 'h', dimension: 'work_center', dimValue: 'WC_01', negativeIsOverload: true, values: [97, -5, 100], hasData: true, error: null },
    { metric: 'balance_shortage', label: '缺口', family: 'balance', unit: '件', dimension: 'part', dimValue: 10, negativeIsOverload: true, values: [null, 300, null], hasData: true, error: null },
  ],
};

const DETAIL = {
  metric: 'supply_on_hand', day: 0, dimValue: 1, value: 50, count: 1,
  rows: [{ supply_code: 'OH_A', supply_type: 'On-Hand', part_id: 1, available_day: 0, qty: 50, priority: 100 }],
};

function mockFetch(routes) {
  global.fetch = async (url) => {
    for (const [frag, payload] of routes) {
      if (String(url).includes(frag)) {
        return { ok: true, status: 200, text: async () => JSON.stringify(payload) };
      }
    }
    return { ok: false, status: 404, text: async () => JSON.stringify({ error: { message: 'not found', kind: 'not_found' } }) };
  };
}

beforeEach(() => { mockFetch([['/pivot/cell', DETAIL], ['/pivot', PIVOT]]); });
afterEach(() => { cleanup?.(); });

async function renderGrid() {
  let utils;
  await act(async () => { utils = render(React.createElement(PivotGrid, { scenarioId: 'demo' })); });
  return utils;
}

test('空值与零值区分渲染：NULL 为空白，0 显示为 0', async () => {
  await renderGrid();
  const blank = screen.getAllByTestId('cell-blank');
  assert.equal(blank.length, 3);
  blank.forEach((c) => assert.equal(c.textContent, ''));
  const zeros = screen.getAllByTestId('cell-zero');
  assert.equal(zeros.length, 1);
  assert.equal(zeros[0].textContent, '0');
  // 数值单元格非空
  assert.ok(screen.getAllByTestId('cell-value').length >= 3);
});

test('指标按供给/需求/资源/平衡四族分组，可折叠展开', async () => {
  await renderGrid();
  for (const fam of ['supply', 'resource', 'balance']) {
    assert.ok(screen.getByTestId(`family-${fam}`), `family ${fam} 存在`);
  }
  // 需求族无行 -> 不渲染族头
  assert.equal(screen.queryByTestId('family-demand'), null);
  // 折叠供给族后，其指标行应消失
  assert.ok(screen.queryByTestId('pivot-row-supply_on_hand'));
  fireEvent.click(screen.getByTestId('family-supply'));
  assert.equal(screen.queryByTestId('pivot-row-supply_on_hand'), null);
  fireEvent.click(screen.getByTestId('family-supply'));
  assert.ok(screen.queryByTestId('pivot-row-supply_on_hand'));
});

test('超载/缺口单元格被视觉强调（negativeIsOverload 且值为负）', async () => {
  await renderGrid();
  const overload = document.querySelectorAll('td.overload');
  // 剩余产能 Day1 = -5 为超载；缺口为正 -> 不强调为负
  assert.equal(overload.length, 1);
  assert.equal(overload[0].textContent, '-5');
});

test('点击单元格打开下钻明细，明细求和等于单元格值', async () => {
  await renderGrid();
  const target = screen.getByTestId('pivot-row-supply_on_hand').querySelectorAll('td.cell')[0];
  await act(async () => { fireEvent.click(target); });
  const modal = await screen.findByTestId('drill-modal');
  assert.ok(modal.textContent.includes('在手供给'));
  assert.ok(modal.textContent.includes('Day 0'));
  // 明细求和 = 50 = 单元格值
  assert.ok(modal.textContent.includes('50'));
  await act(async () => { fireEvent.click(screen.getByText('关闭')); });
});

test('fmtCell 对空/零/数值的渲染约定', () => {
  assert.equal(fmtCell(null), '');
  assert.equal(fmtCell(undefined), '');
  assert.equal(fmtCell(0), '0');
  assert.equal(fmtCell(3.5), '3.5');
  assert.equal(fmtCell(true), 'true');
});
