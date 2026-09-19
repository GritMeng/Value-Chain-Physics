import React, { useEffect, useRef, useState } from 'react';
import api from '../lib/api';
import { useApp } from '../App';
import { Badge, DataTable, Empty, ErrorBox, Kpi, Loading } from '../components/common';

// 交付承诺视图：参数调整 -> 触发 CLI 运行 -> BOM 层级 / 产能水位 / 承诺时间轴 / 回滚 / 逐步回放。
export default function DeliveryView() {
  const { scenarioId, reloadScenarios } = useApp();
  const [params, setParams] = useState({ part_id: 0, due_day: 3, qty: 30, priority: 1 });
  const [running, setRunning] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [result, setResult] = useState<any>(null);
  const [detail, setDetail] = useState<any>(null);
  const [bom, setBom] = useState<any[]>([]);
  const [capacity, setCapacity] = useState<any[]>([]);
  const [steps, setSteps] = useState<any[]>([]);
  const [cursor, setCursor] = useState(0);
  const [playing, setPlaying] = useState(false);
  const timer = useRef<any>(null);

  useEffect(() => { loadLatest(); }, [scenarioId]);

  async function loadLatest() {
    if (!scenarioId) return;
    setResult(null); setDetail(null); setError(null); setPlaying(false);
    api.runs(scenarioId, 'delivery').then((d) => {
      const latest = d.runs.find((r: any) => r.status === 'succeeded');
      if (latest) hydrate(latest.run_id);
    }).catch(() => {});
    api.inputRows(scenarioId, 'capacity', 200).then((d) => setCapacity(d.rows)).catch(() => setCapacity([]));
  }

  async function hydrate(runId: string) {
    const [det, stemps, res, b] = await Promise.all([
      api.runDetail(runId),
      api.runOutputs(runId, 'delivery_steps'),
      api.runOutputs(runId, 'delivery_result'),
      api.runOutputs(runId, 'delivery_bom'),
    ]);
    setDetail(det);
    setSteps(stemps.rows);
    setResult(res.rows[0] ?? null);
    setBom(b.rows);
    setCursor(stemps.rows.length);
  }

  useEffect(() => {
    if (!playing) { if (timer.current) clearInterval(timer.current); return; }
    timer.current = setInterval(() => {
      setCursor((c) => {
        if (c >= steps.length) { setPlaying(false); return c; }
        return c + 1;
      });
    }, 700);
    return () => clearInterval(timer.current);
  }, [playing, steps.length]);

  async function submit(e: React.FormEvent) {
    e.preventDefault();
    if (!scenarioId) return;
    setRunning(true); setError(null);
    try {
      const res = await api.run('delivery', { scenarioId, ...params });
      await hydrate(res.runId);
      await reloadScenarios();
    } catch (e: any) {
      setError(`${e.message}${e.kind ? `（${e.kind}）` : ''}；已保留参数以便重试`);
    } finally {
      setRunning(false);
    }
  }

  if (!scenarioId) return <Empty>请选择一个测试场景</Empty>;

  const capacityAtPromise = result && capacity.find((c) => Number(c.day) === Number(result.promised_day));
  const bottleneck = capacity.length ? Math.min(...capacity.map((c) => Number(c.capacity_hours))) : null;
  const maxDay = Math.max(result?.due_day ?? 3, result?.promised_day ?? 3, ...capacity.map((c) => Number(c.day)), ...steps.map((s: any) => Number(s.attempt_day)), 1);

  return (
    <div>
      <div className="panel">
        <h2>交付承诺（ATP/CTP）</h2>
        <div className="sub">Node 调用 C++ CLI 计算：C++ 读场景 CSV → 输出结果 CSV → Node 导入 DuckDB → 本页展示</div>
        <form className="row" onSubmit={submit} style={{ gap: 12 }}>
          <Field label="物料 (part_id)"><input type="number" data-testid="p-part" value={params.part_id} onChange={(e) => setParams({ ...params, part_id: Number(e.target.value) })} /></Field>
          <Field label="请求交期 (due_day)"><input type="number" data-testid="p-due" value={params.due_day} onChange={(e) => setParams({ ...params, due_day: Number(e.target.value) })} /></Field>
          <Field label="需求数量 (qty)"><input type="number" data-testid="p-qty" value={params.qty} onChange={(e) => setParams({ ...params, qty: Number(e.target.value) })} /></Field>
          <Field label="优先级"><input type="number" data-testid="p-prio" value={params.priority} onChange={(e) => setParams({ ...params, priority: Number(e.target.value) })} /></Field>
          <button className="btn" type="submit" disabled={running} data-testid="run-delivery">{running ? '运行中…' : '运行交付承诺'}</button>
          {running && <span className="hint">正在调用 C++ 引擎并导入 DuckDB…</span>}
        </form>
        <ErrorBox error={error} />
      </div>

      {!result && !error && <Empty>暂无交付承诺结果，请点击「运行交付承诺」</Empty>}

      {result && (
        <>
          <div className="panel">
            <h2>结论</h2>
            <div className="kpis">
              <Kpi label="可承诺" value={result.is_fulfillable ? '是' : '否'} tone={result.is_fulfillable ? 'ok' : 'bad'} />
              <Kpi label="承诺日" value={result.promised_day ?? '—'} unit="天" />
              <Kpi label="承诺量" value={result.promised_qty ?? 0} unit="件" />
              <Kpi label="预留工时" value={result.total_capacity_used ?? 0} unit="h" tone={bottleneck != null && Number(result.total_capacity_used) > bottleneck ? 'bad' : undefined} />
              <Kpi label="回滚步数" value={result.rollback_steps_count ?? 0} tone={Number(result.rollback_steps_count) > 0 ? 'warn' : 'ok'} />
            </div>
            <div style={{ marginTop: 12 }}>
              {result.is_fulfillable
                ? <Badge tone="ok">请求 Day {result.due_day} → 承诺 Day {result.promised_day}（{result.promised_day <= result.due_day ? '按期/提前' : '晚于请求'}）</Badge>
                : <Badge tone="bad">不可承诺：缺口 {Number(result.qty) - Number(result.promised_qty ?? 0)} 件</Badge>}
              {Number(result.rollback_steps_count) > 0 && <span style={{ marginLeft: 8 }}><Badge tone="warn">发生回滚 {result.rollback_steps_count} 步</Badge></span>}
            </div>
          </div>

          <div className="grid-2">
            <div className="panel">
              <h2>承诺时间轴</h2>
              <div className="timeline" data-testid="timeline">
                <Mark day={result.due_day} maxDay={maxDay} kind="req" label={`请求 D${result.due_day}`} />
                <Mark day={result.promised_day} maxDay={maxDay} kind={result.is_fulfillable ? 'done' : 'fail'} label={`承诺 D${result.promised_day}`} />
              </div>
              <div className="hint">时间轴覆盖 Day 0 – Day {maxDay}；橙色为请求交期，绿色/红色为承诺结果。</div>
              <h3>产能水位</h3>
              <div className="attempt-list">
                {capacity.map((c) => {
                  const used = Number(c.day) === Number(result.promised_day) ? Number(result.total_capacity_used) : 0;
                  const pct = Number(c.capacity_hours) ? Math.min(100, (used / Number(c.capacity_hours)) * 100) : 0;
                  return (
                    <div key={c.day} className="attempt">
                      <span style={{ width: 56 }}>Day {c.day}</span>
                      <div className="bar" style={{ flex: 1 }}>
                        <div className="fill" style={{ width: `${pct}%` }} />
                      </div>
                      <span style={{ width: 130, textAlign: 'right' }}>占用 {used} / 可用 {c.capacity_hours} h</span>
                    </div>
                  );
                })}
                {capacity.length === 0 && <Empty>该场景无产能数据</Empty>}
              </div>
            </div>

            <div className="panel">
              <h2>BOM 分级展开</h2>
              <div className="sub">层级结构来自 C++ 引擎输出的 delivery_bom.csv</div>
              {bom.length === 0 ? <Empty>该运行无 BOM 明细</Empty> : <BomTree rows={bom} />}
            </div>
          </div>

          <div className="panel">
            <h2>逐步回放（试算与回滚）</h2>
            <div className="replay">
              <button className="btn secondary small" onClick={() => setCursor((c) => Math.max(0, c - 1))} disabled={cursor === 0} data-testid="replay-back">← 后退</button>
              <button className="btn small" onClick={() => setPlaying((p) => !p)} disabled={steps.length === 0} data-testid="replay-play">
                {playing ? '暂停' : '自动播放'}
              </button>
              <button className="btn secondary small" onClick={() => setCursor((c) => Math.min(steps.length, c + 1))} disabled={cursor >= steps.length} data-testid="replay-forward">前进 →</button>
              <button className="btn ghost small" onClick={() => { setPlaying(false); setCursor(0); }} data-testid="replay-reset">重置</button>
              <span className="frame" data-testid="replay-frame">帧 {cursor} / {steps.length}</span>
            </div>
            <div className="attempt-list">
              {steps.map((s, i) => (
                <div key={i} className={`attempt ${i + 1 === cursor ? 'cur' : ''} ${s.success ? '' : 'fail'}`}>
                  <span style={{ width: 40 }}>#{i + 1}</span>
                  <span style={{ width: 76 }}>Day {s.attempt_day}</span>
                  <Badge tone={s.success ? 'ok' : 'bad'}>{s.success ? '成功' : '回滚'}</Badge>
                  <span style={{ width: 110 }}>占用 {s.capacity_used} h</span>
                  <span style={{ width: 90 }}>回滚 {s.rollback_steps} 步</span>
                  <span className="hint">{s.note}</span>
                </div>
              ))}
              {steps.length === 0 && <Empty>该运行无试算步骤</Empty>}
            </div>
          </div>

          <div className="panel">
            <h2>运行元数据</h2>
            {detail ? (
              <div className="row" style={{ gap: 20 }}>
                <span className="hint">run_id: {detail.run_id}</span>
                <span className="hint">端到端: {detail.duration_ms} ms</span>
                <span className="hint">C++ 自报: {detail.engine_duration_ms ?? '—'} ms</span>
                <span className="hint">退出码: {detail.exit_code}</span>
                <span className="hint">参数: {detail.params_json}</span>
              </div>
            ) : <Loading what="运行元数据" />}
          </div>
        </>
      )}
    </div>
  );
}

function Field({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <label>
      <div className="hint">{label}</div>
      {React.cloneElement(children as any, { style: { padding: '5px 8px', border: '1px solid var(--line-strong)', borderRadius: 4, width: 120 } })}
    </label>
  );
}

function Mark({ day, maxDay, kind, label }: { day: number; maxDay: number; kind: string; label: string }) {
  const left = maxDay ? (Number(day) / maxDay) * 100 : 0;
  return (
    <div className={`mark ${kind}`} style={{ left: `${left}%` }} data-testid={`mark-${kind}`}>
      <div className="dot" />
      {label}
    </div>
  );
}

function BomTree({ rows }: { rows: any[] }) {
  const byParent = new Map<number, any[]>();
  rows.forEach((r) => {
    const p = Number(r.level_parent_id);
    if (!byParent.has(p)) byParent.set(p, []);
    byParent.get(p)!.push(r);
  });
  const roots = [...byParent.keys()].filter((p) => !rows.some((r) => Number(r.level_child_id) === p));
  const render = (pid: number, depth: number): React.ReactNode[] => (byParent.get(pid) ?? []).flatMap((r) => [
    <div key={`${r.level_parent_id}-${r.level_child_id}-${depth}`} className="attempt" style={{ marginLeft: depth * 20 }}>
      <span style={{ width: 100 }}>L{depth} 子件 {r.level_child_id}</span>
      <span style={{ width: 100 }}>用量 ×{r.usage_qty}</span>
      <span style={{ width: 120 }}>需求量 {r.child_required_qty}</span>
      <Badge tone="muted">替代等级 {r.alt_class}</Badge>
    </div>,
    ...render(Number(r.level_child_id), depth + 1),
  ]);
  if (roots.length === 0) return <DataTable columns={Object.keys(rows[0] ?? {})} rows={rows} />;
  return <div className="attempt-list">{roots.flatMap((r) => render(r, 1))}</div>;
}
