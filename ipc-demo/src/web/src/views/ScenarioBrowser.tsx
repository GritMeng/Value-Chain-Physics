import React, { useEffect, useState } from 'react';
import api from '../lib/api';
import { useApp } from '../App';
import { DataTable, Empty, ErrorBox, Loading, Badge } from '../components/common';

// 计划供给 = atp_supply 中类型为计划订单的节点（用户视角的统一类别）
const EXTRA_CATEGORIES = [
  { key: 'planned_supply', label: '计划供给', datasets: ['atp_supply'] },
];

export default function ScenarioBrowser() {
  const { scenarioId, scenarios } = useApp();
  const [overview, setOverview] = useState<any[] | null>(null);
  const [activeCat, setActiveCat] = useState<string>('demand');
  const [activeDs, setActiveDs] = useState<{ key: string; label: string; file: string } | null>(null);
  const [rows, setRows] = useState<any[] | null>(null);
  const [columns, setColumns] = useState<string[]>([]);
  const [error, setError] = useState<string | null>(null);
  const [raw, setRaw] = useState<{ filename: string; content: string } | null>(null);
  const [runs, setRuns] = useState<any[]>([]);
  const [activeRun, setActiveRun] = useState<any>(null);
  const [runOutputs, setRunOutputs] = useState<any[] | null>(null);
  const [activeOut, setActiveOut] = useState<string | null>(null);

  const current = scenarios.find((s) => s.scenarioId === scenarioId) || null;

  useEffect(() => {
    if (!scenarioId) return;
    setOverview(null); setActiveDs(null); setRows(null); setRaw(null); setError(null);
    api.inputs(scenarioId).then((d) => {
      const cats = [...d.categories, ...EXTRA_CATEGORIES];
      setOverview(cats);
      setActiveCat(cats[0]?.category ?? cats[0]?.key ?? 'demand');
    }).catch((e) => setError(e.message));
    api.runs(scenarioId).then((d) => { setRuns(d.runs); setActiveRun(d.runs[0] ?? null); })
      .catch(() => setRuns([]));
  }, [scenarioId]);

  useEffect(() => {
    if (!activeRun) { setRunOutputs(null); return; }
    api.runDetail(activeRun.run_id).then((d) => { setRunOutputs(d.outputs); setActiveOut(d.outputs[0]?.key ?? null); })
      .catch((e) => setError(e.message));
  }, [activeRun]);

  useEffect(() => {
    if (!activeDs || !scenarioId) { setRows(null); return; }
    setRows(null);
    api.inputRows(scenarioId, activeDs.key, 500).then((d) => { setRows(d.rows); setColumns(d.columns); })
      .catch((e) => setError(e.message));
  }, [activeDs, scenarioId]);

  if (!scenarioId) return <Empty>请选择一个测试场景</Empty>;

  const catList: any[] = overview ?? [];
  const cat = catList.find((c) => (c.category ?? c.key) === activeCat);

  return (
    <div>
      {current && (
        <div className="panel">
          <h2>场景：{current.displayName}</h2>
          <div className="sub">
            来源 {current.source} · 数据集 {current.datasetCount}/7 · 路径 {current.rootPath}
          </div>
          <div className="row" style={{ gap: 8 }}>
            {Object.entries(current.engines).map(([eng, e]: any) => (
              <Badge key={eng} tone={e.runnable ? 'ok' : 'bad'}>
                {eng} {e.runnable ? '可运行' : `缺失 ${e.missing.join('/')}`}
              </Badge>
            ))}
            {current.emptyHint && <Badge tone="warn">{current.emptyHint}</Badge>}
          </div>
        </div>
      )}

      <div className="panel">
        <h2>输入数据浏览</h2>
        <div className="sub">按业务类别统一查看需求、供给、资源、计划供给、物料主数据、产品结构与替代料组（数据来自 DuckDB）</div>
        <div className="category-tabs">
          {catList.map((c) => (
            <button key={c.category ?? c.key} data-testid={`cat-${c.category ?? c.key}`}
              className={activeCat === (c.category ?? c.key) ? 'active' : ''}
              onClick={() => { setActiveCat(c.category ?? c.key); setActiveDs(null); setRaw(null); }}>
              {c.label}
            </button>
          ))}
        </div>
        <ErrorBox error={error} />
        {cat && (
          <div>
            <div className="row" style={{ gap: 8, marginBottom: 10 }}>
              {cat.datasets.map((d: any) => {
                const isMissing = !d.rowCount && !d.exists;
                return (
                  <button key={d.key} data-testid={`ds-${d.key}`}
                    className={`btn small ${activeDs?.key === d.key ? '' : 'secondary'}`}
                    onClick={() => { setActiveDs(d); setRaw(null); }}>
                    {d.label}（{d.rowCount} 行{d.exists === false ? ' · 文件缺失' : ''}）
                  </button>
                );
              })}
            </div>
            {activeDs && (
              <>
                <div className="row" style={{ gap: 8, marginBottom: 8 }}>
                  <span className="hint">来源文件：{activeDs.file}</span>
                  <button className="btn ghost small" onClick={() => {
                    api.raw(scenarioId, activeDs.file).then(setRaw).catch((e) => setError(e.message));
                  }} data-testid="show-raw">查看原始 CSV</button>
                </div>
                {raw ? (
                  <>
                    <div className="row" style={{ marginBottom: 6 }}>
                      <span className="hint">原始内容（{raw.filename}）</span>
                      <button className="btn ghost small" onClick={() => setRaw(null)}>返回表格</button>
                    </div>
                    <pre className="pre" data-testid="raw-csv">{raw.content}</pre>
                  </>
                ) : rows === null ? <Loading what="数据集" /> : (
                  <DataTable columns={columns} rows={rows} />
                )}
              </>
            )}
          </div>
        )}
      </div>

      <div className="panel">
        <h2>运行输出浏览</h2>
        <div className="sub">选择一个运行，查看该场景历次运行（run_id）的输出数据</div>
        {runs.length === 0 ? <Empty>该场景暂无运行记录，可在交付承诺 / ITP-IOP / 替代料视图触发一次运行</Empty> : (
          <>
            <div className="row" style={{ gap: 8, marginBottom: 10, alignItems: 'center' }}>
              <select data-testid="run-select" value={activeRun?.run_id ?? ''} onChange={(e) => setActiveRun(runs.find((r) => r.run_id === e.target.value) ?? null)}>
                {runs.map((r) => (
                  <option key={r.run_id} value={r.run_id}>
                    {r.engine_type} · {r.status} · {new Date(r.created_at).toLocaleString()} · {String(r.run_id).slice(0, 8)}
                  </option>
                ))}
              </select>
              {activeRun && <Badge tone={activeRun.status === 'succeeded' ? 'ok' : 'bad'}>{activeRun.status}</Badge>}
              {activeRun?.duration_ms != null && <span className="hint">端到端 {activeRun.duration_ms} ms · C++ {activeRun.engine_duration_ms ?? '—'} ms</span>}
            </div>
            {runOutputs && (
              <>
                <div className="row" style={{ gap: 8, marginBottom: 8 }}>
                  {runOutputs.map((o) => (
                    <button key={o.key} className={`btn small ${activeOut === o.key ? '' : 'secondary'}`}
                      data-testid={`out-${o.key}`} onClick={() => setActiveOut(o.key)}>
                      {o.label}（{o.rowCount} 行）
                    </button>
                  ))}
                </div>
                {activeOut && <RunOutputTable runId={activeRun.run_id} datasetKey={activeOut} />}
              </>
            )}
          </>
        )}
      </div>
    </div>
  );
}

function RunOutputTable({ runId, datasetKey }: { runId: string; datasetKey: string }) {
  const [rows, setRows] = useState<any[] | null>(null);
  const [error, setError] = useState<string | null>(null);
  useEffect(() => {
    setRows(null); setError(null);
    api.runOutputs(runId, datasetKey).then((d) => setRows(d.rows)).catch((e) => setError(e.message));
  }, [runId, datasetKey]);
  if (error) return <ErrorBox error={error} />;
  if (rows === null) return <Loading what="运行输出" />;
  if (!rows.length) return <Empty>该输出数据集为空</Empty>;
  return <DataTable columns={Object.keys(rows[0])} rows={rows} />;
}
