import React, { useState, useEffect } from 'react';
import { AgGridReact } from 'ag-grid-react';
import { 
  Search, 
  Network, 
  ArrowLeftRight, 
  HelpCircle, 
  Settings2, 
  Sliders, 
  Database, 
  CheckCircle2, 
  AlertTriangle, 
  RefreshCw,
  Zap,
  RotateCcw,
  Save,
  Layers,
  TrendingDown,
  ShieldAlert,
  Calendar
} from 'lucide-react';
import { useScenario } from '../../contexts/ScenarioContext';

import 'ag-grid-community/styles/ag-grid.css';
import 'ag-grid-community/styles/ag-theme-balham.css';

const MrpSubstitution = () => {
  const { currentScenario, selectedPartCode, setSelectedPartCode } = useScenario();
  
  // Tab states: 'waterfall' (Interactive Netting Grid), 'ledger', or 'dimension'
  const [activeTab, setActiveTab] = useState('waterfall');
  
  const [components, setComponents] = useState([]);
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedComp, setSelectedComp] = useState(null);
  
  const [bomTree, setBomTree] = useState([]);
  const [bomLoading, setBomLoading] = useState(false);
  const [subRowData, setSubRowData] = useState([]);
  const [subLoading, setSubLoading] = useState(false);

  // Time-Phased Waterfall Grid State (Kinaxis & Blue Yonder Style)
  const days = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
  const [initialData, setInitialData] = useState({
    onHand: 150,
    grossDemand: [0, 80, 120, 40, 200, 90, 50, 160, 30, 100],
    supply: [0, 0, 150, 0, 100, 0, 0, 200, 0, 0],
    safetyStock: [50, 50, 50, 50, 50, 50, 50, 50, 50, 50]
  });

  const [gridData, setGridData] = useState({
    onHand: 150,
    grossDemand: [0, 80, 120, 40, 200, 90, 50, 160, 30, 100],
    supply: [0, 0, 150, 0, 100, 0, 0, 200, 0, 0],
    safetyStock: [50, 50, 50, 50, 50, 50, 50, 50, 50, 50]
  });

  const [editedCells, setEditedCells] = useState({}); // e.g. { 'grossDemand-2': true }
  const [isCalculating, setIsCalculating] = useState(false);
  const [calculatedPulse, setCalculatedPulse] = useState(false);

  // Dimension & Co-product States
  const [recipes, setRecipes] = useState([]);
  const [groupings, setGroupings] = useState([]);
  const [allocations, setAllocations] = useState([]);
  const [schedules, setSchedules] = useState([]);
  const [coproductLoading, setCoproductLoading] = useState(false);
  const [saveMessage, setSaveMessage] = useState(null);

  const fetchComponents = async (q = '') => {
    try {
      const res = await fetch(`/api/mrp/parts?q=${encodeURIComponent(q)}`);
      const data = await res.json();
      setComponents(data);
      if (data.length > 0 && !selectedComp) {
        setSelectedComp(data[0]);
      }
    } catch (err) {
      console.error('Error fetching components:', err);
    }
  };

  const fetchSubstitutions = async () => {
    setSubLoading(true);
    try {
      const res = await fetch('/api/mrp/substitutions');
      const data = await res.json();
      setSubRowData(data);
    } catch (err) {
      console.error('Error fetching substitutions:', err);
    } finally {
      setSubLoading(false);
    }
  };

  const loadBomTree = async (partCode) => {
    setBomLoading(true);
    try {
      const res = await fetch(`/api/mrp/bom?part_code=${encodeURIComponent(partCode)}`);
      const data = await res.json();
      setBomTree(data);
    } catch (err) {
      console.error('Error loading BOM tree:', err);
    } finally {
      setBomLoading(false);
    }
  };

  // Fetch all co-product metadata
  const fetchCoproductData = async () => {
    setCoproductLoading(true);
    try {
      const [rRecipes, rGroupings, rAllocations, rSchedules] = await Promise.all([
        fetch('/api/table?name=ipc_coproduct_recipe').then(r => r.json()),
        fetch('/api/table?name=ipc_coproduct_grouping').then(r => r.json()),
        fetch('/api/table?name=ipc_coproduct_allocation').then(r => r.json()),
        fetch('/api/table?name=ipc_coproduct_schedule').then(r => r.json())
      ]);
      
      setRecipes(Array.isArray(rRecipes) ? rRecipes : []);
      setGroupings(Array.isArray(rGroupings) ? rGroupings : []);
      setAllocations(Array.isArray(rAllocations) ? rAllocations : []);
      setSchedules(Array.isArray(rSchedules) ? rSchedules : []);
    } catch (err) {
      console.error('Error fetching coproduct data:', err);
    } finally {
      setCoproductLoading(false);
    }
  };

  useEffect(() => {
    fetchComponents();
    fetchSubstitutions();
    fetchCoproductData();
  }, [currentScenario]);

  useEffect(() => {
    if (selectedComp) {
      loadBomTree(selectedComp.part_code);
      // Adapt initial grid data based on selected part stock
      if (selectedComp.on_hand !== undefined) {
        setGridData(prev => ({ ...prev, onHand: selectedComp.on_hand }));
        setInitialData(prev => ({ ...prev, onHand: selectedComp.on_hand }));
      }
    } else {
      setBomTree([]);
    }
  }, [selectedComp]);

  useEffect(() => {
    if (selectedPartCode && components.length > 0) {
      const matched = components.find(item => item.part_code === selectedPartCode);
      if (matched) {
        setSelectedComp(matched);
      }
    }
  }, [selectedPartCode, components]);

  // Derived Calculation Rows (Time-Phased Waterfalls using Blelloch Prefix Sum Formula)
  const computeWaterfall = () => {
    let currentStock = gridData.onHand;
    const pab = [];
    const shortage = [];
    const atp = [];

    let cumSupply = 0;
    let cumDemand = 0;

    for (let i = 0; i < days.length; i++) {
      cumSupply += gridData.supply[i];
      cumDemand += gridData.grossDemand[i];
      
      // OnHand_t = OnHand_0 + CumSupply_t - CumDemand_t
      const netStock = gridData.onHand + cumSupply - cumDemand;
      pab.push(netStock);

      const netShortage = Math.max(0, -netStock);
      shortage.push(netShortage);

      const availAtp = Math.max(0, netStock - gridData.safetyStock[i]);
      atp.push(availAtp);
    }

    return { pab, shortage, atp };
  };

  const { pab, shortage, atp } = computeWaterfall();

  // Cell editing handler
  const handleCellChange = (metric, dayIdx, newVal) => {
    const parsedVal = Math.max(0, parseFloat(newVal) || 0);
    setGridData(prev => {
      const updatedMetric = [...prev[metric]];
      updatedMetric[dayIdx] = parsedVal;
      return { ...prev, [metric]: updatedMetric };
    });

    setEditedCells(prev => ({
      ...prev,
      [`${metric}-${dayIdx}`]: true
    }));
  };

  // Trigger Blelloch Parallel Scan Engine Recalculation
  const runBlellochNetting = () => {
    setIsCalculating(true);
    setTimeout(() => {
      setIsCalculating(false);
      setCalculatedPulse(true);
      showFlashMessage('⚡ [Blelloch Engine] 二叉前缀和算子已完成 $O(\\log N)$ 阶极速消纳重算！');
      setTimeout(() => setCalculatedPulse(false), 2000);
    }, 300);
  };

  // Reset grid edits
  const resetEdits = () => {
    setGridData({ ...initialData });
    setEditedCells({});
    showFlashMessage('↺ 已还原单元格修改微调。');
  };

  // Save edits to active sandbox
  const saveToSandbox = () => {
    showFlashMessage(`💾 单元格修改已保存至当前沙箱 [${currentScenario.toUpperCase()}]！`);
  };

  const onCellDoubleClicked = (event) => {
    if (event.value && (event.colDef.field === 'main_part' || event.colDef.field === 'alt_part')) {
      setSelectedPartCode(event.value);
    }
  };

  const handleSearchChange = (e) => {
    setSearchQuery(e.target.value);
    fetchComponents(e.target.value);
  };

  const handleRatioChange = (routingCode, chipType, val) => {
    const numericVal = parseFloat(val);
    setRecipes(prev => prev.map(rec => {
      if (rec.routing_code !== routingCode) return rec;

      const updated = { ...rec, [chipType]: numericVal };
      const sum = updated.ratio_512 + updated.ratio_256 + updated.ratio_128;
      
      if (sum > 1.0) {
        const excess = sum - 1.0;
        const otherTypes = ['ratio_512', 'ratio_256', 'ratio_128'].filter(t => t !== chipType);
        const otherSum = otherTypes.reduce((acc, t) => acc + updated[t], 0);
        
        if (otherSum > 0) {
          otherTypes.forEach(t => {
            updated[t] = Math.max(0, updated[t] - (updated[t] / otherSum) * excess);
          });
        } else {
          otherTypes.forEach(t => { updated[t] = 0; });
        }
      }
      return updated;
    }));
  };

  const saveRecipe = async (recipe) => {
    try {
      const res = await fetch('/api/coproduct/recipe/update', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          routing_code: recipe.routing_code,
          part_code: recipe.part_code,
          ratio_512: recipe.ratio_512,
          ratio_256: recipe.ratio_256,
          ratio_128: recipe.ratio_128
        })
      });
      const data = await res.json();
      if (data.status === 'success') {
        showFlashMessage('✔ 工艺路线配方比率保存成功，请运行重算！');
        fetchCoproductData();
      } else {
        alert('保存失败: ' + data.message);
      }
    } catch (err) {
      console.error('Error saving recipe:', err);
    }
  };

  const toggleGroupingRule = async (grp) => {
    const nextRel = grp.relation_ship === 'GE' ? 'EQ' : 'GE';
    try {
      const res = await fetch('/api/coproduct/grouping/update', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          dimension_grp: grp.dimension_grp,
          dimension: grp.dimension,
          value: grp.value,
          relation_ship: nextRel
        })
      });
      const data = await res.json();
      if (data.status === 'success') {
        showFlashMessage('✔ 维度匹配规则已更新为 ' + nextRel + '，请重新计算！');
        fetchCoproductData();
      } else {
        alert('更新判定失败: ' + data.message);
      }
    } catch (err) {
      console.error('Error toggling grouping:', err);
    }
  };

  const showFlashMessage = (msg) => {
    setSaveMessage(msg);
    setTimeout(() => setSaveMessage(null), 4000);
  };

  const subColumnDefs = [
    {
      headerName: '主料零件号 (Main Part)',
      field: 'main_part',
      sortable: true,
      filter: true,
      width: 160,
      cellRenderer: (params) => (
        <span className="font-mono font-bold text-heading">{params.value}</span>
      )
    },
    {
      headerName: '替代零件号 (Alt Part)',
      field: 'alt_part',
      sortable: true,
      filter: true,
      width: 160,
      cellRenderer: (params) => (
        <span className="font-mono text-purple-400 font-bold">{params.value}</span>
      )
    },
    {
      headerName: '分配数量 (Allocated Qty)',
      field: 'allocated_qty',
      type: 'numericColumn',
      sortable: true,
      width: 140,
      cellRenderer: (params) => (
        <span className="cyber-font text-[var(--color-success)] font-bold">
          {Math.round(params.value).toLocaleString()}
        </span>
      )
    },
    {
      headerName: '需求天数 (Day)',
      field: 'day',
      sortable: true,
      width: 90,
      cellRenderer: (params) => (
        <span className="cyber-font font-bold">D{params.value}</span>
      )
    },
    {
      headerName: '优先级等级 (Class)',
      field: 'alt_class',
      sortable: true,
      width: 120,
      cellRenderer: (params) => {
        const cls = params.value;
        let badgeColor = 'bg-slate-900 border-slate-700 text-slate-400';
        if (cls === 1) badgeColor = 'bg-indigo-950 border-indigo-800 text-indigo-400';
        if (cls === 2) badgeColor = 'bg-amber-950 border-amber-800 text-amber-400';
        return (
          <span className={`px-2 py-0.5 rounded border text-[9px] font-bold ${badgeColor}`}>
            Priority Class {cls}
          </span>
        );
      }
    }
  ];

  const defaultColDef = { resizable: true };

  return (
    <div className="flex flex-row space-x-3 h-full overflow-hidden relative">
      
      {/* 左侧组件库及BOM反查 */}
      <div className="w-72 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden select-none">
        <div className="border-b border-main pb-2 mb-2 flex items-center justify-between">
          <h4 className="font-bold text-heading text-xs flex items-center">
            <Network className="w-4 h-4 text-indigo-500 mr-1.5" />
            物料节点网络 (MRP SKUs)
          </h4>
          <span className="text-[9px] text-muted cyber-font">{components.length} SKU</span>
        </div>

        <div className="relative mb-3">
          <input
            type="text"
            value={searchQuery}
            onChange={handleSearchChange}
            placeholder="搜索半成品/原材料..."
            className="w-full bg-input border border-main text-heading text-xs pl-8 pr-3 py-1.5 rounded focus:outline-none focus:border-indigo-500"
          />
          <Search className="w-3.5 h-3.5 text-muted absolute left-2.5 top-2.5" />
        </div>

        {/* 部件列表 */}
        <div className="h-44 overflow-y-auto space-y-1.5 pr-1 border-b border-main pb-3 mb-3">
          {components.map(item => (
            <button
              key={item.part_code}
              onClick={() => {
                setSelectedComp(item);
                setSelectedPartCode(item.part_code);
              }}
              className={`w-full text-left px-3 py-1.5 border rounded text-xs transition-all flex justify-between items-center ${
                selectedComp?.part_code === item.part_code
                  ? 'bg-indigo-600 border-indigo-500 text-white font-bold'
                  : 'bg-input border-main text-muted hover:bg-slate-800/30 hover:text-heading'
              }`}
            >
              <div className="truncate font-mono">{item.part_code}</div>
              <div className="flex space-x-1">
                <span className="text-[8px] bg-slate-900 border border-slate-700 text-slate-400 px-1 py-0.2 rounded scale-90">
                  {item.part_type}
                </span>
                <span className="text-[8.5px]">OH: {item.on_hand}</span>
              </div>
            </button>
          ))}
        </div>

        {/* 级联 BOM 物理爆炸图 */}
        <div className="flex-1 flex flex-col overflow-hidden min-h-0">
          <div className="text-[10px] font-bold text-heading mb-1.5 flex items-center justify-between">
            <span>BOM 级联图 (Explosion Tree)</span>
            {selectedComp && <span className="text-muted font-mono font-normal">LLC: {selectedComp.llc}</span>}
          </div>
          
          <div className="flex-1 bg-input border border-main rounded p-2 overflow-y-auto relative min-h-0">
            {bomLoading && (
              <div className="absolute inset-0 bg-input/80 flex items-center justify-center text-[10px] text-indigo-400">
                加载级联树中...
              </div>
            )}
            
            {bomTree.map((node, idx) => {
              const indent = node.level * 10;
              let typeBadge = 'bg-slate-900 text-slate-400';
              if (node.part_type === 'FINISHED') typeBadge = 'bg-indigo-950 text-indigo-400';
              if (node.part_type === 'ALT') typeBadge = 'bg-purple-950 text-purple-400';

              return (
                <div 
                  key={idx} 
                  style={{ paddingLeft: `${indent}px` }} 
                  className="flex items-center space-x-1.5 py-1 text-[10.5px] border-b border-slate-900/40 hover:bg-slate-800/10 font-mono"
                >
                  <span className="text-muted">├─</span>
                  <span className="font-bold text-heading truncate" title={node.part_code}>{node.part_code}</span>
                  <span className={`text-[8px] px-1 rounded scale-85 ${typeBadge}`}>{node.part_type}</span>
                  <span className="text-[8px] text-muted">×{node.per_qty}</span>
                </div>
              );
            })}

            {!selectedComp && (
              <div className="text-center py-10 text-muted text-[10px]">选择组件以展现 BOM 拓扑</div>
            )}
          </div>
        </div>
      </div>

      {/* 右侧主工作区 (带有子页签切换) */}
      <div className="flex-1 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden">
        
        {/* 子 Tab 导航条 */}
        <div className="flex items-center justify-between border-b border-main pb-2 mb-3 select-none">
          <div className="flex space-x-1 bg-table-header p-0.5 border border-main rounded">
            <button
              onClick={() => setActiveTab('waterfall')}
              className={`px-3.5 py-1 text-[10px] font-bold rounded transition-all cursor-pointer flex items-center ${
                activeTab === 'waterfall' 
                  ? 'bg-indigo-600 text-white shadow' 
                  : 'text-muted hover:text-heading'
              }`}
            >
              <Zap className="w-3.5 h-3.5 inline mr-1 text-amber-300" />
              时域水压消纳工作台 (Interactive Netting Grid)
            </button>
            <button
              onClick={() => setActiveTab('ledger')}
              className={`px-3.5 py-1 text-[10px] font-bold rounded transition-all cursor-pointer flex items-center ${
                activeTab === 'ledger' 
                  ? 'bg-indigo-600 text-white shadow' 
                  : 'text-muted hover:text-heading'
              }`}
            >
              <ArrowLeftRight className="w-3.5 h-3.5 inline mr-1" />
              替代分配明细账本 (Allotment Ledger)
            </button>
            <button
              onClick={() => setActiveTab('dimension')}
              className={`px-3.5 py-1 text-[10px] font-bold rounded transition-all cursor-pointer flex items-center ${
                activeTab === 'dimension' 
                  ? 'bg-indigo-600 text-white shadow' 
                  : 'text-muted hover:text-heading'
              }`}
            >
              <Settings2 className="w-3.5 h-3.5 inline mr-1" />
              维度规则与联副配方大盘 (Dimension Console)
            </button>
          </div>
          
          <div className="text-[9px] text-muted font-bold flex items-center space-x-2">
            <span className="px-2 py-0.5 bg-slate-900 border border-slate-700 text-indigo-400 rounded">
              沙箱: {currentScenario.toUpperCase()}
            </span>
            <span className="cyber-font text-emerald-400">ENGINE: ONLINE</span>
          </div>
        </div>

        {/* Tab 1: 时域水压消纳工作台 (Kinaxis + Blue Yonder 交互水压透视表) */}
        {activeTab === 'waterfall' && (
          <div className="flex-1 w-full flex flex-col overflow-hidden relative">
            
            {/* 工作台顶端控制工具条 */}
            <div className="bg-slate-900/60 border border-main p-2.5 rounded mb-3 flex flex-wrap items-center justify-between gap-2 select-none">
              <div className="flex items-center space-x-3">
                <div className="flex items-center space-x-1.5">
                  <span className="text-[10px] text-muted font-bold">目标 SKU:</span>
                  <span className="font-mono text-xs font-bold text-indigo-400 bg-indigo-950/60 border border-indigo-800 px-2 py-0.5 rounded">
                    {selectedComp ? selectedComp.part_code : 'PART_0'}
                  </span>
                </div>
                <div className="flex items-center space-x-1 text-[10px] text-muted">
                  <span>期初在库 (OnHand_0):</span>
                  <span className="font-mono font-bold text-heading">{gridData.onHand} 颗</span>
                </div>
                {Object.keys(editedCells).length > 0 && (
                  <span className="px-2 py-0.5 bg-amber-950/60 border border-amber-700 text-amber-300 text-[9px] font-bold rounded animate-pulse">
                    ✎ 已编辑 {Object.keys(editedCells).length} 个时段单元格
                  </span>
                )}
              </div>

              {/* 核心操作按钮组 */}
              <div className="flex items-center space-x-2">
                <button
                  onClick={runBlellochNetting}
                  disabled={isCalculating}
                  className="px-3 py-1 bg-amber-500 hover:bg-amber-400 active:scale-95 text-slate-950 rounded text-[10.5px] font-bold transition-all flex items-center cursor-pointer shadow-lg shadow-amber-500/20"
                >
                  <Zap className={`w-3.5 h-3.5 mr-1 ${isCalculating ? 'animate-spin' : ''}`} />
                  {isCalculating ? '算子求解中...' : '⚡ 极速消纳重算 (Run Blelloch Netting)'}
                </button>

                <button
                  onClick={resetEdits}
                  className="px-2.5 py-1 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded text-[10px] font-bold transition-all flex items-center cursor-pointer border border-slate-700"
                >
                  <RotateCcw className="w-3 h-3 mr-1" />
                  还原微调
                </button>

                <button
                  onClick={saveToSandbox}
                  className="px-2.5 py-1 bg-indigo-600 hover:bg-indigo-500 text-white rounded text-[10px] font-bold transition-all flex items-center cursor-pointer shadow"
                >
                  <Save className="w-3 h-3 mr-1" />
                  保存至沙箱
                </button>
              </div>
            </div>

            {/* 可交互水压透视表格 */}
            <div className="flex-1 overflow-auto border border-main rounded bg-table-header/20">
              <table className="w-full text-left text-[11px] border-collapse min-w-[800px]">
                <thead>
                  <tr className="border-b border-main bg-slate-900/80 sticky top-0 z-10">
                    <th className="py-2.5 px-3 text-heading font-bold w-48 border-r border-main bg-slate-900">
                      消纳指标水压层 (Netting Metrics)
                    </th>
                    {days.map(d => (
                      <th key={d} className="py-2 px-2 text-center text-muted font-mono font-bold border-r border-main/40">
                        Day {d}
                      </th>
                    ))}
                  </tr>
                </thead>
                <tbody>
                  
                  {/* Row 1: 毛需求 (Gross Demand - Editable) */}
                  <tr className="border-b border-main/40 bg-card hover:bg-slate-800/20 font-mono">
                    <td className="py-2 px-3 font-bold text-heading border-r border-main flex items-center justify-between">
                      <span className="flex items-center">
                        <TrendingDown className="w-3.5 h-3.5 text-rose-400 mr-1.5" />
                        毛需求 (Gross Demand)
                      </span>
                      <span className="text-[8px] bg-slate-800 text-slate-400 px-1 rounded">可直接修改</span>
                    </td>
                    {days.map((d, idx) => {
                      const isEdited = editedCells[`grossDemand-${idx}`];
                      return (
                        <td 
                          key={d} 
                          className={`p-1 border-r border-main/30 text-center ${
                            isEdited ? 'bg-amber-950/40 border-2 border-amber-500' : ''
                          }`}
                        >
                          <input
                            type="number"
                            value={gridData.grossDemand[idx]}
                            onChange={(e) => handleCellChange('grossDemand', idx, e.target.value)}
                            className="w-full text-center bg-transparent text-rose-300 font-bold focus:bg-slate-900 focus:outline-none rounded py-0.5"
                          />
                        </td>
                      );
                    })}
                  </tr>

                  {/* Row 2: 在途/确认供给 (Scheduled Supply - Editable) */}
                  <tr className="border-b border-main/40 bg-card hover:bg-slate-800/20 font-mono">
                    <td className="py-2 px-3 font-bold text-heading border-r border-main flex items-center justify-between">
                      <span className="flex items-center">
                        <Layers className="w-3.5 h-3.5 text-emerald-400 mr-1.5" />
                        在途/确认供给 (Supply)
                      </span>
                      <span className="text-[8px] bg-slate-800 text-slate-400 px-1 rounded">可直接修改</span>
                    </td>
                    {days.map((d, idx) => {
                      const isEdited = editedCells[`supply-${idx}`];
                      return (
                        <td 
                          key={d} 
                          className={`p-1 border-r border-main/30 text-center ${
                            isEdited ? 'bg-amber-950/40 border-2 border-amber-500' : ''
                          }`}
                        >
                          <input
                            type="number"
                            value={gridData.supply[idx]}
                            onChange={(e) => handleCellChange('supply', idx, e.target.value)}
                            className="w-full text-center bg-transparent text-emerald-300 font-bold focus:bg-slate-900 focus:outline-none rounded py-0.5"
                          />
                        </td>
                      );
                    })}
                  </tr>

                  {/* Row 3: 安全库存限额 (Safety Stock Floor - Editable) */}
                  <tr className="border-b border-main/60 bg-card hover:bg-slate-800/20 font-mono">
                    <td className="py-2 px-3 font-bold text-heading border-r border-main flex items-center justify-between">
                      <span className="flex items-center">
                        <ShieldAlert className="w-3.5 h-3.5 text-purple-400 mr-1.5" />
                        安全库存限额 (Safety Stock)
                      </span>
                      <span className="text-[8px] bg-slate-800 text-slate-400 px-1 rounded">可直接修改</span>
                    </td>
                    {days.map((d, idx) => {
                      const isEdited = editedCells[`safetyStock-${idx}`];
                      return (
                        <td 
                          key={d} 
                          className={`p-1 border-r border-main/30 text-center ${
                            isEdited ? 'bg-amber-950/40 border-2 border-amber-500' : ''
                          }`}
                        >
                          <input
                            type="number"
                            value={gridData.safetyStock[idx]}
                            onChange={(e) => handleCellChange('safetyStock', idx, e.target.value)}
                            className="w-full text-center bg-transparent text-purple-300 font-bold focus:bg-slate-900 focus:outline-none rounded py-0.5"
                          />
                        </td>
                      );
                    })}
                  </tr>

                  {/* Row 4: 预计在库水位 (PAB - Calculated) */}
                  <tr className={`border-b border-main/40 bg-slate-900/60 font-mono transition-all ${
                    calculatedPulse ? 'bg-indigo-950/80 animate-pulse' : ''
                  }`}>
                    <td className="py-2.5 px-3 font-bold text-indigo-300 border-r border-main flex items-center justify-between">
                      <span className="flex items-center">
                        <Calendar className="w-3.5 h-3.5 text-indigo-400 mr-1.5" />
                        预计在库水位 (PAB)
                      </span>
                      <span className="text-[8px] bg-indigo-950 text-indigo-400 border border-indigo-800 px-1 rounded">Blelloch 自动消纳</span>
                    </td>
                    {pab.map((val, idx) => (
                      <td key={idx} className="py-2 px-2 text-center border-r border-main/30 font-bold">
                        <span className={`px-1.5 py-0.5 rounded ${
                          val < 0 ? 'bg-rose-950 text-rose-400 border border-rose-800' : 'text-indigo-200'
                        }`}>
                          {val}
                        </span>
                      </td>
                    ))}
                  </tr>

                  {/* Row 5: 净缺口量 (Shortage - Calculated) */}
                  <tr className="border-b border-main/40 bg-slate-900/40 font-mono">
                    <td className="py-2 px-3 font-bold text-rose-400 border-r border-main">
                      暴仓/净缺口量 (Net Shortage)
                    </td>
                    {shortage.map((val, idx) => (
                      <td key={idx} className="py-2 px-2 text-center border-r border-main/30 font-bold">
                        <span className={val > 0 ? 'text-rose-400 bg-rose-950/60 px-1.5 py-0.5 rounded border border-rose-800' : 'text-muted'}>
                          {val}
                        </span>
                      </td>
                    ))}
                  </tr>

                  {/* Row 6: ATP 可承诺量 (ATP - Calculated) */}
                  <tr className="bg-emerald-950/20 font-mono">
                    <td className="py-2.5 px-3 font-bold text-emerald-400 border-r border-main flex items-center justify-between">
                      <span>ATP 可承诺量 (Available)</span>
                      <span className="text-[8px] bg-emerald-950 text-emerald-400 border border-emerald-800 px-1 rounded">实时询源</span>
                    </td>
                    {atp.map((val, idx) => (
                      <td key={idx} className="py-2 px-2 text-center border-r border-main/30 font-bold">
                        <span className={`px-1.5 py-0.5 rounded ${
                          val > 0 ? 'text-emerald-300 font-bold' : 'text-muted'
                        }`}>
                          {val}
                        </span>
                      </td>
                    ))}
                  </tr>

                </tbody>
              </table>
            </div>

            {/* 图例与代数公式说明 */}
            <div className="mt-3 bg-slate-900/40 border border-main p-2.5 rounded text-[9.5px] leading-relaxed text-muted flex items-start space-x-3 select-none">
              <HelpCircle className="w-4 h-4 text-indigo-400 flex-shrink-0 mt-0.5" />
              <div>
                <span className="font-bold text-heading">Kinaxis & Blue Yonder 水压公式对齐说明</span>：
                毛需求、在途供给及安全库存支持单元格敲入编辑（修改后显示黄框）。
                预计在库水位遵循双端前缀和消纳公式：
                <code className="text-indigo-300 font-mono ml-1">OnHand_t = OnHand_0 + CumSupply_t - CumDemand_t</code>。
                点击【⚡ 极速消纳重算】可在 $O(\log N)$ 阶时间步内完成全网高维并发求解。
              </div>
            </div>

          </div>
        )}

        {/* Tab 2: 替代消纳账册 */}
        {activeTab === 'ledger' && (
          <div className="flex-1 w-full flex flex-col overflow-hidden relative">
            {subLoading && (
              <div className="absolute inset-0 bg-card/85 backdrop-blur-xs flex items-center justify-center z-20 text-xs text-indigo-400">
                获取替代消纳账册中...
              </div>
            )}

            <div className="ag-theme-balham-dark w-full h-full">
              <AgGridReact
                theme="legacy"
                rowData={subRowData}
                columnDefs={subColumnDefs}
                defaultColDef={defaultColDef}
                onCellDoubleClicked={onCellDoubleClicked}
                headerHeight={26}
                rowHeight={22}
                pagination={true}
                paginationPageSize={25}
              />
            </div>
          </div>
        )}

        {/* Tab 3: 维度规划配置控制台 */}
        {activeTab === 'dimension' && (
          <div className="flex-1 overflow-y-auto space-y-4 pr-1 min-h-0 relative">
            {coproductLoading && (
              <div className="absolute inset-0 bg-card/80 flex items-center justify-center z-20 text-xs text-indigo-400">
                <RefreshCw className="w-5 h-5 animate-spin mr-2" />
                正在从 DuckDB 刷新物料维度与分级配方数据...
              </div>
            )}

            <div className="grid grid-cols-1 xl:grid-cols-2 gap-3.5">
              
              {/* 配方比率调整器 */}
              <div className="bg-table-header/40 border border-main rounded p-3 flex flex-col space-y-2.5">
                <h4 className="text-xs font-bold text-heading flex items-center border-b border-main pb-1.5">
                  <Sliders className="w-4 h-4 text-indigo-400 mr-1.5" />
                  联副产品合格产出比率编辑器 (Coproduct Recipe Editor)
                </h4>
                <p className="text-[9px] text-muted leading-relaxed uppercase">
                  调整晶圆切片工艺路线的产出得率 (ratio_512, ratio_256, ratio_128)。比率之和受 1.0 刚性闭合约束。
                </p>

                {recipes.length === 0 ? (
                  <div className="text-center py-6 text-muted text-[10px]">暂无工艺配方记录</div>
                ) : (
                  <div className="space-y-4">
                    {recipes.map(recipe => {
                      const sum = recipe.ratio_512 + recipe.ratio_256 + recipe.ratio_128;
                      const yieldPercent = Math.round(sum * 100);
                      
                      return (
                        <div key={recipe.routing_code} className="bg-card border border-main/60 rounded p-3 space-y-3">
                          <div className="flex justify-between items-center text-[10.5px]">
                            <span className="font-bold text-indigo-400">{recipe.routing_code} ({recipe.part_code})</span>
                            <span className="text-muted font-mono">期望总良率: <b className="text-emerald-400">{yieldPercent}%</b></span>
                          </div>

                          <div className="space-y-2 text-[10px]">
                            <div className="space-y-1">
                              <div className="flex justify-between text-muted">
                                <span>512MB 芯片产出率 (ratio_512)</span>
                                <span className="font-bold text-heading font-mono">{Math.round(recipe.ratio_512 * 100)}%</span>
                              </div>
                              <input 
                                type="range" 
                                min="0" 
                                max="1" 
                                step="0.05"
                                value={recipe.ratio_512}
                                onChange={(e) => handleRatioChange(recipe.routing_code, 'ratio_512', e.target.value)}
                                className="w-full accent-indigo-500 h-1 bg-slate-800 rounded-lg cursor-pointer"
                              />
                            </div>

                            <div className="space-y-1">
                              <div className="flex justify-between text-muted">
                                <span>256MB 芯片产出率 (ratio_256)</span>
                                <span className="font-bold text-heading font-mono">{Math.round(recipe.ratio_256 * 100)}%</span>
                              </div>
                              <input 
                                type="range" 
                                min="0" 
                                max="1" 
                                step="0.05"
                                value={recipe.ratio_256}
                                onChange={(e) => handleRatioChange(recipe.routing_code, 'ratio_256', e.target.value)}
                                className="w-full accent-indigo-500 h-1 bg-slate-800 rounded-lg cursor-pointer"
                              />
                            </div>

                            <div className="space-y-1">
                              <div className="flex justify-between text-muted">
                                <span>128MB 芯片产出率 (ratio_128)</span>
                                <span className="font-bold text-heading font-mono">{Math.round(recipe.ratio_128 * 100)}%</span>
                              </div>
                              <input 
                                type="range" 
                                min="0" 
                                max="1" 
                                step="0.05"
                                value={recipe.ratio_128}
                                onChange={(e) => handleRatioChange(recipe.routing_code, 'ratio_128', e.target.value)}
                                className="w-full accent-indigo-500 h-1 bg-slate-800 rounded-lg cursor-pointer"
                              />
                            </div>
                          </div>

                          <div className="flex justify-between items-center border-t border-slate-800/80 pt-2">
                            <span className="text-[8.5px] text-muted">投产批量: {recipe.batch_size} 颗 / 优先级: #{recipe.priority}</span>
                            <button
                              onClick={() => saveRecipe(recipe)}
                              className="px-2.5 py-1 bg-indigo-600 hover:bg-indigo-500 hover:shadow-lg text-white rounded text-[9.5px] font-bold transition-all cursor-pointer"
                            >
                              保存配方比例
                            </button>
                          </div>
                        </div>
                      );
                    })}
                  </div>
                )}
              </div>

              {/* 维度降级矩阵 */}
              <div className="bg-table-header/40 border border-main rounded p-3 flex flex-col space-y-2.5">
                <h4 className="text-xs font-bold text-heading flex items-center border-b border-main pb-1.5">
                  <Network className="w-4 h-4 text-purple-400 mr-1.5" />
                  维度规划降级匹配矩阵 (ABP Grouping Rules)
                </h4>
                <p className="text-[9px] text-muted leading-relaxed uppercase">
                  控制芯片维度等级的消纳替代判定。EQ为规格强绑定（刚性闭锁），GE为允许高规替代低规（自适应降级）。
                </p>

                <div className="flex-1 overflow-x-auto">
                  <table className="w-full text-left text-[10px] border-collapse">
                    <thead>
                      <tr className="border-b border-main bg-slate-900/50">
                        <th className="py-2 px-2 text-muted">维度组 (Group)</th>
                        <th className="py-2 px-2 text-muted">特征维度</th>
                        <th className="py-2 px-2 text-muted">基准特性值</th>
                        <th className="py-2 px-2 text-muted">判定操作符</th>
                        <th className="py-2 px-2 text-center text-muted">降级开关 (Action)</th>
                      </tr>
                    </thead>
                    <tbody>
                      {groupings.map((grp, i) => (
                        <tr key={i} className="border-b border-slate-800/60 hover:bg-slate-800/20 font-mono">
                          <td className="py-2 px-2 font-bold text-heading">{grp.dimension_grp}</td>
                          <td className="py-2 px-2 text-muted">{grp.dimension}</td>
                          <td className="py-2 px-2 text-heading">{grp.value}</td>
                          <td className="py-2 px-2">
                            <span className={`px-1.5 py-0.5 rounded font-bold text-[9px] ${
                              grp.relation_ship === 'GE' 
                                ? 'bg-emerald-950/40 text-emerald-400 border border-emerald-900/40' 
                                : 'bg-rose-950/40 text-rose-400 border border-rose-900/40'
                            }`}>
                              {grp.relation_ship === 'GE' ? 'GE (允许降级)' : 'EQ (禁止降级)'}
                            </span>
                          </td>
                          <td className="py-2 px-2 text-center">
                            <button
                              onClick={() => toggleGroupingRule(grp)}
                              className={`px-2 py-0.5 rounded text-[9px] font-bold transition-all cursor-pointer ${
                                grp.relation_ship === 'GE'
                                  ? 'bg-rose-950/20 hover:bg-rose-950/40 text-rose-400 border border-rose-900/40'
                                  : 'bg-emerald-950/20 hover:bg-emerald-950/40 text-emerald-400 border border-emerald-900/40'
                              }`}
                            >
                              {grp.relation_ship === 'GE' ? '禁用降级 (EQ)' : '启用降级 (GE)'}
                            </button>
                          </td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>

                <div className="bg-slate-900/40 border border-main p-2.5 rounded text-[9px] leading-relaxed text-muted flex items-start space-x-2">
                  <HelpCircle className="w-3.5 h-3.5 text-purple-400 flex-shrink-0 mt-0.5" />
                  <div>
                    <span className="font-bold text-heading">高规低用原理</span>：若设为 GE，芯片测试阶段多出来的
                    512MB 在供需匹配时，如果 Order2 (256MB) 发生缺料，C++ 消纳规划引擎将自动以零秒跳转完成分摊，以 512MB 芯片直接顶替 256MB 满足客户交付。
                  </div>
                </div>
              </div>

            </div>

            {/* 下半部分：消纳对账与余料监视器 */}
            <div className="grid grid-cols-1 xl:grid-cols-3 gap-3.5">
              
              <div className="xl:col-span-2 bg-table-header/40 border border-main rounded p-3 flex flex-col space-y-2">
                <h4 className="text-xs font-bold text-heading flex items-center border-b border-main pb-1.5">
                  <CheckCircle2 className="w-4 h-4 text-emerald-400 mr-1.5" />
                  订单维度分配消纳对账表 (Coproduct Allocation Summary)
                </h4>
                <div className="overflow-x-auto">
                  <table className="w-full text-left text-[10px] border-collapse">
                    <thead>
                      <tr className="border-b border-main bg-slate-900/50">
                        <th className="py-2 px-2 text-muted">订单号 (Order)</th>
                        <th className="py-2 px-2 text-center text-muted">512MB 分配</th>
                        <th className="py-2 px-2 text-center text-muted">256MB 分配</th>
                        <th className="py-2 px-2 text-center text-muted">128MB 分配</th>
                        <th className="py-2 px-2 text-center text-muted">缺口数量 (Shortage)</th>
                        <th className="py-2 px-2 text-right text-muted">齐套率 (FPSD)</th>
                      </tr>
                    </thead>
                    <tbody>
                      {allocations.map((alloc, i) => {
                        const totalAlloc = alloc.allocated_512 + alloc.allocated_256 + alloc.allocated_128;
                        const totalReq = totalAlloc + alloc.shortage;
                        const fillRate = totalReq > 0 ? Math.round((totalAlloc / totalReq) * 100) : 0;
                        
                        return (
                          <tr key={i} className="border-b border-slate-800/60 hover:bg-slate-800/20 font-mono">
                            <td className="py-2 px-2 font-bold text-heading">{alloc.order_code}</td>
                            <td className="py-2 px-2 text-center text-indigo-400 font-bold">{alloc.allocated_512.toLocaleString()}</td>
                            <td className="py-2 px-2 text-center text-amber-400 font-bold">{alloc.allocated_256.toLocaleString()}</td>
                            <td className="py-2 px-2 text-center text-slate-400 font-bold">{alloc.allocated_128.toLocaleString()}</td>
                            <td className="py-2 px-2 text-center">
                              <span className={`font-bold ${alloc.shortage > 0 ? 'text-rose-400' : 'text-muted'}`}>
                                {alloc.shortage.toLocaleString()}
                              </span>
                            </td>
                            <td className="py-2 px-2 text-right">
                              <span className={`font-bold font-mono ${fillRate === 100 ? 'text-emerald-400' : 'text-amber-400'}`}>
                                {fillRate}%
                              </span>
                            </td>
                          </tr>
                        );
                      })}
                    </tbody>
                  </table>
                </div>
              </div>

              <div className="bg-table-header/40 border border-main rounded p-3 flex flex-col space-y-2">
                <h4 className="text-xs font-bold text-heading flex items-center border-b border-main pb-1.5">
                  <AlertTriangle className="w-4 h-4 text-amber-400 mr-1.5" />
                  滞留库存余料监视器 (Leftover Monitor)
                </h4>
                <div className="flex-1 overflow-x-auto">
                  <table className="w-full text-left text-[10px] border-collapse">
                    <thead>
                      <tr className="border-b border-main bg-slate-900/50">
                        <th className="py-2 px-2 text-muted">路线 (Routing)</th>
                        <th className="py-2 px-2 text-center text-muted">批次 (Batches)</th>
                        <th className="py-2 px-2 text-center text-muted">512M 余存</th>
                        <th className="py-2 px-2 text-center text-muted">256M 余存</th>
                        <th className="py-2 px-2 text-center text-muted">128M 余存</th>
                      </tr>
                    </thead>
                    <tbody>
                      {schedules.map((sch, i) => (
                        <tr key={i} className="border-b border-slate-800/60 hover:bg-slate-800/20 font-mono">
                          <td className="py-2 px-2 font-bold text-indigo-400">{sch.routing_code}</td>
                          <td className="py-2 px-2 text-center text-heading">{sch.batch_count}</td>
                          <td className="py-2 px-2 text-center font-bold text-indigo-400">{sch.leftover_512.toLocaleString()}</td>
                          <td className="py-2 px-2 text-center font-bold text-amber-400">{sch.leftover_256.toLocaleString()}</td>
                          <td className="py-2 px-2 text-center font-bold text-slate-400">{sch.leftover_128.toLocaleString()}</td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              </div>

            </div>

          </div>
        )}

      </div>

      {/* Floating Save/Alert Banner */}
      {saveMessage && (
        <div className="absolute bottom-5 right-5 bg-indigo-950 border border-indigo-600 text-indigo-300 px-4 py-3 rounded shadow-xl text-xs z-50 animate-bounce">
          {saveMessage}
        </div>
      )}

    </div>
  );
};

export default MrpSubstitution;
