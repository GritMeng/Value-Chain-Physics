import { useState, useEffect } from 'react';
import { ScenarioProvider, useScenario } from './contexts/ScenarioContext';
import AiCopilot from './components/AiCopilot';
import { 
  TowerControl, 
  SunMoon, 
  GitBranch, 
  Plus, 
  RefreshCw, 
  ShoppingCart, 
  Projector, 
  TrendingUp, 
  Warehouse, 
  Trash2, 
  GitMerge,
  Search,
  Bot,
  SlidersHorizontal,
  ChevronRight,
  ChevronLeft,
  X,
  Play,
  Split,
  PlusCircle,
  Database,
  AlertTriangle,
  CheckCircle2,
  Cpu,
  ShieldCheck,
  Truck,
  Layers,
  Factory,
  Landmark,
  Info
} from 'lucide-react';

import { AgGridReact } from 'ag-grid-react';
import 'ag-grid-community/styles/ag-grid.css';
import 'ag-grid-community/styles/ag-theme-balham.css';

// Import Workbook Worksheets Views
import ControlTower from './views/ControlTower/ControlTower';
import MpsDashboard from './views/MPS/MpsDashboard';
import MrpSubstitution from './views/MRP/MrpSubstitution';
import DemandSupplyPegging from './views/Pegging/DemandSupplyPegging';
import SupplierCollab from './views/Collab/SupplierCollab';
import InventoryOptimization from './views/IO/InventoryOptimization';
import IbpFinance from './views/IBP/IbpFinance';
import AllotmentWorkbench from './views/Allotment/AllotmentWorkbench';
import ExecutionDispatch from './views/Execution/ExecutionDispatch';
import ProjectEto from './views/ETO/ProjectEto';
import CapacityWorkbench from './views/Capacity/CapacityWorkbench';

// Helper to calculate day offset from base date 2026-05-29
const getDayOffset = (dateStr) => {
  if (!dateStr) return 0;
  const baseDate = new Date('2026-05-29');
  const targetDate = new Date(dateStr);
  const diffTime = targetDate - baseDate;
  const diffDays = Math.round(diffTime / (1000 * 60 * 60 * 24));
  return diffDays;
};

// Formats number to currency
const formatCurrency = (val) => {
  if (val === undefined || val === null) return '¥0';
  return '¥' + Math.round(val).toLocaleString();
};

const DashboardContent = () => {
  const {
    currentScenario,
    scenarios,
    kpis,
    isAutopilot,
    isLoading,
    switchScenario,
    createScenario,
    deleteScenario,
    mergeScenario,
    runSimulation,
    activeTab,
    setActiveTab,
    setSelectedPartCode,
    setSelectedDemandId
  } = useScenario();

  // Local state
  const [isLightMode, setIsLightMode] = useState(false);
  const [bounceAlert, setBounceAlert] = useState(null);
  const [refreshTrigger, setRefreshTrigger] = useState(0);

  // Active result explorer sub-tab for ledger worksheet
  const [ledgerSubTab, setLedgerSubTab] = useState('planned_orders');

  // Workbook sidebar collapse states
  const [isSidebarCollapsed, setIsSidebarCollapsed] = useState(false);
  const [sidebarSearch, setSidebarSearch] = useState('');

  // Search filter for result explorer table
  const [gridSearch, setGridSearch] = useState('');

  // Demands list in the left panel
  const [demands, setDemands] = useState([]);
  const [demandsLoading, setDemandsLoading] = useState(false);

  // Auto-populated part codes for demand insertion form
  const [partCodes, setPartCodes] = useState([]);

  // Form state for demand insertion
  const [newDemand, setNewDemand] = useState({
    part_code: '',
    qty: '',
    day: '',
    customer: '',
    priority: 1
  });

  // Split Demand Modal states
  const [isSplitModalOpen, setIsSplitModalOpen] = useState(false);
  const [selectedDemandForSplit, setSelectedDemandForSplit] = useState(null);
  const [splitQty, setSplitQty] = useState('');
  const [splitDay, setSplitDay] = useState('');

  // Branch differences & conflicts states
  const [pendingChanges, setPendingChanges] = useState({ pending_commits: [], pending_updates: [], conflicts: [] });
  const [diffLoading, setDiffLoading] = useState(false);

  // AI Copilot collapsible state
  const [isAiCopilotOpen, setIsAiCopilotOpen] = useState(false);

  // Data rows for active results grid
  const [gridData, setGridData] = useState([]);
  const [gridLoading, setGridLoading] = useState(false);

  const tabTableMap = {
    planned_orders: 'ipc_planned_order_ledger',
    alternates: 'ipc_alternate_allocation',
    wbs: 'ipc_project_wbs',
    supply_assignments: 'ipc_planned_supply_assignment'
  };

  // Fetch grids data for right tabbed panel
  const fetchGridData = async () => {
    setGridLoading(true);
    try {
      const res = await fetch(`/api/table?name=${tabTableMap[ledgerSubTab]}`);
      if (res.ok) {
        const data = await res.json();
        setGridData(data);
      } else {
        setGridData([]);
      }
    } catch (err) {
      console.error('Error fetching grid data:', err);
      setGridData([]);
    } finally {
      setGridLoading(false);
    }
  };

  // Fetch demands for the left panel
  const fetchDemands = async () => {
    setDemandsLoading(true);
    try {
      const res = await fetch('/api/table?name=ipc_independent_demand');
      if (res.ok) {
        const data = await res.json();
        setDemands(data);
      } else {
        setDemands([]);
      }
    } catch (err) {
      console.error('Error fetching demands:', err);
      setDemands([]);
    } finally {
      setDemandsLoading(false);
    }
  };

  // Fetch list of distinct part codes
  const fetchPartCodes = async () => {
    try {
      const res = await fetch('/api/parts/all');
      if (res.ok) {
        const data = await res.json();
        setPartCodes(data);
      }
    } catch (err) {
      console.error('Error fetching parts list:', err);
    }
  };

  // Fetch branch differences / conflicts
  const fetchPendingChanges = async () => {
    if (currentScenario === 'baseline') {
      setPendingChanges({ pending_commits: [], pending_updates: [], conflicts: [] });
      return;
    }
    setDiffLoading(true);
    try {
      const res = await fetch(`/api/scenarios/pending_changes?scenario_code=${currentScenario}`);
      if (res.ok) {
        const data = await res.json();
        setPendingChanges(data);
      }
    } catch (err) {
      console.error('Error fetching pending changes:', err);
    } finally {
      setDiffLoading(false);
    }
  };

  // Triggers fetches on active tab/scenario/trigger changes
  useEffect(() => {
    if (activeTab === 'planned_orders') {
      fetchGridData();
    }
  }, [ledgerSubTab, activeTab, currentScenario, refreshTrigger]);

  useEffect(() => {
    fetchDemands();
    fetchPendingChanges();
  }, [currentScenario, refreshTrigger]);

  useEffect(() => {
    fetchPartCodes();
  }, []);

  // Theme effect
  useEffect(() => {
    if (isLightMode) {
      document.body.classList.add('theme-light');
    } else {
      document.body.classList.remove('theme-light');
    }
  }, [isLightMode]);

  // WebSocket for real-time notifications
  useEffect(() => {
    let wsUrl = `ws://${window.location.host}/api/ws/alerts`;
    if (window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1') {
      wsUrl = `ws://127.0.0.1:8501/api/ws/alerts`;
    }
    
    let ws;
    let reconnectTimeout;
    
    const connectWs = () => {
      ws = new WebSocket(wsUrl);
      ws.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          if (data.type === 'ALERT') {
            setBounceAlert(`🔔 [实时警报] ${data.message}`);
            setTimeout(() => setBounceAlert(null), 6000);
          } else if (data.type === 'SYSTEM') {
            setBounceAlert(`⚙ [系统通知] ${data.message}`);
            setTimeout(() => setBounceAlert(null), 4000);
          }
        } catch (e) {
          console.error(e);
        }
      };
      
      ws.onclose = () => {
        reconnectTimeout = setTimeout(connectWs, 5000);
      };
    };
    
    connectWs();
    return () => {
      if (ws) ws.close();
      if (reconnectTimeout) clearTimeout(reconnectTimeout);
    };
  }, []);

  // Sandbox creation
  const handleCreateSandbox = () => {
    const code = prompt('请输入新沙箱代码 (字母、数字、下划线):');
    if (!code) return;
    const cleanCode = code.trim();
    if (!/^[a-zA-Z0-9_]+$/.test(cleanCode)) {
      alert('沙箱代码无效，仅支持字母、数字及下划线！');
      return;
    }
    const name = prompt('请输入新沙箱名称:', `沙箱推演 - ${cleanCode}`);
    if (!name) return;
    createScenario(cleanCode, name.trim());
  };

  // Run solver (Simulation)
  const handleSimulate = async () => {
    const data = await runSimulation();
    if (data.status === 'success' || data.status === 'mocked') {
      setRefreshTrigger(prev => prev + 1);
      setBounceAlert(`✔ [重算成功] 引擎求解循环已完成！`);
      setTimeout(() => setBounceAlert(null), 3000);
    } else {
      alert(`重算失败: ${data.message}`);
    }
  };

  // Insert Demand form submit
  const handleInsertDemand = async (e) => {
    e.preventDefault();
    if (!newDemand.part_code || !newDemand.qty || !newDemand.day) {
      alert('请填写完整零件代号、数量和交付天数！');
      return;
    }
    try {
      const res = await fetch('/api/demands/insert', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          part_code: newDemand.part_code,
          qty: parseFloat(newDemand.qty),
          day: parseInt(newDemand.day),
          customer: newDemand.customer || 'GENERIC_CUST',
          priority: parseInt(newDemand.priority)
        })
      });
      const data = await res.json();
      if (data.status === 'success' || data.status === 'ok') {
        setNewDemand({ part_code: '', qty: '', day: '', customer: '', priority: 1 });
        setRefreshTrigger(prev => prev + 1);
        setBounceAlert('✔ 独立需求插入成功，计划已重算！');
        setTimeout(() => setBounceAlert(null), 3000);
      } else {
        alert('插入需求失败: ' + data.message);
      }
    } catch (err) {
      console.error('Error inserting demand:', err);
    }
  };

  // Trigger Split modal opening
  const triggerSplitModal = (demandRow) => {
    const day = getDayOffset(demandRow.request_due_date);
    setSelectedDemandForSplit(demandRow);
    setSplitQty(Math.round(demandRow.request_qty / 2));
    setSplitDay(day + 3); // Default split date shifted by 3 days
    setIsSplitModalOpen(true);
  };

  // Submit split demand API
  const handleSplitDemand = async () => {
    if (splitQty <= 0 || splitQty >= selectedDemandForSplit.request_qty) {
      alert('拆分数量必须大于0且小于原始需求数量！');
      return;
    }
    try {
      const res = await fetch('/api/demands/split', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          demand_id: selectedDemandForSplit.demand,
          split_qty: parseFloat(splitQty),
          new_day: parseInt(splitDay)
        })
      });
      const data = await res.json();
      if (data.status === 'success') {
        setIsSplitModalOpen(false);
        setSelectedDemandForSplit(null);
        setRefreshTrigger(prev => prev + 1);
        setBounceAlert('✔ 需求成功拆分，计划已重算！');
        setTimeout(() => setBounceAlert(null), 3000);
      } else {
        alert('拆分失败: ' + data.message);
      }
    } catch (err) {
      console.error('Error splitting demand:', err);
    }
  };

  // Resolve conflict API
  const handleResolveConflict = async (table, key, resolution) => {
    try {
      const res = await fetch('/api/scenarios/resolve_conflict', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          scenario_code: currentScenario,
          table,
          key,
          resolution
        })
      });
      const data = await res.json();
      if (data.status === 'success') {
        setRefreshTrigger(prev => prev + 1);
        setBounceAlert('✔ 冲突已成功解决！');
        setTimeout(() => setBounceAlert(null), 3000);
      } else {
        alert('解决冲突失败: ' + data.message);
      }
    } catch (err) {
      console.error('Error resolving conflict:', err);
    }
  };

  // Handle ag-grid cell changes (Inline Editing)
  const onCellValueChanged = async (event) => {
    const { data, colDef, newValue } = event;
    if (colDef.field === 'wbs_status') {
      try {
        const res = await fetch('/api/eto/wbs/update', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            wbs_code: data.wbs_code,
            status: newValue
          })
        });
        const resData = await res.json();
        if (resData.status === 'success') {
          setRefreshTrigger(prev => prev + 1);
          setBounceAlert(`✔ WBS 任务 ${data.wbs_code} 进度已调整为 ${newValue}，已自动重新解算关键路径！`);
          setTimeout(() => setBounceAlert(null), 3000);
        } else {
          alert('更新进度失败: ' + resData.message);
        }
      } catch (err) {
        console.error('Error updating WBS status:', err);
      }
    }
  };

  // KPI Diffs Helper Renderer
  const renderDelta = (current, baseline, format = 'number', invertSuccess = false) => {
    if (!baseline) return null;
    const diff = current - baseline;
    if (diff === 0) return null;
    const isPositive = diff > 0;
    const isSuccess = invertSuccess ? !isPositive : isPositive;
    const formatStr = format === 'currency'
      ? (isPositive ? '+' : '') + '¥' + Math.abs(diff).toLocaleString()
      : (isPositive ? '+' : '-') + Math.abs(diff).toLocaleString();
    
    return (
      <span className={`text-[9px] font-mono font-bold ml-2 px-1.5 py-0.2 rounded ${
        isSuccess 
          ? 'bg-emerald-950/40 text-emerald-400 border border-emerald-900/40 animate-pulse' 
          : 'bg-rose-950/40 text-rose-400 border border-rose-900/40 animate-pulse'
      }`}>
        {formatStr}
      </span>
    );
  };

  // ag-Grid Columns for Planned Orders Worksheet
  const getGridColumns = () => {
    if (ledgerSubTab === 'planned_orders') {
      return [
        { headerName: '计划零件代号 (Part Code) 🔍', field: 'part_code', sortable: true, filter: true, width: 180, cellRenderer: (p) => <span className="font-mono font-bold text-heading hover:underline cursor-pointer">{p.value}</span> },
        { headerName: '计划批量 (Qty)', field: 'order_qty', sortable: true, width: 120, cellRenderer: (p) => <span className="cyber-font font-bold text-indigo-400">{Math.round(p.value).toLocaleString()}</span> },
        { headerName: '计划开工日 (Start)', field: 'start_day', sortable: true, width: 120, cellRenderer: (p) => <span className="cyber-font">D{p.value}</span> },
        { headerName: '交付日 (Finish)', field: 'finish_day', sortable: true, width: 120, cellRenderer: (p) => <span className="cyber-font text-emerald-500 font-bold">D{p.value}</span> },
        { headerName: '规格尺寸 (Dim)', field: 'dimension_val', sortable: true, width: 120, cellRenderer: (p) => <span className="text-muted">{p.value ? `${p.value}MB` : '-'}</span> }
      ];
    } else if (ledgerSubTab === 'alternates') {
      return [
        { headerName: '意向主料 (Main Part) 🔍', field: 'main_part', sortable: true, filter: true, width: 170, cellRenderer: (p) => <span className="font-mono text-muted hover:underline cursor-pointer">{p.value}</span> },
        { headerName: '替代消纳料 (Alt Part) 🔍', field: 'alt_part', sortable: true, filter: true, width: 170, cellRenderer: (p) => <span className="font-mono font-bold text-heading text-amber-500 hover:underline cursor-pointer">{p.value}</span> },
        { headerName: '配分消纳量', field: 'allocated_qty', sortable: true, width: 120, cellRenderer: (p) => <span className="cyber-font font-bold text-indigo-400">{Math.round(p.value).toLocaleString()}</span> },
        { headerName: '消纳日期', field: 'day', sortable: true, width: 100, cellRenderer: (p) => <span className="cyber-font font-semibold">D{p.value}</span> },
        { 
          headerName: '替代评级 (Class)', 
          field: 'alt_class', 
          sortable: true, 
          width: 120,
          cellRenderer: (p) => {
            const classColors = { 1: 'bg-emerald-950/40 text-emerald-400 border border-emerald-900/40', 2: 'bg-indigo-950/40 text-indigo-400 border border-indigo-900/40', 3: 'bg-amber-950/40 text-amber-400 border border-amber-900/40' };
            return (
              <span className={`px-2 py-0.5 rounded text-[9px] font-bold ${classColors[p.value] || 'bg-slate-800 text-slate-300'}`}>
                Level {p.value}
              </span>
            );
          }
        }
      ];
    } else if (ledgerSubTab === 'wbs') {
      return [
        { headerName: '项目代号', field: 'project_code', sortable: true, filter: true, width: 120, cellRenderer: (p) => <span className="font-bold text-heading">{p.value}</span> },
        { headerName: 'WBS工作节点', field: 'wbs_code', sortable: true, filter: true, width: 130, cellRenderer: (p) => <span className="font-mono text-indigo-400 font-bold">{p.value}</span> },
        { headerName: '任务描述', field: 'description', sortable: true, width: 180 },
        { 
          headerName: '节点状态 ✏️', 
          field: 'wbs_status', 
          sortable: true, 
          width: 120,
          editable: true,
          cellEditor: 'agSelectCellEditor',
          cellEditorParams: {
            values: ['ACTIVE', 'FINISHED', 'COMPLETED', 'HOLD']
          },
          cellRenderer: (p) => {
            const isCompleted = p.value === 'COMPLETED' || p.value === 'FINISHED';
            const isHold = p.value === 'HOLD';
            return (
              <span className={`px-1.5 py-0.5 rounded text-[8.5px] font-bold ${
                isCompleted 
                  ? 'bg-emerald-950/40 text-emerald-400 border border-emerald-900/40' 
                  : isHold 
                    ? 'bg-rose-950/40 text-rose-400 border border-rose-900/40' 
                    : 'bg-amber-950/40 text-amber-400 border border-amber-900/40'
              }`}>
                {p.value}
              </span>
            );
          }
        },
        { headerName: '工期', field: 'duration', sortable: true, width: 80, cellRenderer: (p) => <span className="cyber-font">{p.value}天</span> },
        { headerName: '最早开工/完工', width: 130, valueGetter: (p) => `D${p.data.early_start} - D${p.data.early_finish}`, cellRenderer: (p) => <span className="cyber-font text-emerald-500">{p.value}</span> },
        { headerName: '最迟开工/完工', width: 130, valueGetter: (p) => `D${p.data.late_start} - D${p.data.late_finish}`, cellRenderer: (p) => <span className="cyber-font text-muted">{p.value}</span> },
        { headerName: '自由时差 (Slack)', field: 'slack_days', sortable: true, width: 100, cellRenderer: (p) => <span className={`cyber-font ${p.value > 0 ? 'text-amber-500' : 'text-muted'}`}>{p.value}天</span> },
        { 
          headerName: '关键路径', 
          field: 'is_critical', 
          sortable: true, 
          width: 110,
          cellRenderer: (p) => p.value ? (
            <span className="bg-rose-950 border border-rose-800 text-rose-400 text-[8px] font-bold px-1.5 py-0.5 rounded">
              CRITICAL TASK
            </span>
          ) : <span className="text-muted">-</span>
        }
      ];
    } else { // supply_assignments
      return [
        { headerName: '需求订单 ID 🔍', field: 'demand', sortable: true, filter: true, width: 150, cellRenderer: (p) => <span className="font-mono font-bold text-heading hover:underline cursor-pointer">{p.value}</span> },
        { headerName: '物料代号 🔍', field: 'part', sortable: true, filter: true, width: 140, cellRenderer: (p) => <span className="font-mono text-indigo-400 font-bold hover:underline cursor-pointer">{p.value}</span> },
        { headerName: '匹配供应 ID', field: 'supply', sortable: true, width: 150, cellRenderer: (p) => <span className="font-mono text-muted">{p.value || '-'}</span> },
        { headerName: '分配数量 (Qty)', field: 'assigned_qty', sortable: true, width: 110, cellRenderer: (p) => <span className="cyber-font font-bold text-heading">{Math.round(p.value).toLocaleString()}</span> },
        { headerName: '交期', field: 'due_date', sortable: true, width: 100, cellRenderer: (p) => <span className="cyber-font">{p.value}</span> },
        { headerName: '可用日期', field: 'available_date', sortable: true, width: 100, cellRenderer: (p) => <span className="cyber-font text-emerald-400 font-bold">{p.value || '-'}</span> },
        { headerName: '对应生产订单', field: 'ipc_planned_order', sortable: true, width: 150, cellRenderer: (p) => <span className="font-mono text-muted">{p.value || '-'}</span> }
      ];
    }
  };

  // Right-Click Context Menu / Double Click cell jump (Kinaxis habit details)
  const onCellDoubleClicked = (event) => {
    const { colDef, value } = event;
    if (!colDef || !value) return;
    const field = colDef.field;
    if ((field === 'part_code' || field === 'part' || field === 'assigned_part' || field === 'request_part' || field === 'main_part' || field === 'alt_part') && value) {
      setSelectedPartCode(value);
      setActiveTab('mps'); // Navigates workbook view to MPS Dashboard
      setBounceAlert(`🔍 已联动跳转至主生产计划大盘 (Part: ${value})`);
      setTimeout(() => setBounceAlert(null), 3000);
    } else if ((field === 'demand' || field === 'demand_id' || field === 'order_code') && value) {
      setSelectedDemandId(value);
      setActiveTab('pegging'); // Navigates workbook view to Demand-Supply Pegging
      setBounceAlert(`🔍 已因果追溯跳转至供需钉结谱 (Order: ${value})`);
      setTimeout(() => setBounceAlert(null), 3000);
    }
  };

  // Workbook sidebar worksheets catalog (Redesign worksheets list)
  const renderWorkbookSidebar = () => {
    const worksheets = [
      {
        category: '1. 决策大脑与控制塔 (Cognitive Studio)',
        sheets: [
          { key: 'control_tower', label: '智能决策控制塔 (Control Tower)', icon: TowerControl },
          { key: 'pegging', label: '供需钉结溯源 (Demand-Supply Pegging)', icon: Split },
        ]
      },
      {
        category: '2. 计划与优化工作台 (Planning & Optimization)',
        sheets: [
          { key: 'mps', label: '主生产计划大盘 (MPS Dashboard)', icon: Cpu },
          { key: 'mrp', label: '物料级联消纳 (MRP Substitution)', icon: GitBranch },
          { key: 'capacity', label: '有限能力排产 (Capacity Workbench)', icon: Factory },
          { key: 'io', label: '库存策略优化 (Inventory Optimization)', icon: ShieldCheck },
        ]
      },
      {
        category: '3. 供应链网络协同 (Network Collaboration)',
        sheets: [
          { key: 'collab', label: '采购协同承诺 (Supplier Collab)', icon: Truck },
          { key: 'ibp', label: '集成业务财务对账 (IBP Finance)', icon: Landmark },
          { key: 'allotment', label: '战略分摊配额 (Strategic Allotment)', icon: Layers },
          { key: 'execution', label: '车间排产派工 (Execution Dispatch)', icon: Factory },
          { key: 'eto', label: 'CPM关键路径项目 (ETO Projects)', icon: Projector },
        ]
      },
      {
        category: '4. 原生计划台账明细 (System Ledgers)',
        sheets: [
          { key: 'planned_orders', label: '计划订单台账 (Planned Orders)', icon: Database },
        ]
      }
    ];

    if (isSidebarCollapsed) {
      return (
        <div className="w-14 bg-card border border-main rounded-xs py-3.5 flex flex-col items-center justify-between h-full select-none flex-shrink-0 transition-all duration-200">
          <div className="flex flex-col items-center space-y-4 w-full">
            <button 
              onClick={() => setIsSidebarCollapsed(false)}
              className="p-1.5 hover:bg-slate-800/60 rounded text-muted hover:text-heading cursor-pointer"
              title="展开 Workbook 列表"
            >
              <ChevronRight className="w-4.5 h-4.5 text-indigo-500" />
            </button>
            <div className="w-8 h-px bg-slate-800" />
            
            <div className="flex flex-col items-center space-y-3 w-full overflow-y-auto">
              {worksheets.flatMap(g => g.sheets).map(s => {
                const Icon = s.icon;
                const isActive = activeTab === s.key;
                return (
                  <button
                    key={s.key}
                    onClick={() => setActiveTab(s.key)}
                    className={`p-2.5 rounded transition-all cursor-pointer ${
                      isActive 
                        ? 'bg-indigo-600/20 border border-indigo-500 text-indigo-400' 
                        : 'text-muted hover:text-heading hover:bg-slate-800/40 border border-transparent'
                    }`}
                    title={s.label}
                  >
                    <Icon className="w-4.5 h-4.5" />
                  </button>
                );
              })}
            </div>
          </div>
          
          <div className="text-[8px] text-muted font-bold font-mono tracking-tighter scale-90">IPC</div>
        </div>
      );
    }

    // Filter sheets based on search query
    const filteredWorksheets = worksheets.map(g => {
      const sheets = g.sheets.filter(s => 
        s.label.toLowerCase().includes(sidebarSearch.toLowerCase()) || 
        s.key.toLowerCase().includes(sidebarSearch.toLowerCase())
      );
      return { ...g, sheets };
    }).filter(g => g.sheets.length > 0);

    return (
      <div className="w-64 bg-card border border-main rounded-xs p-3 flex flex-col space-y-3.5 h-full overflow-hidden select-none flex-shrink-0 transition-all duration-200">
        
        {/* Sidebar Header with collapse & search */}
        <div className="flex justify-between items-center border-b border-main pb-2">
          <span className="font-bold text-heading text-[11px] uppercase tracking-wider flex items-center">
            <Layers className="w-4 h-4 text-indigo-400 mr-1.5" />
            Workbook 工作簿
          </span>
          <button 
            onClick={() => setIsSidebarCollapsed(true)}
            className="p-1 hover:bg-slate-800/60 rounded text-muted hover:text-heading cursor-pointer"
            title="收起侧边栏"
          >
            <ChevronLeft className="w-4.5 h-4.5" />
          </button>
        </div>

        {/* Quick Search */}
        <div className="relative">
          <input
            type="text"
            value={sidebarSearch}
            onChange={(e) => setSidebarSearch(e.target.value)}
            placeholder="搜索工作表/指标..."
            className="w-full bg-input border border-main text-heading text-[10px] pl-7 pr-2.5 py-1.2 rounded focus:outline-none focus:border-indigo-500"
          />
          <Search className="w-3 h-3 text-muted absolute left-2 top-2" />
          {sidebarSearch && (
            <button onClick={() => setSidebarSearch('')} className="absolute right-2 top-2 text-muted hover:text-heading">
              <X className="w-3 h-3" />
            </button>
          )}
        </div>

        {/* Grouped Tree List */}
        <div className="flex-1 overflow-y-auto space-y-3.5 pr-1">
          {filteredWorksheets.map(g => (
            <div key={g.category} className="space-y-1">
              <div className="text-[8.5px] text-muted font-bold uppercase tracking-widest pl-1">
                {g.category}
              </div>
              <div className="space-y-0.5">
                {g.sheets.map(s => {
                  const Icon = s.icon;
                  const isActive = activeTab === s.key;
                  return (
                    <button
                      key={s.key}
                      onClick={() => {
                        setActiveTab(s.key);
                      }}
                      className={`w-full flex items-center space-x-2.5 px-2.5 py-2 rounded-xs text-[10.5px] transition-all cursor-pointer font-medium text-left border ${
                        isActive
                          ? 'bg-indigo-600/15 text-indigo-400 border-indigo-500/50 font-bold'
                          : 'text-main hover:text-heading hover:bg-slate-800/20 border-transparent'
                      }`}
                    >
                      <Icon className={`w-3.5 h-3.5 ${isActive ? 'text-indigo-400' : 'text-muted'}`} />
                      <span className="truncate">{s.label}</span>
                    </button>
                  );
                })}
              </div>
            </div>
          ))}
          {filteredWorksheets.length === 0 && (
            <div className="text-center py-6 text-muted text-[10px]">无匹配的工作表</div>
          )}
        </div>
        
        {/* Footer */}
        <div className="text-[9px] border-t border-main/50 pt-2 text-muted text-center font-mono">
          Decoupled Engine v1.3
        </div>
      </div>
    );
  };

  // Original double-panel App.jsx ledger layout encapsulated as a worksheet
  const renderLedgerView = () => {
    return (
      <div className="w-full h-full flex flex-row space-x-3 overflow-hidden min-h-0">
        
        {/* 左侧操作区 (Operations Center - Width 320px) */}
        <div className="w-[330px] bg-card border border-main rounded-xs p-3 flex flex-col space-y-3.5 h-full overflow-y-auto select-none flex-shrink-0">
          
          {/* Section 1: Simulate Planner */}
          <div className="workbench-card p-3 border border-main rounded bg-slate-900/10">
            <h3 className="text-xs font-bold text-heading mb-2.5 flex items-center">
              <Play className="w-3.5 h-3.5 text-indigo-500 mr-1.5" />
              计划求解引擎控制 (Simulation Loop)
            </h3>
            
            <button 
              onClick={handleSimulate} 
              disabled={isLoading}
              className="w-full btn-premium-primary flex items-center justify-center space-x-2 py-2 px-4 rounded text-white bg-indigo-600 disabled:opacity-50 font-bold text-xs cursor-pointer shadow-lg hover:shadow-indigo-500/10"
            >
              <RefreshCw className={`w-4 h-4 ${isLoading ? 'animate-spin' : ''}`} />
              <span>{isLoading ? '核心求解器解算中...' : '提交修改并执行重算'}</span>
            </button>
            
            {isLoading && (
              <div className="mt-2.5 space-y-1">
                <div className="w-full bg-slate-800 h-1 rounded overflow-hidden">
                  <div className="bg-indigo-500 h-full animate-progress" style={{ width: '70%' }}></div>
                </div>
                <div className="text-[9px] text-muted text-center">正在爆破级联 BOM 拓扑并解算 WBS 关键路径...</div>
              </div>
            )}
          </div>

          {/* Section 2: Insert Demand Form */}
          <div className="workbench-card p-3 border border-main rounded">
            <h3 className="text-xs font-bold text-heading mb-2.5 flex items-center">
              <PlusCircle className="w-3.5 h-3.5 text-indigo-500 mr-1.5" />
              独立需求快速录入 (Insert Demand)
            </h3>
            
            <form onSubmit={handleInsertDemand} className="space-y-2.5 text-[10.5px]">
              <div className="grid grid-cols-2 gap-2">
                <div>
                  <label className="block text-muted font-bold mb-1">零件代号 (Part)</label>
                  <select
                    value={newDemand.part_code}
                    onChange={(e) => setNewDemand(prev => ({ ...prev, part_code: e.target.value }))}
                    className="w-full bg-input border border-main text-heading text-[10.5px] p-1.5 rounded outline-none focus:border-indigo-500 cursor-pointer"
                  >
                    <option value="">-- 选择零件 --</option>
                    {partCodes.map(part => (
                      <option key={part} value={part}>{part}</option>
                    ))}
                  </select>
                </div>
                <div>
                  <label className="block text-muted font-bold mb-1">需求数量 (Qty)</label>
                  <input
                    type="number"
                    value={newDemand.qty}
                    onChange={(e) => setNewDemand(prev => ({ ...prev, qty: e.target.value }))}
                    placeholder="例如 500"
                    className="w-full bg-input border border-main text-heading text-[10.5px] p-1.5 rounded outline-none focus:border-indigo-500"
                  />
                </div>
              </div>

              <div className="grid grid-cols-2 gap-2">
                <div>
                  <label className="block text-muted font-bold mb-1">交付需求日 (Day)</label>
                  <input
                    type="number"
                    value={newDemand.day}
                    onChange={(e) => setNewDemand(prev => ({ ...prev, day: e.target.value }))}
                    placeholder="天数, 例如 10"
                    className="w-full bg-input border border-main text-heading text-[10.5px] p-1.5 rounded outline-none focus:border-indigo-500"
                  />
                </div>
                <div>
                  <label className="block text-muted font-bold mb-1">优先级 (Priority)</label>
                  <select
                    value={newDemand.priority}
                    onChange={(e) => setNewDemand(prev => ({ ...prev, priority: parseInt(e.target.value) }))}
                    className="w-full bg-input border border-main text-heading text-[10.5px] p-1.5 rounded outline-none focus:border-indigo-500 cursor-pointer"
                  >
                    <option value="1">1 (最高优先)</option>
                    <option value="2">2 (常规订单)</option>
                    <option value="3">3 (计划预测)</option>
                    <option value="4">4 (补货填充)</option>
                    <option value="5">5 (极低水位)</option>
                  </select>
                </div>
              </div>

              <div>
                <label className="block text-muted font-bold mb-1">客户名称 (Customer)</label>
                <input
                  type="text"
                  value={newDemand.customer}
                  onChange={(e) => setNewDemand(prev => ({ ...prev, customer: e.target.value }))}
                  placeholder="VVIP_CUST / NORMAL_CUST"
                  className="w-full bg-input border border-main text-heading text-[10.5px] p-1.5 rounded outline-none focus:border-indigo-500"
                />
              </div>

              <button
                type="submit"
                className="w-full py-1.5 bg-indigo-600/20 hover:bg-indigo-600 hover:text-white border border-indigo-500/50 text-indigo-400 text-xs font-bold rounded cursor-pointer transition-all shadow-sm flex items-center justify-center space-x-1.5"
              >
                <PlusCircle className="w-3.5 h-3.5" />
                <span>录入独立需求并重算</span>
              </button>
            </form>
          </div>

          {/* Section 3: Demand List & Splitter */}
          <div className="workbench-card p-3 border border-main rounded flex-1 flex flex-col min-h-[220px]">
            <h3 className="text-xs font-bold text-heading mb-2 flex items-center flex-shrink-0">
              <Database className="w-3.5 h-3.5 text-indigo-500 mr-1.5" />
              独立需求台账与拆分 (Demands)
            </h3>
            
            <div className="flex-1 overflow-y-auto border border-main bg-input p-1.5 rounded">
              {demandsLoading && demands.length === 0 ? (
                <div className="text-center py-10 text-muted">正在载入独立需求台账...</div>
              ) : (
                <table className="w-full text-left border-collapse text-[10px]">
                  <thead>
                    <tr className="border-b border-main text-muted uppercase tracking-wider font-bold">
                      <th className="py-1">零件</th>
                      <th className="py-1 text-center">交期</th>
                      <th className="py-1 text-right">数量</th>
                      <th className="py-1 text-center">操作</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-slate-800/40 text-main font-mono">
                    {demands.map((row, idx) => {
                      const day = getDayOffset(row.request_due_date);
                      return (
                        <tr key={idx} className="hover:bg-slate-800/20">
                          <td className="py-1.5 font-bold text-heading">{row.part}</td>
                          <td className="py-1.5 text-center text-indigo-400 font-bold">D{day}</td>
                          <td className="py-1.5 text-right text-heading">{Math.round(row.request_qty).toLocaleString()}</td>
                          <td className="py-1.5 text-center">
                            <button
                              onClick={() => triggerSplitModal(row)}
                              className="px-1.5 py-0.5 bg-indigo-950/40 hover:bg-indigo-900 text-indigo-400 border border-indigo-900/60 rounded text-[9px] font-bold cursor-pointer transition-all flex items-center mx-auto"
                              title="对账订单拆分"
                            >
                              <Split className="w-2.5 h-2.5 mr-0.5" />
                              拆分
                            </button>
                          </td>
                        </tr>
                      );
                    })}
                    {demands.length === 0 && (
                      <tr>
                        <td colSpan={4} className="py-8 text-center text-muted">台账空空如也</td>
                      </tr>
                    )}
                  </tbody>
                </table>
              )}
            </div>
          </div>

          {/* Section 4: Branch Differences & Conflicts */}
          {currentScenario !== 'baseline' && (
            <div className="workbench-card p-3 border border-main rounded bg-slate-900/10 flex-shrink-0">
              <h3 className="text-xs font-bold text-heading mb-2 flex items-center text-amber-500">
                <AlertTriangle className="w-4 h-4 mr-1.5" />
                沙箱分支差异与并发冲突
              </h3>
              
              <div className="text-[10px] space-y-2 max-h-48 overflow-y-auto">
                {diffLoading ? (
                  <div className="text-muted text-center py-2">计算差异中...</div>
                ) : (
                  <>
                    {/* Commits */}
                    {pendingChanges.pending_commits && pendingChanges.pending_commits.length > 0 && (
                      <div className="space-y-1 border-b border-main/40 pb-2">
                        <div className="text-muted font-bold uppercase tracking-wider flex items-center">
                          <span className="w-1.5 h-1.5 rounded-full bg-indigo-400 mr-1.5"></span>
                          待核准合并的更改 ({pendingChanges.pending_commits.length})
                        </div>
                        {pendingChanges.pending_commits.slice(0, 3).map((c, i) => (
                          <div key={i} className="text-heading font-mono bg-slate-900/30 p-1 border border-main/50 rounded flex justify-between">
                            <span>{c.key} ({c.table})</span>
                            <span className="text-indigo-400 font-bold">{c.old_val} ➔ {c.new_val}</span>
                          </div>
                        ))}
                      </div>
                    )}

                    {/* Conflicts */}
                    {pendingChanges.conflicts && pendingChanges.conflicts.length > 0 ? (
                      <div className="space-y-2">
                        <div className="text-rose-400 font-bold uppercase tracking-wider flex items-center">
                          <span className="w-1.5 h-1.5 rounded-full bg-rose-500 mr-1.5 animate-ping"></span>
                          检测到并发冲突 ({pendingChanges.conflicts.length})
                        </div>
                        {pendingChanges.conflicts.map((c, i) => (
                          <div key={i} className="bg-rose-950/20 border border-rose-900/60 p-2 rounded text-[9.5px] space-y-1">
                            <div className="font-bold text-heading font-mono">{c.key}</div>
                            <div className="text-muted flex justify-between font-mono text-[9px]">
                              <span>原值: {c.baseline_val}</span>
                              <span>主计划值: {c.parent_val}</span>
                              <span className="text-amber-400 font-bold">沙箱修改: {c.sandbox_val}</span>
                            </div>
                            <div className="flex space-x-1 pt-1 justify-end">
                              <button
                                onClick={() => handleResolveConflict(c.table, c.key, 'accept_parent')}
                                className="px-1.5 py-0.5 bg-emerald-950/60 hover:bg-emerald-900 border border-emerald-900/60 text-emerald-400 rounded-xs font-bold text-[8.5px] cursor-pointer"
                              >
                                接受母版
                              </button>
                              <button
                                onClick={() => handleResolveConflict(c.table, c.key, 'keep_sandbox')}
                                className="px-1.5 py-0.5 bg-indigo-950/60 hover:bg-indigo-900 border border-indigo-900/60 text-indigo-400 rounded-xs font-bold text-[8.5px] cursor-pointer"
                              >
                                保留沙箱
                              </button>
                            </div>
                          </div>
                        ))}
                      </div>
                    ) : (
                      <div className="text-emerald-400 font-bold flex items-center py-1">
                        <CheckCircle2 className="w-3.5 h-3.5 mr-1.5" />
                        当前分支无合并冲突
                      </div>
                    )}
                  </>
                )}
              </div>
            </div>
          )}

        </div>

        {/* 右侧核心结果展示区 (Result Explorer - Width flex-1) */}
        <div className="flex-1 bg-card border border-main rounded-xs p-3 flex flex-col space-y-3 h-full overflow-hidden">
          
          {/* KPI Dashboard */}
          <div className="grid grid-cols-2 md:grid-cols-4 gap-3 select-none flex-shrink-0">
            
            <div className="flex items-center space-x-3 border-r border-main pr-3">
              <span className="text-2xl text-indigo-500"><ShoppingCart className="w-6 h-6 animate-pulse" /></span>
              <div className="flex-1 min-w-0">
                <p className="text-[9px] text-muted uppercase tracking-wider font-semibold">独立需求消纳 / 计划供应</p>
                <h4 className="text-xs font-bold text-heading flex items-center cyber-font">
                  <span>{kpis.demands.toLocaleString()} / {kpis.planned.toLocaleString()}</span>
                  {kpis.baseline && renderDelta(kpis.planned, kpis.baseline.planned, 'number', false)}
                </h4>
              </div>
            </div>

            <div className="flex items-center space-x-3 border-r border-main pr-3">
              <span className="text-2xl text-purple-500"><Projector className="w-6 h-6" /></span>
              <div className="flex-1 min-w-0">
                <p className="text-[9px] text-muted uppercase tracking-wider font-semibold">活跃项目 / WBS工序数</p>
                <h4 className="text-xs font-bold text-heading flex items-center cyber-font">
                  <span>{kpis.projects} P / {kpis.wbs} N</span>
                  {kpis.baseline && renderDelta(kpis.wbs, kpis.baseline.wbs, 'number', false)}
                </h4>
              </div>
            </div>

            <div className="flex items-center space-x-3 border-r border-main pr-3">
              <span className="text-2xl text-emerald-500"><TrendingUp className="w-6 h-6" /></span>
              <div className="flex-1 min-w-0">
                <p className="text-[9px] text-muted uppercase tracking-wider font-semibold">共识收入预测对账</p>
                <h4 className="text-xs font-bold text-heading flex items-center cyber-font">
                  <span>{formatCurrency(kpis.revenue)}</span>
                  {kpis.baseline && renderDelta(kpis.revenue, kpis.baseline.revenue, 'currency', false)}
                </h4>
              </div>
            </div>

            <div className="flex items-center space-x-3">
              <span className="text-2xl text-amber-500"><Warehouse className="w-6 h-6" /></span>
              <div className="flex-1 min-w-0">
                <p className="text-[9px] text-muted uppercase tracking-wider font-semibold">联副产出滞留余料</p>
                <h4 className="text-xs font-bold text-heading flex items-center cyber-font">
                  <span>{kpis.leftovers} 颗</span>
                  {kpis.baseline && renderDelta(kpis.leftovers, kpis.baseline.leftovers, 'number', true)}
                </h4>
              </div>
            </div>

          </div>

          {/* Result Tabs & Table */}
          <div className="flex-1 flex flex-col overflow-hidden min-h-0 border border-main rounded-xs bg-slate-950/20 p-2">
            
            {/* Tabs Selector Bar */}
            <div className="flex justify-between items-center border-b border-main pb-2 mb-2 flex-shrink-0 select-none">
              <div className="flex space-x-1.5 overflow-x-auto whitespace-nowrap scrollbar-none">
                <button
                  onClick={() => setLedgerSubTab('planned_orders')}
                  className={`px-3.5 py-1.5 rounded-xs text-[10.5px] transition-all flex items-center space-x-1.5 cursor-pointer font-bold border ${
                    ledgerSubTab === 'planned_orders'
                      ? 'bg-indigo-600/15 text-indigo-400 border-indigo-500/50'
                      : 'text-muted hover:text-heading hover:bg-slate-800/20 border-transparent'
                  }`}
                >
                  <SlidersHorizontal className="w-3.5 h-3.5" />
                  <span>计划订单台账 (Planned Orders)</span>
                </button>
                <button
                  onClick={() => setLedgerSubTab('alternates')}
                  className={`px-3.5 py-1.5 rounded-xs text-[10.5px] transition-all flex items-center space-x-1.5 cursor-pointer font-bold border ${
                    ledgerSubTab === 'alternates'
                      ? 'bg-indigo-600/15 text-indigo-400 border-indigo-500/50'
                      : 'text-muted hover:text-heading hover:bg-slate-800/20 border-transparent'
                  }`}
                >
                  <Split className="w-3.5 h-3.5" />
                  <span>替代消纳明细 (Alternate Swaps)</span>
                </button>
                <button
                  onClick={() => setLedgerSubTab('wbs')}
                  className={`px-3.5 py-1.5 rounded-xs text-[10.5px] transition-all flex items-center space-x-1.5 cursor-pointer font-bold border ${
                    ledgerSubTab === 'wbs'
                      ? 'bg-indigo-600/15 text-indigo-400 border-indigo-500/50'
                      : 'text-muted hover:text-heading hover:bg-slate-800/20 border-transparent'
                  }`}
                >
                  <Projector className="w-3.5 h-3.5" />
                  <span>项目WBS工程任务 (ETO Projects)</span>
                </button>
                <button
                  onClick={() => setLedgerSubTab('supply_assignments')}
                  className={`px-3.5 py-1.5 rounded-xs text-[10.5px] transition-all flex items-center space-x-1.5 cursor-pointer font-bold border ${
                    ledgerSubTab === 'supply_assignments'
                      ? 'bg-indigo-600/15 text-indigo-400 border-indigo-500/50'
                      : 'text-muted hover:text-heading hover:bg-slate-800/20 border-transparent'
                  }`}
                >
                  <CheckCircle2 className="w-3.5 h-3.5" />
                  <span>供应分配明细 (Supply Assignment)</span>
                </button>
              </div>

              {/* Floating search bar */}
              <div className="flex items-center space-x-2">
                <div className="relative">
                  <input
                    type="text"
                    value={gridSearch}
                    onChange={(e) => setGridSearch(e.target.value)}
                    placeholder="快速搜索过滤表数据..."
                    className="w-48 bg-input border border-main text-heading text-[10px] pl-7 pr-2.5 py-1.2 rounded-xs focus:outline-none focus:border-indigo-500"
                  />
                  <Search className="w-3 h-3 text-muted absolute left-2 top-2" />
                  {gridSearch && (
                    <button onClick={() => setGridSearch('')} className="absolute right-2 top-2 text-muted hover:text-heading">
                      <X className="w-3 h-3" />
                    </button>
                  )}
                </div>
              </div>
            </div>

            {/* WBS Edit Hint Banner */}
            {ledgerSubTab === 'wbs' && (
              <div className="text-[10px] text-amber-500 mb-2 flex items-center bg-amber-950/10 border border-amber-900/30 px-3 py-1.5 rounded-xs select-none animate-fade-in">
                <AlertTriangle className="w-3.5 h-3.5 mr-1.5 flex-shrink-0" />
                <span>💡 <b>操作提示：</b>双击“节点状态”列中的单元格，即可直接下拉选择任务进度（ACTIVE, COMPLETED 等）。系统会自动修改 DuckDB 数据库并重新求解 WBS 关键时间参数！</span>
              </div>
            )}

            {/* Hint for double click jumps */}
            {ledgerSubTab !== 'wbs' && (
              <div className="text-[9.5px] text-indigo-400 mb-2 flex items-center bg-indigo-950/10 border border-indigo-900/20 px-3 py-1 rounded-xs select-none animate-fade-in font-mono">
                <Info className="w-3.5 h-3.5 mr-1.5 flex-shrink-0" />
                <span>💡 <b>Kinaxis 联动跳转习惯：</b>双击“零件”列可直接跳转至 MPS 计划大盘；双击“需求订单 ID”可直接跳转至供需钉结溯源。</span>
              </div>
            )}

            {/* ag-Grid Content */}
            <div className="flex-1 w-full overflow-hidden relative border border-main rounded-xs bg-card">
              {gridLoading && (
                <div className="absolute inset-0 bg-card/85 backdrop-blur-xs flex items-center justify-center z-20 text-xs text-indigo-400 font-bold">
                  正在同步 DuckDB 物理镜像活表数据...
                </div>
              )}
              
              <div className="ag-theme-balham-dark w-full h-full">
                <AgGridReact
                  theme="legacy"
                  rowData={gridData}
                  columnDefs={getGridColumns()}
                  defaultColDef={{ resizable: true, flex: 1 }}
                  quickFilterText={gridSearch}
                  headerHeight={26}
                  rowHeight={22}
                  pagination={true}
                  paginationPageSize={30}
                  onCellValueChanged={onCellValueChanged}
                  onCellDoubleClicked={onCellDoubleClicked}
                />
              </div>
            </div>

          </div>

        </div>

      </div>
    );
  };

  // Render the currently active worksheet view component
  const renderActiveSheet = () => {
    switch (activeTab) {
      case 'control_tower':
        return <ControlTower />;
      case 'mps':
        return <MpsDashboard />;
      case 'pegging':
        return <DemandSupplyPegging />;
      case 'mrp':
        return <MrpSubstitution />;
      case 'capacity':
        return <CapacityWorkbench />;
      case 'collab':
        return <SupplierCollab />;
      case 'io':
        return <InventoryOptimization />;
      case 'ibp':
        return <IbpFinance />;
      case 'allotment':
        return <AllotmentWorkbench />;
      case 'execution':
        return <ExecutionDispatch />;
      case 'eto':
        return <ProjectEto />;
      case 'planned_orders':
      default:
        return renderLedgerView();
    }
  };

  const renderSplitModal = () => {
    if (!isSplitModalOpen || !selectedDemandForSplit) return null;
    return (
      <div className="fixed inset-0 bg-black/60 backdrop-blur-xs flex items-center justify-center z-50">
        <div className="bg-card border border-main w-[360px] p-4 rounded shadow-2xl space-y-4 animate-fade-in text-[11px]">
          <div className="flex justify-between items-center border-b border-main pb-2">
            <h3 className="font-bold text-heading text-xs flex items-center">
              <Split className="w-4 h-4 text-indigo-500 mr-1.5" />
              独立需求订单拆分
            </h3>
            <button 
              onClick={() => { setIsSplitModalOpen(false); setSelectedDemandForSplit(null); }}
              className="text-muted hover:text-heading cursor-pointer"
            >
              <X className="w-4 h-4" />
            </button>
          </div>

          <div className="space-y-3 font-mono">
            <div className="p-2.5 bg-slate-900/30 border border-main rounded space-y-1">
              <div><span className="text-muted">需求 ID:</span> <span className="text-heading font-bold">{selectedDemandForSplit.demand}</span></div>
              <div><span className="text-muted">零件代号:</span> <span className="text-heading font-bold">{selectedDemandForSplit.part}</span></div>
              <div><span className="text-muted">原始数量:</span> <span className="text-heading font-bold">{Math.round(selectedDemandForSplit.request_qty).toLocaleString()}</span></div>
              <div><span className="text-muted">原交货日:</span> <span className="text-heading font-bold">D{getDayOffset(selectedDemandForSplit.request_due_date)} ({selectedDemandForSplit.request_due_date})</span></div>
            </div>

            <div className="space-y-3 font-sans">
              <div>
                <label className="block text-muted font-bold mb-1">拆分数量 (从原需求扣减并移出)</label>
                <input
                  type="number"
                  value={splitQty}
                  onChange={(e) => setSplitQty(e.target.value)}
                  placeholder="拆分出的数量"
                  className="w-full bg-input border border-main text-heading text-xs p-2 rounded outline-none focus:border-indigo-500 font-mono"
                />
                <div className="text-[9.5px] text-muted mt-1">
                  拆分后：原需求保留 <span className="text-indigo-400 font-mono">{(selectedDemandForSplit.request_qty - splitQty) || 0}</span>，新需求分配 <span className="text-indigo-400 font-mono">{splitQty || 0}</span>
                </div>
              </div>

              <div>
                <label className="block text-muted font-bold mb-1">新分配的需求交付日 (Split Day)</label>
                <input
                  type="number"
                  value={splitDay}
                  onChange={(e) => setSplitDay(e.target.value)}
                  placeholder="例如 D12"
                  className="w-full bg-input border border-main text-heading text-xs p-2 rounded outline-none focus:border-indigo-500 font-mono"
                />
              </div>
            </div>
          </div>

          <div className="flex space-x-2 justify-end pt-2">
            <button
              onClick={() => { setIsSplitModalOpen(false); setSelectedDemandForSplit(null); }}
              className="px-3.5 py-1.5 border border-main text-muted hover:text-heading rounded font-bold cursor-pointer text-[10.5px] transition-all"
            >
              取消
            </button>
            <button
              onClick={handleSplitDemand}
              className="px-3.5 py-1.5 bg-indigo-600 text-white rounded font-bold cursor-pointer text-[10.5px] transition-all shadow-md hover:bg-indigo-500"
            >
              确认并重新排产
            </button>
          </div>
        </div>
      </div>
    );
  };

  return (
    <div className="flex flex-col h-screen w-screen p-3 space-y-3 bg-main text-main transition-colors duration-200">
      
      {/* 头部导航栏 (Occam's Razor Simplified Header) */}
      <header className="flex flex-col md:flex-row justify-between md:items-center space-y-2 md:space-y-0 bg-card border border-main rounded-xs px-4 py-2 select-none">
        <div className="flex items-center space-x-2.5">
          <TowerControl className="w-5 h-5 text-indigo-500" />
          <div>
            <h1 className="text-sm font-bold tracking-tight text-heading flex items-center">
              IPC 智能计划决策工作台
            </h1>
            <p className="text-[9.5px] text-muted font-medium">
              协同推演与运作管控台 (SCM Unified Control Tower)
            </p>
          </div>
        </div>
        
        <div className="flex flex-wrap items-center gap-2 md:space-x-3.5">
          {/* 主题切换 */}
          <button 
            onClick={() => setIsLightMode(!isLightMode)} 
            className="p-1.5 hover:bg-slate-800/40 rounded-xs text-muted hover:text-heading cursor-pointer transition-colors border border-transparent"
            title={isLightMode ? "切换至深色模式" : "切换至浅色模式"}
          >
            <SunMoon className="w-4 h-4 text-indigo-500" />
          </button>

          {/* AI Copilot Drawer Toggle */}
          <button 
            onClick={() => setIsAiCopilotOpen(!isAiCopilotOpen)}
            className={`flex items-center space-x-1 px-2.5 py-1 text-[10px] border rounded-xs cursor-pointer transition-all ${
              isAiCopilotOpen 
                ? 'bg-indigo-650 border-indigo-550 text-white shadow-md' 
                : 'bg-card border-main text-indigo-400 hover:bg-indigo-900/10'
            }`}
          >
            <Bot className="w-3.5 h-3.5" />
            <span>AI Copilot</span>
          </button>

          {/* 分支选择器 */}
          <div className="flex items-center space-x-1.5 bg-table-header/40 px-2 py-0.8 border border-main text-[10px] rounded-xs">
            <span className="text-muted font-medium flex items-center">
              <GitBranch className="w-3.5 h-3.5 text-indigo-500 mr-1" />
              当前分支:
            </span>
            <select 
              value={currentScenario}
              onChange={(e) => switchScenario(e.target.value)}
              className="bg-card text-heading border border-main text-[10px] px-1.5 py-0.5 rounded-xs font-bold outline-none cursor-pointer"
            >
              {scenarios.map(s => (
                <option key={s.scenario_code} value={s.scenario_code}>
                  {s.scenario_code === 'baseline' ? '🟢 主计划' : `🟡 沙箱: ${s.scenario_code}`}
                </option>
              ))}
            </select>
            
            <button 
              onClick={handleCreateSandbox}
              className="p-1 hover:bg-slate-800/50 rounded-xs text-indigo-400 cursor-pointer transition-colors"
              title="新建沙箱分支"
            >
              <Plus className="w-3.5 h-3.5" />
            </button>

            {currentScenario !== 'baseline' && (
              <>
                <button 
                  onClick={() => mergeScenario(currentScenario)}
                  className="p-1 hover:bg-slate-800/50 rounded-xs text-emerald-450 cursor-pointer transition-colors"
                  title="合并至主计划 (Baseline)"
                >
                  <GitMerge className="w-3.5 h-3.5" />
                </button>
                <button 
                  onClick={() => deleteScenario(currentScenario)}
                  className="p-1 hover:bg-slate-800/50 rounded-xs text-rose-450 cursor-pointer transition-colors"
                  title="删除当前沙箱分支"
                >
                  <Trash2 className="w-3.5 h-3.5" />
                </button>
              </>
            )}
          </div>

          {/* Autopilot LED */}
          <div className="flex items-center space-x-1.5 text-[10px] bg-table-header/30 px-2 py-1 rounded-xs border border-main/50 select-none">
            <span className="relative flex h-1.5 w-1.5">
              {isAutopilot && <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>}
              <span className={`relative inline-flex rounded-full h-1.5 w-1.5 ${isAutopilot ? 'bg-emerald-500' : 'bg-amber-500'}`}></span>
            </span>
            <span className={`font-mono font-bold text-[9px] uppercase tracking-wider ${isAutopilot ? 'text-emerald-400' : 'text-amber-500'}`}>
              Autopilot: {isAutopilot ? 'Online' : 'Standby'}
            </span>
          </div>
        </div>
      </header>

      {/* 沙箱对比快捷视图 */}
      {currentScenario !== 'baseline' && kpis.baseline && (
        <div className="bg-[#121620]/65 backdrop-blur-md border border-amber-900/40 px-4 py-2 rounded-xs flex flex-col md:flex-row items-start md:items-center justify-between gap-2 md:gap-0 select-none">
          <div className="flex items-center space-x-2.5">
            <span className="relative flex h-2 w-2">
              <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-amber-400 opacity-75"></span>
              <span className="relative inline-flex rounded-full h-2 w-2 bg-amber-500"></span>
            </span>
            <div className="text-[10px]">
              <span className="font-bold text-amber-400">⚡ 沙箱推演模式激活: </span>
              <span className="text-heading font-mono font-bold bg-[#090b10] border border-[#1e2538] px-1.5 py-0.5 rounded ml-1">{currentScenario}</span>
              <span className="text-muted ml-2">正在对比当前沙盘计划与生产主计划 (Baseline) 收益对账...</span>
            </div>
          </div>
          
          <div className="flex flex-wrap items-center gap-3.5 text-[10px]">
            <div className="flex space-x-3.5 font-mono">
              <div className="flex items-center space-x-1.5">
                <span className="text-muted">共识营收:</span>
                <span className={`font-bold ${kpis.revenue - kpis.baseline.revenue >= 0 ? 'text-emerald-400' : 'text-rose-400'}`}>
                  {(kpis.revenue - kpis.baseline.revenue >= 0 ? '+' : '') + '¥' + (kpis.revenue - kpis.baseline.revenue).toLocaleString()}
                </span>
              </div>
              <div className="w-px h-3 bg-[#1e2538]"></div>
              <div className="flex items-center space-x-1.5">
                <span className="text-muted">交付供应:</span>
                <span className={`font-bold ${kpis.planned - kpis.baseline.planned >= 0 ? 'text-emerald-400' : 'text-rose-400'}`}>
                  {(kpis.planned - kpis.baseline.planned >= 0 ? '+' : '') + (kpis.planned - kpis.baseline.planned).toLocaleString()} 颗
                </span>
              </div>
              <div className="w-px h-3 bg-[#1e2538]"></div>
              <div className="flex items-center space-x-1.5">
                <span className="text-muted">滞留余料:</span>
                <span className={`font-bold ${kpis.leftovers - kpis.baseline.leftovers <= 0 ? 'text-emerald-400' : 'text-rose-400'}`}>
                  {(kpis.leftovers - kpis.baseline.leftovers >= 0 ? '+' : '') + (kpis.leftovers - kpis.baseline.leftovers).toLocaleString()} 颗
                </span>
              </div>
            </div>
            
            <div className="flex space-x-2">
              <button 
                onClick={() => mergeScenario(currentScenario)}
                className="px-2.5 py-1 bg-emerald-600 hover:bg-emerald-500 text-white rounded font-bold text-[9.5px] flex items-center space-x-1 shadow-md transition-all cursor-pointer"
              >
                <GitMerge className="w-3.5 h-3.5" />
                <span>核准推送到生产</span>
              </button>
            </div>
          </div>
        </div>
      )}

      {/* 主控制面板区 */}
      <main className="flex-1 flex flex-row space-x-3 overflow-hidden min-h-0 relative">
        
        {/* Collapsible Left Explorer Workbook Sidebar */}
        {renderWorkbookSidebar()}

        {/* Center Active Worksheet View Container */}
        <div className="flex-grow flex-1 min-w-0 h-full overflow-hidden">
          {renderActiveSheet()}
        </div>

        {/* 右侧折叠式 AI 协同副驾驶侧边栏 (AiCopilot Drawer) */}
        {isAiCopilotOpen && (
          <div className="w-[330px] h-full flex flex-col flex-shrink-0 animate-fade-in relative z-10">
            <AiCopilot />
          </div>
        )}

      </main>

      {/* Split Demand Modal Popup */}
      {renderSplitModal()}

      {/* Floating Bounce Alert Notification */}
      {bounceAlert && (
        <div className="fixed bottom-5 right-5 bg-emerald-950 border border-emerald-600 text-emerald-300 px-4 py-3 rounded shadow-xl text-xs z-50 animate-bounce">
          {bounceAlert}
        </div>
      )}
    </div>
  );
};

function App() {
  return (
    <ScenarioProvider>
      <DashboardContent />
    </ScenarioProvider>
  );
}

export default App;
