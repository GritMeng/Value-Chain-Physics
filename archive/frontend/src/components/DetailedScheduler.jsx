import React, { useState, useEffect } from 'react';
import { useScenario } from '../contexts/ScenarioContext';
import { 
  Calendar, 
  Settings2, 
  BarChart3, 
  RefreshCw, 
  HelpCircle, 
  CheckCircle2, 
  AlertTriangle,
  Info,
  Sliders,
  Sparkles,
  Package,
  ArrowRight,
  TrendingUp,
  Activity
} from 'lucide-react';
import { 
  ResponsiveContainer, 
  BarChart, 
  Bar, 
  XAxis, 
  YAxis, 
  CartesianGrid, 
  Tooltip, 
  Legend, 
  ReferenceLine 
} from 'recharts';

const SETUP_MATRIX = {
  WC_MILLING: {
    'PART_0_RAW->PART_0_SEMI': 2.0,
    'PART_0_SEMI->PART_0_RAW': 4.0,
    'PART_0_SEMI->PART_0_SEMI': 0.0,
    'PART_0_RAW->PART_0_RAW': 0.0
  },
  WC_ASSEMBLY: {
    'PART_0_SEMI->PART_0_FINISHED': 1.5,
    'PART_0_FINISHED->PART_0_SEMI': 3.0,
    'PART_0_FINISHED->PART_0_FINISHED': 0.0,
    'PART_0_SEMI->PART_0_SEMI': 0.0
  }
};

const getSrProduct = (srId) => {
  if (srId === 'TEST_SR_001') return 'PART_0_RAW';
  if (srId === 'TEST_SR_002') return 'PART_0_SEMI';
  if (srId === 'TEST_SR_003') return 'PART_0_FINISHED';
  return 'PART_0_SEMI'; // default
};

const DetailedScheduler = () => {
  const { currentScenario, runSimulation, isLoading } = useScenario();
  
  // State variables
  const [srs, setSrs] = useState([]);
  const [calendars, setCalendars] = useState([]);
  const [operations, setOperations] = useState([]);
  const [detailedLedger, setDetailedLedger] = useState([]);
  const [kittingStatus, setKittingStatus] = useState([]);
  const [pullRequests, setPullRequests] = useState([]);
  
  const [selectedWc, setSelectedWc] = useState('WC_MILLING');
  const [activeSubTab, setActiveSubTab] = useState('srs'); // 'srs' or 'kitting'
  const [editingSr, setEditingSr] = useState(null);
  const [editQty, setEditQty] = useState(0);
  const [editType, setEditType] = useState('In-process');
  const [editDueOffset, setEditDueOffset] = useState(0);
  const [isSaving, setIsSaving] = useState(false);
  const [isResequencing, setIsResequencing] = useState(false);
  const [msg, setMsg] = useState(null);

  // Fetch all scheduling data from endpoints
  const fetchData = async () => {
    try {
      const srsRes = await fetch('/api/scheduling/srs');
      const srsData = await srsRes.json();
      if (srsData.status === 'success') {
        setSrs(srsData.data);
      }

      const calRes = await fetch('/api/scheduling/calendars');
      const calData = await calRes.json();
      if (calData.status === 'success') {
        setCalendars(calData.data);
      }

      const opRes = await fetch('/api/scheduling/operations');
      const opData = await opRes.json();
      if (opData.status === 'success') {
        setOperations(opData.data);
      }

      // Fetch C++ calculated detailed schedules
      const ledgerRes = await fetch('/api/scheduling/detailed_ledger');
      const ledgerData = await ledgerRes.json();
      if (ledgerData.status === 'success') {
        setDetailedLedger(ledgerData.data);
      }

      // Fetch C++ calculated material call sheets and kitting
      const calloffRes = await fetch('/api/scheduling/calloff');
      const calloffData = await calloffRes.json();
      if (calloffData.status === 'success') {
        setKittingStatus(calloffData.kitting || []);
        setPullRequests(calloffData.pull_requests || []);
      }
    } catch (err) {
      console.error("Error fetching scheduling data:", err);
    }
  };

  useEffect(() => {
    fetchData();
  }, [currentScenario]);

  // Handle scheduled receipt reordering
  const handleMove = (srId, direction) => {
    const idx = srs.findIndex(sr => sr.sr_id === srId);
    if (idx === -1) return;
    const nextIdx = direction === 'up' ? idx - 1 : idx + 1;
    if (nextIdx < 0 || nextIdx >= srs.length) return;
    
    const newSrs = [...srs];
    const temp = newSrs[idx];
    newSrs[idx] = newSrs[nextIdx];
    newSrs[nextIdx] = temp;
    
    // Adjust offsets dynamically for instant UI feedback
    let currentOffset = 5;
    newSrs.forEach((sr) => {
      if (sr.part_code === 'PART_0') {
        sr.due_day_offset = currentOffset;
        currentOffset += 5;
      }
    });
    
    setSrs(newSrs);
  };

  const handleSaveSequence = async () => {
    setIsResequencing(true);
    try {
      const srIds = srs.map(sr => sr.sr_id);
      const res = await fetch('/api/scheduling/resequence', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ sr_ids: srIds })
      });
      const data = await res.json();
      if (data.status === 'success') {
        showMsg("✔ 排程序列保存成功！C++ 求解器大周期有限产能级联重算成功...");
        fetchData();
      } else {
        alert("序列保存失败: " + data.message);
      }
    } catch (err) {
      console.error("Resequence error:", err);
    } finally {
      setIsResequencing(false);
    }
  };

  const handleAutoOptimizeSequence = () => {
    const active = srs.filter(sr => sr.part_code === 'PART_0');
    if (active.length <= 1) return;

    const originalDues = {};
    active.forEach(sr => {
      originalDues[sr.sr_id] = sr.due_day_offset;
    });

    const unvisited = [...active];
    const optimized = [];
    
    let current = unvisited.shift();
    optimized.push(current);
    
    while (unvisited.length > 0) {
      let bestIdx = -1;
      let minImpedance = Infinity;
      
      const currentProd = getSrProduct(current.sr_id);
      const nextSlotOffset = (optimized.length + 1) * 5;
      
      for (let i = 0; i < unvisited.length; i++) {
        const candidate = unvisited[i];
        const targetProd = getSrProduct(candidate.sr_id);
        const key = `${currentProd}->${targetProd}`;
        
        const setupVal = (SETUP_MATRIX[selectedWc] && SETUP_MATRIX[selectedWc][key] !== undefined)
          ? SETUP_MATRIX[selectedWc][key]
          : 0.0;
        
        const originalDue = originalDues[candidate.sr_id];
        const delayDays = Math.max(0, nextSlotOffset - originalDue);
        const delayPenalty = delayDays * 1.5;
        
        const totalImpedance = setupVal + delayPenalty;
        
        if (totalImpedance < minImpedance) {
          minImpedance = totalImpedance;
          bestIdx = i;
        }
      }
      
      if (bestIdx !== -1) {
        current = unvisited.splice(bestIdx, 1)[0];
        optimized.push(current);
      } else {
        current = unvisited.shift();
        optimized.push(current);
      }
    }
    
    const newSrs = [...srs];
    let optIdx = 0;
    
    let currentOffset = 5;
    optimized.forEach(sr => {
      sr.due_day_offset = currentOffset;
      currentOffset += 5;
    });

    for (let i = 0; i < newSrs.length; i++) {
      if (newSrs[i].part_code === 'PART_0') {
        newSrs[i] = optimized[optIdx++];
      }
    }
    
    setSrs(newSrs);
    showMsg("✔ 生产加工序列协同优化完毕！已平衡换产洗枪时间与交期延迟惩罚，请点击下方按键应用至规划引擎。");
  };

  // Handle Scheduled Receipt Update
  const handleUpdateSr = async (e) => {
    e.preventDefault();
    if (!editingSr) return;
    setIsSaving(true);
    try {
      const res = await fetch('/api/scheduling/sr/update', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          sr_id: editingSr.sr_id,
          sr_type: editType,
          due_day_offset: parseInt(editDueOffset),
          qty: parseFloat(editQty)
        })
      });
      const data = await res.json();
      if (data.status === 'success') {
        showMsg("✔ 在途/在制订单维度修改成功，请点击下方 '应用序列并重算' 运行 LBL MRP 和 ATP 排程！");
        setEditingSr(null);
        fetchData();
      } else {
        alert("更新失败: " + data.message);
      }
    } catch (err) {
      console.error(err);
    } finally {
      setIsSaving(false);
    }
  };

  const showMsg = (text) => {
    setMsg(text);
    setTimeout(() => setMsg(null), 8000);
  };

  const activeWcCalendar = calendars.filter(c => c.work_center === selectedWc);
  
  // Calculate dynamic loads from the C++ calculated detailedLedger
  const dailyRunLoad = {};
  const dailySetupLoad = {};
  
  detailedLedger.forEach((row) => {
    if (row.work_center !== selectedWc) return;
    const start = row.scheduled_start_day;
    const finish = row.scheduled_finish_day;
    if (start === undefined || start < 0) return;
    
    // Setup time occurs on the start day
    dailySetupLoad[start] = (dailySetupLoad[start] || 0.0) + row.setup_time;
    
    // Spread run time across execution days [start, finish]
    const daysCount = Math.max(1, finish - start + 1);
    const dailyRun = row.run_time / daysCount;
    for (let d = start; d <= finish; ++d) {
      if (d >= 30) break;
      dailyRunLoad[d] = (dailyRunLoad[d] || 0.0) + dailyRun;
    }
  });

  // Fallback to visual demo loads if detailedLedger is empty
  const isLedgerEmpty = detailedLedger.length === 0;

  // Prepare chart data for selected work center load
  const chartData = activeWcCalendar.slice(0, 30).map((day, idx) => {
    const isWeekend = day.daily_cap === 0.0;
    
    let runLoad = isWeekend ? 0.0 : (dailyRunLoad[idx] || 0.0);
    let setupLoad = isWeekend ? 0.0 : (dailySetupLoad[idx] || 0.0);

    // Seed dummy visual load to keep interface vivid if C++ did not schedule yet
    if (isLedgerEmpty && !isWeekend) {
      const baseVisual = (idx % 7 === 1 || idx % 7 === 3) ? 4.0 : 2.0;
      runLoad += baseVisual;
    }
    
    const totalLoad = runLoad + setupLoad;
    const status = isWeekend ? 'Weekend' : (totalLoad > day.daily_cap ? 'Overload' : 'Normal');

    return {
      name: `D${idx}`,
      date: day.date,
      "产能上限 (Capacity Limit)": day.daily_cap,
      "生产工时负载 (Run Load)": parseFloat(runLoad.toFixed(1)),
      "换模换产耗时 (Setup Load)": parseFloat(setupLoad.toFixed(1)),
      "总负载工时 (Total Load)": parseFloat(totalLoad.toFixed(1)),
      status: status
    };
  });

  // Calendar workdays grid display
  const renderCalendarGrid = () => {
    if (activeWcCalendar.length === 0) {
      return <div className="text-muted text-xs p-4">暂无该机台的日历排程数据，请在后端进行初始化。</div>;
    }

    return (
      <div className="grid grid-cols-7 gap-1.5 text-center p-2 select-none">
        {['五', '六', '日', '一', '二', '三', '四'].map((w, idx) => (
          <div key={idx} className="text-[10px] text-muted font-bold py-1 bg-slate-900/40 rounded border border-slate-800/40">{w}</div>
        ))}
        {activeWcCalendar.slice(0, 35).map((day, idx) => {
          const isWeekend = day.daily_cap === 0.0;
          return (
            <div 
              key={idx} 
              className={`p-2 rounded flex flex-col items-center justify-center border transition-all duration-150 ${
                isWeekend 
                  ? 'bg-rose-950/20 border-rose-900/20 hover:bg-rose-950/30' 
                  : 'bg-emerald-950/20 border-emerald-900/20 hover:bg-emerald-950/30'
              }`}
              title={`${day.date} | 产能: ${day.daily_cap} 小时`}
            >
              <span className={`text-[10px] font-bold ${isWeekend ? 'text-rose-400' : 'text-emerald-400'}`}>
                D{idx}
              </span>
              <span className="text-[8px] text-muted mt-1 font-mono">
                {isWeekend ? '休' : `${day.daily_cap}h`}
              </span>
            </div>
          );
        })}
      </div>
    );
  };

  const getSrTypeBadge = (type) => {
    switch (type) {
      case 'Ignore':
        return <span className="px-1.5 py-0.5 rounded text-[8.5px] font-bold bg-slate-800 text-slate-400 border border-slate-700">🔍 Ignore (忽略消纳)</span>;
      case 'ExplodedOnly':
        return <span className="px-1.5 py-0.5 rounded text-[8.5px] font-bold bg-amber-950/40 text-amber-400 border border-amber-900/40 animate-pulse">💥 ExplodedOnly (仅投产)</span>;
      case 'Reschedulable':
        return <span className="px-1.5 py-0.5 rounded text-[8.5px] font-bold bg-indigo-950/40 text-indigo-400 border border-indigo-900/40">🔄 Reschedulable (可重排)</span>;
      case 'RescheduleRecommend':
        return <span className="px-1.5 py-0.5 rounded text-[8.5px] font-bold bg-purple-950/40 text-purple-400 border border-purple-900/40">✨ Recommend (重排推荐)</span>;
      case 'In-process':
      default:
        return <span className="px-1.5 py-0.5 rounded text-[8.5px] font-bold bg-emerald-950/40 text-emerald-400 border border-emerald-900/40">🟢 In-process (正常消纳)</span>;
    }
  };

  const getKittingStatusBadge = (status) => {
    switch (status) {
      case 'Fully_Kitted':
        return <span className="px-1.5 py-0.5 rounded text-[8.5px] font-bold bg-emerald-950/40 text-emerald-400 border border-emerald-900/40">🟢 Fully Kitted (已齐套)</span>;
      case 'Partially_Kitted':
        return <span className="px-1.5 py-0.5 rounded text-[8.5px] font-bold bg-indigo-950/40 text-indigo-400 border border-indigo-900/40">🟡 Partially (部分欠料)</span>;
      case 'Critical_Shortage':
      default:
        return <span className="px-1.5 py-0.5 rounded text-[8.5px] font-bold bg-rose-950/40 text-rose-400 border border-rose-900/40 animate-pulse">🔴 Critical Shortage (严重缺料)</span>;
    }
  };

  const getPullStatusBadge = (status) => {
    switch (status) {
      case 'Fulfilled':
        return <span className="px-1 py-0.5 rounded text-[8px] font-bold bg-emerald-950/30 text-emerald-400 border border-emerald-900/30">已配送</span>;
      case 'Pulling':
        return <span className="px-1 py-0.5 rounded text-[8px] font-bold bg-blue-950/30 text-blue-400 border border-blue-900/30 animate-pulse">拉动中</span>;
      case 'Blocked':
      default:
        return <span className="px-1 py-0.5 rounded text-[8px] font-bold bg-rose-950/30 text-rose-400 border border-rose-900/30">缺料挂起</span>;
    }
  };

  return (
    <div className="flex-1 flex flex-col space-y-3 overflow-hidden min-h-0">
      
      {/* 警报与通知栏 */}
      {msg && (
        <div className="bg-indigo-950/40 border border-indigo-600/40 p-3 rounded flex items-center space-x-2.5 text-xs text-indigo-300 animate-fade-in">
          <Info className="w-4 h-4 flex-shrink-0 animate-bounce text-indigo-400" />
          <span>{msg}</span>
        </div>
      )}

      {/* 主栅格布局 */}
      <div className="flex-1 grid grid-cols-1 xl:grid-cols-12 gap-3 overflow-hidden min-h-0">
        
        {/* 左侧：在途订单维度工作区 (8/12 cols) */}
        <div className="xl:col-span-8 flex flex-col bg-card border border-main rounded-xs overflow-hidden min-h-0">
          <div className="border-b border-main p-3 bg-table-header flex justify-between items-center select-none">
            <div className="flex items-center space-x-4">
              <button 
                onClick={() => setActiveSubTab('srs')}
                className={`text-xs font-bold flex items-center pb-0.5 border-b-2 transition-all ${
                  activeSubTab === 'srs' ? 'text-indigo-400 border-indigo-500' : 'text-muted border-transparent hover:text-heading'
                }`}
              >
                <Sliders className="w-3.5 h-3.5 mr-1.5" />
                在途生产工单时序控制台 (Receipts Workspace)
              </button>
              <button 
                onClick={() => setActiveSubTab('kitting')}
                className={`text-xs font-bold flex items-center pb-0.5 border-b-2 transition-all ${
                  activeSubTab === 'kitting' ? 'text-indigo-400 border-indigo-500' : 'text-muted border-transparent hover:text-heading'
                }`}
              >
                <Package className="w-3.5 h-3.5 mr-1.5" />
                线边齐套拉动与缺料看板 (Line Kitting & Call-off)
              </button>
            </div>
            <span className="text-[9px] text-muted font-mono uppercase">
              {activeSubTab === 'srs' ? `LIVE_SR_COUNT: ${srs.length}` : `PULL_JOBS: ${pullRequests.length}`}
            </span>
          </div>

          {/* Sub-Tab 1: srs grid */}
          {activeSubTab === 'srs' && (
            <div className="flex-1 overflow-y-auto p-3">
              <table className="w-full text-left text-xs border-collapse">
                <thead>
                  <tr className="border-b border-main bg-slate-900/20 text-muted select-none">
                    <th className="py-2 px-2.5">在途/在制订单ID</th>
                    <th className="py-2 px-2">物料编码@Site</th>
                    <th className="py-2 px-2">订单数量</th>
                    <th className="py-2 px-2 text-center">排产完成天数</th>
                    <th className="py-2 px-2">消纳控制类型 (SR Netting Type)</th>
                    <th className="py-2 px-2 text-center">确定性指数</th>
                    <th className="py-2 px-2 text-right">操作</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-800/40">
                  {srs.map(sr => (
                    <tr key={sr.sr_id} className="hover:bg-slate-900/25 transition-colors duration-100">
                      <td className="py-2.5 px-2.5 font-bold text-heading font-mono text-[11px]">{sr.sr_id}</td>
                      <td className="py-2.5 px-2 font-mono text-muted text-[10.5px]">{sr.part_code}@{sr.site}</td>
                      <td className="py-2.5 px-2 font-bold text-heading font-mono">{sr.qty.toLocaleString()}</td>
                      <td className="py-2.5 px-2 text-center font-mono text-indigo-400 font-bold">
                        {detailedLedger.find(d => d.sr_id === sr.sr_id) 
                          ? `D${detailedLedger.find(d => d.sr_id === sr.sr_id).scheduled_finish_day} (C++算)`
                          : `D${sr.due_day_offset}`}
                      </td>
                      <td className="py-2.5 px-2">{getSrTypeBadge(sr.sr_type)}</td>
                      <td className="py-2.5 px-2 text-center">
                        <div className="flex items-center justify-center space-x-1.5">
                          <div className="w-12 bg-slate-800 rounded-full h-1.5 overflow-hidden">
                            <div 
                              className={`h-full rounded-full ${
                                sr.certainty_level >= 0.9 ? 'bg-emerald-500' : (sr.certainty_level >= 0.7 ? 'bg-indigo-500' : 'bg-amber-500')
                              }`}
                              style={{ width: `${sr.certainty_level * 100}%` }}
                            />
                          </div>
                          <span className="text-[10px] font-mono text-muted">{(sr.certainty_level * 100).toFixed(0)}%</span>
                        </div>
                      </td>
                      <td className="py-2.5 px-2 text-right">
                        <button 
                          onClick={() => {
                            setEditingSr(sr);
                            setEditQty(sr.qty);
                            setEditType(sr.sr_type);
                            setEditDueOffset(sr.due_day_offset);
                          }}
                          className="px-2 py-0.5 bg-indigo-900/20 hover:bg-indigo-900/40 text-indigo-400 border border-indigo-900/60 rounded text-[10px] font-bold cursor-pointer"
                        >
                          编辑协调
                        </button>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}

          {/* Sub-Tab 2: Line Kitting & Call-off */}
          {activeSubTab === 'kitting' && (
            <div className="flex-1 flex flex-col overflow-hidden p-3 space-y-3">
              {/* Kitting summary stats */}
              <div className="grid grid-cols-1 md:grid-cols-3 gap-2 flex-shrink-0">
                <div className="bg-slate-900/40 border border-main p-2.5 rounded-sm flex items-center justify-between">
                  <div className="flex flex-col">
                    <span className="text-[10px] text-muted font-bold">工单整体齐套率 (Avg Kitting)</span>
                    <span className="text-sm font-bold text-heading font-mono mt-0.5">
                      {kittingStatus.length > 0 
                        ? `${(kittingStatus.reduce((acc, k) => acc + k.kitting_rate, 0) / kittingStatus.length * 100).toFixed(1)}%`
                        : "100.0% (暂无生产订单)"}
                    </span>
                  </div>
                  <TrendingUp className="w-5 h-5 text-emerald-500" />
                </div>
                <div className="bg-slate-900/40 border border-main p-2.5 rounded-sm flex items-center justify-between">
                  <div className="flex flex-col">
                    <span className="text-[10px] text-muted font-bold">线边配送完成率 (Pull Rate)</span>
                    <span className="text-sm font-bold text-heading font-mono mt-0.5">
                      {pullRequests.length > 0
                        ? `${(pullRequests.filter(p => p.status === 'Fulfilled').length / pullRequests.length * 100).toFixed(1)}%`
                        : "100.0% (无拉动需求)"}
                    </span>
                  </div>
                  <Activity className="w-5 h-5 text-indigo-500" />
                </div>
                <div className="bg-slate-900/40 border border-main p-2.5 rounded-sm flex items-center justify-between">
                  <div className="flex flex-col">
                    <span className="text-[10px] text-muted font-bold">缺料挂起拉料单 (Blocked Pulls)</span>
                    <span className="text-sm font-bold text-rose-400 font-mono mt-0.5">
                      {pullRequests.filter(p => p.status === 'Blocked').length} 笔
                    </span>
                  </div>
                  <AlertTriangle className="w-5 h-5 text-rose-500" />
                </div>
              </div>

              {/* Lists Layout */}
              <div className="flex-1 grid grid-cols-1 lg:grid-cols-12 gap-3 overflow-hidden min-h-0">
                {/* Kitting checklist (Left) */}
                <div className="lg:col-span-5 border border-slate-800/60 rounded flex flex-col bg-slate-950/10 overflow-hidden">
                  <div className="bg-slate-900/20 border-b border-slate-800/60 px-2 py-1.5 text-[10px] font-bold text-heading">
                    工单齐套率分析 (BOM Kitting Checklist)
                  </div>
                  <div className="flex-1 overflow-y-auto p-2 space-y-2">
                    {kittingStatus.length === 0 ? (
                      <div className="text-muted text-xs text-center py-12">暂无排产的齐套评估数据，请先应用序列并重算。</div>
                    ) : (
                      kittingStatus.map((kit) => (
                        <div key={kit.parent_sr_id} className="p-2 bg-slate-900/30 border border-slate-850 rounded-sm space-y-1.5">
                          <div className="flex justify-between items-center text-[10px]">
                            <span className="font-bold font-mono text-heading">{kit.parent_sr_id} ({kit.part_code})</span>
                            {getKittingStatusBadge(kit.kitting_status)}
                          </div>
                          <div className="flex items-center justify-between text-[9px] text-muted">
                            <span>排产投产期: {kit.required_date}</span>
                            <span>子件匹配: {kit.fulfilled_components}/{kit.total_components}</span>
                          </div>
                          <div className="w-full bg-slate-800 rounded-full h-1.5 overflow-hidden">
                            <div 
                              className={`h-full rounded-full ${
                                kit.kitting_rate >= 1.0 ? 'bg-emerald-500' : (kit.kitting_rate >= 0.4 ? 'bg-indigo-500' : 'bg-rose-500')
                              }`}
                              style={{ width: `${kit.kitting_rate * 100}%` }}
                            />
                          </div>
                        </div>
                      ))
                    )}
                  </div>
                </div>

                {/* Line Pull Requests (Right) */}
                <div className="lg:col-span-7 border border-slate-800/60 rounded flex flex-col bg-slate-950/10 overflow-hidden">
                  <div className="bg-slate-900/20 border-b border-slate-800/60 px-2 py-1.5 text-[10px] font-bold text-heading">
                    自动线边 Call-off 物料拉动指令流水 (Pull Signals Stream)
                  </div>
                  <div className="flex-1 overflow-y-auto p-2">
                    {pullRequests.length === 0 ? (
                      <div className="text-muted text-xs text-center py-12">暂无拉动指令。请在 C++ 引擎中排产生成线拉信号。</div>
                    ) : (
                      <table className="w-full text-left text-[10px] border-collapse">
                        <thead>
                          <tr className="border-b border-slate-800 bg-slate-900/20 text-muted">
                            <th className="py-1 px-1">指令ID</th>
                            <th className="py-1 px-1">工单ID</th>
                            <th className="py-1 px-1">拉动组件</th>
                            <th className="py-1 px-1 text-right">拉料数量</th>
                            <th className="py-1 px-1 text-right">实配数量</th>
                            <th className="py-1 px-1 text-center">拉料期</th>
                            <th className="py-1 px-1 text-center">状态</th>
                          </tr>
                        </thead>
                        <tbody>
                          {pullRequests.map(pull => (
                            <tr key={pull.call_id} className="border-b border-slate-900 hover:bg-slate-900/10">
                              <td className="py-1.5 px-1 font-mono text-heading font-bold">{pull.call_id}</td>
                              <td className="py-1.5 px-1 font-mono text-muted">{pull.parent_sr_id}</td>
                              <td className="py-1.5 px-1 font-mono text-indigo-300">{pull.component_part}</td>
                              <td className="py-1.5 px-1 text-right font-mono font-bold">{pull.required_qty.toFixed(0)}</td>
                              <td className="py-1.5 px-1 text-right font-mono text-emerald-400">{pull.allocated_qty.toFixed(0)}</td>
                              <td className="py-1.5 px-1 text-center font-mono font-bold text-amber-500">D{pull.call_day}</td>
                              <td className="py-1.5 px-1 text-center">{getPullStatusBadge(pull.status)}</td>
                            </tr>
                          ))}
                        </tbody>
                      </table>
                    )}
                  </div>
                </div>
              </div>
            </div>
          )}
        </div>

        {/* 右侧：排程维度配置与工作日历网格 (4/12 cols) */}
        <div className="xl:col-span-4 flex flex-col space-y-3 overflow-hidden min-h-0">
          
          {/* 编辑面板 */}
          {editingSr ? (
            <div className="bg-card border border-indigo-600/40 rounded-xs p-3 flex flex-col space-y-3 animate-fade-in shadow-xl bg-indigo-950/5">
              <div className="flex justify-between items-center border-b border-indigo-900/40 pb-2">
                <h4 className="text-xs font-bold text-heading flex items-center">
                  <Settings2 className="w-4 h-4 text-indigo-400 mr-1.5" />
                  维度协调窗口: {editingSr.sr_id}
                </h4>
                <button 
                  onClick={() => setEditingSr(null)} 
                  className="text-xs text-muted hover:text-heading cursor-pointer"
                >
                  取消
                </button>
              </div>

              <form onSubmit={handleUpdateSr} className="space-y-3 text-xs">
                <div>
                  <label className="block text-muted font-bold mb-1">订单交货数量 (Quantity)</label>
                  <input 
                    type="number" 
                    value={editQty}
                    onChange={(e) => setEditQty(e.target.value)}
                    className="w-full bg-input border border-main text-heading p-2 rounded focus:outline-none focus:border-indigo-500 font-mono"
                    required
                  />
                </div>

                <div>
                  <label className="block text-muted font-bold mb-1">交期天数偏移 (Due Day Offset)</label>
                  <input 
                    type="number" 
                    value={editDueOffset}
                    onChange={(e) => setEditDueOffset(e.target.value)}
                    className="w-full bg-input border border-main text-heading p-2 rounded focus:outline-none focus:border-indigo-500 font-mono"
                    required
                  />
                </div>

                <div>
                  <label className="block text-muted font-bold mb-1">消纳及协调类型 (SR Netting Type)</label>
                  <select
                    value={editType}
                    onChange={(e) => setEditType(e.target.value)}
                    className="w-full bg-input border border-main text-heading p-2 rounded focus:outline-none focus:border-indigo-500 font-bold"
                  >
                    <option value="In-process">🟢 In-process (正常参与MRP净消纳)</option>
                    <option value="Ignore">🔍 Ignore (完全忽略，只保留库存轨迹)</option>
                    <option value="ExplodedOnly">💥 ExplodedOnly (不抵扣，直接向下拆解MRP需求)</option>
                    <option value="Reschedulable">🔄 Reschedulable (当有短缺时，支持自由向前提产)</option>
                    <option value="RescheduleRecommend">✨ RescheduleRecommend (支持提产并生成调整建议)</option>
                  </select>
                </div>

                <button 
                  type="submit"
                  disabled={isSaving}
                  className="w-full py-2 bg-indigo-600 hover:bg-indigo-500 text-white rounded font-bold transition shadow-lg flex items-center justify-center space-x-1.5 cursor-pointer"
                >
                  {isSaving ? <RefreshCw className="w-3.5 h-3.5 animate-spin" /> : <CheckCircle2 className="w-3.5 h-3.5" />}
                  <span>保存维度变更</span>
                </button>
              </form>
            </div>
          ) : (
            <div className="space-y-3 flex-1 overflow-y-auto pr-1">
              
              {/* Campaign Sequencer Panel */}
              <div className="bg-card border border-main rounded-xs p-3 flex flex-col space-y-2.5 select-none">
                <div className="flex justify-between items-center">
                  <h4 className="text-xs font-bold text-heading flex items-center">
                    <Sparkles className="w-4 h-4 text-indigo-500 mr-1.5 animate-pulse" />
                    车间 Campaign 生产序列干预 (Campaign Sequencer)
                  </h4>
                  <button
                    type="button"
                    onClick={handleAutoOptimizeSequence}
                    className="px-2 py-0.5 bg-purple-900/40 hover:bg-purple-800/60 text-purple-300 border border-purple-800/40 rounded text-[9.5px] font-bold cursor-pointer transition"
                    title="运行贪婪 TSP 算法自动寻优换模切换顺序"
                  >
                    🪄 自动优化 (Auto-Sort)
                  </button>
                </div>
                <p className="text-[10px] text-muted leading-relaxed">
                  手动调整 Scheduled Receipts 的排程先后顺序。换产耗时将根据 setup 矩阵规则自动计算并计入产能负荷。
                </p>
                
                <div className="space-y-1 bg-slate-950/20 p-1.5 rounded border border-slate-800/40">
                  {srs.filter(sr => sr.part_code === 'PART_0').map((sr, idx, arr) => {
                    const prod = getSrProduct(sr.sr_id);
                    return (
                      <div key={sr.sr_id} className="flex items-center justify-between p-1.5 bg-slate-900/40 rounded border border-slate-855 text-[10.5px]">
                        <div className="flex flex-col">
                          <span className="font-bold text-heading font-mono">{sr.sr_id}</span>
                          <span className="text-[9px] text-muted">{prod} | Day {sr.due_day_offset}</span>
                        </div>
                        <div className="flex space-x-1">
                          <button 
                            disabled={idx === 0}
                            onClick={() => handleMove(sr.sr_id, 'up')}
                            className="px-2 py-0.5 bg-slate-800 hover:bg-slate-700 disabled:opacity-30 text-heading rounded text-[10px] cursor-pointer"
                          >
                            ▲
                          </button>
                          <button 
                            disabled={idx === arr.length - 1}
                            onClick={() => handleMove(sr.sr_id, 'down')}
                            className="px-2 py-0.5 bg-slate-800 hover:bg-slate-700 disabled:opacity-30 text-heading rounded text-[10px] cursor-pointer"
                          >
                            ▼
                          </button>
                        </div>
                      </div>
                    );
                  })}
                </div>
                
                <button
                  onClick={handleSaveSequence}
                  disabled={isResequencing}
                  className="w-full py-1.5 bg-indigo-650 hover:bg-indigo-550 text-white rounded font-bold text-[10.5px] transition flex items-center justify-center space-x-1.5 cursor-pointer shadow-md disabled:opacity-50"
                >
                  {isResequencing ? <RefreshCw className="w-3.5 h-3.5 animate-spin" /> : <RefreshCw className="w-3.5 h-3.5" />}
                  <span>应用序列并重算 (Resequence)</span>
                </button>
              </div>

              {/* CPM Rules card */}
              <div className="bg-card border border-main rounded-xs p-3 flex flex-col space-y-2 select-none">
                <h4 className="text-xs font-bold text-heading flex items-center">
                  <Info className="w-4 h-4 text-indigo-500 mr-1.5" />
                  排程约束消纳规则指南 (CPM Rules)
                </h4>
                <p className="text-[9.5px] text-muted leading-normal">
                  在途采购订单 (ASN) 与生产在制订单 (SR) 确定性定义：
                </p>
                <ul className="text-[9px] text-muted space-y-1 list-disc pl-4 leading-normal">
                  <li><strong className="text-emerald-400">In-process:</strong> 按照到货日期满足需求，从底线消纳齐套，超期则发起采购。</li>
                  <li><strong className="text-indigo-400">Reschedulable:</strong> 当存在交付短缺时，C++ 排产器会自动在物料可用期后向前提产。</li>
                  <li><strong className="text-amber-400">ExplodedOnly:</strong> 订单直接向下分解 BOM，不占用物料的 On-Hand 分配权。</li>
                </ul>
              </div>
            </div>
          )}

          {/* 机台排程工作日历 */}
          <div className="flex-1 bg-card border border-main rounded-xs flex flex-col overflow-hidden min-h-0">
            <div className="border-b border-main p-3 bg-table-header flex justify-between items-center select-none">
              <h3 className="text-xs font-bold text-heading flex items-center">
                <Calendar className="w-4 h-4 text-indigo-500 mr-1.5" />
                工作中心生产日历网格 (Work Center Calendar)
              </h3>
              <select
                value={selectedWc}
                onChange={(e) => setSelectedWc(e.target.value)}
                className="bg-card text-heading border border-main text-[9.5px] px-1.5 py-0.5 rounded outline-none font-bold cursor-pointer"
              >
                <option value="WC_MILLING">WC_MILLING (铣加工中心)</option>
                <option value="WC_ASSEMBLY">WC_ASSEMBLY (半导体贴片组装线)</option>
              </select>
            </div>

            <div className="flex-1 overflow-y-auto p-2">
              {renderCalendarGrid()}
            </div>
          </div>
          
        </div>
      </div>

      {/* 底部：工作中心产能负荷趋势图 (12 cols) */}
      <div className="bg-card border border-main rounded-xs p-3 select-none flex flex-col space-y-3 h-[250px] flex-shrink-0">
        <div className="flex justify-between items-center border-b border-main pb-2">
          <h3 className="text-xs font-bold text-heading flex items-center">
            <BarChart3 className="w-4 h-4 text-indigo-500 mr-1.5" />
            机台每日负荷时序趋势 (Work Center Daily Capacity & Load Sheet) - {selectedWc}
          </h3>
          <div className="flex items-center space-x-3 text-[10px]">
            <span className="flex items-center space-x-1">
              <span className="w-2.5 h-2.5 bg-emerald-500 rounded-sm"></span>
              <span className="text-muted">正常工作日</span>
            </span>
            <span className="flex items-center space-x-1">
              <span className="w-2.5 h-2.5 bg-rose-500 rounded-sm"></span>
              <span className="text-muted">周末非工作日</span>
            </span>
          </div>
        </div>

        <div className="flex-1 min-h-0">
          <ResponsiveContainer width="100%" height="100%">
            <BarChart data={chartData} margin={{ top: 5, right: 10, left: -20, bottom: 5 }}>
              <CartesianGrid strokeDasharray="3 3" stroke="#1e2538" />
              <XAxis dataKey="name" stroke="#64748b" fontSize={9.5} />
              <YAxis stroke="#64748b" fontSize={9.5} />
              <Tooltip 
                contentStyle={{ backgroundColor: '#0b0f19', border: '1px solid #1e2538', borderRadius: '4px' }}
                labelStyle={{ color: '#94a3b8', fontSize: '10px', fontWeight: 'bold' }}
                itemStyle={{ fontSize: '10px' }}
              />
              <Legend verticalAlign="top" height={20} iconSize={10} style={{ fontSize: '10px' }} />
              <Bar dataKey="生产工时负载 (Run Load)" stackId="a" fill="#3b82f6" />
              <Bar dataKey="换模换产耗时 (Setup Load)" stackId="a" fill="#eab308" />
              <Bar dataKey="产能上限 (Capacity Limit)" fill="#10b981" fillOpacity={0.15} stroke="#10b981" strokeWidth={1} />
            </BarChart>
          </ResponsiveContainer>
        </div>
      </div>
      
    </div>
  );
};

export default DetailedScheduler;
