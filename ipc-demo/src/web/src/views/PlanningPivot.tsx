import React, { useEffect, useState, useMemo } from 'react';
import api from '../lib/api';
import { useApp } from '../App';
import PivotGrid from '../components/PivotGrid';
import { Empty, ErrorBox } from '../components/common';

const DIMENSIONS = [
  { key: '', label: '全部维度' },
  { key: 'part', label: '物料 (part)' },
  { key: 'work_center', label: '工作中心' },
  { key: 'family', label: '物料族 (family)' },
];

// 计划透视图：天粒度 × 关键指标，SAP IBP 风格。
export default function PlanningPivot() {
  const { scenarioId, scenarios } = useApp();
  const [runId, setRunId] = useState<string>('');
  const [dimension, setDimension] = useState<string>('');
  const [dimValue, setDimValue] = useState<string>('');
  const [runs, setRuns] = useState<any[]>([]);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    if (!scenarioId) return;
    api.runs(scenarioId).then((d) => setRuns(d.runs.filter((r: any) => r.status === 'succeeded')))
      .catch((e) => setError(e.message));
    setRunId(''); setDimension(''); setDimValue('');
  }, [scenarioId]);

  const current = scenarios.find((s) => s.scenarioId === scenarioId) || null;
  if (!scenarioId) return <Empty>请选择一个测试场景</Empty>;

  const dimOptions = useMemo(() => {
    const set = new Set<string>();
    runs.filter((r) => r.engine_type === 'iop' || r.engine_type === 'itp').forEach((r) => set.add(String(r.run_id)));
    return set;
  }, [runs]);

  return (
    <div>
      <div className="panel">
        <h2>计划透视图（天粒度）</h2>
        <div className="sub">行 = 关键指标 × 维度成员，列 = 天。按供给 / 需求 / 资源 / 平衡四族分组；点击族标题折叠，点击单元格下钻明细。</div>
        <div className="row" style={{ gap: 12 }}>
          <label>
            <div className="hint">运行批次（平衡族指标按 run_id 取值）</div>
            <select data-testid="pivot-run" value={runId} onChange={(e) => setRunId(e.target.value)} style={{ minWidth: 260 }}>
              <option value="">全部成功运行</option>
              {runs.map((r) => (
                <option key={r.run_id} value={r.run_id}>
                  {r.engine_type} · {new Date(r.created_at).toLocaleString()} · {String(r.run_id).slice(0, 8)}
                </option>
              ))}
            </select>
          </label>
          <label>
            <div className="hint">筛选维度</div>
            <select data-testid="pivot-dimension" value={dimension} onChange={(e) => { setDimension(e.target.value); setDimValue(''); }}>
              {DIMENSIONS.map((d) => <option key={d.key} value={d.key}>{d.label}</option>)}
            </select>
          </label>
          <label>
            <div className="hint">维度成员值</div>
            <input data-testid="pivot-dimvalue" value={dimValue} placeholder={dimension === 'work_center' ? '如 WC_01' : '如 10'}
              onChange={(e) => setDimValue(e.target.value)} style={{ padding: '5px 8px', border: '1px solid var(--line-strong)', borderRadius: 4, width: 180 }} />
          </label>
          <button className="btn secondary" onClick={() => { setDimension(''); setDimValue(''); }}>清除筛选</button>
        </div>
        {!current?.isComplete && (
          <div className="hint" style={{ marginTop: 8 }}>当前场景非完整场景，部分指标可能无数据（以空单元格呈现，区别于 0）。</div>
        )}
      </div>
      <ErrorBox error={error} />
      <div className="panel">
        <PivotGrid
          scenarioId={scenarioId}
          runId={runId || undefined}
          dimension={dimension || undefined}
          dimValue={dimValue.trim() || undefined}
        />
        {dimension && !dimValue.trim() && (
          <div className="hint" style={{ marginTop: 8 }}>提示：选择「{DIMENSIONS.find((d) => d.key === dimension)?.label}」后输入成员值，透视将仅保留该维度的指标行并按值重算。</div>
        )}
      </div>
    </div>
  );
}
