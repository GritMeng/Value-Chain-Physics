import React, { useState, useEffect } from 'react';
import { AgGridReact } from 'ag-grid-react';
import { useScenario } from '../../contexts/ScenarioContext';
import { 
  Search, 
  Save, 
  RotateCcw, 
  AlertTriangle, 
  Cpu, 
  RefreshCw, 
  SlidersHorizontal,
  BarChart3,
  TrendingUp,
  Activity
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
  Legend,
  ResponsiveContainer,
  ReferenceLine
} from 'recharts';

import 'ag-grid-community/styles/ag-grid.css';
import 'ag-grid-community/styles/ag-theme-balham.css';

const CustomChartTooltip = ({ active, payload }) => {
  if (active && payload && payload.length) {
    const data = payload[0].payload;
    const isOver = data.capacity > 100;
    return (
      <div className="bg-[#121620]/95 border border-[#1e2538] p-2 rounded shadow-2xl text-[9.5px] font-mono space-y-1">
        <p className="font-bold text-heading">{data.day} 计划状态</p>
        <p className={isOver ? 'text-rose-400 font-bold animate-pulse' : 'text-indigo-400 font-bold'}>
          设备负载: {data.capacity.toFixed(1)}% {isOver ? '(⚠️ 超载)' : ''}
        </p>
        <p className="text-emerald-400">
          计划生产: {Math.round(data.plan).toLocaleString()} 颗
        </p>
      </div>
    );
  }
  return null;
};

const MpsDashboard = () => {
  const { currentScenario, fetchKpis, runSimulation, setSelectedPartCode, setActiveTab } = useScenario();
  
  const [parts, setParts] = useState([]);
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedPart, setSelectedPart] = useState(null);
  
  const [rowData, setRowData] = useState([]);
  const [dirtyCells, setDirtyCells] = useState({}); // key: 'part_code-day_idx', value: number
  const [originalValues, setOriginalValues] = useState({}); // key: 'part_code-day_idx', value: number
  const [gridLoading, setGridLoading] = useState(false);
  const [isSubmitting, setIsSubmitting] = useState(false);

  // Scenario Compare states
  const [comparisonData, setComparisonData] = useState(null);
  const [compareLoading, setCompareLoading] = useState(false);

  // Spreading Tool states
  const [showSpreader, setShowSpreader] = useState(false);
  const [spreadQty, setSpreadQty] = useState(30000);
  const [spreadStart, setSpreadStart] = useState(5);
  const [spreadEnd, setSpreadEnd] = useState(15);
  const [spreadMethod, setSpreadMethod] = useState('evenly');

  // Capacity Leveling states
  const [showLeveler, setShowLeveler] = useState(false);
  const [levelingDir, setLevelingDir] = useState('backward'); // 'forward' or 'backward'
  const [levelingLimit, setLevelingLimit] = useState(100); // 100% capacity limit

  const fetchParts = async (q = '') => {
    try {
      const res = await fetch(`/api/mps/parts?q=${encodeURIComponent(q)}`);
      const data = await res.json();
      setParts(data);
      if (data.length > 0 && !selectedPart) {
        setSelectedPart(data[0]);
      }
    } catch (err) {
      console.error('Error fetching MPS parts:', err);
    }
  };

  const fetchComparisonData = async (partCode) => {
    if (!partCode) return;
    setCompareLoading(true);
    try {
      const res = await fetch(`/api/scenarios/timephased_compare?part_code=${encodeURIComponent(partCode)}`);
      const data = await res.json();
      if (data.status === 'success') {
        setComparisonData(data.data);
      }
    } catch (err) {
      console.error("Error fetching scenario compare data:", err);
    } finally {
      setCompareLoading(false);
    }
  };

  const transformCompareData = () => {
    if (!comparisonData) return [];
    const days = Array.from({ length: 30 }).map((_, d) => {
      const dayObj = { day: `D${d}` };
      Object.keys(comparisonData).forEach(sc => {
        dayObj[sc] = comparisonData[sc][d] !== undefined ? comparisonData[sc][d] : 0.0;
      });
      return dayObj;
    });
    return days;
  };

  const loadTimephasedGrid = async (partCode) => {
    setGridLoading(true);
    try {
      const res = await fetch(`/api/mps/timephased?part_code=${encodeURIComponent(partCode)}`);
      const data = await res.json();
      if (data.parts) {
        // Flatten the hierarchy to rows for ag-Grid
        const rows = [];
        data.parts.forEach(part => {
          const measures = [
            { key: 'gross', label: '毛需求 (Gross Demand)', type: 'Gross Demand' },
            { key: 'sr', label: '在途接收 (Scheduled Receipts)', type: 'Scheduled Receipts' },
            { key: 'on_hand', label: '预计可用库存 (Projected On-hand)', type: 'Projected On-Hand' },
            { key: 'plan', label: '计划订单 (Planned Orders)', type: 'Planned Orders' },
            { key: 'capacity', label: '设备负载 (Capacity Load)', type: 'Capacity Load' }
          ];

          measures.forEach(m => {
            const row = {
              part_code: part.part_code,
              level: part.level,
              part_type: part.part_type,
              on_hand: part.on_hand,
              llc: part.llc,
              measure: m.type,
              measure_label: m.label
            };

            // Inject 30 days values
            part.grid.forEach(day => {
              row[`D${day.day_idx}`] = day[m.key];
            });

            rows.push(row);
          });
        });
        setRowData(rows);
      }
    } catch (err) {
      console.error('Error loading timephased grid:', err);
    } finally {
      setGridLoading(false);
    }
  };

  // Load Finished Goods list on mount or when scenario changes
  useEffect(() => {
    fetchParts();
  }, [currentScenario]);

  // Re-load grid if selected part changes
  useEffect(() => {
    if (selectedPart) {
      loadTimephasedGrid(selectedPart.part_code);
      fetchComparisonData(selectedPart.part_code);
    } else {
      setRowData([]);
      setComparisonData(null);
    }
    setDirtyCells({});
    setOriginalValues({});
    setShowSpreader(false);
    setShowLeveler(false);
  }, [selectedPart, currentScenario]);

  const handleSearchChange = (e) => {
    setSearchQuery(e.target.value);
    fetchParts(e.target.value);
  };

  // Local grid calculation helper for concurrent planning look-ahead
  const recalculateLocalGrid = (updatedGrossDemands, currentRows, partCode) => {
    const grossRow = currentRows.find(r => r.part_code === partCode && r.measure === 'Gross Demand');
    const srRow = currentRows.find(r => r.part_code === partCode && r.measure === 'Scheduled Receipts');
    
    if (!grossRow) return currentRows;

    const newRows = currentRows.map(r => ({ ...r }));

    const newGross = newRows.find(r => r.part_code === partCode && r.measure === 'Gross Demand');
    const newOh = newRows.find(r => r.part_code === partCode && r.measure === 'Projected On-Hand');
    const newPlan = newRows.find(r => r.part_code === partCode && r.measure === 'Planned Orders');
    const newCap = newRows.find(r => r.part_code === partCode && r.measure === 'Capacity Load');

    // Pre-calculate dependencies if we can find initial on_hand
    const partInfo = parts.find(p => p.part_code === partCode);
    const initialOh = partInfo ? partInfo.on_hand : (selectedPart?.on_hand || 0);
    let prevOh = initialOh;

    // Pre-fetch global loads if any
    const base_load = 65.0;

    for (let d = 0; d < 30; d++) {
      const dayKey = `D${d}`;
      const gross_d = updatedGrossDemands[d] !== undefined ? updatedGrossDemands[d] : (parseFloat(grossRow[dayKey]) || 0);
      const sr_d = srRow ? parseFloat(srRow[dayKey]) || 0 : 0;

      const net_d = Math.max(0, gross_d - prevOh - sr_d);
      const plan_d = net_d;
      const on_hand_d = prevOh + sr_d + plan_d - gross_d;
      prevOh = on_hand_d;

      let capacity_d = base_load;
      if (plan_d > 0) {
        capacity_d = Math.max(capacity_d, 70.0 + (plan_d / 50.0));
      }

      if (newGross) newGross[dayKey] = gross_d;
      if (newOh) newOh[dayKey] = on_hand_d;
      if (newPlan) newPlan[dayKey] = plan_d;
      if (newCap) newCap[dayKey] = capacity_d;
    }

    return newRows;
  };

  // Capture edits
  const onCellValueChanged = (params) => {
    const field = params.colDef.field; // e.g., 'D5'
    const dayIdx = parseInt(field.replace('D', ''));
    if (!params.data) return;
    const partCode = params.data.part_code;
    const newValue = parseFloat(params.newValue) || 0;
    const oldValue = parseFloat(params.oldValue) || 0;

    const cellKey = `${partCode}-${dayIdx}`;

    // Record original value if not already recorded
    if (originalValues[cellKey] === undefined) {
      setOriginalValues(prev => ({ ...prev, [cellKey]: oldValue }));
    }

    // Set dirty cell value
    const newDirtyCells = { ...dirtyCells };
    if (newValue === originalValues[cellKey]) {
      delete newDirtyCells[cellKey];
    } else {
      newDirtyCells[cellKey] = newValue;
    }
    setDirtyCells(newDirtyCells);

    // Recalculate local grid dependencies instantly
    const targetRow = rowData.find(r => r.part_code === partCode && r.measure === 'Gross Demand');
    const updatedDemands = Array.from({ length: 30 }).map((_, d) => {
      const key = `${partCode}-${d}`;
      return newDirtyCells[key] !== undefined ? newDirtyCells[key] : (parseFloat(targetRow[`D${d}`]) || 0);
    });

    const nextRowData = recalculateLocalGrid(updatedDemands, rowData, partCode);
    setRowData(nextRowData);
  };

  // Spreading Application Logic
  const handleApplySpreading = () => {
    if (!selectedPart) return;
    if (spreadStart > spreadEnd) {
      alert('起始日期不能晚于结束日期！');
      return;
    }

    const partCode = selectedPart.part_code;
    const N = spreadEnd - spreadStart + 1;
    const Q = spreadQty;

    // Find the row for selectedPart and Gross Demand
    const targetRowIdx = rowData.findIndex(r => r.part_code === partCode && r.measure === 'Gross Demand');
    if (targetRowIdx === -1) {
      alert('未能在网格中找到对应的毛需求行！');
      return;
    }

    const targetRow = { ...rowData[targetRowIdx] };
    const newDirtyCells = { ...dirtyCells };
    const newOriginalValues = { ...originalValues };

    let values = [];
    if (spreadMethod === 'evenly') {
      const val = Q / N;
      values = Array.from({ length: N }).map(() => val);
    } else if (spreadMethod === 'proportional') {
      let existingSum = 0;
      for (let d = spreadStart; d <= spreadEnd; d++) {
        existingSum += parseFloat(targetRow[`D${d}`]) || 0;
      }
      if (existingSum === 0) {
        const val = Q / N;
        values = Array.from({ length: N }).map(() => val);
      } else {
        for (let d = spreadStart; d <= spreadEnd; d++) {
          const currentVal = parseFloat(targetRow[`D${d}`]) || 0;
          values.push(Q * (currentVal / existingSum));
        }
      }
    } else if (spreadMethod === 'front_loaded') {
      const alpha = 0.3;
      let weights = [];
      let sumW = 0;
      for (let i = 0; i < N; i++) {
        const w = Math.exp(-i * alpha);
        weights.push(w);
        sumW += w;
      }
      values = weights.map(w => Q * (w / sumW));
    } else if (spreadMethod === 'back_loaded') {
      const alpha = 0.3;
      let weights = [];
      let sumW = 0;
      for (let i = 0; i < N; i++) {
        const w = Math.exp(-(N - 1 - i) * alpha);
        weights.push(w);
        sumW += w;
      }
      values = weights.map(w => Q * (w / sumW));
    }

    // Apply updates to the cloned row and dirty stats
    const demands = Array.from({ length: 30 }).map((_, d) => {
      const cellKey = `${partCode}-${d}`;
      return newDirtyCells[cellKey] !== undefined ? newDirtyCells[cellKey] : (parseFloat(targetRow[`D${d}`]) || 0);
    });

    let idx = 0;
    for (let d = spreadStart; d <= spreadEnd; d++) {
      const newVal = Math.round(values[idx]);
      const oldVal = parseFloat(targetRow[`D${d}`]) || 0;
      const cellKey = `${partCode}-${d}`;

      if (newOriginalValues[cellKey] === undefined) {
        newOriginalValues[cellKey] = oldVal;
      }

      if (newVal === newOriginalValues[cellKey]) {
        delete newDirtyCells[cellKey];
      } else {
        newDirtyCells[cellKey] = newVal;
      }

      demands[d] = newVal;
      idx++;
    }

    // Recalculate local grid dependencies instantly
    const nextRowData = recalculateLocalGrid(demands, rowData, partCode);
    setRowData(nextRowData);
    setDirtyCells(newDirtyCells);
    setOriginalValues(newOriginalValues);
    setShowSpreader(false);
  };

  // Capacity Leveling algorithm (SCM constraint leveling)
  const handleApplyLeveling = () => {
    if (!selectedPart) return;
    
    const partCode = selectedPart.part_code;
    const limit = levelingLimit;
    const direction = levelingDir;

    // Find Gross Demand and Capacity Load rows
    const gdRowIdx = rowData.findIndex(r => r.part_code === partCode && r.measure === 'Gross Demand');
    const capRow = rowData.find(r => r.part_code === partCode && r.measure === 'Capacity Load');
    
    if (gdRowIdx === -1 || !capRow) {
      alert('未能在网格中找到对应的行数据！');
      return;
    }

    const gdRow = { ...rowData[gdRowIdx] };
    const newDirtyCells = { ...dirtyCells };
    const newOriginalValues = { ...originalValues };

    // Get current Gross Demands and Capacity Loads
    const demands = Array.from({ length: 30 }).map((_, d) => {
      const cellKey = `${partCode}-${d}`;
      return newDirtyCells[cellKey] !== undefined ? newDirtyCells[cellKey] : (parseFloat(gdRow[`D${d}`]) || 0);
    });
    const capacities = Array.from({ length: 30 }).map((_, d) => parseFloat(capRow[`D${d}`]) || 0);

    let changed = false;
    const base_load = 65.0;
    const getCapacityLimitInUnits = (l) => Math.max(0, (l - 70.0) * 50.0);

    for (let d = 0; d < 30; d++) {
      const capVal = capacities[d];
      if (capVal > limit) {
        let plan_d = (capVal - 70.0) * 50.0;
        let limitUnits = getCapacityLimitInUnits(limit);
        let excessQty = Math.round(plan_d - limitUnits);
        
        let amountToShift = Math.min(excessQty, demands[d]);
        if (amountToShift <= 0) continue;

        // Search adjacent candidate days
        let step = direction === 'forward' ? -1 : 1;
        let c = d + step;

        while (c >= 0 && c < 30 && amountToShift > 0) {
          let cap_c = capacities[c];
          if (cap_c < limit) {
            let plan_c = (cap_c > 70.0) ? (cap_c - 70.0) * 50.0 : 0.0;
            let limitUnits_c = getCapacityLimitInUnits(limit);
            let spareUnits = Math.round(limitUnits_c - plan_c);

            if (spareUnits > 0) {
              let shifted = Math.min(amountToShift, spareUnits);
              
              demands[d] -= shifted;
              demands[c] += shifted;
              
              amountToShift -= shifted;
              changed = true;

              // Local re-estimate for the candidate loop
              capacities[d] = Math.max(base_load, 70.0 + (demands[d] / 50.0));
              capacities[c] = Math.max(base_load, 70.0 + (demands[c] / 50.0));
            }
          }
          c += step;
        }
      }
    }

    if (!changed) {
      alert('没有检测到超载的产能，或者邻近天已无剩余可用产能进行平摊！');
      setShowLeveler(false);
      return;
    }

    // Apply back to dirtyCells
    for (let d = 0; d < 30; d++) {
      const cellKey = `${partCode}-${d}`;
      const oldVal = parseFloat(gdRow[`D${d}`]) || 0;
      const newVal = demands[d];

      if (newVal !== oldVal) {
        if (newOriginalValues[cellKey] === undefined) {
          newOriginalValues[cellKey] = oldVal;
        }

        if (newVal === newOriginalValues[cellKey]) {
          delete newDirtyCells[cellKey];
        } else {
          newDirtyCells[cellKey] = newVal;
        }
      }
    }

    // Recalculate full grid dependencies
    const nextRowData = recalculateLocalGrid(demands, rowData, partCode);
    setRowData(nextRowData);
    setDirtyCells(newDirtyCells);
    setOriginalValues(newOriginalValues);
    setShowLeveler(false);

    // Show bubble notification
    const alertDiv = document.createElement('div');
    alertDiv.className = "fixed bottom-5 right-5 bg-indigo-950 border border-indigo-600 text-indigo-300 px-4 py-3 rounded shadow-xl text-xs z-50 animate-bounce";
    alertDiv.innerHTML = `✔ <b>[产能削峰平滑完成]</b><br/> 毛需求负载已成功按期望平移至空闲日，请保存并提交排产。`;
    document.body.appendChild(alertDiv);
    setTimeout(() => alertDiv.remove(), 4000);
  };

  const handleDiscardChanges = () => {
    if (Object.keys(dirtyCells).length === 0) return;
    if (window.confirm('确定要放弃所有的草稿改动并刷新网格吗？')) {
      if (selectedPart) {
        loadTimephasedGrid(selectedPart.part_code);
      }
      setDirtyCells({});
      setOriginalValues({});
      setShowSpreader(false);
      setShowLeveler(false);
    }
  };

  const handleSubmitChanges = async () => {
    const keys = Object.keys(dirtyCells);
    if (keys.length === 0) return;

    setIsSubmitting(true);
    try {
      // Loop edits and post to /api/mps/update
      for (const key of keys) {
        const [partCode, dayStr] = key.split('-');
        const day = parseInt(dayStr);
        const qty = dirtyCells[key];

        await fetch('/api/mps/update', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            part_code: partCode,
            day: day,
            qty: qty
          })
        });
      }

      // Trigger Simulation re-calculation on active DB
      await runSimulation();
      
      // Reload current grid data and clear local dirty states
      if (selectedPart) {
        await loadTimephasedGrid(selectedPart.part_code);
      }
      setDirtyCells({});
      setOriginalValues({});
      await fetchKpis();
    } catch (err) {
      console.error('Error submitting MPS updates:', err);
      alert('排产优化重算发生异常，请重试！');
    } finally {
      setIsSubmitting(false);
    }
  };

  const onCellDoubleClicked = (event) => {
    if (event.colDef.field === 'part_code' && event.value) {
      const partCode = event.value;
      const partType = event.data?.part_type;
      if (partType === 'FINISHED') {
        const matched = parts.find(p => p.part_code === partCode);
        if (matched) setSelectedPart(matched);
      } else {
        setSelectedPartCode(partCode);
        setActiveTab('mrp');
      }
    }
  };

  // ag-Grid Column Definition
  const columnDefs = [
    {
      headerName: '零件代号 (Part Code)',
      field: 'part_code',
      pinned: 'left',
      width: 175,
      cellRenderer: (params) => {
        if (!params.data) return null;
        if (params.data.measure === 'Gross Demand') {
          const indent = params.data.level * 12;
          return (
            <div style={{ paddingLeft: `${indent}px` }} className="flex items-center font-bold text-heading">
              <span className="mr-1.5 text-indigo-500 font-normal">├─</span>
              <span title={params.value}>{params.value}</span>
            </div>
          );
        }
        return <span className="text-muted/30 pl-2">│</span>;
      }
    },
    {
      headerName: '计划测度 (Measure)',
      field: 'measure_label',
      pinned: 'left',
      width: 155,
      cellStyle: (params) => {
        if (!params.data) return null;
        if (params.data.measure === 'Gross Demand') return { color: 'var(--color-primary-hover)' };
        if (params.data.measure === 'Projected On-Hand') return { color: 'var(--color-success)' };
        if (params.data.measure === 'Capacity Load') {
          return { color: 'var(--color-purple)' };
        }
        return null;
      }
    },
    // Generate 30 planning columns (D0 to D29)
    ...Array.from({ length: 30 }).map((_, i) => ({
      headerName: `D${i}`,
      field: `D${i}`,
      width: 62,
      type: 'numericColumn',
      editable: (params) => params.data && params.data.measure === 'Gross Demand',
      cellClassRules: {
        'grid-cell-dirty': (params) => {
          if (!params.data) return false;
          const cellKey = `${params.data.part_code}-${i}`;
          return dirtyCells[cellKey] !== undefined;
        }
      },
      cellRenderer: (params) => {
        if (!params.data) return null;
        const partCode = params.data.part_code;
        const dayIdx = i;
        const isDirty = dirtyCells[`${partCode}-${dayIdx}`] !== undefined;
        const val = params.value;
        const isOverloaded = params.data.measure === 'Capacity Load' && val > 100;
        
        return (
          <div className="cell-container w-full h-full relative flex items-center justify-center">
            {isDirty && (
              <div 
                className={currentScenario === 'baseline' ? "dirty-marker" : "dirty-marker-sandbox"} 
                title="已做草稿修改，尚未提交重算"
              />
            )}
            <span className={`cyber-font text-[10.5px] ${isOverloaded ? 'text-rose-500 font-bold animate-pulse' : ''}`}>
              {val !== null && val !== undefined ? Math.round(val).toLocaleString() : '-'}
            </span>
          </div>
        );
      },
      valueParser: (params) => {
        const parsed = parseFloat(params.newValue);
        return isNaN(parsed) ? 0 : parsed;
      }
    }))
  ];

  const defaultColDef = {
    sortable: false,
    resizable: true,
    filter: false,
    suppressMovable: true,
  };

  const dirtyCount = Object.keys(dirtyCells).length;

  // Build capacity trend chart data dynamically
  const getChartData = () => {
    if (!selectedPart) return [];
    const capRow = rowData.find(r => r.part_code === selectedPart.part_code && r.measure === 'Capacity Load');
    const planRow = rowData.find(r => r.part_code === selectedPart.part_code && r.measure === 'Planned Orders');

    return Array.from({ length: 30 }).map((_, d) => {
      const dayKey = `D${d}`;
      return {
        day: `D${d}`,
        dayIdx: d,
        capacity: capRow ? parseFloat(capRow[dayKey]) || 0 : 0,
        plan: planRow ? parseFloat(planRow[dayKey]) || 0 : 0
      };
    });
  };

  const chartData = getChartData();

  return (
    <div className="flex flex-row space-x-3 h-full overflow-hidden">
      
      {/* 左侧成品零件检索栏 */}
      <div className="w-60 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden">
        <div className="border-b border-main pb-2 mb-2 flex items-center justify-between">
          <h4 className="font-bold text-heading text-xs flex items-center">
            <Cpu className="w-4 h-4 text-indigo-500 mr-1.5" />
            成品物料库 (FG Parts)
          </h4>
          <span className="text-[9px] text-muted cyber-font">{parts.length} SKU</span>
        </div>

        <div className="relative mb-3">
          <input
            type="text"
            value={searchQuery}
            onChange={handleSearchChange}
            placeholder="搜索成品零件号..."
            className="w-full bg-input border border-main text-heading text-xs pl-8 pr-3 py-1.5 rounded focus:outline-none focus:border-indigo-500"
          />
          <Search className="w-3.5 h-3.5 text-muted absolute left-2.5 top-2.5" />
        </div>

        <div className="flex-1 overflow-y-auto space-y-1.5 pr-1">
          {parts.map(p => (
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
                OH: {p.on_hand}
              </span>
            </button>
          ))}
          {parts.length === 0 && (
            <div className="text-center py-10 text-muted text-xs">无匹配的成品</div>
          )}
        </div>
      </div>

      {/* 右侧 ag-Grid 时序排产与趋势图谱 */}
      <div className="flex-1 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden">
        
        {/* 工具栏 */}
        <div className="flex justify-between items-center border-b border-main pb-2.5 mb-2.5">
          <div className="flex items-center space-x-2">
            <h3 className="text-xs font-bold text-heading flex items-center">
              时序平衡排产网格 (Time-Phased Production Grid)
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
                <span>草稿区有 {dirtyCount} 项待保存的修改</span>
              </div>
            )}
            
            {/* Spreader Button */}
            {selectedPart && (
              <button
                onClick={() => {
                  setShowSpreader(!showSpreader);
                  setShowLeveler(false);
                }}
                className={`py-1 px-3 text-[10.5px] flex items-center space-x-1 border rounded transition-all cursor-pointer ${
                  showSpreader 
                    ? 'bg-indigo-600 border-indigo-500 text-white font-bold' 
                    : 'btn-premium-secondary'
                }`}
              >
                <SlidersHorizontal className="w-3.5 h-3.5" />
                <span>需求批量分摊 (Spreader)</span>
              </button>
            )}

            {/* Capacity Leveler Button */}
            {selectedPart && (
              <button
                onClick={() => {
                  setShowLeveler(!showLeveler);
                  setShowSpreader(false);
                }}
                className={`py-1 px-3 text-[10.5px] flex items-center space-x-1 border rounded transition-all cursor-pointer ${
                  showLeveler 
                    ? 'bg-indigo-600 border-indigo-500 text-white font-bold' 
                    : 'btn-premium-secondary'
                }`}
              >
                <Activity className="w-3.5 h-3.5 text-purple-400" />
                <span>产能削峰平滑 (Leveling)</span>
              </button>
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
              className="btn-premium-primary py-1 px-3 text-[10.5px] flex items-center space-x-1 disabled:opacity-50"
            >
              <Save className="w-3.5 h-3.5" />
              <span>{isSubmitting ? '正在重算...' : '保存并重排产'}</span>
            </button>
          </div>
        </div>

        {/* Spreader Panel (Kinaxis style Gross Demand Disaggregation) */}
        {showSpreader && selectedPart && (
          <div className="bg-table-header border border-main p-3 rounded-xs mb-3 flex flex-row items-end space-x-4 select-none animate-fade-in">
            <div className="flex-1 space-y-1">
              <label className="text-[9.5px] text-muted font-bold block">总分摊量 (Total Quantity)</label>
              <input
                type="number"
                value={spreadQty}
                onChange={(e) => setSpreadQty(parseInt(e.target.value) || 0)}
                className="w-full bg-input border border-main text-heading text-xs px-2 py-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono"
                min="0"
              />
            </div>
            
            <div className="flex space-x-2">
              <div className="w-20 space-y-1">
                <label className="text-[9.5px] text-muted font-bold block">起始天数</label>
                <select
                  value={spreadStart}
                  onChange={(e) => setSpreadStart(parseInt(e.target.value))}
                  className="w-full bg-input border border-main text-heading text-xs px-1 py-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono cursor-pointer"
                >
                  {Array.from({ length: 30 }).map((_, i) => (
                    <option key={i} value={i}>D{i}</option>
                  ))}
                </select>
              </div>
              
              <div className="w-20 space-y-1">
                <label className="text-[9.5px] text-muted font-bold block">结束天数</label>
                <select
                  value={spreadEnd}
                  onChange={(e) => setSpreadEnd(parseInt(e.target.value))}
                  className="w-full bg-input border border-main text-heading text-xs px-1 py-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono cursor-pointer"
                >
                  {Array.from({ length: 30 }).map((_, i) => (
                    <option key={i} value={i}>D{i}</option>
                  ))}
                </select>
              </div>
            </div>

            <div className="flex-1 space-y-1">
              <label className="text-[9.5px] text-muted font-bold block">分摊算法 (Method)</label>
              <select
                value={spreadMethod}
                onChange={(e) => setSpreadMethod(e.target.value)}
                className="w-full bg-input border border-main text-heading text-xs px-2 py-1.5 rounded focus:outline-none focus:border-indigo-500 cursor-pointer"
              >
                <option value="evenly">均等分摊 (Evenly)</option>
                <option value="proportional">按历史比例分摊 (Proportionally)</option>
                <option value="front_loaded">前倾分摊 (Front-Loaded)</option>
                <option value="back_loaded">后倾分摊 (Back-Loaded)</option>
              </select>
            </div>

            <div className="flex space-x-1.5">
              <button
                onClick={handleApplySpreading}
                className="btn-premium-primary py-1.5 px-3.5 text-[10.5px] font-bold cursor-pointer"
              >
                分摊应用
              </button>
              <button
                onClick={() => setShowSpreader(false)}
                className="btn-premium-secondary py-1.5 px-2 text-[10.5px] cursor-pointer"
              >
                取消
              </button>
            </div>
          </div>
        )}

        {/* Capacity Leveler Panel (Constraint Smoothing) */}
        {showLeveler && selectedPart && (
          <div className="bg-table-header border border-[#8b5cf6]/40 p-3 rounded-xs mb-3 flex flex-row items-end space-x-4 select-none animate-fade-in">
            <div className="flex-1 space-y-1">
              <label className="text-[9.5px] text-muted font-bold block">设备最大负荷阈值 (%)</label>
              <input
                type="number"
                value={levelingLimit}
                onChange={(e) => setLevelingLimit(parseInt(e.target.value) || 100)}
                className="w-full bg-input border border-main text-heading text-xs px-2 py-1.5 rounded focus:outline-none focus:border-indigo-500 font-mono"
                min="70"
                max="150"
              />
            </div>
            
            <div className="flex-1 space-y-1">
              <label className="text-[9.5px] text-muted font-bold block">平滑削峰转移方向 (Direction)</label>
              <select
                value={levelingDir}
                onChange={(e) => setLevelingDir(e.target.value)}
                className="w-full bg-input border border-main text-heading text-xs px-2 py-1.5 rounded focus:outline-none focus:border-indigo-500 cursor-pointer font-bold"
              >
                <option value="backward">👉 向后平滑 (Shift Overload to Future Days)</option>
                <option value="forward">👈 向前平滑 (Shift Overload to Past Days)</option>
              </select>
            </div>

            <div className="flex space-x-1.5">
              <button
                onClick={handleApplyLeveling}
                className="btn-premium-primary py-1.5 px-4 bg-purple-700 hover:bg-purple-600 text-white text-[10.5px] font-bold cursor-pointer flex items-center space-x-1"
              >
                <Activity className="w-3.5 h-3.5" />
                <span>执行产能平滑</span>
              </button>
              <button
                onClick={() => setShowLeveler(false)}
                className="btn-premium-secondary py-1.5 px-2.5 text-[10.5px] cursor-pointer"
              >
                取消
              </button>
            </div>
          </div>
        )}

        {/* Grid and Chart split container */}
        <div className="flex-1 flex flex-col min-h-0 space-y-3">
          {/* ag-Grid 容器 */}
          <div className="flex-[3] w-full min-h-0 relative border border-main/50 rounded overflow-hidden">
            {gridLoading && (
              <div className="absolute inset-0 bg-card/85 backdrop-blur-xs flex items-center justify-center z-20 text-xs text-indigo-400">
                <RefreshCw className="w-5 h-5 animate-spin mr-2" />
                正在加载 BOM 级联网格...
              </div>
            )}

            <div className="ag-theme-balham-dark w-full h-full">
              <AgGridReact
                theme="legacy"
                rowData={rowData}
                columnDefs={columnDefs}
                defaultColDef={defaultColDef}
                onCellValueChanged={onCellValueChanged}
                onCellDoubleClicked={onCellDoubleClicked}
                getRowId={(params) => params.data ? `${params.data.part_code}-${params.data.measure}` : ''}
                headerHeight={26}
                rowHeight={22}
              />
            </div>
          </div>

          {/* Bottom charts grid */}
          {selectedPart && chartData.length > 0 && (
            <div className="flex-[2] min-h-[180px] grid grid-cols-1 lg:grid-cols-2 gap-3">
              
              {/* Capacity Load Trend Chart */}
              <div className="bg-slate-950/25 border border-main rounded-xs p-3 flex flex-col justify-between select-none relative">
                <div className="absolute top-2 right-3 flex items-center space-x-2 text-[9px] text-muted">
                  <span className="flex items-center"><span className="w-2 h-2 bg-indigo-500 rounded-full mr-1"></span>设备负载率 (%)</span>
                  <span className="flex items-center"><span className="w-2 h-px border-t border-rose-500 border-dashed mr-1"></span>100% 警戒线</span>
                </div>
                
                <h4 className="text-[10px] font-bold text-heading flex items-center mb-1">
                  <TrendingUp className="w-3.5 h-3.5 text-purple-400 mr-1.5" />
                  设备产能负载平稳度分析 (Capacity Load Horizon Simulation Profile)
                </h4>

                <div className="flex-1 w-full min-h-0 mt-1">
                  <ResponsiveContainer width="100%" height="100%">
                    <AreaChart data={chartData} margin={{ top: 5, right: 5, left: -25, bottom: -5 }}>
                      <defs>
                        <linearGradient id="colorCap" x1="0" y1="0" x2="0" y2="1">
                          <stop offset="5%" stopColor="#8b5cf6" stopOpacity="0.35"/>
                          <stop offset="95%" stopColor="#8b5cf6" stopOpacity={0.0}/>
                        </linearGradient>
                      </defs>
                      <CartesianGrid strokeDasharray="3 3" stroke="#1e2538" />
                      <XAxis dataKey="day" stroke="#475569" fontSize={9} tickLine={false} />
                      <YAxis stroke="#475569" fontSize={9} domain={[0, Math.max(120, ...chartData.map(d => d.capacity + 15))]} tickLine={false} />
                      <Tooltip content={<CustomChartTooltip />} />
                      <ReferenceLine y={100} stroke="#ef4444" strokeWidth={1.2} strokeDasharray="4 2" />
                      <Area type="monotone" dataKey="capacity" stroke="#8b5cf6" strokeWidth={1.5} fillOpacity={1} fill="url(#colorCap)" />
                    </AreaChart>
                  </ResponsiveContainer>
                </div>
              </div>

              {/* Sandbox Stock Overlay Chart */}
              <div className="bg-slate-950/25 border border-main rounded-xs p-3 flex flex-col justify-between select-none relative">
                <h4 className="text-[10px] font-bold text-heading flex items-center mb-1">
                  <TrendingUp className="w-3.5 h-3.5 text-emerald-400 mr-1.5" />
                  多沙箱预计可用库存重叠对比 (Projected Stock Sandbox Overlays)
                </h4>
                
                {compareLoading ? (
                  <div className="flex-1 flex items-center justify-center text-[10px] text-muted">
                    <RefreshCw className="w-4 h-4 animate-spin mr-1.5 text-emerald-400" />
                    正在载入对比曲线...
                  </div>
                ) : (
                  <div className="flex-1 w-full min-h-0 mt-1">
                    <ResponsiveContainer width="100%" height="100%">
                      <LineChart data={transformCompareData()} margin={{ top: 5, right: 5, left: -25, bottom: -5 }}>
                        <CartesianGrid strokeDasharray="3 3" stroke="#1e2538" />
                        <XAxis dataKey="day" stroke="#475569" fontSize={9} tickLine={false} />
                        <YAxis stroke="#475569" fontSize={9} tickLine={false} />
                        <Tooltip 
                          contentStyle={{ backgroundColor: '#0b0f19', border: '1px solid #1e2538', borderRadius: '4px' }}
                          labelStyle={{ color: '#94a3b8', fontSize: '9px', fontWeight: 'bold' }}
                          itemStyle={{ fontSize: '9px' }}
                        />
                        <Legend verticalAlign="top" height={20} iconSize={10} style={{ fontSize: '9px' }} />
                        {comparisonData && Object.keys(comparisonData).map((sc, idx) => {
                          const colors = ['#10b981', '#3b82f6', '#f59e0b', '#ec4899', '#8b5cf6'];
                          const color = colors[idx % colors.length];
                          return (
                            <Line 
                              key={sc}
                              type="monotone"
                              dataKey={sc}
                              name={sc === 'baseline' ? '🟢 生产主计划' : `🟡 沙箱: ${sc}`}
                              stroke={color}
                              strokeWidth={sc === currentScenario ? 2 : 1.2}
                              dot={false}
                            />
                          );
                        })}
                      </LineChart>
                    </ResponsiveContainer>
                  </div>
                )}
              </div>

            </div>
          )}
        </div>

        {/* 注解说明 */}
        <div className="mt-2.5 text-[9px] text-muted flex justify-between select-none">
          <span>💡 提示：双击带有 <span className="text-indigo-400 font-bold">毛需求 (Gross Demand)</span> 的单元格或点击 Spreader 按钮可进行计划批量微调分摊。</span>
          <span>系统基于 DuckDB 物理表直接映射，D8 为产线共试超载核算点（100%+ 负载红色警报）。</span>
        </div>

      </div>

    </div>
  );
};

export default MpsDashboard;
