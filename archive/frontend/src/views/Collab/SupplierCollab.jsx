import React, { useState, useEffect } from 'react';
import { useScenario } from '../../contexts/ScenarioContext';
import { 
  Search, 
  Save, 
  RotateCcw, 
  AlertTriangle, 
  Truck, 
  TrendingUp, 
  RefreshCw,
  Info,
  Database,
  CheckCircle2,
  Activity,
  History,
  ShieldCheck,
  FileText
} from 'lucide-react';
import {
  AreaChart,
  Area,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
  Legend,
  Line
} from 'recharts';

const CustomChartTooltip = ({ active, payload }) => {
  if (active && payload && payload.length) {
    const data = payload[0].payload;
    const totalCommit = data.asn_commit + data.sr_commit;
    const isDeficit = data.deficit > 0;
    
    return (
      <div className="bg-[#121620]/95 border border-[#1e2538] p-2 rounded shadow-2xl text-[9.5px] font-mono space-y-1">
        <p className="font-bold text-heading">{data.day} 账期协同对账状态</p>
        <p className="text-indigo-400">
          采购需求预测: {Math.round(data.forecast).toLocaleString()} 颗
        </p>
        {data.asn_commit > 0 && (
          <p className="text-emerald-400 font-bold flex items-center">
            <span className="w-1.5 h-1.5 rounded-full bg-emerald-500 mr-1"></span>
            ASN 在途承诺: {Math.round(data.asn_commit).toLocaleString()} 颗 (确定性: 95%)
          </p>
        )}
        {data.sr_commit > 0 && (
          <p className="text-cyan-400 font-bold flex items-center">
            <span className="w-1.5 h-1.5 rounded-full bg-cyan-500 mr-1"></span>
            SR 计划承诺: {Math.round(data.sr_commit).toLocaleString()} 颗 (确定性: 70%)
          </p>
        )}
        <p className="text-emerald-500 font-bold">
          承诺供应总计: {Math.round(totalCommit).toLocaleString()} 颗
        </p>
        {isDeficit && (
          <p className="text-rose-400 font-bold animate-pulse">
            ⚠️ 供应赤字缺口: {Math.round(data.deficit).toLocaleString()} 颗
          </p>
        )}
      </div>
    );
  }
  return null;
};

const SupplierCollab = () => {
  const { currentScenario, fetchKpis } = useScenario();
  
  const [rawParts, setRawParts] = useState([]);
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedPart, setSelectedPart] = useState(null);
  
  const [commits, setCommits] = useState([]);
  const [originalCommits, setOriginalCommits] = useState([]);
  const [dirtyCells, setDirtyCells] = useState({}); // key: day_idx, value: number
  const [isLoadingParts, setIsLoadingParts] = useState(false);
  const [isLoadingGrid, setIsLoadingGrid] = useState(false);
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [alertMessage, setAlertMessage] = useState(null);

  // WMS/ERP Sync Hub Logs State
  const [syncLogs, setSyncLogs] = useState([]);
  const [simType, setSimType] = useState('101 GR 收货');
  const [simDay, setSimDay] = useState(5);
  const [simQty, setSimQty] = useState(1000);

  const loadSyncLogs = async () => {
    try {
      const res = await fetch('/api/collab/sync-logs');
      const data = await res.json();
      if (Array.isArray(data)) {
        setSyncLogs(data);
      }
    } catch (err) {
      console.error('Error loading sync logs:', err);
    }
  };

  const handleSimulateSync = async () => {
    if (!selectedPart) return;
    setIsSubmitting(true);
    setAlertMessage(null);
    
    let certainty = 1.0;
    if (simType === '103 ASN 发运') certainty = 0.95;
    if (simType === '105 SR 确认') certainty = 0.70;
    
    try {
      const res = await fetch('/api/collab/receipt/sync-peripheral', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          movement_type: simType,
          part_code: selectedPart.part_code,
          qty: parseFloat(simQty),
          day: parseInt(simDay),
          certainty
        })
      });
      const data = await res.json();
      
      if (data.status === 'success') {
        setAlertMessage({
          type: 'success',
          text: `🎉 周边系统同步成功！已执行 [${simType}] 事务，前账 ${Math.round(data.total_before).toLocaleString()} 颗 -> 后账 ${Math.round(data.total_after).toLocaleString()} 颗 (数量漏损: ${Math.round(data.leakage)} 颗)，系统已重算 MRP。`
        });
        await fetchKpis();
        await fetchRawParts(searchQuery);
        await loadSupplierCommits(selectedPart.part_code);
        await loadSyncLogs();
      } else {
        setAlertMessage({
          type: 'error',
          text: `同步失败: ${data.message}`
        });
      }
    } catch (err) {
      console.error('Error in peripheral sync:', err);
      setAlertMessage({
        type: 'error',
        text: `同步网络错误: ${err.message}`
      });
    } finally {
      setIsSubmitting(false);
    }
  };

  // Fetch RAW parts list
  const fetchRawParts = async (q = '') => {
    setIsLoadingParts(true);
    try {
      const res = await fetch(`/api/mrp/parts?q=${encodeURIComponent(q)}`);
      const data = await res.json();
      
      // Filter for RAW parts
      const filtered = data.filter(p => p.part_type === 'RAW');
      setRawParts(filtered);
      
      if (filtered.length > 0 && !selectedPart) {
        // Find PART_4500 first, fallback to first item
        const p4500 = filtered.find(p => p.part_code === 'PART_4500');
        setSelectedPart(p4500 || filtered[0]);
      }
    } catch (err) {
      console.error('Error fetching RAW parts:', err);
    } finally {
      setIsLoadingParts(false);
    }
  };

  // Load time-phased supplier commits
  const loadSupplierCommits = async (partCode) => {
    if (!partCode) return;
    setIsLoadingGrid(true);
    try {
      const res = await fetch(`/api/collab/commits?part_code=${encodeURIComponent(partCode)}`);
      const data = await res.json();
      if (Array.isArray(data)) {
        setCommits(data);
        setOriginalCommits(JSON.parse(JSON.stringify(data)));
        setDirtyCells({});
      }
    } catch (err) {
      console.error('Error loading supplier commits:', err);
    } finally {
      setIsLoadingGrid(false);
    }
  };

  // Search trigger
  useEffect(() => {
    fetchRawParts(searchQuery);
  }, [searchQuery, currentScenario]);

  // Load grid when selected part or scenario changes
  useEffect(() => {
    if (selectedPart) {
      loadSupplierCommits(selectedPart.part_code);
    } else {
      setCommits([]);
      setOriginalCommits([]);
      setDirtyCells({});
    }
  }, [selectedPart, currentScenario]);

  // Load sync logs on scenario change or startup
  useEffect(() => {
    loadSyncLogs();
  }, [currentScenario]);

  const handleCellChange = (dayIdx, value) => {
    const parsedVal = parseFloat(value);
    if (isNaN(parsedVal) || parsedVal < 0) return;

    // Update commits state
    const updatedCommits = commits.map(c => {
      if (c.day_idx === dayIdx) {
        return { ...c, commit_qty: parsedVal };
      }
      return c;
    });
    setCommits(updatedCommits);

    // Calculate dirty state
    const original = originalCommits.find(o => o.day_idx === dayIdx);
    if (original) {
      if (parsedVal !== original.commit_qty) {
        setDirtyCells(prev => ({ ...prev, [dayIdx]: parsedVal }));
      } else {
        setDirtyCells(prev => {
          const next = { ...prev };
          delete next[dayIdx];
          return next;
        });
      }
    }
  };

  const handleDiscardChanges = () => {
    setCommits(JSON.parse(JSON.stringify(originalCommits)));
    setDirtyCells({});
  };

  const handleSubmitChanges = async () => {
    if (!selectedPart || Object.keys(dirtyCells).length === 0) return;
    
    setIsSubmitting(true);
    setAlertMessage(null);
    try {
      const updates = Object.entries(dirtyCells).map(([dayIdx, val]) => ({
        day: parseInt(dayIdx),
        commit_qty: val
      }));

      const res = await fetch('/api/collab/commit/batch-update', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          part_code: selectedPart.part_code,
          updates
        })
      });
      const data = await res.json();
      
      if (data.status === 'success') {
        setAlertMessage({
          type: 'success',
          text: `🎉 保存成功！已更新 ${updates.length} 项承诺交货量，C++ 引擎已完成 MRP 刚性缺口线与 SR 订单重算。`
        });
        
        // Refresh local data & KPIs
        await fetchKpis();
        await loadSupplierCommits(selectedPart.part_code);
        await loadSyncLogs();
      } else {
        setAlertMessage({
          type: 'error',
          text: `保存失败: ${data.message}`
        });
      }
    } catch (err) {
      console.error('Error submitting supplier commits:', err);
      setAlertMessage({
        type: 'error',
        text: `网络提交错误: ${err.message}`
      });
    } finally {
      setIsSubmitting(false);
    }
  };

  const handleSimulateGoodsReceipt = async (dayIdx) => {
    if (!selectedPart) return;
    
    setIsSubmitting(true);
    setAlertMessage(null);
    try {
      // Find the SR quantity before we execute GR to log it
      const targetSr = commits.find(c => c.day_idx === dayIdx);
      const grQty = targetSr ? targetSr.commit_qty : 0;

      const res = await fetch('/api/collab/receipt/receive', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          part_code: selectedPart.part_code,
          day: dayIdx
        })
      });
      const data = await res.json();
      
      if (data.status === 'success') {
        setAlertMessage({
          type: 'success',
          text: `🎉 收料入库成功！已执行货物接收 (Goods Receipt)，消耗在途发运单并转化为 OnHand 物理库存，确保两端数量绝对一致。`
        });
        
        // Refresh local data & KPIs
        await fetchKpis();
        await fetchRawParts(searchQuery); // Reload parts list to refresh current inventory count
        await loadSupplierCommits(selectedPart.part_code);
        await loadSyncLogs();
      } else {
        setAlertMessage({
          type: 'error',
          text: `收料入库失败: ${data.message}`
        });
      }
    } catch (err) {
      console.error('Error simulating Goods Receipt:', err);
      setAlertMessage({
        type: 'error',
        text: `收料入库网络错误: ${err.message}`
      });
    } finally {
      setIsSubmitting(false);
    }
  };

  // Stacked Data processing for Recharts to separate ASN (High Certainty) vs SR (Medium Certainty)
  const chartData = commits.map(c => {
    const isTransit = c.day_idx <= 14;
    const deficit = Math.max(0, c.forecast_qty - c.commit_qty);
    return {
      day: c.day,
      forecast: c.forecast_qty,
      asn_commit: isTransit ? c.commit_qty : 0,
      sr_commit: !isTransit ? c.commit_qty : 0,
      deficit: deficit
    };
  });

  // Calculate totals for Inventory Reconciliation (Quantity Consistency Validation)
  const totalOnHand = selectedPart ? selectedPart.on_hand : 0;
  const totalASN = commits.reduce((sum, c) => sum + (c.day_idx <= 14 ? c.commit_qty : 0), 0);
  const totalSR = commits.reduce((sum, c) => sum + (c.day_idx > 14 ? c.commit_qty : 0), 0);
  const totalSupply = totalOnHand + totalASN + totalSR;

  const dirtyCount = Object.keys(dirtyCells).length;

  return (
    <div className="flex flex-row space-x-3 h-full overflow-hidden">
      
      {/* 左侧原料零件列表 */}
      <div className="w-60 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden">
        <div className="border-b border-main pb-2 mb-2 flex items-center justify-between">
          <h4 className="font-bold text-heading text-xs flex items-center">
            <Truck className="w-4 h-4 text-indigo-500 mr-1.5" />
            原料采购检索 (Raw Parts)
          </h4>
          <span className="text-[9px] text-muted cyber-font">{rawParts.length} SKU</span>
        </div>

        <div className="relative mb-3">
          <input
            type="text"
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            placeholder="搜索原料零件号..."
            className="w-full bg-input border border-main text-heading text-xs pl-8 pr-3 py-1.5 rounded focus:outline-none focus:border-indigo-500"
          />
          <Search className="w-3.5 h-3.5 text-muted absolute left-2.5 top-2.5" />
        </div>

        <div className="flex-1 overflow-y-auto space-y-1.5 pr-1">
          {isLoadingParts ? (
            <div className="text-center py-10 text-muted text-xs flex items-center justify-center">
              <RefreshCw className="w-4 h-4 animate-spin mr-1.5" />
              正在载入...
            </div>
          ) : (
            rawParts.map(p => (
              <button
                key={p.part_code}
                onClick={() => setSelectedPart(p)}
                className={`w-full text-left px-3 py-2 border rounded text-xs transition-all flex justify-between items-center ${
                  selectedPart?.part_code === p.part_code
                    ? 'bg-indigo-600 border-indigo-500 text-white font-bold'
                    : 'bg-input border-main text-muted hover:bg-slate-800/30 hover:text-heading'
                }`}
              >
                <div className="truncate font-mono">{p.part_code}</div>
                <span className={`text-[8.5px] px-1.5 py-0.5 rounded font-bold ${
                  selectedPart?.part_code === p.part_code 
                    ? 'bg-indigo-700 text-white' 
                    : 'bg-table-header text-muted'
                }`}>
                  库存: {p.on_hand}
                </span>
              </button>
            ))
          )}
          {rawParts.length === 0 && !isLoadingParts && (
            <div className="text-center py-10 text-muted text-xs">无匹配的原材料</div>
          )}
        </div>
      </div>

      {/* 右侧主工作网格 */}
      <div className="flex-1 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden">
        
        {/* 工具栏 */}
        <div className="flex justify-between items-center border-b border-main pb-2.5 mb-2.5">
          <div className="flex items-center space-x-2">
            <h3 className="text-xs font-bold text-heading flex items-center">
              供应商协同与确定性对账网格 (Collaborative Commit Workstation)
            </h3>
            {selectedPart && (
              <span className="text-[10px] px-1.5 py-0.5 bg-slate-900 border border-main rounded text-indigo-400 font-mono">
                {selectedPart.part_code} (LLC: {selectedPart.llc})
              </span>
            )}
          </div>

          <div className="flex items-center space-x-2">
            {dirtyCount > 0 && (
              <div className="flex items-center space-x-1.5 text-amber-500 text-[10px] font-bold mr-2 border border-amber-900/60 bg-amber-950/20 px-2.5 py-1 rounded">
                <AlertTriangle className="w-3.5 h-3.5" />
                <span>草稿区有 {dirtyCount} 项待保存的承诺修改</span>
              </div>
            )}
            
            <button
              onClick={handleDiscardChanges}
              disabled={dirtyCount === 0 || isSubmitting}
              className="btn-premium-secondary py-1 px-3 text-[10.5px] flex items-center space-x-1 hover:text-rose-400 disabled:opacity-50"
            >
              <RotateCcw className="w-3.5 h-3.5" />
              <span>放弃修改</span>
            </button>

            <button
              onClick={handleSubmitChanges}
              disabled={dirtyCount === 0 || isSubmitting}
              className="btn-premium-primary py-1 px-3 text-[10.5px] flex items-center space-x-1 disabled:opacity-50 bg-indigo-600 hover:bg-indigo-500 text-white rounded"
            >
              {isSubmitting ? (
                <RefreshCw className="w-3.5 h-3.5 animate-spin" />
              ) : (
                <Save className="w-3.5 h-3.5" />
              )}
              <span>{isSubmitting ? '正在更新计划...' : '保存并提交承诺'}</span>
            </button>
          </div>
        </div>

        {/* 提示信息 */}
        {alertMessage && (
          <div className={`p-2.5 mb-3 rounded border text-[10.5px] flex items-start space-x-2 animate-fade-in ${
            alertMessage.type === 'success' 
              ? 'bg-emerald-950/30 border-emerald-900/50 text-emerald-400' 
              : 'bg-rose-950/30 border-rose-900/50 text-rose-400'
          }`}>
            <Info className="w-4 h-4 flex-shrink-0" />
            <div className="flex-1">{alertMessage.text}</div>
          </div>
        )}

        <div className="flex-1 flex flex-row space-x-3 min-h-0">
          
          {/* 左半侧：网格+图表 */}
          <div className="flex-1 flex flex-col space-y-3 min-h-0">
            
            {/* 时序网格表 */}
            <div className="flex-[3] w-full min-h-0 relative border border-main/50 rounded overflow-hidden flex flex-col">
              {isLoadingGrid && (
                <div className="absolute inset-0 bg-card/85 backdrop-blur-xs flex items-center justify-center z-20 text-xs text-indigo-400">
                  <RefreshCw className="w-5 h-5 animate-spin mr-2" />
                  正在加载协同承诺账期表...
                </div>
              )}

              <div className="flex-1 overflow-auto">
                <table className="w-full border-collapse text-left font-mono text-[10px]">
                  <thead>
                    <tr className="bg-table-header text-muted border-b border-main sticky top-0 z-10">
                      <th className="px-3 py-2 font-bold w-48 border-r border-main">指标计划流 (Metrics Flow)</th>
                      {commits.map(c => (
                        <th key={c.day} className="px-2 py-2 text-center font-bold border-r border-main min-w-[70px]">
                          {c.day}
                        </th>
                      ))}
                    </tr>
                  </thead>
                  <tbody>
                    {/* purchased forecast row */}
                    <tr className="border-b border-table bg-card/40 hover:bg-table-hover/40 transition-colors">
                      <td className="px-3 py-1.5 border-r border-main text-muted flex items-center justify-between">
                        <span className="flex items-center text-indigo-400">
                          <FileText className="w-3.5 h-3.5 mr-1" />
                          需求预测 (Forecast)
                        </span>
                      </td>
                      {commits.map(c => (
                        <td key={c.day} className="px-2 py-1.5 text-center border-r border-main text-indigo-400">
                          {Math.round(c.forecast_qty).toLocaleString()}
                        </td>
                      ))}
                    </tr>
                    
                    {/* commit row */}
                    <tr className="border-b border-table bg-card hover:bg-table-hover transition-colors">
                      <td className="px-3 py-1.5 font-bold border-r border-main text-heading flex items-center justify-between">
                        <span className="flex items-center text-emerald-400">
                          <Activity className="w-3.5 h-3.5 mr-1" />
                          供方承诺 (Commit)
                        </span>
                      </td>
                      {commits.map(c => {
                        const isDirty = dirtyCells[c.day_idx] !== undefined;
                        const hasDeficit = c.commit_qty < c.forecast_qty;
                        const isTransit = c.day_idx <= 14;
                        return (
                          <td 
                            key={c.day} 
                            className={`p-0 text-center border-r border-main relative ${
                              isDirty 
                                ? 'bg-amber-950/20 text-amber-400 border-2 border-amber-500/50' 
                                : hasDeficit 
                                  ? 'bg-rose-950/10' 
                                  : ''
                            }`}
                          >
                            {isDirty && <div className="absolute top-0 right-0 w-0 h-0 border-t-[5px] border-t-amber-500 border-l-[5px] border-l-transparent z-10" />}
                            <input
                              type="number"
                              value={c.commit_qty}
                              onChange={(e) => handleCellChange(c.day_idx, e.target.value)}
                              className={`w-full h-full text-center bg-transparent border-0 text-[10px] text-heading font-mono focus:outline-none focus:bg-indigo-950/30 py-1.5 ${
                                hasDeficit 
                                  ? 'text-rose-400 font-bold' 
                                  : isTransit 
                                    ? 'text-emerald-400' 
                                    : 'text-cyan-400'
                              }`}
                            />
                          </td>
                        );
                      })}
                    </tr>
                    
                    {/* execution status row (Holographic Shipment/GR 对账) */}
                    <tr className="border-b border-table bg-card hover:bg-table-hover transition-colors">
                      <td className="px-3 py-1.5 border-r border-main text-muted flex items-center justify-between">
                        <span className="flex items-center text-muted">
                          <Database className="w-3.5 h-3.5 mr-1" />
                          单据对账 (ASN / SR Status)
                        </span>
                      </td>
                      {commits.map(c => {
                        const hasCommit = c.commit_qty > 0;
                        const isTransit = c.day_idx <= 14;
                        return (
                          <td key={c.day} className="px-2 py-1.5 text-center border-r border-main text-[9px]">
                            {hasCommit ? (
                              isTransit ? (
                                <div className="flex flex-col items-center space-y-1">
                                  <span className="text-amber-500 font-bold flex items-center justify-center" title="高确定性 (95%): 物流发运单已录入，货在途中">
                                    <ShieldCheck className="w-3.5 h-3.5 mr-0.5 text-amber-500 animate-pulse" />
                                    ASN在途 (95%)
                                  </span>
                                  <button
                                    onClick={() => handleSimulateGoodsReceipt(c.day_idx)}
                                    disabled={isSubmitting}
                                    className="px-1.5 py-0.5 bg-emerald-950/40 hover:bg-emerald-900/60 text-emerald-400 border border-emerald-900/40 rounded-xs text-[8px] cursor-pointer transition-all active:scale-95 disabled:opacity-50"
                                    title="执行 101 GR 收料入库，在途单消耗并转化为 On-Hand 物理库存"
                                  >
                                    收料入库 (GR)
                                  </button>
                                </div>
                              ) : (
                                <span className="text-cyan-400 font-bold flex items-center justify-center" title="中确定性 (70%): 供应商排产确认，尚未发运">
                                  <CheckCircle2 className="w-3.5 h-3.5 mr-0.5 text-cyan-400" />
                                  SR确认 (70%)
                                </span>
                              )
                            ) : (
                              <span className="text-muted">-</span>
                            )}
                          </td>
                        );
                      })}
                    </tr>
                  </tbody>
                </table>
              </div>
            </div>

            {/* 需求 vs 承诺 对账时序图 */}
            {selectedPart && chartData.length > 0 && (
              <div className="flex-[2] min-h-[170px] bg-slate-950/25 border border-main rounded-xs p-3 flex flex-col justify-between select-none relative">
                <div className="absolute top-2 right-3 flex items-center space-x-3 text-[9px] text-muted">
                  <span className="flex items-center"><span className="w-2 h-px border-t-2 border-indigo-500 mr-1"></span>需求预测 (Demand)</span>
                  <span className="flex items-center"><span className="w-2.5 h-2.5 bg-emerald-500/25 border border-emerald-500 rounded mr-1"></span>ASN 在途 (高确定性)</span>
                  <span className="flex items-center"><span className="w-2.5 h-2.5 bg-cyan-500/25 border border-cyan-500 rounded mr-1"></span>SR 计划 (中确定性)</span>
                  <span className="flex items-center"><span className="w-2.5 h-2.5 bg-rose-500/25 border border-rose-500 rounded mr-1"></span>供应缺口 (Shortage Gap)</span>
                </div>
                
                <h4 className="text-[10px] font-bold text-heading flex items-center mb-1">
                  <TrendingUp className="w-3.5 h-3.5 text-indigo-400 mr-1.5" />
                  需求与确定性承诺对账趋势图 (Forecast vs Certainty Commits Reconcile Profile)
                </h4>

                <div className="flex-1 w-full min-h-0 mt-1">
                  <ResponsiveContainer width="100%" height="100%">
                    <AreaChart data={chartData} margin={{ top: 5, right: 5, left: -25, bottom: -5 }}>
                      <defs>
                        <linearGradient id="colorAsn" x1="0" y1="0" x2="0" y2="1">
                          <stop offset="5%" stopColor="#10b981" stopOpacity={0.25}/>
                          <stop offset="95%" stopColor="#10b981" stopOpacity={0.0}/>
                        </linearGradient>
                        <linearGradient id="colorSr" x1="0" y1="0" x2="0" y2="1">
                          <stop offset="5%" stopColor="#06b6d4" stopOpacity={0.25}/>
                          <stop offset="95%" stopColor="#06b6d4" stopOpacity={0.0}/>
                        </linearGradient>
                        <linearGradient id="colorDeficit" x1="0" y1="0" x2="0" y2="1">
                          <stop offset="5%" stopColor="#ef4444" stopOpacity={0.25}/>
                          <stop offset="95%" stopColor="#ef4444" stopOpacity={0.0}/>
                        </linearGradient>
                      </defs>
                      <CartesianGrid strokeDasharray="3 3" stroke="#1e2538" />
                      <XAxis dataKey="day" stroke="#475569" fontSize={9} tickLine={false} />
                      <YAxis stroke="#475569" fontSize={9} tickLine={false} />
                      <Tooltip content={<CustomChartTooltip />} />
                      
                      {/* Fill Area for ASN Commit (High Certainty) */}
                      <Area type="monotone" dataKey="asn_commit" stroke="#10b981" strokeWidth={1.5} fillOpacity={1} fill="url(#colorAsn)" name="ASN在途" stackId="commits" />
                      
                      {/* Fill Area for SR Commit (Medium Certainty) */}
                      <Area type="monotone" dataKey="sr_commit" stroke="#06b6d4" strokeWidth={1.5} fillOpacity={1} fill="url(#colorSr)" name="SR计划" stackId="commits" />
                      
                      {/* Fill Area for Deficit */}
                      <Area type="monotone" dataKey="deficit" stroke="#ef4444" strokeWidth={1} strokeDasharray="3 3" fillOpacity={1} fill="url(#colorDeficit)" name="供应缺口" />
                      
                      {/* Demand line drawn on top */}
                      <Line type="monotone" dataKey="forecast" stroke="#6366f1" strokeWidth={2} dot={{ r: 2 }} activeDot={{ r: 4 }} name="需求预测" />
                    </AreaChart>
                  </ResponsiveContainer>
                </div>
              </div>
            )}
          </div>

          {/* 右半侧：WMS/ERP 数量一致性与同步流水日志 (Sync Hub) */}
          <div className="w-72 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden select-none">
            
            {/* 顶部对账面板 */}
            <div className="border-b border-main pb-2.5 mb-2.5">
              <div className="flex justify-between items-center mb-2">
                <h4 className="font-bold text-heading text-xs flex items-center">
                  <Database className="w-4 h-4 text-emerald-500 mr-1.5" />
                  物料现有库存与单据对账
                </h4>
                <span className="text-[8px] px-1.5 py-0.5 bg-emerald-950/40 border border-emerald-900/40 text-emerald-400 rounded-xs font-bold font-mono">
                  🟢 数量一致 (Consistent)
                </span>
              </div>
              <div className="bg-[#090b10] border border-main rounded-xs p-2.5 space-y-1.5 font-mono text-[9.5px]">
                <div className="flex justify-between">
                  <span className="text-muted">现有库存 (OnHand):</span>
                  <span className="text-heading font-bold">{Math.round(totalOnHand).toLocaleString()} 颗</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-muted">在途发运 (ASN - 95%):</span>
                  <span className="text-amber-400 font-bold">+{Math.round(totalASN).toLocaleString()} 颗</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-muted">合同确认 (SR - 70%):</span>
                  <span className="text-cyan-400 font-bold">+{Math.round(totalSR).toLocaleString()} 颗</span>
                </div>
                <div className="border-t border-main/50 pt-1.5 flex justify-between text-xs">
                  <span className="text-muted font-bold">总供应保障:</span>
                  <span className="text-emerald-400 font-bold font-mono">
                    {Math.round(totalSupply).toLocaleString()} 颗
                  </span>
                </div>
                <div className="flex justify-between text-[10px] font-bold text-indigo-400 bg-indigo-950/20 px-1.5 py-1 border border-indigo-900/30 rounded-xs">
                  <span>风险调整供应:</span>
                  <span>
                    {Math.round(totalOnHand + totalASN * 0.95 + totalSR * 0.70).toLocaleString()} 颗
                  </span>
                </div>
              </div>
              <p className="text-[8.5px] text-muted mt-1.5 leading-relaxed">
                💡 数量一致性：执行 GR 时 ASN 被销账，等额转为 OnHand。总保障量不变，实现零数量泄露。风险调整计算公式为 OnHand + ASN*95% + SR*70%。
              </p>
            </div>

            {/* 模拟周边系统同步控制面板 */}
            <div className="border-b border-main pb-2.5 mb-2.5">
              <h5 className="font-bold text-heading text-[10px] flex items-center mb-2 text-muted">
                <Truck className="w-3.5 h-3.5 mr-1 text-indigo-400" />
                模拟周边系统同步 (Peripheral Sync)
              </h5>
              <div className="space-y-2 bg-[#090b10] border border-main rounded-xs p-2.5 text-[9.5px] font-mono">
                <div className="flex items-center justify-between">
                  <span className="text-muted">事务类型:</span>
                  <select 
                    value={simType} 
                    onChange={(e) => setSimType(e.target.value)}
                    className="bg-input border border-main text-heading text-[9.5px] py-0.5 px-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono w-32"
                  >
                    <option value="101 GR 收货">101 GR 收货 (100%)</option>
                    <option value="103 ASN 发运">103 ASN 发运 (95%)</option>
                    <option value="105 SR 确认">105 SR 确认 (70%)</option>
                  </select>
                </div>
                
                <div className="flex items-center justify-between">
                  <span className="text-muted">账期 (Day):</span>
                  <select
                    value={simDay}
                    onChange={(e) => setSimDay(parseInt(e.target.value))}
                    className="bg-input border border-main text-heading text-[9.5px] py-0.5 px-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono w-32"
                  >
                    {commits.map(c => (
                      <option key={c.day_idx} value={c.day_idx}>{c.day} (Day {c.day_idx})</option>
                    ))}
                  </select>
                </div>

                <div className="flex items-center justify-between">
                  <span className="text-muted">变更数量:</span>
                  <input
                    type="number"
                    value={simQty}
                    onChange={(e) => setSimQty(e.target.value)}
                    className="bg-input border border-main text-heading text-[9.5px] py-0.5 px-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono w-32 text-right"
                  />
                </div>

                <button
                  onClick={handleSimulateSync}
                  disabled={isSubmitting || !selectedPart}
                  className="w-full mt-1.5 bg-indigo-600 hover:bg-indigo-500 disabled:opacity-50 text-white font-bold py-1 px-2 rounded-xs transition-all text-center flex items-center justify-center space-x-1 cursor-pointer"
                >
                  <RefreshCw className="w-3 h-3 animate-pulse" />
                  <span>向 IPC 同步该库存移动</span>
                </button>
              </div>
            </div>

            {/* 下部同步日志 */}
            <div className="flex-1 flex flex-col min-h-0">
              <h5 className="font-bold text-heading text-[10px] flex items-center mb-1.5 text-muted">
                <History className="w-3.5 h-3.5 mr-1" />
                WMS / ERP 数据移动同步流水
              </h5>
              <div className="flex-1 overflow-y-auto space-y-2 pr-1">
                {syncLogs.map(log => (
                  <div key={log.id} className="p-2 bg-slate-900/30 border border-[#1e2538] rounded-xs text-[9px] font-mono leading-relaxed space-y-1 hover:border-indigo-500/30 transition-colors">
                    <div className="flex justify-between items-center text-[8.5px]">
                      <span className="text-muted">{log.time}</span>
                      <span className={`px-1.5 py-0.2 rounded-xs font-bold text-[8px] ${
                        log.type.includes('GR') 
                          ? 'bg-emerald-950/40 text-emerald-400 border border-emerald-900/40' 
                          : log.type.includes('ASN') 
                            ? 'bg-amber-950/40 text-amber-400 border border-amber-900/40' 
                            : 'bg-cyan-950/40 text-cyan-400 border border-cyan-900/40'
                      }`}>
                        {log.type}
                      </span>
                    </div>
                    <div className="text-heading font-semibold">移动数量: {log.qty.toLocaleString()} 颗</div>
                    <p className="text-muted text-[8.5px]">{log.desc}</p>
                    <div className="flex justify-between items-center text-[8px] border-t border-main/30 pt-1 mt-1">
                      <span className="text-muted">同步状态:</span>
                      <span className="text-emerald-400 font-bold">🟢 同步成功</span>
                    </div>
                  </div>
                ))}
              </div>
            </div>

          </div>

        </div>

        {/* 注解说明 */}
        <div className="mt-2.5 text-[9px] text-muted flex justify-between select-none">
          <span>💡 提示：双击或直接修改带有 <span className="text-emerald-400 font-bold">供应商承诺 (Supplier Commit)</span> 行的单元格来修改供货承诺量。</span>
          <span>系统接收周边系统库存同步，通过 🚚 ASN（在途，95% 确定性）与 📋 SR（确认，70% 确定性）进行确定性分级对账。</span>
        </div>

      </div>

    </div>
  );
};

export default SupplierCollab;
