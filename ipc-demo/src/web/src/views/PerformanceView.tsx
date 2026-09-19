import React, { useEffect, useState } from 'react';
import { BarChart, Bar as RBar, XAxis, YAxis, Tooltip, Legend, ResponsiveContainer, CartesianGrid } from 'recharts';
import api from '../lib/api';
import { useApp } from '../App';
import { Badge, DataTable, Empty, ErrorBox, Kpi, Loading } from '../components/common';

// 性能与历史视图：端到端耗时 vs C++ 自报耗时、处理条数、吞吐量、历史对比。
export default function PerformanceView() {
  const { scenarioId } = useApp();
  const [runs, setRuns] = useState<any[] | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [selected, setSelected] = useState<any>(null);
  const [detail, setDetail] = useState<any>(null);
  const [engineFilter, setEngineFilter] = useState('');

  useEffect(() => {
    if (!scenarioId) return;
    setRuns(null); setSelected(null); setDetail(null); setError(null);
    api.runs(scenarioId, engineFilter || undefined)
      .then((d) => { setRuns(d.runs); const first = d.runs[0]; if (first) select(first); })
      .catch((e) => setError(e.message));
  }, [scenarioId, engineFilter]);

  async function select(run: any) {
    setSelected(run);
    try { setDetail(await api.runDetail(run.run_id)); } catch (e: any) { setError(e.message); }
  }

  if (!scenarioId) return <Empty>请选择一个测试场景</Empty>;

  const list = runs ?? [];
  const chartData = list.slice(0, 20).reverse().map((r) => ({
    name: `${r.engine_type}-${String(r.run_id).slice(0, 4)}`,
    endToEnd: r.duration_ms,
    cpp: r.engine_duration_ms,
  }));
  const rowsProcessed = detail ? detail.outputs.reduce((s: number, o: any) => s + Number(o.rowCount), 0) : 0;
  const throughput = detail?.duration_ms ? (rowsProcessed / (Number(detail.duration_ms) / 1000)) : null;

  return (
    <div>
      <div className="panel">
        <h2>性能与运行历史</h2>
        <div className="sub">端到端耗时 = Node 编排 + CSV 往返 + 子进程；C++ 自报耗时来自 CLI 的 duration_ms，两者分别展示</div>
        <div className="row" style={{ gap: 12, alignItems: 'flex-end' }}>
          <label>
            <div className="hint">引擎类型筛选</div>
            <select value={engineFilter} onChange={(e) => setEngineFilter(e.target.value)} data-testid="perf-filter">
              <option value="">全部</option>
              {['delivery', 'itp', 'iop', 'substitution'].map((e) => <option key={e} value={e}>{e}</option>)}
            </select>
          </label>
          <span className="hint">共 {list.length} 次运行</span>
        </div>
        <ErrorBox error={error} />
      </div>

      {runs === null ? <Loading what="运行历史" /> : list.length === 0 ? (
        <Empty>该场景暂无运行记录</Empty>
      ) : (
        <>
          <div className="panel">
            <h2>历史运行耗时对比</h2>
            <div style={{ width: '100%', height: 280 }} data-testid="perf-chart">
              <ResponsiveContainer>
                <BarChart data={chartData} margin={{ top: 8, right: 16, bottom: 8, left: 0 }}>
                  <CartesianGrid strokeDasharray="3 3" stroke="#e6ebf0" />
                  <XAxis dataKey="name" tick={{ fontSize: 11 }} />
                  <YAxis tick={{ fontSize: 11 }} unit="ms" />
                  <Tooltip />
                  <Legend />
                  <RBar dataKey="endToEnd" name="端到端 (ms)" fill="#0a6ebd" />
                  <RBar dataKey="cpp" name="C++ 自报 (ms)" fill="#f0a14a" />
                </BarChart>
              </ResponsiveContainer>
            </div>
          </div>

          <div className="grid-2">
            <div className="panel">
              <h2>运行列表（点击加载 run_id）</h2>
              <div className="pivot-wrap" style={{ maxHeight: 360 }}>
                <table className="data" data-testid="runs-table">
                  <thead><tr><th>时间</th><th>引擎</th><th>状态</th><th>端到端</th><th>C++</th></tr></thead>
                  <tbody>
                    {list.map((r) => (
                      <tr key={r.run_id} onClick={() => select(r)} style={{ cursor: 'pointer', background: selected?.run_id === r.run_id ? 'var(--brand-soft)' : undefined }}>
                        <td>{new Date(r.created_at).toLocaleString()}</td>
                        <td>{r.engine_type}</td>
                        <td><Badge tone={r.status === 'succeeded' ? 'ok' : 'bad'}>{r.status}</Badge></td>
                        <td className="num">{r.duration_ms} ms</td>
                        <td className="num">{r.engine_duration_ms ?? '—'} ms</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>

            <div className="panel">
              <h2>单次运行性能</h2>
              {!detail ? <Loading what="运行详情" /> : (
                <>
                  <div className="kpis">
                    <Kpi label="端到端耗时" value={detail.duration_ms} unit="ms" />
                    <Kpi label="C++ 自报耗时" value={detail.engine_duration_ms ?? '—'} unit="ms" />
                    <Kpi label="处理条数" value={rowsProcessed} unit="行" />
                    <Kpi label="吞吐量" value={throughput ? Math.round(throughput) : '—'} unit="行/秒" />
                  </div>
                  <div className="row" style={{ gap: 16, marginTop: 12 }}>
                    <span className="hint">run_id: {detail.run_id}</span>
                    <span className="hint">退出码: {detail.exit_code}</span>
                    <span className="hint">参数: {detail.params_json}</span>
                  </div>
                  <h3>输出数据集</h3>
                  <DataTable columns={['key', 'label', 'rowCount']} rows={detail.outputs} maxHeight={220} />
                </>
              )}
            </div>
          </div>
        </>
      )}
    </div>
  );
}
