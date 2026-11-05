import React, { useState, useEffect } from 'react';
import { useScenario } from '../../contexts/ScenarioContext';
import { 
  TrendingUp, 
  Shield, 
  BarChart3, 
  AlertCircle, 
  RefreshCw, 
  GitBranch, 
  Edit, 
  Save, 
  Zap, 
  Info, 
  Layers, 
  CheckCircle2, 
  ShieldAlert, 
  ArrowDownUp,
  Cpu,
  ChevronRight,
  Database
} from 'lucide-react';

const AllotmentWorkbench = () => {
  const { currentScenario, runSimulation, isLoading } = useScenario();
  
  // States for ITP Forecast/Allocation
  const [forecasts, setForecasts] = useState([]);
  const [forecastLoading, setForecastLoading] = useState(false);
  const [editingItem, setEditingItem] = useState(null);
  const [editQty, setEditQty] = useState('');
  const [editPrice, setEditPrice] = useState('');
  
  // States for Disaggregation
  const [targetQty, setTargetQty] = useState(30000.0);
  const [disaggRule, setDisaggRule] = useState('proportional_history');
  const [disaggMsg, setDisaggMsg] = useState(null);
  
  // States for IOP Allotment & Alternate allocations
  const [materialNodes, setMaterialNodes] = useState([]);
  const [alternateAllocations, setAlternateAllocations] = useState([]);
  const [plannedOrders, setPlannedOrders] = useState([]);
  const [onHandStock, setOnHandStock] = useState([]);
  const [scheduledReceipts, setScheduledReceipts] = useState([]);
  const [iopLoading, setIopLoading] = useState(false);
  
  // Fetch ITP Forecasts
  const fetchForecasts = async () => {
    setForecastLoading(true);
    try {
      const res = await fetch('/api/table?name=ipc_consensus_forecast');
      if (res.ok) {
        const data = await res.json();
        setForecasts(Array.isArray(data) ? data : []);
      }
    } catch (err) {
      console.error("Error fetching forecasts:", err);
    } finally {
      setForecastLoading(false);
    }
  };

  // Fetch IOP Allotments, Stock and Alternate Allocations
  const fetchIopData = async () => {
    setIopLoading(true);
    try {
      const [rNodes, rAlternates, rPlanned, rOnHand, rSR] = await Promise.all([
        fetch('/api/table?name=ipc_material_node').then(r => r.ok ? r.json() : []),
        fetch('/api/table?name=ipc_alternate_allocation').then(r => r.ok ? r.json() : []),
        fetch('/api/table?name=ipc_planned_order_ledger').then(r => r.ok ? r.json() : []),
        fetch('/api/table?name=ipc_onhand').then(r => r.ok ? r.json() : []),
        fetch('/api/table?name=ipc_scheduled_receipt').then(r => r.ok ? r.json() : [])
      ]);

      setMaterialNodes(Array.isArray(rNodes) ? rNodes : []);
      setAlternateAllocations(Array.isArray(rAlternates) ? rAlternates : []);
      setPlannedOrders(Array.isArray(rPlanned) ? rPlanned : []);
      setOnHandStock(Array.isArray(rOnHand) ? rOnHand : []);
      setScheduledReceipts(Array.isArray(rSR) ? rSR : []);
    } catch (err) {
      console.error("Error fetching IOP data:", err);
    } finally {
      setIopLoading(false);
    }
  };

  useEffect(() => {
    fetchForecasts();
    fetchIopData();
  }, [currentScenario]);

  const handleEditForecast = (item) => {
    setEditingItem(item);
    setEditQty(item.qty);
    setEditPrice(item.unit_price);
  };

  const handleSaveForecast = async () => {
    if (!editingItem) return;
    setForecastLoading(true);
    try {
      const res = await fetch('/api/ibp/forecast/update', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          part: editingItem.part,
          customer: editingItem.customer,
          qty: parseFloat(editQty),
          unit_price: parseFloat(editPrice)
        })
      });
      const data = await res.json();
      if (data.status === 'success') {
        showMsg("✔ ITP 战术预测更新成功！");
        setEditingItem(null);
        fetchForecasts();
        fetchIopData();
      } else {
        alert("更新失败: " + data.message);
      }
    } catch (err) {
      console.error(err);
    } finally {
      setForecastLoading(false);
    }
  };

  const handleDisaggregate = async (e) => {
    if (e) e.preventDefault();
    setForecastLoading(true);
    showMsg("⚡ 正在触发 ITP 预测按分配比例时空拆解...");
    try {
      const res = await fetch('/api/ibp/disaggregate', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          target_qty: parseFloat(targetQty),
          rule: disaggRule
        })
      });
      const data = await res.json();
      if (data.status === 'success') {
        showMsg("✔ " + data.message);
        fetchForecasts();
        fetchIopData();
      } else {
        alert("分解失败: " + data.message);
      }
    } catch (err) {
      console.error(err);
    } finally {
      setForecastLoading(false);
    }
  };

  const handleSimulate = async () => {
    showMsg("⚡ 正在提交计划并运行 C++ 有限能力排程引擎...");
    await runSimulation();
    showMsg("✔ 引擎重新计算完成，所有 Allotment 消纳关系已重新校准对账！");
    fetchForecasts();
    fetchIopData();
  };

  const showMsg = (text) => {
    setDisaggMsg(text);
    setTimeout(() => setDisaggMsg(null), 5000);
  };

  // Helper to aggregate stock by part
  const getPartStock = (partCode) => {
    const oh = onHandStock.filter(s => s.part === partCode).reduce((acc, s) => acc + s.qty, 0);
    const sr = scheduledReceipts.filter(s => s.to_part === partCode).reduce((acc, s) => acc + s.qty, 0);
    return { oh, sr, total: oh + sr };
  };

  // Helper to calculate total planned requirements
  const getPartPlanned = (partCode) => {
    return plannedOrders.filter(p => p.part_code === partCode).reduce((acc, p) => acc + p.order_qty, 0);
  };

  // Computed metrics for cards
  const totalForecastRevenue = forecasts.reduce((acc, f) => acc + (f.consensus_forecast || 0), 0);
  const totalForecastQty = forecasts.reduce((acc, f) => acc + (f.qty || 0), 0);
  const totalAlternateAllocated = alternateAllocations.reduce((acc, a) => acc + (a.allocated_qty || 0), 0);
  
  // Total shortages from alternates & planned splits
  const activePreemptionsCount = alternateAllocations.filter(a => a.alt_class === 1).length;

  return (
    <div className="flex-1 flex flex-col space-y-3 overflow-hidden min-h-0 text-xs">
      
      {/* 状态与控制栏 */}
      {disaggMsg && (
        <div className="bg-indigo-950/40 border border-indigo-600/40 p-3 rounded flex items-center space-x-2.5 text-indigo-300 animate-fade-in select-none">
          <Info className="w-4 h-4 flex-shrink-0 animate-pulse text-indigo-400" />
          <span>{disaggMsg}</span>
        </div>
      )}

      {/* 顶部指标卡 (Kinaxis-style What-If Comparator Row) */}
      <div className="grid grid-cols-1 md:grid-cols-4 gap-2.5 bg-card border border-main rounded-xs p-3 select-none">
        
        <div className="flex items-center space-x-3 border-r border-main pr-3">
          <span className="text-xl text-indigo-500"><TrendingUp className="w-5 h-5" /></span>
          <div className="flex-1 min-w-0">
            <p className="text-[9px] text-muted uppercase tracking-wider">ITP 大盘总预测量</p>
            <h4 className="text-xs font-bold text-heading font-mono">
              {totalForecastQty.toLocaleString()} 颗
            </h4>
          </div>
        </div>

        <div className="flex items-center space-x-3 border-r border-main pr-3">
          <span className="text-xl text-purple-500"><Layers className="w-5 h-5" /></span>
          <div className="flex-1 min-w-0">
            <p className="text-[9px] text-muted uppercase tracking-wider">共识财务预测大盘</p>
            <h4 className="text-xs font-bold text-heading font-mono">
              ¥{totalForecastRevenue.toLocaleString()}
            </h4>
          </div>
        </div>

        <div className="flex items-center space-x-3 border-r border-main pr-3">
          <span className="text-xl text-emerald-500"><ArrowDownUp className="w-5 h-5" /></span>
          <div className="flex-1 min-w-0">
            <p className="text-[9px] text-muted uppercase tracking-wider">IOP 替代消纳总量</p>
            <h4 className="text-xs font-bold text-heading font-mono">
              {totalAlternateAllocated.toLocaleString()} 颗
            </h4>
          </div>
        </div>

        <div className="flex items-center space-x-3">
          <span className="text-xl text-amber-500"><ShieldAlert className="w-5 h-5 animate-pulse text-amber-500" /></span>
          <div className="flex-1 min-w-0">
            <p className="text-[9px] text-muted uppercase tracking-wider">OTP 战略前移置换次数</p>
            <h4 className="text-xs font-bold text-heading font-mono flex items-center text-amber-400">
              {activePreemptionsCount} 次
              <span className="ml-2 text-[8px] bg-amber-950 border border-amber-900 px-1 py-0.2 rounded font-normal text-amber-400 uppercase scale-90">
                ACTIVE
              </span>
            </h4>
          </div>
        </div>

      </div>

      {/* 核心工作区 (ITP & IOP 纵向解耦) */}
      <div className="flex-grow grid grid-cols-1 lg:grid-cols-12 gap-3 min-h-0 overflow-hidden">
        
        {/* 左侧：ITP 战术预测与分摊大盘 (5/12 cols) */}
        <div className="lg:col-span-5 bg-card border border-main rounded-xs overflow-hidden flex flex-col min-h-0 select-none">
          <div className="border-b border-main p-3 bg-table-header flex justify-between items-center">
            <h3 className="text-xs font-bold text-heading flex items-center">
              <TrendingUp className="w-4 h-4 text-indigo-500 mr-1.5" />
              ITP 战术层：战略共识预测与宏观配额 (Strategic Allocation Plan)
            </h3>
            <span className="text-[9px] text-indigo-400 font-mono font-bold">Time Bucket: WEEKLY</span>
          </div>

          <div className="flex-grow overflow-y-auto p-3 space-y-4">
            
            {/* 预测大盘拆解控制器 */}
            <form onSubmit={handleDisaggregate} className="bg-slate-900/10 border border-slate-800/40 p-3 rounded space-y-3">
              <div className="text-[10px] font-bold text-heading uppercase tracking-wider flex items-center">
                <Zap className="w-3.5 h-3.5 text-indigo-400 mr-1" />
                S&OP 预测量时空分解算子 (Disaggregation Matrix)
              </div>
              
              <div className="grid grid-cols-2 gap-3 text-xs">
                <div className="flex flex-col space-y-1">
                  <label className="text-[10px] text-muted">拆解目标总量 (Target Qty)</label>
                  <input
                    type="number"
                    value={targetQty}
                    onChange={(e) => setTargetQty(e.target.value)}
                    className="bg-input border border-main text-heading text-xs p-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono font-bold"
                  />
                </div>
                <div className="flex flex-col space-y-1">
                  <label className="text-[10px] text-muted">分摊决策算法 (Sourcing Rule)</label>
                  <select
                    value={disaggRule}
                    onChange={(e) => setDisaggRule(e.target.value)}
                    className="bg-input border border-main text-heading text-xs p-1.5 rounded focus:outline-none focus:border-indigo-500 font-bold"
                  >
                    <option value="proportional_history">proportional_history (历史销量占比)</option>
                    <option value="proportional">proportional (当前大盘占比)</option>
                    <option value="equal">equal (等权均摊)</option>
                  </select>
                </div>
              </div>

              <div className="flex justify-end pt-1">
                <button
                  type="submit"
                  disabled={forecastLoading}
                  className="px-3.5 py-1.5 bg-indigo-600 hover:bg-indigo-500 text-white rounded font-bold text-[10.5px] transition flex items-center space-x-1 cursor-pointer"
                >
                  <Cpu className="w-3.5 h-3.5" />
                  <span>执行时空分解并重排</span>
                </button>
              </div>
            </form>

            {/* ITP 需求表 */}
            <div className="space-y-1.5">
              <div className="text-[10px] text-muted font-bold uppercase tracking-wider">大类预测明细记录 (Consensus Forecast Sheet)</div>
              
              {forecastLoading && forecasts.length === 0 ? (
                <div className="text-center py-6 text-indigo-400">正在刷新预测数据...</div>
              ) : (
                <div className="border border-main rounded overflow-hidden">
                  <table className="w-full text-left border-collapse">
                    <thead>
                      <tr className="bg-slate-900/40 border-b border-main text-muted">
                        <th className="py-2 px-2.5">大类/零件号</th>
                        <th className="py-2 px-2">主要客户</th>
                        <th className="py-2 px-2 text-right">预测数 (Qty)</th>
                        <th className="py-2 px-2 text-right">共识金额</th>
                        <th className="py-2 px-2.5 text-center">操作</th>
                      </tr>
                    </thead>
                    <tbody className="divide-y divide-slate-800/40">
                      {forecasts.map((item, idx) => (
                        <tr key={idx} className="hover:bg-slate-900/15 font-mono">
                          <td className="py-2 px-2.5 font-bold text-heading truncate">{item.part}</td>
                          <td className="py-2 px-2 text-muted">{item.customer}</td>
                          <td className="py-2 px-2 text-right text-heading font-bold">{Math.round(item.qty).toLocaleString()}</td>
                          <td className="py-2 px-2 text-right text-indigo-400 font-bold">¥{Math.round(item.consensus_forecast).toLocaleString()}</td>
                          <td className="py-2 px-2.5 text-center">
                            <button
                              onClick={() => handleEditForecast(item)}
                              className="px-1.5 py-0.5 bg-slate-800 hover:bg-slate-700 text-heading border border-slate-700 rounded text-[9px] font-bold cursor-pointer"
                            >
                              修改
                            </button>
                          </td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              )}
            </div>

            {/* 编辑预测弹窗/抽屉 */}
            {editingItem && (
              <div className="border border-indigo-900/60 bg-indigo-950/20 p-3 rounded space-y-2.5 animate-fade-in">
                <div className="flex justify-between items-center text-[10px] font-bold text-indigo-400 uppercase">
                  <span>✏ 编辑大盘预测: {editingItem.part} ({editingItem.customer})</span>
                  <button onClick={() => setEditingItem(null)} className="text-muted hover:text-heading">取消</button>
                </div>
                <div className="grid grid-cols-2 gap-3">
                  <div className="flex flex-col space-y-1">
                    <span className="text-[9px] text-muted">预测数量 (Qty)</span>
                    <input
                      type="number"
                      value={editQty}
                      onChange={(e) => setEditQty(e.target.value)}
                      className="bg-input border border-main text-heading p-1 rounded font-bold font-mono"
                    />
                  </div>
                  <div className="flex flex-col space-y-1">
                    <span className="text-[9px] text-muted">产品单价 (Price)</span>
                    <input
                      type="number"
                      value={editPrice}
                      onChange={(e) => setEditPrice(e.target.value)}
                      className="bg-input border border-main text-heading p-1 rounded font-bold font-mono"
                    />
                  </div>
                </div>
                <div className="flex justify-end pt-1">
                  <button
                    onClick={handleSaveForecast}
                    className="px-3 py-1 bg-indigo-600 hover:bg-indigo-500 text-white rounded font-bold text-[10px] flex items-center space-x-1 cursor-pointer"
                  >
                    <Save className="w-3.5 h-3.5" />
                    <span>保存并重算</span>
                  </button>
                </div>
              </div>
            )}

          </div>
        </div>

        {/* 右侧：IOP 运营层解耦配额与 Allotment 刚性隔离屏障 (7/12 cols) */}
        <div className="lg:col-span-7 bg-card border border-main rounded-xs overflow-hidden flex flex-col min-h-0 select-none">
          <div className="border-b border-main p-3 bg-table-header flex justify-between items-center">
            <h3 className="text-xs font-bold text-heading flex items-center">
              <Shield className="w-4 h-4 text-emerald-500 mr-1.5" />
              IOP 运营层：解耦点配额与防波堤隔离屏障 (Decoupled Allotment Barrier)
            </h3>
            <span className="text-[9px] text-emerald-400 font-mono font-bold flex items-center animate-pulse">
              <span className="w-1.5 h-1.5 bg-emerald-500 rounded-full mr-1"></span>
              Allotment Lock: ENABLED
            </span>
          </div>

          <div className="flex-grow overflow-y-auto p-3 space-y-4">
            
            {/* 刚性配额防波堤概念卡控图解 */}
            <div className="bg-[#121620]/60 border border-slate-800/40 p-3 rounded-xs text-xs space-y-2">
              <div className="font-bold text-heading flex items-center text-[10.5px]">
                <ShieldAlert className="w-4 h-4 text-indigo-400 mr-1.5" />
                配额防波堤机制说明 (Allotment Protection Rule)
              </div>
              <p className="text-[10px] text-muted leading-relaxed">
                在多级 BOM 解耦点处锚定配额上限。微观排程计算时，普通订单的用料与产能扣减被**强行闭锁**在自身 Allotment 以内。
                即使低优先级订单由于洗枪优化先行跑排产，也绝对无法侵占战略大客（VVIP）在同一物料处被锁死确权的战略配额。
              </p>
              
              {/* 视觉图示 */}
              <div className="flex justify-between items-center bg-[#090b10] border border-[#1e2538] p-2.5 rounded text-[10px] font-mono">
                <div className="flex flex-col items-center space-y-1 w-[42%]">
                  <span className="text-muted">普通订单 (Normal FG)</span>
                  <div className="w-full bg-slate-850 h-5 border border-slate-700 rounded relative overflow-hidden flex items-center justify-center">
                    <div className="absolute left-0 top-0 bottom-0 bg-indigo-900/60 w-[40%]"></div>
                    <span className="z-10 font-bold text-heading">已分配 40% (Allotment上限)</span>
                  </div>
                  <span className="text-rose-400 text-[8.5px]">剩余 60% 被闭锁拦截顺延</span>
                </div>
                <div className="flex flex-col items-center w-[12%] text-indigo-500">
                  <ArrowDownUp className="w-5 h-5 animate-pulse" />
                  <span className="text-[8px] text-muted mt-1">刚性墙</span>
                </div>
                <div className="flex flex-col items-center space-y-1 w-[42%]">
                  <span className="text-muted">战略大客 (VVIP FG)</span>
                  <div className="w-full bg-slate-850 h-5 border border-slate-700 rounded relative overflow-hidden flex items-center justify-center">
                    <div className="absolute left-0 top-0 bottom-0 bg-emerald-950 w-full"></div>
                    <span className="z-10 font-bold text-emerald-400">100% 安全锁死保护 (600片)</span>
                  </div>
                  <span className="text-emerald-400 text-[8.5px]">物理防抢占阻断成功</span>
                </div>
              </div>
            </div>

            {/* 解耦物料配额大盘 */}
            <div className="space-y-1.5">
              <div className="text-[10px] text-muted font-bold uppercase tracking-wider">解耦点物理库存与计划供应 (Decoupled Material Nodes)</div>
              
              {iopLoading ? (
                <div className="text-center py-6 text-indigo-400">获取 IOP 引擎配额大盘中...</div>
              ) : (
                <div className="border border-main rounded overflow-hidden">
                  <table className="w-full text-left border-collapse">
                    <thead>
                      <tr className="bg-slate-900/40 border-b border-main text-muted">
                        <th className="py-2 px-2.5">物料编码</th>
                        <th className="py-2 px-2">物料类型</th>
                        <th className="py-2 px-2 text-right">在手库存 (OH)</th>
                        <th className="py-2 px-2 text-right">在途接收 (SR)</th>
                        <th className="py-2 px-2 text-right">计划供应 (PO)</th>
                        <th className="py-2 px-2.5 text-center">计划策略</th>
                      </tr>
                    </thead>
                    <tbody className="divide-y divide-slate-800/40">
                      {materialNodes.map((node, idx) => {
                        const stock = getPartStock(node.part);
                        const poQty = getPartPlanned(node.part);
                        
                        return (
                          <tr key={idx} className="hover:bg-slate-900/15 font-mono">
                            <td className="py-2.5 px-2.5 font-bold text-heading">{node.part}</td>
                            <td className="py-2.5 px-2">
                              <span className={`px-1 rounded text-[9px] font-bold ${
                                node.part_type === 'FINISHED' ? 'bg-indigo-950 text-indigo-400 border border-indigo-900' : 'bg-purple-950 text-purple-400 border border-purple-900'
                              }`}>
                                {node.part_type}
                              </span>
                            </td>
                            <td className="py-2.5 px-2 text-right text-heading">{Math.round(stock.oh).toLocaleString()}</td>
                            <td className="py-2.5 px-2 text-right text-muted">{Math.round(stock.sr).toLocaleString()}</td>
                            <td className="py-2.5 px-2 text-right text-heading font-bold">{Math.round(poQty).toLocaleString()}</td>
                            <td className="py-2.5 px-2.5 text-center">
                              <span className="px-1.5 py-0.5 rounded bg-slate-900 text-muted border border-slate-800 text-[9px]">
                                {node.sourcing_policy}
                              </span>
                            </td>
                          </tr>
                        );
                      })}
                    </tbody>
                  </table>
                </div>
              )}
            </div>

            {/* 运营层替换料分配与消纳阻断明细 */}
            <div className="space-y-1.5">
              <div className="text-[10px] text-muted font-bold uppercase tracking-wider">替代配额分配明细表 (Substitute Allocation Ledger)</div>
              <div className="border border-main rounded overflow-hidden">
                <table className="w-full text-left border-collapse">
                  <thead>
                    <tr className="bg-slate-900/40 border-b border-main text-muted">
                      <th className="py-2 px-2.5">主料零件</th>
                      <th className="py-2 px-2">替代零件</th>
                      <th className="py-2 px-2 text-right">分配量 (Qty)</th>
                      <th className="py-2 px-2 text-center">需求天</th>
                      <th className="py-2 px-2.5 text-right">替换规则等级</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-slate-800/40">
                    {alternateAllocations.length === 0 ? (
                      <tr>
                        <td colSpan={5} className="text-center py-6 text-muted font-sans text-[10px]">
                          暂无微观替代消纳记录 (运行重算以激活)
                        </td>
                      </tr>
                    ) : (
                      alternateAllocations.slice(0, 8).map((alt, idx) => (
                        <tr key={idx} className="hover:bg-slate-900/15 font-mono">
                          <td className="py-2 px-2.5 font-bold text-heading">{alt.main_part}</td>
                          <td className="py-2 px-2 text-purple-400 font-bold">{alt.alt_part}</td>
                          <td className="py-2 px-2 text-right text-heading font-bold">{Math.round(alt.allocated_qty).toLocaleString()}</td>
                          <td className="py-2 px-2 text-center text-muted">D{alt.day}</td>
                          <td className="py-2 px-2.5 text-right">
                            <span className={`px-1.5 py-0.5 rounded text-[9px] font-bold ${
                              alt.alt_class === 1 ? 'bg-indigo-950 border border-indigo-900 text-indigo-400' : 'bg-amber-950 border border-amber-900 text-amber-400'
                            }`}>
                              Class {alt.alt_class} {alt.alt_class === 1 ? '(温差绝对值优先)' : '(供应商评级优先)'}
                            </span>
                          </td>
                        </tr>
                      ))
                    )}
                  </tbody>
                </table>
              </div>
            </div>

          </div>
        </div>

      </div>

      {/* 底部功能条: 重算 Workbench */}
      <footer className="bg-card border border-main rounded-xs p-3 flex flex-col sm:flex-row justify-between items-center gap-2 sm:gap-0 select-none">
        <div className="flex items-center space-x-2 text-[10.5px] text-muted">
          <Database className="w-4 h-4 text-indigo-500" />
          <span>控制塔与计划工作台联动：您在工作台修改数据并重算后，引擎会自动更新控制塔大盘。</span>
        </div>
        
        <button 
          onClick={handleSimulate} 
          disabled={isLoading}
          className="w-full sm:w-auto px-5 py-2 bg-indigo-600 hover:bg-indigo-500 disabled:opacity-50 text-white rounded font-bold transition flex items-center justify-center space-x-1.5 cursor-pointer shadow-lg"
        >
          <RefreshCw className={`w-3.5 h-3.5 ${isLoading ? 'animate-spin' : ''}`} />
          <span>运行 C++ 战术大盘重算 (Simulate Allotment)</span>
        </button>
      </footer>

    </div>
  );
};

export default AllotmentWorkbench;
