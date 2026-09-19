import React, { useEffect, useState } from 'react';
import api from '../lib/api';
import { useApp } from '../App';
import { Badge, DataTable, Empty, ErrorBox, Kpi } from '../components/common';

// 替代料决策视图：三类替代料的配额比例/历史量/本次分配、安全库存保护带、选中成员高亮。
export default function SubstitutionView() {
  const { scenarioId, reloadScenarios } = useApp();
  const [params, setParams] = useState({ alt_class: 1, alt_group: 1, net_demand: 50, day: 1, parent_id: 0 });
  const [running, setRunning] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [decision, setDecision] = useState<any>(null);
  const [water, setWater] = useState<any[]>([]);
  const [allocations, setAllocations] = useState<any[]>([]);
  const [runId, setRunId] = useState<string | null>(null);

  useEffect(() => { loadLatest(); }, [scenarioId]);

  async function loadLatest() {
    if (!scenarioId) return;
    setError(null); setDecision(null); setWater([]); setAllocations([]);
    const d = await api.runs(scenarioId, 'substitution');
    const latest = d.runs.find((r: any) => r.status === 'succeeded');
    if (latest) hydrate(latest.run_id);
  }

  async function hydrate(id: string) {
    setRunId(id);
    const [dec, w, a] = await Promise.all([
      api.runOutputs(id, 'substitution_decisions'),
      api.runOutputs(id, 'substitution_water'),
      api.runOutputs(id, 'substitution_allocations'),
    ]);
    setDecision(dec.rows[0] ?? null);
    setWater(w.rows);
    setAllocations(a.rows);
  }

  async function submit(e: React.FormEvent) {
    e.preventDefault();
    if (!scenarioId) return;
    setRunning(true); setError(null);
    try {
      const res = await api.run('substitution', { scenarioId, ...params });
      await hydrate(res.runId);
      await reloadScenarios();
    } catch (e: any) {
      // 业务不可分配（exit 0，0 分配）会在结论里体现；编排失败才是错误
      setError(`${e.message}${e.kind ? `（${e.kind}）` : ''}；已保留参数以便重试`);
    } finally { setRunning(false); }
  }

  if (!scenarioId) return <Empty>请选择一个测试场景</Empty>;

  const selectedId = decision?.chosen_part_id != null ? Number(decision.chosen_part_id) : null;
  const allocById = new Map<number, number>();
  allocations.forEach((a) => allocById.set(Number(a.alt_part_id), (allocById.get(Number(a.alt_part_id)) ?? 0) + Number(a.allocated_qty)));
  const totalAllocated = allocations.reduce((s, a) => s + Number(a.allocated_qty), 0);

  return (
    <div>
      <div className="panel">
        <h2>替代料决策（三类分支）</h2>
        <div className="sub">类别 1 = 按目标比例分配；类别 2 = 按优先级/水位；类别 3 = 安全库存保护下分配</div>
        <form className="row" onSubmit={submit} style={{ gap: 12 }}>
          <SField label="替代料类别 (1/2/3)"><input type="number" min="1" max="3" data-testid="s-class" value={params.alt_class} onChange={(e) => setParams({ ...params, alt_class: Number(e.target.value) })} /></SField>
          <SField label="替代料组 (alt_group)"><input type="number" data-testid="s-group" value={params.alt_group} onChange={(e) => setParams({ ...params, alt_group: Number(e.target.value) })} /></SField>
          <SField label="净需求"><input type="number" data-testid="s-demand" value={params.net_demand} onChange={(e) => setParams({ ...params, net_demand: Number(e.target.value) })} /></SField>
          <SField label="分配日"><input type="number" data-testid="s-day" value={params.day} onChange={(e) => setParams({ ...params, day: Number(e.target.value) })} /></SField>
          <SField label="父物料"><input type="number" data-testid="s-parent" value={params.parent_id} onChange={(e) => setParams({ ...params, parent_id: Number(e.target.value) })} /></SField>
          <button className="btn" type="submit" disabled={running} data-testid="run-substitution">{running ? '运行中…' : '运行替代料决策'}</button>
        </form>
        <ErrorBox error={error} />
      </div>

      {!decision && !error && <Empty>暂无替代料决策结果，请点击「运行替代料决策」</Empty>}

      {decision && (
        <>
          <div className="panel">
            <h2>决策结论 {runId && <span className="hint">· run {String(runId).slice(0, 8)}</span>}</h2>
            <div className="kpis">
              <Kpi label="类别" value={decision.category} />
              <Kpi label="选中替代料" value={decision.chosen_part_id ?? '—'} tone={decision.chosen_part_id != null ? 'ok' : 'bad'} />
              <Kpi label="选择依据" value={decision.basis ?? '—'} />
              <Kpi label="净需求" value={decision.net_demand} unit="件" />
              <Kpi label="本次分配合计" value={totalAllocated} unit="件" tone={totalAllocated > 0 ? 'ok' : 'bad'} />
              <Kpi label="分配后在手" value={decision.remaining_on_hand_after ?? '—'} unit="件" />
            </div>
            {totalAllocated === 0 && <div className="hint" style={{ marginTop: 8, color: 'var(--warn)' }}>不可分配：该类别在当前水位与安全库存约束下无可用替代料（业务结论，非执行错误）。</div>}
          </div>

          <div className="panel">
            <h2>成员水位与安全库存保护带</h2>
            <div className="legend">
              <span><span className="sw" style={{ background: 'repeating-linear-gradient(45deg,#f6c98a,#f6c98a 4px,#fbe3c4 4px,#fbe3c4 8px)' }} />安全库存保护带（不可侵占）</span>
              <span><span className="sw" style={{ background: '#1f8a4c' }} />本次分配</span>
              <span><span className="sw" style={{ background: '#2f8fd8' }} />可分配水位</span>
            </div>
            {water.length === 0 ? <Empty>该运行无水位明细</Empty> : (
              <div className="attempt-list" data-testid="water-list">
                {water.map((w, i) => {
                  const onHand = Number(w.on_hand_before);
                  const max = Math.max(onHand, Number(w.safety_stock) || 0, 1);
                  const safetyPct = (Number(w.safety_stock) / max) * 100;
                  const allocPct = ((allocById.get(Number(w.member_part_id)) ?? 0) / max) * 100;
                  const allocatablePct = (Math.max(0, Number(w.allocatable_before)) / max) * 100;
                  const isSelected = selectedId === Number(w.member_part_id);
                  return (
                    <div key={i} className={`attempt ${isSelected ? 'cur' : ''}`}>
                      <span style={{ width: 140 }}>
                        物料 {w.member_part_id} {isSelected && <mark className="sel">选中</mark>}
                      </span>
                      <span style={{ width: 90 }}>目标 {Math.round(Number(w.target_ratio) * 100)}%</span>
                      <span style={{ width: 90 }}>历史 {w.historical_qty}</span>
                      <div className="bar" style={{ flex: 1, height: 20 }} title={`在手 ${w.on_hand_before} / 安全库存 ${w.safety_stock} / 可分配 ${w.allocatable_before}`}>
                        <div className="fill" style={{ width: '100%', background: '#dbe7f3' }} />
                        <div className="fill" style={{ width: `${allocatablePct}%`, background: '#2f8fd8', opacity: .55 }} />
                        <div className="fill" style={{ width: `${safetyPct}%`, background: 'repeating-linear-gradient(45deg,#f6c98a,#f6c98a 4px,#fbe3c4 4px,#fbe3c4 8px)' }} />
                        <div className="fill" style={{ width: `${allocPct}%`, background: '#1f8a4c', opacity: .85 }} />
                      </div>
                      <span style={{ width: 200, textAlign: 'right' }}>本次分配 {allocById.get(Number(w.member_part_id)) ?? 0} · 分配后 {w.on_hand_after}</span>
                    </div>
                  );
                })}
              </div>
            )}
          </div>

          <div className="panel">
            <h2>本次分配明细</h2>
            {allocations.length === 0 ? <Empty>无分配记录</Empty> : (
              <DataTable columns={['day', 'parent_part_id', 'alt_part_id', 'allocated_qty', 'day_allocated', 'alt_class']} rows={allocations} />
            )}
          </div>
        </>
      )}
    </div>
  );
}

function SField({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <label>
      <div className="hint">{label}</div>
      {React.cloneElement(children as any, { style: { padding: '5px 8px', border: '1px solid var(--line-strong)', borderRadius: 4, width: 120 } })}
    </label>
  );
}
