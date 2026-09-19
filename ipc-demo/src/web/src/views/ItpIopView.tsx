import React, { useEffect, useState } from 'react';
import api from '../lib/api';
import { useApp } from '../App';
import { Badge, DataTable, Empty, ErrorBox, Kpi } from '../components/common';

// ITP / IOP 协同视图：四维约束键配额 vs 消耗、阻断订单明细与聚合指标。
export default function ItpIopView() {
  const { scenarioId, reloadScenarios } = useApp();
  const [running, setRunning] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [bufferFactor, setBufferFactor] = useState(1.1);
  const [itpRun, setItpRun] = useState<any>(null);
  const [iopRun, setIopRun] = useState<any>(null);
  const [allotments, setAllotments] = useState<any[]>([]);
  const [orders, setOrders] = useState<any[]>([]);
  const [summary, setSummary] = useState<any>(null);
  const [runs, setRuns] = useState<any[]>([]);
  const [tab, setTab] = useState<'quota' | 'blocked'>('quota');

  useEffect(() => { load(); }, [scenarioId]);

  async function load() {
    if (!scenarioId) return;
    setError(null); setAllotments([]); setOrders([]); setSummary(null);
    const d = await api.runs(scenarioId);
    setRuns(d.runs);
    const itp = d.runs.find((r: any) => r.engine_type === 'itp' && r.status === 'succeeded');
    const iop = d.runs.find((r: any) => r.engine_type === 'iop' && r.status === 'succeeded');
    setItpRun(itp ?? null); setIopRun(iop ?? null);
    if (itp) api.runOutputs(itp.run_id, 'itp_allotments').then((r) => setAllotments(r.rows)).catch(() => {});
    if (iop) {
      api.runOutputs(iop.run_id, 'iop_orders').then((r) => setOrders(r.rows)).catch(() => {});
      api.runOutputs(iop.run_id, 'iop_summary').then((r) => setSummary(r.rows[0] ?? null)).catch(() => {});
    }
  }

  async function runBoth() {
    if (!scenarioId) return;
    setRunning(true); setError(null);
    try {
      await api.run('itp', { scenarioId, buffer_factor: bufferFactor });
      await api.run('iop', { scenarioId });
      await load();
      await reloadScenarios();
    } catch (e: any) {
      setError(`${e.message}${e.kind ? `（${e.kind}）` : ''}；已保留参数以便重试`);
    } finally { setRunning(false); }
  }

  if (!scenarioId) return <Empty>请选择一个测试场景</Empty>;

  const blocked = orders.filter((o) => String(o.status).toLowerCase() === 'blocked' || o.status === '阻断');
  const scheduled = orders.filter((o) => !blocked.includes(o));
  const nearLimit = allotments.filter((a) => Number(a.total_quota) > 0 && Number(a.consumed_qty) / Number(a.total_quota) >= 0.8);

  return (
    <div>
      <div className="panel">
        <h2>ITP / IOP 协同（主计划配额 vs 执行计划消耗）</h2>
        <div className="sub">约束键 day / family_id / cust_group_id / region_id 由 C++ CLI 计算并写入 CSV，Node 与前端不重算</div>
        <div className="row" style={{ gap: 12, alignItems: 'flex-end' }}>
          <label>
            <div className="hint">主计划缓冲系数 (buffer_factor)</div>
            <input type="number" step="0.01" value={bufferFactor} onChange={(e) => setBufferFactor(Number(e.target.value))}
              style={{ padding: '5px 8px', border: '1px solid var(--line-strong)', borderRadius: 4, width: 120 }} data-testid="iop-buffer" />
          </label>
          <button className="btn" onClick={runBoth} disabled={running} data-testid="run-itp-iop">
            {running ? '运行中…' : '运行 ITP → IOP'}
          </button>
          {running && <span className="hint">先运行 ITP 生成配额，再运行 IOP 做执行对齐…</span>}
        </div>
        <ErrorBox error={error} />
      </div>

      {summary && (
        <div className="panel">
          <h2>聚合指标</h2>
          <div className="kpis">
            <Kpi label="总订单数" value={summary.total_orders} />
            <Kpi label="已下派" value={summary.scheduled_orders} tone="ok" />
            <Kpi label="已阻断" value={summary.blocked_orders} tone={Number(summary.blocked_orders) > 0 ? 'bad' : 'ok'} />
            <Kpi label="履行量" value={summary.total_fulfilled_qty} unit="件" />
            <Kpi label="配额消耗率" value={`${Math.round(Number(summary.quota_utilization) * 10000) / 100}%`} tone={Number(summary.quota_utilization) >= 0.95 ? 'warn' : undefined} />
          </div>
        </div>
      )}

      <div className="panel">
        <div className="category-tabs">
          <button className={tab === 'quota' ? 'active' : ''} onClick={() => setTab('quota')} data-testid="tab-quota">配额 vs 消耗（{allotments.length}）</button>
          <button className={tab === 'blocked' ? 'active' : ''} onClick={() => setTab('blocked')} data-testid="tab-blocked">阻断订单（{blocked.length}）</button>
        </div>

        {tab === 'quota' && (
          <>
            <div className="legend">
              <span><span className="sw" style={{ background: '#2f8fd8' }} />已消耗</span>
              <span><span className="sw" style={{ background: '#eef1f5' }} />配额上限</span>
              <span>≥80% 视为临近上限，≥100% 视为达到上限</span>
            </div>
            {allotments.length === 0 ? <Empty>该场景暂无 ITP 配额数据，请先运行 ITP</Empty> : (
              <>
                {nearLimit.length > 0 && (
                  <div className="hint" style={{ marginBottom: 8 }}>
                    临近/达到上限的约束键：{nearLimit.map((a) => `${a.day}/${a.family_id}/${a.cust_group_id}/${a.region_id}`).join('、')}
                  </div>
                )}
                <div className="pivot-wrap" style={{ maxHeight: 400 }}>
                  <table className="data" data-testid="quota-table">
                    <thead>
                      <tr><th>day</th><th>family_id</th><th>cust_group</th><th>region</th><th>配额上限</th><th>已消耗</th><th>消耗率</th><th style={{ width: 180 }}>水位</th></tr>
                    </thead>
                    <tbody>
                      {allotments.map((a, i) => {
                        const ratio = Number(a.total_quota) ? Number(a.consumed_qty) / Number(a.total_quota) : 0;
                        const tone = ratio >= 1 ? 'bad' : ratio >= 0.8 ? 'warn' : 'muted';
                        return (
                          <tr key={i}>
                            <td className="num">{a.day}</td>
                            <td className="num">{a.family_id}</td>
                            <td className="num">{a.cust_group_id}</td>
                            <td className="num">{a.region_id}</td>
                            <td className="num">{a.total_quota}</td>
                            <td className="num">{a.consumed_qty}</td>
                            <td className="num"><Badge tone={tone as any}>{Math.round(ratio * 10000) / 100}%</Badge></td>
                            <td>
                              <div className="bar">
                                <div className="fill consumed" style={{ width: `${Math.min(100, ratio * 100)}%` }} />
                              </div>
                            </td>
                          </tr>
                        );
                      })}
                    </tbody>
                  </table>
                </div>
                <div className="hint" style={{ marginTop: 8 }}>说明：配额余量 = 配额上限 − 已消耗。四维键由 C++ 侧同一二进制计算，避免跨进程 hash 不一致。</div>
              </>
            )}
          </>
        )}

        {tab === 'blocked' && (
          blocked.length === 0 ? <Empty>该场景无阻断订单</Empty> : (
            <>
              <div className="legend"><span>被刚性阻断的执行需求，突破其约束键配额上限</span></div>
              <DataTable columns={['demand_id', 'part_id', 'due_day', 'qty', 'priority', 'family_id', 'cust_group_id', 'region_id', 'status', 'reason']} rows={blocked} />
            </>
          )
        )}
      </div>

      <div className="grid-2">
        <div className="panel">
          <h2>已下派订单（{scheduled.length}）</h2>
          {scheduled.length === 0 ? <Empty>无下派订单</Empty> : (
            <DataTable columns={['demand_id', 'part_id', 'due_day', 'qty', 'priority', 'family_id', 'cust_group_id', 'region_id', 'status', 'reason']} rows={scheduled} />
          )}
        </div>
        <div className="panel">
          <h2>运行元数据</h2>
          <div className="hint">ITP run: {itpRun?.run_id ?? '—'}</div>
          <div className="hint">IOP run: {iopRun?.run_id ?? '—'}</div>
          <div className="hint">历史运行数：{runs.length}</div>
        </div>
      </div>
    </div>
  );
}
