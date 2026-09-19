import React, { createContext, useContext, useEffect, useMemo, useState } from 'react';
import api, { ScenarioSummary } from './lib/api';
import ScenarioBrowser from './views/ScenarioBrowser';
import PlanningPivot from './views/PlanningPivot';
import DeliveryView from './views/DeliveryView';
import ItpIopView from './views/ItpIopView';
import SubstitutionView from './views/SubstitutionView';
import PerformanceView from './views/PerformanceView';

// 全局场景上下文：所有视图随所选场景切换。
interface Ctx {
  scenarios: ScenarioSummary[];
  scenarioId: string | null;
  setScenarioId: (id: string) => void;
  reloadScenarios: () => Promise<void>;
  health: any;
}
const AppCtx = createContext<Ctx>(null!);
export const useApp = () => useContext(AppCtx);

const ROUTES = [
  { path: 'scenario', label: '场景浏览' },
  { path: 'pivot', label: '计划透视' },
  { path: 'delivery', label: '交付承诺' },
  { path: 'itp-iop', label: 'ITP / IOP' },
  { path: 'substitution', label: '替代料' },
  { path: 'performance', label: '性能与历史' },
];

function useHashRoute() {
  const [route, setRoute] = useState(() => window.location.hash.replace(/^#\/?/, '') || 'scenario');
  useEffect(() => {
    const on = () => setRoute(window.location.hash.replace(/^#\/?/, '') || 'scenario');
    window.addEventListener('hashchange', on);
    return () => window.removeEventListener('hashchange', on);
  }, []);
  const nav = (r: string) => { window.location.hash = `#/${r}`; };
  return [route, nav] as const;
}

export default function App() {
  const [scenarios, setScenarios] = useState<ScenarioSummary[]>([]);
  const [scenarioId, setScenarioId] = useState<string | null>(null);
  const [health, setHealth] = useState<any>(null);
  const [error, setError] = useState<string | null>(null);
  const [route, nav] = useHashRoute();

  async function reloadScenarios() {
    try {
      const [sc, h] = await Promise.all([api.scenarios(), api.health()]);
      setScenarios(sc.scenarios);
      setHealth(h);
      if (!scenarioId && sc.scenarios.length) setScenarioId(sc.scenarios[0].scenarioId);
    } catch (e: any) {
      setError(e.message);
    }
  }
  useEffect(() => { reloadScenarios(); }, []);

  const ctx = useMemo<Ctx>(() => ({ scenarios, scenarioId, setScenarioId, reloadScenarios, health }),
    [scenarios, scenarioId, health]);

  const current = scenarios.find((s) => s.scenarioId === scenarioId) || null;

  return (
    <AppCtx.Provider value={ctx}>
      <div className="app">
        <header className="topbar">
          <div className="brand">IPC 优化引擎可视化演示</div>
          <div className="scenario-picker">
            <label>测试场景</label>
            <select value={scenarioId ?? ''} onChange={(e) => setScenarioId(e.target.value)} data-testid="scenario-select">
              {scenarios.map((s) => (
                <option key={s.scenarioId} value={s.scenarioId}>{s.displayName}</option>
              ))}
            </select>
          </div>
          <div className="health">
            {health && (
              <span className={health.checks.engine.ok ? 'ok' : 'bad'}>
                引擎 {health.checks.engine.ok ? '就绪' : '缺失'} · DuckDB {health.checks.duckdb.ok ? '就绪' : '异常'}
              </span>
            )}
          </div>
        </header>
        <nav className="tabs">
          {ROUTES.map((r) => (
            <button key={r.path} className={route === r.path ? 'active' : ''} onClick={() => nav(r.path)}>
              {r.label}
            </button>
          ))}
        </nav>
        {error && <div className="error">{error}</div>}
        {current && !current.engines.delivery.runnable && (
          <div className="warn">当前场景部分引擎不可运行：缺失 {Object.entries(current.engines).filter(([, e]) => !e.runnable).map(([k, e]) => `${k}(${e.missing.join('/')})`).join(', ')}</div>
        )}
        <main>
          {route === 'scenario' && <ScenarioBrowser />}
          {route === 'pivot' && <PlanningPivot />}
          {route === 'delivery' && <DeliveryView />}
          {route === 'itp-iop' && <ItpIopView />}
          {route === 'substitution' && <SubstitutionView />}
          {route === 'performance' && <PerformanceView />}
        </main>
      </div>
    </AppCtx.Provider>
  );
}
