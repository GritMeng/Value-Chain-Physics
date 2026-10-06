import React, { useState, useEffect } from 'react';
import { useScenario } from '../../contexts/ScenarioContext';
import { 
  Layers, 
  Settings, 
  Play, 
  Activity, 
  Cpu, 
  Info, 
  CheckCircle2, 
  AlertCircle,
  HelpCircle,
  Sliders,
  ChevronRight,
  TrendingUp,
  Boxes
} from 'lucide-react';

const ABPConfig = () => {
  const { currentScenario, runSimulation, isLoading } = useScenario();
  
  // State variables for configurations
  const [groupings, setGroupings] = useState([]);
  const [recipes, setRecipes] = useState([]);
  const [demands, setDemands] = useState([]);
  
  // State variables for config settings
  const [downbinningEnabled, setDownbinningEnabled] = useState(true);
  const [coproductOpt, setCoproductOpt] = useState(true);
  const [priorityOrder, setPriorityOrder] = useState('EXACT_FIRST');
  
  // State variables for outputs
  const [schedules, setSchedules] = useState([]);
  const [allocations, setAllocations] = useState([]);
  
  const [isUpdating, setIsUpdating] = useState(false);
  const [msg, setMsg] = useState(null);

  // Fetch setups & results
  const fetchSetupData = async () => {
    try {
      const res = await fetch('/api/coproduct/setup_data');
      const data = await res.json();
      if (data.status === 'success') {
        setGroupings(data.groupings);
        setRecipes(data.recipes);
        setDemands(data.demands);
      }
      
      const configRes = await fetch('/api/coproduct/config');
      const configData = await configRes.json();
      if (configData.status === 'success') {
        setDownbinningEnabled(configData.data.downbinning_enabled === 'true');
        setCoproductOpt(configData.data.coproduct_optimization === 'true');
        setPriorityOrder(configData.data.downbinning_priority || 'EXACT_FIRST');
      }

      fetchExecutionResults();
    } catch (err) {
      console.error("Error loading ABP data:", err);
    }
  };

  const fetchExecutionResults = async () => {
    try {
      const res = await fetch('/api/execution/coproducts');
      if (res.ok) {
        const data = await res.json();
        setSchedules(data.schedules || []);
        setAllocations(data.allocations || []);
      }
    } catch (err) {
      console.error("Error fetching execution results:", err);
    }
  };

  useEffect(() => {
    fetchSetupData();
  }, [currentScenario]);

  // Handle saving configurations
  const handleSaveConfig = async (e) => {
    if (e) e.preventDefault();
    setIsUpdating(true);
    try {
      const res = await fetch('/api/coproduct/config/update', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          downbinning_enabled: downbinningEnabled ? 'true' : 'false',
          coproduct_optimization: coproductOpt ? 'true' : 'false',
          downbinning_priority: priorityOrder
        })
      });
      const data = await res.json();
      if (data.status === 'success') {
        showMsg("✔ 维度规划消纳规则更新成功！请运行引擎重新计算。");
      }
    } catch (err) {
      console.error(err);
    } finally {
      setIsUpdating(false);
    }
  };

  const handleRunEngine = async () => {
    await handleSaveConfig();
    showMsg("⚡ 正在触发 C++ 维度与分级消纳规划引擎...");
    await runSimulation();
    showMsg("✔ 规划引擎计算完成，结果已加载并更新！");
    fetchExecutionResults();
  };

  const showMsg = (text) => {
    setMsg(text);
    setTimeout(() => setMsg(null), 6000);
  };

  return (
    <div className="flex-1 flex flex-col space-y-3 overflow-hidden min-h-0">
      
      {/* 警报与通知栏 */}
      {msg && (
        <div className="bg-indigo-950/40 border border-indigo-600/40 p-3 rounded flex items-center space-x-2.5 text-xs text-indigo-300 animate-fade-in select-none">
          <Info className="w-4 h-4 flex-shrink-0 animate-pulse text-indigo-400" />
          <span>{msg}</span>
        </div>
      )}

      {/* 控制卡片 & 配置面板 */}
      <div className="grid grid-cols-1 xl:grid-cols-12 gap-3 min-h-0">
        
        {/* 左侧：参数控制区 (4/12 cols) */}
        <div className="xl:col-span-4 bg-card border border-main rounded-xs p-4 flex flex-col space-y-4 select-none">
          <div className="border-b border-main pb-2">
            <h3 className="text-xs font-bold text-heading flex items-center">
              <Sliders className="w-4 h-4 text-indigo-500 mr-1.5" />
              降级消纳规则设定 (Netting Constraints)
            </h3>
          </div>

          <div className="space-y-4 text-xs">
            <div className="flex items-center justify-between p-2 bg-slate-900/10 rounded border border-slate-800/40">
              <div className="flex flex-col space-y-0.5">
                <span className="font-bold text-heading">启用产品降级 (downbinning_enabled)</span>
                <span className="text-[10px] text-muted">允许高规格库存用于满足低规格订单</span>
              </div>
              <input 
                type="checkbox" 
                checked={downbinningEnabled}
                onChange={(e) => setDownbinningEnabled(e.target.checked)}
                className="w-4 h-4 rounded border-slate-700 bg-slate-800 text-indigo-600 focus:ring-indigo-500"
              />
            </div>

            <div className="flex items-center justify-between p-2 bg-slate-900/10 rounded border border-slate-800/40">
              <div className="flex flex-col space-y-0.5">
                <span className="font-bold text-heading">联副产品优化 (coproduct_optimization)</span>
                <span className="text-[10px] text-muted">运行工艺时产出的旁路废料用于满足后续订单</span>
              </div>
              <input 
                type="checkbox" 
                checked={coproductOpt}
                onChange={(e) => setCoproductOpt(e.target.checked)}
                className="w-4 h-4 rounded border-slate-700 bg-slate-800 text-indigo-600 focus:ring-indigo-500"
              />
            </div>

            <div className="flex flex-col space-y-1.5">
              <label className="font-bold text-heading">降级分配顺序优先级 (Downbinning Priority)</label>
              <select
                value={priorityOrder}
                onChange={(e) => setPriorityOrder(e.target.value)}
                className="w-full bg-input border border-main text-heading p-2 rounded focus:outline-none focus:border-indigo-500 font-bold"
              >
                <option value="EXACT_FIRST">🎯 EXACT_FIRST (优先同规格库存，不足再降级)</option>
                <option value="HIGHER_FIRST">⚡ HIGHER_FIRST (从最高规格向低规格顺序检查 - Scenario 2)</option>
              </select>
            </div>

            <div className="pt-2 flex space-x-2">
              <button 
                onClick={() => handleSaveConfig()}
                disabled={isUpdating}
                className="flex-1 py-2 bg-slate-800 hover:bg-slate-700 text-heading border border-slate-700 rounded font-bold cursor-pointer transition text-center"
              >
                保存参数配置
              </button>
              <button 
                onClick={handleRunEngine}
                disabled={isLoading}
                className="flex-1 py-2 bg-indigo-600 hover:bg-indigo-500 text-white rounded font-bold transition flex items-center justify-center space-x-1.5 cursor-pointer shadow-lg"
              >
                <Play className="w-3.5 h-3.5 fill-white" />
                <span>运行消纳引擎</span>
              </button>
            </div>
          </div>
        </div>

        {/* 右侧：维度规则架构展示 (8/12 cols) */}
        <div className="xl:col-span-8 bg-card border border-main rounded-xs p-4 flex flex-col space-y-3 overflow-hidden min-h-0 select-none">
          <div className="border-b border-main pb-2">
            <h3 className="text-xs font-bold text-heading flex items-center">
              <Layers className="w-4 h-4 text-indigo-500 mr-1.5" />
              维度分组与工艺收率矩阵 (Dimension & Yield Recipes)
            </h3>
          </div>

          <div className="flex-1 grid grid-cols-1 md:grid-cols-2 gap-4 overflow-y-auto min-h-0 pr-1 text-xs">
            {/* 维度映射关系 */}
            <div className="space-y-2">
              <h4 className="font-bold text-heading flex items-center text-[11px] text-muted">
                <ChevronRight className="w-3.5 h-3.5 mr-1" />
                维度分组与约束关系 (Mapping Rules)
              </h4>
              <div className="border border-main rounded overflow-hidden">
                <table className="w-full text-left border-collapse">
                  <thead>
                    <tr className="bg-slate-900/30 text-muted">
                      <th className="py-1.5 px-2">维度分组</th>
                      <th className="py-1.5 px-2">维度名称</th>
                      <th className="py-1.5 px-2 text-center">规格值</th>
                      <th className="py-1.5 px-2 text-right">约束关系</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-slate-800/40">
                    {groupings.map((g, idx) => (
                      <tr key={idx} className="hover:bg-slate-900/10">
                        <td className="py-2 px-2 font-bold text-heading font-mono">{g.dimension_grp}</td>
                        <td className="py-2 px-2 text-muted">{g.dimension}</td>
                        <td className="py-2 px-2 text-center font-mono text-heading">{g.value}</td>
                        <td className="py-2 px-2 text-right">
                          <span className={`px-1 rounded text-[10px] font-bold ${
                            g.relation_ship === 'EQ' ? 'bg-indigo-950 text-indigo-400 border border-indigo-900' : 'bg-emerald-950 text-emerald-400 border border-emerald-900'
                          }`}>
                            {g.relation_ship}
                          </span>
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>

            {/* 工艺收率配方 */}
            <div className="space-y-2">
              <h4 className="font-bold text-heading flex items-center text-[11px] text-muted">
                <ChevronRight className="w-3.5 h-3.5 mr-1" />
                产线收率及联产品收率配方 (Yield Ratios)
              </h4>
              <div className="border border-main rounded overflow-hidden">
                <table className="w-full text-left border-collapse">
                  <thead>
                    <tr className="bg-slate-900/30 text-muted">
                      <th className="py-1.5 px-2">工艺路径</th>
                      <th className="py-1.5 px-2 font-mono">512MB</th>
                      <th className="py-1.5 px-2 font-mono">256MB</th>
                      <th className="py-1.5 px-2 font-mono">128MB</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-slate-800/40">
                    {recipes.map((r, idx) => (
                      <tr key={idx} className="hover:bg-slate-900/10">
                        <td className="py-2 px-2 font-bold text-heading font-mono">{r.routing_code}</td>
                        <td className="py-2 px-2 font-mono font-bold text-heading">{(r.ratio_512 * 100).toFixed(0)}%</td>
                        <td className="py-2 px-2 font-mono font-bold text-heading">{(r.ratio_256 * 100).toFixed(0)}%</td>
                        <td className="py-2 px-2 font-mono font-bold text-heading">{(r.ratio_128 * 100).toFixed(0)}%</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>
          </div>
        </div>

      </div>

      {/* 订单明细与规划消纳结果 (左右对齐并排展示) */}
      <div className="flex-1 grid grid-cols-1 lg:grid-cols-12 gap-3 min-h-0 overflow-hidden">
        
        {/* 左侧：需求订单与顺序 (5/12 cols) */}
        <div className="lg:col-span-5 bg-card border border-main rounded-xs overflow-hidden flex flex-col min-h-0 select-none">
          <div className="border-b border-main p-3 bg-table-header">
            <h3 className="text-xs font-bold text-heading flex items-center">
              <Boxes className="w-4 h-4 text-indigo-500 mr-1.5" />
              需求订单与消纳序列 (Demand Sequence)
            </h3>
          </div>
          <div className="flex-grow overflow-y-auto p-3">
            <table className="w-full text-left text-xs border-collapse">
              <thead>
                <tr className="border-b border-main bg-slate-900/10 text-muted">
                  <th className="py-2 px-2">订单编号</th>
                  <th className="py-2 px-2">需求数量</th>
                  <th className="py-2 px-2">匹配维度组</th>
                  <th className="py-2 px-2 text-right">消纳顺序</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-800/40">
                {demands.map(d => (
                  <tr key={d.order_code} className="hover:bg-slate-900/15">
                    <td className="py-2.5 px-2 font-bold text-heading font-mono">{d.order_code}</td>
                    <td className="py-2.5 px-2 font-mono text-heading font-bold">{d.qty.toLocaleString()}</td>
                    <td className="py-2.5 px-2 font-mono text-muted">{d.dimension_grp}</td>
                    <td className="py-2.5 px-2 text-right">
                      <span className="px-1.5 py-0.5 rounded font-mono font-bold bg-slate-850 text-indigo-400 border border-slate-800 text-[10px]">
                        Seq {d.sequence}
                      </span>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>

        {/* 右侧：联副产品消纳与排产结果 (7/12 cols) */}
        <div className="lg:col-span-7 bg-card border border-main rounded-xs overflow-hidden flex flex-col min-h-0 select-none">
          <div className="border-b border-main p-3 bg-table-header flex justify-between items-center">
            <h3 className="text-xs font-bold text-heading flex items-center">
              <Activity className="w-4 h-4 text-indigo-500 mr-1.5" />
              协同分配与联产品库存留存结果 (Engine Netting Output)
            </h3>
            <span className="text-[9px] text-emerald-400 font-mono font-bold flex items-center animate-pulse">
              <span className="w-1.5 h-1.5 bg-emerald-500 rounded-full mr-1"></span>
              SOLVER_STATE: SOLVED
            </span>
          </div>

          <div className="flex-1 overflow-y-auto p-3 space-y-4 text-xs">
            {/* 订单级分配明细 */}
            <div className="space-y-1.5">
            <div className="text-[10px] text-muted font-bold uppercase tracking-wider">订单维度消纳匹配 (Order Allocation Map)</div>
              <div className="border border-main rounded overflow-hidden">
                <table className="w-full text-left border-collapse">
                  <thead>
                    <tr className="bg-slate-900/30 text-muted">
                      <th className="py-1.5 px-2">订单</th>
                      <th className="py-1.5 px-2 font-mono text-center">分配 512MB</th>
                      <th className="py-1.5 px-2 font-mono text-center">分配 256MB</th>
                      <th className="py-1.5 px-2 font-mono text-center">分配 128MB</th>
                      <th className="py-1.5 px-2 text-right">未满足缺口</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-slate-800/40">
                    {allocations.map((a, idx) => (
                      <tr key={idx} className="hover:bg-slate-900/10">
                        <td className="py-2 px-2 font-bold text-heading font-mono">{a.order_code}</td>
                        <td className="py-2 px-2 font-mono text-center text-heading font-bold">{a.allocated_512?.toLocaleString()}</td>
                        <td className="py-2 px-2 font-mono text-center text-heading font-bold">{a.allocated_256?.toLocaleString()}</td>
                        <td className="py-2 px-2 font-mono text-center text-heading font-bold">{a.allocated_128?.toLocaleString()}</td>
                        <td className="py-2 px-2 text-right font-mono font-bold">
                          {a.shortage > 0 ? (
                            <span className="text-rose-400">{a.shortage.toLocaleString()}</span>
                          ) : (
                            <span className="text-emerald-500">0</span>
                          )}
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>

            {/* 产线投产批次与留存 leftovers */}
            <div className="space-y-1.5">
              <div className="text-[10px] text-muted font-bold uppercase tracking-wider">工艺投产批次及库存留存 (Leftover Schedule)</div>
              <div className="border border-main rounded overflow-hidden">
                <table className="w-full text-left border-collapse">
                  <thead>
                    <tr className="bg-slate-900/30 text-muted">
                      <th className="py-1.5 px-2">生产工艺</th>
                      <th className="py-1.5 px-2 text-center">开工批次数</th>
                      <th className="py-1.5 px-2 font-mono text-center">留存 512MB</th>
                      <th className="py-1.5 px-2 font-mono text-center">留存 256MB</th>
                      <th className="py-1.5 px-2 font-mono text-center">留存 128MB</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-slate-800/40">
                    {schedules.map((s, idx) => (
                      <tr key={idx} className="hover:bg-slate-900/10">
                        <td className="py-2 px-2 font-bold text-heading font-mono">{s.routing_code}</td>
                        <td className="py-2 px-2 text-center text-indigo-400 font-bold font-mono">{s.batch_count} 批</td>
                        <td className="py-2 px-2 font-mono text-center text-muted">{s.leftover_512?.toLocaleString()}</td>
                        <td className="py-2 px-2 font-mono text-center text-muted">{s.leftover_256?.toLocaleString()}</td>
                        <td className="py-2 px-2 font-mono text-center text-muted">{s.leftover_128?.toLocaleString()}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>

          </div>
        </div>

      </div>

    </div>
  );
};

export default ABPConfig;
