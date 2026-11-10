import React, { useState, useEffect } from 'react';
import { AgGridReact } from 'ag-grid-react';
import { 
  Landmark, 
  TrendingUp, 
  ShieldAlert, 
  BadgeDollarSign, 
  Edit, 
  Send, 
  Sliders, 
  HelpCircle,
  TrendingDown,
  Sparkles,
  Calculator
} from 'lucide-react';
import { 
  AreaChart, 
  Area, 
  LineChart, 
  Line, 
  XAxis, 
  YAxis, 
  CartesianGrid, 
  Tooltip, 
  ResponsiveContainer, 
  Legend 
} from 'recharts';
import { useScenario } from '../../contexts/ScenarioContext';

import 'ag-grid-community/styles/ag-grid.css';
import 'ag-grid-community/styles/ag-theme-balham.css';

const IbpFinance = () => {
  const { currentScenario, fetchKpis, runSimulation } = useScenario();
  
  const [financials, setFinancials] = useState({
    total_revenue: 1560000000.0,
    inventory_carrying_cost: 3450000.0,
    purchasing_cost: 480000000.0
  });
  
  const [forecasts, setForecasts] = useState([]);
  const [loading, setLoading] = useState(false);
  
  // Override form states
  const [selectedPart, setSelectedPart] = useState('PART_3');
  const [salesQty, setSalesQty] = useState(0);
  const [marketingQty, setMarketingQty] = useState(0);
  const [statisticalQty, setStatisticalQty] = useState(0);
  const [consensusQty, setConsensusQty] = useState(0);
  const [isUpdating, setIsUpdating] = useState(false);

  // Tab Control and Disaggregation states
  const [activeFormTab, setActiveFormTab] = useState('override');
  const [familyTarget, setFamilyTarget] = useState(0);
  const [disaggregationRule, setDisaggregationRule] = useState('proportional');
  const [isDisaggregating, setIsDisaggregating] = useState(false);

  // Demand Shaping states
  const [priceChange, setPriceChange] = useState(0.0);
  const [elasticity, setElasticity] = useState(-1.5);
  const [promoMultiplier, setPromoMultiplier] = useState(1.0);
  const [isShaping, setIsShaping] = useState(false);

  const familyCurrentTotal = forecasts.reduce((acc, curr) => acc + (parseFloat(curr.qty) || 0), 0);

  const fetchFinancialLedger = async () => {
    try {
      const res = await fetch('/api/table?name=ipc_financial_ledger');
      const data = await res.json();
      if (data.length > 0) {
        const row = data.find(r => r.scenario_code === currentScenario) || data[0];
        setFinancials({
          total_revenue: row.total_revenue || 0,
          inventory_carrying_cost: row.inventory_carrying_cost || 0,
          purchasing_cost: row.purchasing_cost || 0
        });
      }
    } catch (err) {
      console.error('Error fetching financial ledger:', err);
    }
  };

  const fetchConsensusForecast = async () => {
    setLoading(true);
    try {
      const res = await fetch('/api/table?name=ipc_consensus_forecast');
      const data = await res.json();
      setForecasts(data);
      
      // Auto fill form fields for initially selected part
      const initialPart = data.find(f => f.part === selectedPart) || data[0];
      if (initialPart) {
        setSalesQty(Math.round(initialPart.sales_qty || initialPart.qty * 1.05));
        setMarketingQty(Math.round(initialPart.marketing_qty || initialPart.qty * 0.98));
        setStatisticalQty(Math.round(initialPart.statistical_qty || initialPart.qty * 0.95));
        setConsensusQty(Math.round(initialPart.qty));
      }
    } catch (err) {
      console.error('Error fetching consensus forecasts:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchFinancialLedger();
    fetchConsensusForecast();
  }, [currentScenario]);

  // Update form values when selected part changes
  useEffect(() => {
    if (forecasts.length > 0) {
      const matched = forecasts.find(f => f.part === selectedPart);
      if (matched) {
        setSalesQty(Math.round(matched.sales_qty || matched.qty * 1.05));
        setMarketingQty(Math.round(matched.marketing_qty || matched.qty * 0.98));
        setStatisticalQty(Math.round(matched.statistical_qty || matched.qty * 0.95));
        setConsensusQty(Math.round(matched.qty));
      }
    }
  }, [selectedPart, forecasts]);

  const handleDisaggregate = async (e) => {
    e.preventDefault();
    setIsDisaggregating(true);
    try {
      const res = await fetch('/api/ibp/disaggregate', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          target_qty: parseFloat(familyTarget),
          rule: disaggregationRule
        })
      });
      const data = await res.json();
      if (data.status === 'success') {
        const alertDiv = document.createElement('div');
        alertDiv.className = "fixed bottom-5 right-5 bg-gradient-to-r from-purple-950 to-indigo-950 border border-purple-600 text-purple-200 px-5 py-4 rounded shadow-2xl text-xs z-50 animate-bounce max-w-sm";
        alertDiv.innerHTML = `
          <div class="font-bold mb-1 text-purple-300 flex items-center">
            <span class="w-2 h-2 bg-purple-400 rounded-full mr-2 animate-ping"></span>
            预测层级分解完成！
          </div>
          <div>已按照 ${disaggregationRule === 'proportional' ? '销售比例' : '等额规则'} 分摊至各明细零件，正在运行重算...</div>
        `;
        document.body.appendChild(alertDiv);
        setTimeout(() => alertDiv.remove(), 4000);
        
        await runSimulation();
        await fetchConsensusForecast();
        await fetchFinancialLedger();
        await fetchKpis();
      } else {
        alert("层级分解失败: " + data.message);
      }
    } catch (err) {
      console.error(err);
    } finally {
      setIsDisaggregating(false);
    }
  };

  const handleDemandShaping = async (e) => {
    e.preventDefault();
    setIsShaping(true);
    try {
      const res = await fetch('/api/ibp/demand_shaping', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          price_change: parseFloat(priceChange),
          elasticity: parseFloat(elasticity),
          promo_multiplier: parseFloat(promoMultiplier),
          part_code: selectedPart
        })
      });
      const data = await res.json();
      if (data.status === 'success') {
        const lift = Math.round((data.lift_multiplier - 1.0) * 100);
        const alertDiv = document.createElement('div');
        alertDiv.className = "fixed bottom-5 right-5 bg-gradient-to-r from-emerald-950 to-indigo-950 border border-emerald-600 text-emerald-200 px-5 py-4 rounded shadow-2xl text-xs z-50 animate-bounce max-w-sm";
        alertDiv.innerHTML = `
          <div class="font-bold mb-1 text-emerald-300 flex items-center">
            <span class="w-2 h-2 bg-emerald-400 rounded-full mr-2 animate-ping"></span>
            需求塑造应用成功！
          </div>
          <div>预期毛需求增幅 ${lift}%，正在重新核算供应容量...</div>
        `;
        document.body.appendChild(alertDiv);
        setTimeout(() => alertDiv.remove(), 4000);
        
        await runSimulation();
        await fetchConsensusForecast();
        await fetchFinancialLedger();
        await fetchKpis();
      } else {
        alert("需求塑造失败: " + data.message);
      }
    } catch (err) {
      console.error(err);
    } finally {
      setIsShaping(false);
    }
  };

  const handleForecastUpdate = async (e) => {
    e.preventDefault();
    setIsUpdating(true);
    
    const matched = forecasts.find(f => f.part === selectedPart);
    const customer = matched?.customer || 'Samsung';
    const price = matched?.unit_price || 60.0;

    try {
      const res = await fetch('/api/ibp/forecast/update', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          part: selectedPart,
          customer: customer,
          qty: consensusQty,
          unit_price: price,
          sales_qty: salesQty,
          marketing_qty: marketingQty,
          statistical_qty: statisticalQty
        })
      });
      const data = await res.json();
      
      if (data.status === 'success') {
        const alertDiv = document.createElement('div');
        alertDiv.className = "fixed bottom-5 right-5 bg-gradient-to-r from-emerald-950 to-indigo-950 border border-emerald-600 text-emerald-200 px-5 py-4 rounded shadow-2xl text-xs z-50 animate-bounce max-w-sm";
        alertDiv.innerHTML = `
          <div class="font-bold mb-1 text-emerald-300 flex items-center">
            <span class="w-2 h-2 bg-emerald-400 rounded-full mr-2 animate-ping"></span>
            协同共识预测保存成功！
          </div>
          <div>${data.message}</div>
        `;
        document.body.appendChild(alertDiv);
        setTimeout(() => alertDiv.remove(), 4000);
        
        await fetchConsensusForecast();
        await fetchFinancialLedger();
        await fetchKpis();
      } else {
        alert(`修改失败: ${data.message}`);
      }
    } catch (err) {
      console.error('Error updating IBP forecast:', err);
    } finally {
      setIsUpdating(false);
    }
  };

  // Quick consensus actions
  const applyConsensus = (type) => {
    if (type === 'sales') {
      setConsensusQty(salesQty);
    } else if (type === 'marketing') {
      setConsensusQty(marketingQty);
    } else if (type === 'ai') {
      setConsensusQty(statisticalQty);
    } else if (type === 'average') {
      // 40% AI + 40% Sales + 20% Marketing
      const avg = Math.round(statisticalQty * 0.4 + salesQty * 0.4 + marketingQty * 0.2);
      setConsensusQty(avg);
    }
  };

  // Simulate 12-week time-series probability band for the selected part
  const getTimeSeriesChartData = () => {
    const matched = forecasts.find(f => f.part === selectedPart);
    if (!matched) return [];

    const baseQty = parseFloat(consensusQty) || 30000.0;
    const baseSales = parseFloat(salesQty) || baseQty * 1.05;
    const baseMkt = parseFloat(marketingQty) || baseQty * 0.98;
    const baseAi = parseFloat(statisticalQty) || baseQty * 0.95;

    const weeks = [];
    const baseDate = new Date(matched.date || '2026-05-29');

    for (let w = 1; w <= 12; w++) {
      // Add seasonality fluctuation (sine wave + small random)
      const factor = 1.0 + Math.sin((w / 12) * Math.PI * 2) * 0.12;
      
      const p50 = Math.round(baseAi * factor);
      // As time horizon increases, forecast uncertainty (band width) increases
      const uncertaintyFactor = 0.08 + w * 0.015;
      const p90 = Math.round(baseSales * factor * (1 + uncertaintyFactor));
      const p10 = Math.round(baseMkt * factor * (1 - uncertaintyFactor));
      const consensus = Math.round(baseQty * factor);

      const d = new Date(baseDate);
      d.setDate(baseDate.getDate() + (w - 1) * 7);
      const dateStr = `${d.getMonth() + 1}/${d.getDate()}`;

      weeks.push({
        week: `W${w}`,
        date: dateStr,
        P10: Math.max(0, p10),
        P50: p50,
        P90: p90,
        Consensus: consensus
      });
    }
    return weeks;
  };

  const timeSeriesData = getTimeSeriesChartData();

  const forecastColumnDefs = [
    {
      headerName: '产品零件号 (Part)',
      field: 'part',
      sortable: true,
      filter: true,
      width: 110,
      cellRenderer: (params) => <span className="font-mono font-bold text-heading">{params.value}</span>
    },
    {
      headerName: '销售客户 (Customer)',
      field: 'customer',
      sortable: true,
      filter: true,
      width: 110,
      cellRenderer: (params) => (
        <span className="px-2 py-0.5 rounded bg-main border border-main text-[9px] font-bold">
          {params.value}
        </span>
      )
    },
    {
      headerName: '销售提报 (Sales)',
      field: 'sales_qty',
      type: 'numericColumn',
      sortable: true,
      width: 100,
      cellRenderer: (params) => (
        <span className="cyber-font text-indigo-400">{Math.round(params.value || params.data.qty * 1.05).toLocaleString()}</span>
      )
    },
    {
      headerName: '市场预测 (Mkt)',
      field: 'marketing_qty',
      type: 'numericColumn',
      sortable: true,
      width: 100,
      cellRenderer: (params) => (
        <span className="cyber-font text-amber-500">{Math.round(params.value || params.data.qty * 0.98).toLocaleString()}</span>
      )
    },
    {
      headerName: 'AI统计预测 (AI P50)',
      field: 'statistical_qty',
      type: 'numericColumn',
      sortable: true,
      width: 120,
      cellRenderer: (params) => (
        <span className="cyber-font text-purple-400">{Math.round(params.value || params.data.qty * 0.95).toLocaleString()}</span>
      )
    },
    {
      headerName: '最终共识量 (Consensus)',
      field: 'qty',
      type: 'numericColumn',
      sortable: true,
      width: 120,
      cellRenderer: (params) => (
        <span className="cyber-font text-heading font-bold">{Math.round(params.value).toLocaleString()}</span>
      )
    },
    {
      headerName: '单价 (Price)',
      field: 'unit_price',
      type: 'numericColumn',
      sortable: true,
      width: 90,
      cellRenderer: (params) => (
        <span className="cyber-font text-muted">¥{parseFloat(params.value).toFixed(1)}</span>
      )
    },
    {
      headerName: '共识总收入 (Consensus Rev)',
      field: 'consensus_forecast',
      type: 'numericColumn',
      sortable: true,
      width: 140,
      cellRenderer: (params) => {
        const val = parseFloat(params.value);
        return (
          <span className="cyber-font text-[var(--color-success)] font-bold">
            ¥{val ? val.toLocaleString() : '0.00'}
          </span>
        );
      }
    }
  ];

  const defaultColDef = {
    sortable: true,
    resizable: true,
    filter: true,
  };

  const partOptions = [...new Set(forecasts.map(f => f.part))].filter(Boolean);

  return (
    <div className="flex flex-col h-full overflow-hidden space-y-3">
      
      {/* 顶部财务指标面板 */}
      <div className="grid grid-cols-3 gap-3 select-none">
        
        <div className="workbench-card p-3 flex items-center space-x-3 bg-card">
          <div className="p-2.5 bg-emerald-500/10 rounded text-emerald-500"><Landmark className="w-5 h-5" /></div>
          <div>
            <p className="text-[9px] text-muted uppercase tracking-wider">Consensus Total Revenue (共识总营收)</p>
            <h3 className="text-md font-bold text-heading cyber-font">¥{financials.total_revenue.toLocaleString()}</h3>
          </div>
        </div>

        <div className="workbench-card p-3 flex items-center space-x-3 bg-card">
          <div className="p-2.5 bg-rose-500/10 rounded text-rose-500"><ShieldAlert className="w-5 h-5" /></div>
          <div>
            <p className="text-[9px] text-muted uppercase tracking-wider">Inventory Carrying Cost (库存持有成本)</p>
            <h3 className="text-md font-bold text-heading cyber-font">
              ¥{financials.inventory_carrying_cost.toLocaleString()}
              <span className="text-[8.5px] text-emerald-400 font-bold ml-1.5">(¥50k optimized)</span>
            </h3>
          </div>
        </div>

        <div className="workbench-card p-3 flex items-center space-x-3 bg-card">
          <div className="p-2.5 bg-indigo-500/10 rounded text-indigo-500"><BadgeDollarSign className="w-5 h-5" /></div>
          <div>
            <p className="text-[9px] text-muted uppercase tracking-wider">Total Procurement Cost (采购总成本)</p>
            <h3 className="text-md font-bold text-heading cyber-font">¥{financials.purchasing_cost.toLocaleString()}</h3>
          </div>
        </div>

      </div>

      {/* 下部数据大格及调整表单 */}
      <div className="flex-1 flex flex-col lg:flex-row gap-3 overflow-hidden min-h-0">
        
        {/* 左侧 IBP 对账大工作簿 */}
        <div className="flex-1 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden">
          <div className="border-b border-main pb-2 mb-3 select-none flex justify-between items-center">
            <h3 className="text-xs font-bold text-heading flex items-center">
              <TrendingUp className="w-4 h-4 text-indigo-500 mr-1.5" />
              S&OP 协同多轨需求共识表 (Consensus Forecast Worksheet)
            </h3>
            <span className="text-[9px] text-muted font-bold">并排对账销售、市场、AI多轨提报量</span>
          </div>

          <div className="flex-1 w-full overflow-hidden relative min-h-[180px]">
            {loading && (
              <div className="absolute inset-0 bg-card/85 backdrop-blur-xs flex items-center justify-center z-20 text-xs text-indigo-400">
                获取共识明细账册...
              </div>
            )}

            <div className="ag-theme-balham-dark w-full h-full">
              <AgGridReact
                theme="legacy"
                rowData={forecasts}
                columnDefs={forecastColumnDefs}
                defaultColDef={defaultColDef}
                headerHeight={26}
                rowHeight={22}
              />
            </div>
          </div>
        </div>

        {/* 右侧时序 AI 预测大盘与共识对账面板 */}
        <div className="w-full lg:w-[380px] flex flex-col gap-3 h-full overflow-hidden flex-shrink-0">
          
          {/* AI 12周滚动预测带图表 */}
          <div className="bg-card border border-main rounded p-3 flex flex-col h-[180px] select-none">
            <div className="flex justify-between items-center border-b border-main pb-1.5 mb-1.5">
              <span className="text-[10px] font-bold text-heading flex items-center">
                <Sparkles className="w-3.5 h-3.5 text-indigo-400 mr-1" />
                AI时序滚动预测包络图 (AI Probability Band)
              </span>
              <span className="text-[8px] bg-slate-900 border border-slate-700 text-indigo-400 px-1 rounded font-mono">
                {selectedPart} 12W
              </span>
            </div>

            <div className="flex-1 min-h-0 bg-input/20 border border-main/60 rounded p-1">
              <ResponsiveContainer width="100%" height="100%">
                <AreaChart data={timeSeriesData} margin={{ top: 5, right: 5, left: -25, bottom: 0 }}>
                  <XAxis dataKey="week" stroke="var(--text-muted)" fontSize={7.5} tickLine={false} />
                  <YAxis stroke="var(--text-muted)" fontSize={7.5} tickLine={false} />
                  <Tooltip 
                    contentStyle={{ backgroundColor: 'var(--bg-card)', borderColor: 'var(--border-color)', fontSize: '8.5px', fontFamily: 'monospace' }}
                    labelFormatter={(label, items) => `周次: ${label} (${items[0]?.payload?.date || ''})`}
                  />
                  {/* P10 - P90 Probability Band shaded area */}
                  <Area 
                    name="P10-P90 区间"
                    type="monotone" 
                    dataKey="P90" 
                    stroke="none" 
                    fill="#3b82f6" 
                    fillOpacity={0.15} 
                  />
                  <Area 
                    type="monotone" 
                    dataKey="P10" 
                    stroke="none" 
                    fill="var(--bg-card)" // Effectively clips the area below P10
                    fillOpacity={1.0} 
                  />
                  {/* AI P50 Line */}
                  <Line 
                    name="AI P50 预测"
                    type="monotone" 
                    dataKey="P50" 
                    stroke="#8b5cf6" 
                    strokeWidth={1} 
                    dot={false} 
                    strokeDasharray="3 3"
                  />
                  {/* Consensus Line */}
                  <Line 
                    name="最终共识"
                    type="monotone" 
                    dataKey="Consensus" 
                    stroke="#10b981" 
                    strokeWidth={1.8} 
                    dot={{ r: 1 }}
                  />
                </AreaChart>
              </ResponsiveContainer>
            </div>
            
            <div className="flex justify-between items-center text-[7.5px] text-muted mt-1 font-mono">
              <div className="flex items-center"><span className="w-1.5 h-1.5 bg-[#3b82f6]/20 mr-1 rounded-sm"></span> P10-P90 波动带</div>
              <div className="flex items-center"><span className="w-1.5 h-px border-t border-dashed border-[#8b5cf6] mr-1"></span> AI P50 推荐</div>
              <div className="flex items-center"><span className="w-2 h-0.5 bg-[#10b981] mr-1"></span> 最终共识</div>
            </div>
          </div>

          {/* 共识对账 Override & 预测层级分解控制面板 */}
          <div className="flex-1 bg-card border border-main rounded-xs p-3 flex flex-col overflow-y-auto select-none min-h-0">
            {/* Sub-tabs header */}
            <div className="flex border-b border-main pb-2 mb-3">
              <button
                type="button"
                onClick={() => setActiveFormTab('override')}
                className={`flex-1 text-center py-1 text-[10.5px] font-bold rounded-l transition-all cursor-pointer ${
                  activeFormTab === 'override' 
                    ? 'bg-indigo-900/20 text-indigo-400 border border-indigo-900/40' 
                    : 'text-muted hover:text-heading'
                }`}
              >
                📝 多轨 共识
              </button>
              <button
                type="button"
                onClick={() => setActiveFormTab('disaggregate')}
                className={`flex-1 text-center py-1 text-[10.5px] font-bold transition-all cursor-pointer ${
                  activeFormTab === 'disaggregate' 
                    ? 'bg-purple-900/20 text-purple-400 border border-purple-900/40' 
                    : 'text-muted hover:text-heading'
                }`}
              >
                🔀 层级分解
              </button>
              <button
                type="button"
                onClick={() => setActiveFormTab('shaping')}
                className={`flex-1 text-center py-1 text-[10.5px] font-bold rounded-r transition-all cursor-pointer ${
                  activeFormTab === 'shaping' 
                    ? 'bg-emerald-900/20 text-emerald-400 border border-emerald-900/40' 
                    : 'text-muted hover:text-heading'
                }`}
              >
                🎯 需求塑造
              </button>
            </div>

            {activeFormTab === 'override' && (
              <form onSubmit={handleForecastUpdate} className="space-y-3 flex-1 flex flex-col">
                <div className="space-y-1">
                  <label className="text-[9.5px] text-muted font-bold block">调整目标零件代号 (Select Target Part)</label>
                  <select
                    value={selectedPart}
                    onChange={(e) => setSelectedPart(e.target.value)}
                    className="w-full bg-input border border-main text-heading text-[11px] p-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono cursor-pointer"
                  >
                    {partOptions.map(opt => (
                      <option key={opt} value={opt}>{opt}</option>
                    ))}
                  </select>
                </div>

                {/* 多轨输入 */}
                <div className="grid grid-cols-2 gap-2">
                  <div className="space-y-0.5">
                    <label className="text-[8.5px] text-indigo-400 font-bold block">销售提报量 (Sales Qty)</label>
                    <input
                      type="number"
                      value={salesQty}
                      onChange={(e) => setSalesQty(parseInt(e.target.value) || 0)}
                      className="w-full bg-input border border-main text-heading text-[11px] p-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono"
                    />
                  </div>

                  <div className="space-y-0.5">
                    <label className="text-[8.5px] text-amber-500 font-bold block">市场提报量 (Marketing Qty)</label>
                    <input
                      type="number"
                      value={marketingQty}
                      onChange={(e) => setMarketingQty(parseInt(e.target.value) || 0)}
                      className="w-full bg-input border border-main text-heading text-[11px] p-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono"
                    />
                  </div>

                  <div className="space-y-0.5">
                    <label className="text-[8.5px] text-purple-400 font-bold block">AI 统计推荐 (AI P50 Qty)</label>
                    <input
                      type="number"
                      value={statisticalQty}
                      onChange={(e) => setStatisticalQty(parseInt(e.target.value) || 0)}
                      className="w-full bg-input border border-main text-heading text-[11px] p-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono"
                    />
                  </div>

                  <div className="space-y-0.5">
                    <label className="text-[8.5px] text-heading font-bold block">最终共识量 (Consensus Qty)</label>
                    <input
                      type="number"
                      value={consensusQty}
                      onChange={(e) => setConsensusQty(parseInt(e.target.value) || 0)}
                      className="w-full bg-input border border-indigo-500 text-heading text-[11px] p-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono bg-indigo-950/20 font-bold"
                    />
                  </div>
                </div>

                {/* 协同共识一键按键 */}
                <div className="space-y-1.5 border-t border-slate-800/80 pt-2.5 mt-1">
                  <span className="text-[9px] text-muted font-bold block">一键达成共识快捷决策 (Consensus Actions):</span>
                  <div className="grid grid-cols-2 gap-1.5 text-[9.5px]">
                    <button
                      type="button"
                      onClick={() => applyConsensus('sales')}
                      className="px-2 py-1 bg-input border border-main text-muted hover:text-indigo-400 hover:border-indigo-500/50 rounded text-left transition-all cursor-pointer font-medium"
                    >
                      🚀 采用销售建议值
                    </button>
                    <button
                      type="button"
                      onClick={() => applyConsensus('marketing')}
                      className="px-2 py-1 bg-input border border-main text-muted hover:text-amber-400 hover:border-amber-500/50 rounded text-left transition-all cursor-pointer font-medium"
                    >
                      📉 采用市场保守值
                    </button>
                    <button
                      type="button"
                      onClick={() => applyConsensus('ai')}
                      className="px-2 py-1 bg-input border border-main text-muted hover:text-purple-400 hover:border-purple-500/50 rounded text-left transition-all cursor-pointer font-medium"
                    >
                      🤖 采用 AI 推荐 (P50)
                    </button>
                    <button
                      type="button"
                      onClick={() => applyConsensus('average')}
                      className="px-2 py-1 bg-indigo-950/30 border border-indigo-900/50 text-indigo-400 hover:bg-indigo-900/30 hover:border-indigo-700 rounded text-left transition-all cursor-pointer flex items-center font-bold"
                    >
                      <Calculator className="w-3.5 h-3.5 mr-1" />
                      加权平均达成共识
                    </button>
                  </div>
                </div>

                {/* 提交按钮 */}
                <div className="pt-3 mt-auto">
                  <button
                    type="submit"
                    disabled={isUpdating}
                    className="w-full btn-premium-primary py-2 text-xs flex items-center justify-center space-x-1.5 disabled:opacity-50"
                  >
                    <Send className="w-3.5 h-3.5" />
                    <span>{isUpdating ? '同步中...' : '提交共识结果并核算财务大盘'}</span>
                  </button>
                </div>
              </form>
            )}

            {activeFormTab === 'disaggregate' && (
              <form onSubmit={handleDisaggregate} className="space-y-4 flex-1 flex flex-col justify-between">
                <div className="space-y-3">
                  <div className="bg-[#0f121d] border border-main p-2.5 rounded text-[10px] space-y-1 text-muted">
                    <span className="font-bold text-purple-400 block">💡 什么是 S&OP 预测层级分解？</span>
                    <p className="leading-relaxed">
                      允许您在聚合层级录入预测总量，系统会自动按照所选规则分配下传至明细物料零件（PART_0 至 PART_3），从而保持宏观规划与微观执行的物理一致性。
                    </p>
                  </div>

                  <div className="space-y-1">
                    <label className="text-[10px] text-muted font-bold block">产品系列 (Product Family)</label>
                    <div className="w-full bg-[#171b29] border border-main text-purple-400 text-xs px-2 py-1.5 rounded font-mono font-bold select-none">
                      CHIP_FAMILY (全部成品芯片)
                    </div>
                  </div>

                  <div className="space-y-1">
                    <label className="text-[10px] text-muted font-bold block">当前系列预测总量 (Current Sum)</label>
                    <div className="w-full bg-[#171b29] border border-main text-[#f8fafc]/55 text-xs px-2 py-1.5 rounded font-mono select-none">
                      {Math.round(familyCurrentTotal).toLocaleString()} 颗
                    </div>
                  </div>

                  <div className="space-y-1">
                    <label className="text-[10px] text-muted font-bold block">新设定预测目标总量 (Target Qty)</label>
                    <input
                      type="number"
                      value={familyTarget}
                      onChange={(e) => setFamilyTarget(parseInt(e.target.value) || 0)}
                      className="w-full bg-input border border-main text-heading text-xs p-2 rounded focus:outline-none focus:border-purple-500 font-mono font-bold"
                    />
                  </div>

                  <div className="space-y-1">
                    <label className="text-[10px] text-muted font-bold block">预测分解规则 (Disaggregation Rule)</label>
                    <select
                      value={disaggregationRule}
                      onChange={(e) => setDisaggregationRule(e.target.value)}
                      className="w-full bg-input border border-main text-heading text-xs p-2 rounded focus:outline-none focus:border-purple-500 cursor-pointer"
                    >
                      <option value="proportional">📐 历史销售混配比例分摊 (Proportional Mix)</option>
                      <option value="evenly">⚖️ 零件等额分配分摊 (Even Distribution)</option>
                    </select>
                  </div>
                </div>

                <div className="pt-4 mt-auto">
                  <button
                    type="submit"
                    disabled={isDisaggregating || familyTarget <= 0}
                    className="w-full py-2 bg-purple-700 hover:bg-purple-600 text-white rounded font-bold text-xs flex items-center justify-center space-x-1.5 disabled:opacity-50 transition-all shadow-lg shadow-purple-900/20 cursor-pointer"
                  >
                    <Sliders className="w-3.5 h-3.5" />
                    <span>{isDisaggregating ? '正在计算并下钻分解...' : '执行预测层级分解并对账'}</span>
                  </button>
                </div>
              </form>
            )}

            {activeFormTab === 'shaping' && (
              <form onSubmit={handleDemandShaping} className="space-y-3 flex-1 flex flex-col justify-between min-h-0">
                <div className="space-y-3">
                  <div className="bg-[#0f121d] border border-main p-2.5 rounded text-[10px] space-y-1 text-muted">
                    <span className="font-bold text-emerald-400 block">💡 什么是 S&OP 需求塑造 (Demand Shaping)？</span>
                    <p className="leading-relaxed">
                      通过促销活动（Promotion）或降价打折（Discount），利用市场价格弹性引导客户购买，主动拉升或推延需求量，以匹配现有的产能上限。
                    </p>
                  </div>

                  <div className="space-y-1">
                    <label className="text-[10px] text-muted font-bold block">目标物料 (Target Part)</label>
                    <select
                      value={selectedPart}
                      onChange={(e) => setSelectedPart(e.target.value)}
                      className="w-full bg-input border border-main text-heading text-[11px] p-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono cursor-pointer"
                    >
                      {partOptions.map(opt => (
                        <option key={opt} value={opt}>{opt}</option>
                      ))}
                    </select>
                  </div>

                  <div className="space-y-1">
                    <div className="flex justify-between items-center text-[10px] text-muted">
                      <span className="font-bold">价格变化比例 (Price Change):</span>
                      <span className="font-mono font-bold text-heading">{(priceChange * 100).toFixed(0)}%</span>
                    </div>
                    <input
                      type="range"
                      min="-0.5"
                      max="0.5"
                      step="0.05"
                      value={priceChange}
                      onChange={(e) => setPriceChange(parseFloat(e.target.value))}
                      className="w-full h-1.5 bg-slate-800 rounded-lg appearance-none cursor-pointer accent-indigo-500"
                    />
                  </div>

                  <div className="grid grid-cols-2 gap-2">
                    <div className="space-y-0.5">
                      <label className="text-[8.5px] text-muted font-bold block">弹性系数 (Elasticity)</label>
                      <input
                        type="number"
                        step="0.1"
                        value={elasticity}
                        onChange={(e) => setElasticity(parseFloat(e.target.value) || 0)}
                        className="w-full bg-input border border-main text-heading text-[11px] p-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono"
                      />
                    </div>

                    <div className="space-y-0.5">
                      <div className="flex justify-between items-center text-[8.5px] text-muted">
                        <span className="font-bold">促销拉动 (Promo):</span>
                        <span className="font-mono font-bold text-heading">x{promoMultiplier.toFixed(2)}</span>
                      </div>
                      <input
                        type="number"
                        step="0.05"
                        min="1.0"
                        max="2.0"
                        value={promoMultiplier}
                        onChange={(e) => setPromoMultiplier(parseFloat(e.target.value) || 1.0)}
                        className="w-full bg-input border border-main text-heading text-[11px] p-1.5 rounded focus:outline-none"
                      />
                    </div>
                  </div>

                  {/* 即时齐套性容量分析看板 */}
                  <div className="bg-emerald-950/20 border border-emerald-900/40 p-2.5 rounded text-[10.5px] space-y-1.5">
                    <span className="font-bold text-emerald-400 block uppercase tracking-wider text-[8px]">Simulated Demand Lift Forecast</span>
                    <div className="flex justify-between items-center">
                      <span className="text-muted">预计需求拉升 (Expected Lift):</span>
                      <span className="font-mono font-bold text-emerald-400 text-xs">
                        +{(((1.0 + priceChange * elasticity) * promoMultiplier - 1.0) * 100).toFixed(1)}%
                      </span>
                    </div>
                    <div className="flex justify-between items-center">
                      <span className="text-muted">即时产能警戒 (Capacity Status):</span>
                      <span className={`font-mono font-bold px-1 py-0.2 rounded text-[8px] ${
                        priceChange < 0 || promoMultiplier > 1.1 ? 'bg-amber-950 text-amber-400 border border-amber-900' : 'bg-emerald-950 text-emerald-400 border border-emerald-900'
                      }`}>
                        {priceChange < 0 || promoMultiplier > 1.1 ? '⚠️ CAPACITY_OVERFLOW_WARNING' : '🟢 CAPACITY_LOAD_OK'}
                      </span>
                    </div>
                  </div>
                </div>

                <div className="pt-4 mt-auto">
                  <button
                    type="submit"
                    disabled={isShaping}
                    className="w-full py-2 bg-emerald-600 hover:bg-emerald-500 text-white rounded font-bold text-xs flex items-center justify-center space-x-1.5 disabled:opacity-50 transition-all shadow-lg cursor-pointer"
                  >
                    <Sliders className="w-3.5 h-3.5" />
                    <span>{isShaping ? '重算中...' : '模拟需求塑造并核算供应'}</span>
                  </button>
                </div>
              </form>
            )}
          </div>

        </div>

      </div>

    </div>
  );
};

export default IbpFinance;
