import React, { useMemo, useState } from 'react';
import api, { PivotResponse, PivotRow } from '../lib/api';
import { Loading, Empty, ErrorBox } from './common';

const FAMILY_LABEL: Record<string, string> = { supply: '供给', demand: '需求', resource: '资源', balance: '平衡' };
const FAMILY_ORDER = ['supply', 'demand', 'resource', 'balance'];

function fmt(v: number | null): string {
  if (v === null || v === undefined) return '';
  if (Number.isInteger(v)) return String(v);
  return String(Math.round(v * 1000) / 1000);
}

interface Props {
  scenarioId: string;
  runId?: string;
  dimValue?: string;
  dimension?: string;
}

// 计划透视网格：行 = 指标 × 维度成员，列 = 天；按族分组可折叠；点击单元格下钻。
export default function PivotGrid({ scenarioId, runId, dimValue, dimension }: Props) {
  const [data, setData] = useState<PivotResponse | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(true);
  const [collapsed, setCollapsed] = useState<Record<string, boolean>>({});
  const [drill, setDrill] = useState<{ row: PivotRow; day: number } | null>(null);

  React.useEffect(() => {
    let cancelled = false;
    setLoading(true); setError(null);
    api.pivot(scenarioId, { runId, dimValue, dimension })
      .then((d) => { if (!cancelled) { setData(d); setLoading(false); } })
      .catch((e) => { if (!cancelled) { setError(e.message); setLoading(false); } });
    return () => { cancelled = true; };
  }, [scenarioId, runId, dimValue, dimension]);

  const grouped = useMemo(() => {
    if (!data) return [];
    return FAMILY_ORDER
      .map((f) => ({ key: f, rows: data.rows.filter((r) => r.family === f) }))
      .filter((g) => g.rows.length > 0);
  }, [data]);

  if (loading) return <Loading what="计划透视" />;
  if (error) return <ErrorBox error={error} />;
  if (!data || data.days.length === 0) {
    return <Empty>该场景暂无可推导的时间范围（无需求/供给/产能数据）</Empty>;
  }

  return (
    <div>
      <div className="legend">
        <span><span className="sw" style={{ background: '#fff', border: '1px solid var(--line-strong)' }} />空白 = 无计划(NULL)</span>
        <span><span className="sw" style={{ background: '#f5f6f8' }} />0 = 计划为零</span>
        <span><span className="sw" style={{ background: 'var(--bad-soft)' }} />超载 / 缺口</span>
        <span className="hint">天粒度 · 共 {data.days.length} 天（Day {data.lo} – Day {data.hi}）· 点击单元格查看明细</span>
      </div>
      <div className="pivot-wrap" data-testid="pivot-grid">
        <table className="pivot">
          <thead>
            <tr>
              <th className="metric-col">关键指标</th>
              {data.days.map((d) => <th key={d} style={{ textAlign: 'right' }}>Day {d}</th>)}
            </tr>
          </thead>
          <tbody>
            {grouped.map((g) => (
              <React.Fragment key={g.key}>
                <tr className="family-head" onClick={() => setCollapsed((c) => ({ ...c, [g.key]: !c[g.key] }))} data-testid={`family-${g.key}`}>
                  <td className="metric-col">
                    <span className="caret">{collapsed[g.key] ? '▸' : '▾'}</span>
                    {FAMILY_LABEL[g.key] || g.key}（{g.rows.length}）
                  </td>
                  <td colSpan={data.days.length} />
                </tr>
                {!collapsed[g.key] && g.rows.map((row, idx) => (
                  <tr key={`${row.metric}-${row.dimValue}-${idx}`} data-testid={`pivot-row-${row.metric}`} data-has-data={row.hasData}>
                    <td className="metric-col">
                      {row.label}
                      {row.dimension !== 'none' && row.dimValue !== null && (
                        <span className="day-dim"> · {row.dimension}={row.dimValue}</span>
                      )}
                      <span className="day-dim"> ({row.unit})</span>
                    </td>
                    {row.values.map((v, i) => {
                      const isBlank = v === null || v === undefined;
                      const overload = !isBlank && row.negativeIsOverload && v < 0;
                      const cls = ['num', 'cell'];
                      if (isBlank) cls.push('blank');
                      else if (v === 0) cls.push('zero');
                      if (overload) cls.push('overload');
                      return (
                        <td
                          key={i}
                          className={cls.join(' ')}
                          data-testid={isBlank ? 'cell-blank' : v === 0 ? 'cell-zero' : 'cell-value'}
                          data-day={data.days[i]}
                          title={isBlank ? '无计划' : `${fmt(v)} — 点击下钻`}
                          onClick={() => !isBlank && setDrill({ row, day: data.days[i] })}
                        >
                          {fmt(v)}
                        </td>
                      );
                    })}
                  </tr>
                ))}
              </React.Fragment>
            ))}
          </tbody>
        </table>
      </div>
      {data.rows.some((r) => r.error) && (
        <div className="hint" style={{ marginTop: 8, color: 'var(--warn)' }}>
          部分指标当前无可用运行结果：{data.rows.filter((r) => r.error).map((r) => r.label).join('、')}
        </div>
      )}
      {drill && <DrillModal scenarioId={scenarioId} runId={runId} row={drill.row} day={drill.day} onClose={() => setDrill(null)} />}
    </div>
  );
}

function DrillModal({ scenarioId, runId, row, day, onClose }: {
  scenarioId: string; runId?: string; row: PivotRow; day: number; onClose: () => void;
}) {
  const [detail, setDetail] = useState<any>(null);
  const [error, setError] = useState<string | null>(null);

  React.useEffect(() => {
    api.cell(scenarioId, { metric: row.metric, day, dimValue: row.dimValue, runId })
      .then(setDetail).catch((e) => setError(e.message));
  }, [scenarioId, runId, row.metric, day, row.dimValue]);

  const cellValue = row.values[day - (detail?.day !== undefined ? 0 : 0)];
  return (
    <div className="modal-backdrop" onClick={onClose}>
      <div className="modal" onClick={(e) => e.stopPropagation()} data-testid="drill-modal">
        <header>
          <h3>{row.label} · Day {day}{row.dimValue !== null ? ` · ${row.dimension}=${row.dimValue}` : ''}</h3>
          <button className="btn ghost small" onClick={onClose}>关闭</button>
        </header>
        <div className="body">
          {!detail && !error && <Loading what="明细" />}
          {error && <ErrorBox error={error} />}
          {detail && (
            <>
              <div className="kpis" style={{ marginBottom: 12 }}>
                <div className="kpi"><div className="k">单元格值</div><div className="v">{fmt(detail.value)}</div></div>
                <div className="kpi"><div className="k">明细行数</div><div className="v">{detail.count}</div></div>
                <div className="kpi"><div className="k">明细求和</div><div className="v">{fmt(detail.rows.reduce((a: number, r: any) => a + Number(pickValue(row.metric, r) ?? 0), 0))}</div></div>
              </div>
              {detail.rows.length === 0 ? <Empty>该单元格无底层明细记录</Empty> : (
                <div className="pivot-wrap" style={{ maxHeight: 380 }}>
                  <table className="data">
                    <thead><tr>{Object.keys(detail.rows[0]).map((c) => <th key={c}>{c}</th>)}</tr></thead>
                    <tbody>
                      {detail.rows.map((r: any, i: number) => (
                        <tr key={i}>{Object.entries(r).map(([k, v]) => <td key={k} className={typeof v === 'number' ? 'num' : undefined}>{v === null || v === undefined ? '' : String(v)}</td>)}</tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              )}
            </>
          )}
        </div>
      </div>
    </div>
  );
}

function pickValue(metric: string, row: any): number | null {
  switch (metric) {
    case 'resource_available_capacity':
    case 'resource_remaining_capacity': return row.capacity_hours;
    case 'resource_consumed_capacity': return row.total_capacity_used;
    case 'balance_committed_qty': return row.promised_qty;
    case 'balance_quota_total': return row.total_quota;
    case 'balance_quota_consumed': return row.consumed_qty;
    default: return row.qty ?? 0;
  }
}
