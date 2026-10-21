import React, { useState, useEffect, useRef, useMemo } from 'react';
import { MapContainer, TileLayer, Marker, Popup, Polyline } from 'react-leaflet';
import L from 'leaflet';
import { 
  Map, 
  AlertTriangle, 
  Zap, 
  RefreshCw, 
  Undo, 
  Eye, 
  Truck, 
  Factory, 
  Warehouse, 
  Users, 
  ArrowRightLeft,
  Columns,
  Rows,
  X,
  Lock,
  Unlock,
  Key,
  HelpCircle,
  Database,
  Grid,
  TrendingUp,
  Sliders,
  CheckCircle,
  AlertOctagon,
  ZoomIn,
  ZoomOut,
  FolderOpen,
  Shield,
  ShieldAlert,
  Search,
  ChevronRight,
  ChevronDown,
  Download,
  Play,
  Info
} from 'lucide-react';
import { useScenario } from '../../contexts/ScenarioContext';
import { BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip as RechartsTooltip, Legend, ResponsiveContainer } from 'recharts';

import 'leaflet/dist/leaflet.css';

// Leaflet marker setup
const createCustomIcon = (color, label) => L.divIcon({
  html: `
    <div class="flex items-center space-x-1.5 select-none pointer-events-none">
      <div class="relative flex h-3.5 w-3.5">
        <span class="animate-ping absolute inline-flex h-full w-full rounded-full opacity-75" style="background-color: ${color}"></span>
        <span class="relative inline-flex rounded-full h-3.5 w-3.5 border-2 border-[#0b0e14] shadow" style="background-color: ${color}"></span>
      </div>
      <span class="text-[9.5px] bg-[#121620]/95 border border-[#1e2538] text-[#f8fafc] px-1.5 py-0.5 rounded font-mono font-bold tracking-tight shadow-md">${label}</span>
    </div>
  `,
  className: 'custom-leaflet-icon',
  iconSize: [120, 20],
  iconAnchor: [7, 7]
});

// The 30+ Engine Database Tables Metadata Catalog
const ENGINE_TABLES = [
  // 1. 物料与主数据
  { name: 'ipc_material_node', category: '物料与主数据', label: '零件物料主数据表', desc: 'SKU与物料主属性，包含Lead Time、整型取整及安全库存状态', columns: ['part_code (VARCHAR)', 'part_name (VARCHAR)', 'part_type (VARCHAR)', 'lead_time (DOUBLE)', 'round_to_integer (BOOLEAN)', 'safety_stock (DOUBLE)'] },
  { name: 'ipc_part_status', category: '物料与主数据', label: '物料状态主表', desc: '物料级联收支可用量、缺口数及战略等级标记', columns: ['part_code (VARCHAR)', 'status (VARCHAR)', 'shortage_qty (DOUBLE)', 'vvip_lock (BOOLEAN)'] },
  { name: 'ipc_bom_item', category: '物料与主数据', label: 'BOM物料构成表', desc: '物料清单关系，子母件物料配比率及批量因子', columns: ['bomid (VARCHAR)', 'component (VARCHAR)', 'qty (DOUBLE)', 'lot_size (DOUBLE)'] },
  { name: 'ipc_bom_route', category: '物料与主数据', label: 'BOM工艺路线表', desc: '零件到BOM清单的工艺加工指向映射', columns: ['part (VARCHAR)', 'bomid (VARCHAR)', 'routing_code (VARCHAR)'] },
  { name: 'ipc_source', category: '物料与主数据', label: '供货渠道主表', desc: '外部供应渠道与货源区域、运输提前期Lt约束', columns: ['source (VARCHAR)', 'source_site (VARCHAR)', 'transit_lt (DOUBLE)', 'carrier (VARCHAR)'] },
  { name: 'ipc_source_rule', category: '物料与主数据', label: '分配渠道法则表', desc: '渠道供求占比分配规则定义', columns: ['allocation_rule (VARCHAR)', 'source_rule (VARCHAR)', 'description (VARCHAR)'] },
  { name: 'ipc_source_type', category: '物料与主数据', label: '采购供求类型表', desc: '源采购类型与供应级别映射', columns: ['control_class (VARCHAR)', 'source_type (VARCHAR)', 'supply_type (VARCHAR)'] },
  
  // 2. 需求与计划
  { name: 'ipc_independent_demand', category: '需求与计划', label: '客户独立需求表', desc: '销售订单、预测需求、Due Day与客户战略级（VVIP）约束', columns: ['demand (VARCHAR)', 'part (VARCHAR)', 'request_qty (DOUBLE)', 'due_day (INTEGER)', 'customer (VARCHAR)', 'tier_val (INTEGER)'] },
  { name: 'ipc_consensus_forecast', category: '需求与计划', label: '多流共识预测表', desc: '集成销售、市场、财务预测及Holt-Winters统计预测量', columns: ['part_code (VARCHAR)', 'period_code (VARCHAR)', 'sales_qty (DOUBLE)', 'marketing_qty (DOUBLE)', 'statistical_qty (DOUBLE)', 'qty (DOUBLE)', 'consensus_forecast (DOUBLE)'] },
  { name: 'ipc_speard_profile', category: '需求与计划', label: '预测分摊权重表', desc: '大盘预测按周期/客户比例进行自顶向下分摊权重系数', columns: ['control_class (VARCHAR)', 'id (VARCHAR)', 'weight (DOUBLE)', 'demand_type (VARCHAR)'] },
  { name: 'ipc_sr_line', category: '需求与计划', label: '销售预测需求明细', desc: '销售订单与承诺交付明细行', columns: ['sr_id (VARCHAR)', 'request_qty (DOUBLE)', 'open_qty (DOUBLE)', 'request_due_date (DATE)'] },

  // 3. 计划工单与匹配
  { name: 'ipc_planned_order_ledger', category: '计划工单与匹配', label: '计划工单表 (LOCKED MRP)', desc: 'LBL MRP netting生成的计划工单主台账', columns: ['order_code (VARCHAR)', 'part_code (VARCHAR)', 'qty (DOUBLE)', 'finish_day (INTEGER)', 'start_day (INTEGER)'] },
  { name: 'ipc_supply_assignment', category: '计划工单与匹配', label: '供需配额匹配表', desc: 'MRP净需求消纳中，工单挂载对应供应源的配额分配明细', columns: ['demand (VARCHAR)', 'part (VARCHAR)', 'supply (VARCHAR)', 'assigned_qty (DOUBLE)'] },
  { name: 'ipc_planned_supply_assignment', category: '计划工单与匹配', label: '计划供求装载表', desc: '计划生成的拟定配额分配关系', columns: ['demand (VARCHAR)', 'part (VARCHAR)', 'supply (VARCHAR)', 'qty (DOUBLE)'] },
  { name: 'ipc_alternate_allocation', category: '计划工单与匹配', label: '替换料匹配分配表', desc: '级联BOM消纳中替换件零件的实际分配量及比例', columns: ['part_code (VARCHAR)', 'alternate_part (VARCHAR)', 'allocated_qty (DOUBLE)', 'day (INTEGER)'] },
  { name: 'ipc_scheduled_receipt', category: '计划工单与匹配', label: '在途采购与确认计划(SR)', desc: '在途（In-Transit）或确认（Confirmed）的采购计划明细', columns: ['sr_id (VARCHAR)', 'part_code (VARCHAR)', 'qty (DOUBLE)', 'day (INTEGER)', 'supply_status (VARCHAR)', 'certainty_level (DOUBLE)'] },
  { name: 'ipc_supply_order', category: '计划工单与匹配', label: '采购交货单主表', desc: '指向对应交货工厂的PO主表记录', columns: ['sr_id (VARCHAR)', 'to_site (VARCHAR)', 'supply_type (VARCHAR)'] },
  { name: 'ipc_allotment_constraint', category: '计划工单与匹配', label: '分摊与额度限制约束表 (ITP)', desc: 'ITP层级战略配额确权算子产出的硬防波堤约束限制', columns: ['scenario_id (VARCHAR)', 'part_code (VARCHAR)', 'site_code (VARCHAR)', 'product_family (VARCHAR)', 'day (INTEGER)', 'itp_calculated_qty (DOUBLE)', 'override_qty (DOUBLE)', 'is_locked (BOOLEAN)'] },
  { name: 'ipc_allotment_ledger', category: '计划工单与匹配', label: '分摊分配与消纳台账 (ITP/IOP)', desc: 'IOP消纳运算时继承自ITP的防波堤配额及当前消耗、剩余、被阻断量明细', columns: ['scenario_id (VARCHAR)', 'part_code (VARCHAR)', 'site_code (VARCHAR)', 'product_family (VARCHAR)', 'day (INTEGER)', 'region (VARCHAR)', 'customer_group (VARCHAR)', 'allotment_limit (DOUBLE)', 'consumed_qty (DOUBLE)', 'available_qty (DOUBLE)', 'blocked_demand_qty (DOUBLE)'] },

  // 4. 生产与排产
  { name: 'ipc_dispatch_ledger', category: '生产与排产', label: '排产派工表 (DBD)', desc: 'Decoupled Dispatcher算法生成的排产派工明细与交付时间', columns: ['dispatch_id (VARCHAR)', 'order_code (VARCHAR)', 'part_code (VARCHAR)', 'qty (DOUBLE)', 'start_day (INTEGER)', 'finish_day (INTEGER)'] },
  { name: 'ipc_work_center_capacity', category: '生产与排产', label: '工作中心产能表', desc: '车间工时效率、设备产能、资源份数及过载率', columns: ['work_center (VARCHAR)', 'date (DATE)', 'working_hour (DOUBLE)', 'number_of_resources (DOUBLE)', 'efficiency (DOUBLE)'] },
  { name: 'ipc_work_center', category: '生产与排产', label: '工作中心基础信息表', desc: '生产车间工作中心物理地址与主时钟日历、设备成本', columns: ['work_center (VARCHAR)', 'description (VARCHAR)', 'site (VARCHAR)', 'type (VARCHAR)'] },
  { name: 'ipc_work_center_type', category: '生产与排产', label: '工作中心类型与效率', desc: '工作中心在不同效率模式下的主资源运转系数', columns: ['control_class (VARCHAR)', 'work_center_type (VARCHAR)', 'hours_per_day (DOUBLE)'] },
  { name: 'ipc_operation', category: '生产与排产', label: '制造工序节点表', desc: '零件生产的路由Routing与具体工段Sequence、Run Time', columns: ['operation (VARCHAR)', 'routing (VARCHAR)', 'sequence (INTEGER)', 'work_center (VARCHAR)', 'run_time (DOUBLE)'] },
  { name: 'ipc_setup_matrix', category: '生产与排产', label: '工序切换洗机矩阵', desc: '连续排产中切换产品导致的设置准备时间成本', columns: ['work_center (VARCHAR)', 'from_product (VARCHAR)', 'to_product (VARCHAR)', 'setup_hours (DOUBLE)'] },
  
  // 5. 联副产品
  { name: 'ipc_coproduct_schedule', category: '联副产品', label: '联副产品产出排程表', desc: '联副主品与副品联合排程产出流水，包含余料盈余', columns: ['routing_code (VARCHAR)', 'day (INTEGER)', 'batches (DOUBLE)', 'leftover_512 (DOUBLE)', 'leftover_256 (DOUBLE)', 'leftover_128 (DOUBLE)'] },
  { name: 'ipc_coproduct_recipe', category: '联副产品', label: '联副产品生产配比表', desc: '主品工艺中512MB/256MB/128MB各晶粒产出配方比例', columns: ['routing_code (VARCHAR)', 'part_code (VARCHAR)', 'ratio_512 (DOUBLE)', 'ratio_256 (DOUBLE)', 'ratio_128 (DOUBLE)'] },
  { name: 'ipc_coproduct_allocation', category: '联副产品', label: '联副产品订单分配表', desc: '订单匹配联副产品各晶粒规格（512/256/128）消纳账', columns: ['order_code (VARCHAR)', 'part_code (VARCHAR)', 'allocated_512 (DOUBLE)', 'allocated_256 (DOUBLE)', 'allocated_128 (DOUBLE)'] },

  // 6. 协同与大盘
  { name: 'ipc_project_wbs', category: '协同与大盘', label: 'ETO WBS项目进度表', desc: 'WBS项目工序网，包含持续时间与CPM关键路径ES/EF/LS/LF时间', columns: ['wbs_code (VARCHAR)', 'duration (DOUBLE)', 'early_start (INTEGER)', 'early_finish (INTEGER)', 'late_start (INTEGER)', 'late_finish (INTEGER)'] },
  { name: 'ipc_project', category: '协同与大盘', label: 'ETO 项目订单主表', desc: 'ETO工程项目主订单，总提前期与工单指向', columns: ['project_code (VARCHAR)', 'delivery_lead_time (INTEGER)'] },
  { name: 'ipc_stock_movement', category: '协同与大盘', label: '库存收发流水账', desc: 'WMS系统同步的GR收货、发料及ASN发运流水明细', columns: ['id (VARCHAR)', 'timestamp (VARCHAR)', 'movement_type (VARCHAR)', 'qty (DOUBLE)', 'certainty (DOUBLE)'] },
  { name: 'ipc_supplier_commit', category: '协同与大盘', label: '协同供应商确认表', desc: '协同端供应商对零件承诺供应排程', columns: ['part_code (VARCHAR)', 'day (INTEGER)', 'forecast_qty (DOUBLE)', 'commit_qty (DOUBLE)'] },
  { name: 'ipc_financial_ledger', category: '协同与大盘', label: '大盘收支账单表', desc: '经营大盘财务报表，包括共识预测营收、持库与采购成本', columns: ['scenario_code (VARCHAR)', 'period_code (VARCHAR)', 'total_revenue (DOUBLE)', 'inventory_carrying_cost (DOUBLE)'] }
];

const ControlTower = () => {
  const { 
    currentScenario, 
    scenarios, 
    switchScenario, 
    fetchKpis, 
    setSelectedDemandId, 
    setActiveTab 
  } = useScenario();

  const isSpreadingRef = useRef(false);

  // 1. Layout State Manager (AST-based Splitting)
  const [layout, setLayout] = useState({
    type: 'split',
    orientation: 'vertical',
    children: [
      { type: 'leaf', view: 'ekg', zoom: 1 },
      { type: 'leaf', view: 'crosstab', zoom: 1 }
    ]
  });

  // AI Copilot & Right Panel States
  const [isRightPanelOpen, setIsRightPanelOpen] = useState(true);
  const [copilotMessages, setCopilotMessages] = useState([
    {
      sender: 'copilot',
      text: '🤖 IPC AI Copilot 准备就绪。\n正在连接 DuckDB 瞬态计算引擎... 成功 (12ms)。\n当前活跃沙盒：Scenario B (S&OP)。\n检测到 1 处物理供应链阻断：陆运滞留阻断。建议执行空运加急以保护 ¥12M 战略营收。',
      timestamp: '11:50:00'
    }
  ]);
  const [chatInput, setChatInput] = useState('');

  const logToCopilot = (text) => {
    setCopilotMessages(prev => [
      ...prev,
      {
        sender: 'copilot',
        text,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' })
      }
    ]);
  };

  const [activePlaybookStep, setActivePlaybookStep] = useState(1);

  const triggerPlaybookStep = (step) => {
    setActivePlaybookStep(step);
    if (step === 1) {
      setSiteFilter('ALL');
      applyPresetFilter('SHORTAGE');
      setSelectedNode('Beijing Customer');
      logToCopilot("🤖 [Demo Playbook] 触发：定位交付延迟！\n发现 BJ_VVIP_CUSTOMER 的工单齐套受阻。\n原因：深圳 wafer 供应陆运发生滞留。请执行第二步【追溯】。");
    } else if (step === 2) {
      setHierarchyPath('SEMI');
      setContextBoundingBox({
        scenario: currentScenario,
        filterId: 'BJ_NORMAL_CUSTOMER',
        siteFilterId: 'SH_ASSEMBLY_HUB',
        dateBucket: 'Week 1 (Transition)',
        metricName: 'BOM Pegging Link Drill',
        target: 1200,
        current: 800,
        constraintState: 'CRITICAL_BREACH'
      });
      logToCopilot("🤖 [Demo Playbook] 触发：级联图图层追溯！\n逆因果树分析已在 C++ 连续内存向量层完成：\nWafer (延迟) -> 512MB (缺口) -> SH封装 (齐套失败) -> BJ客户 (订单延期)。请执行第三步【处方】。");
    } else if (step === 3) {
      executeResolveAction('REROUTE_AIR');
      logToCopilot("🤖 [Demo Playbook] 触发：模拟投产空运加急！\n提前期由 6d 缩短为 2d，晶圆原料于 D8 抢先入库，上海齐套约束恢复正常。请执行第四步【对比】。");
    } else if (step === 4) {
      logToCopilot("🤖 [Demo Playbook] 触发：双沙盒大盘收益比对！\nWhat-If 沙箱结果分析：\n• 财务 Consensus Revenue 恢复 ¥40M 营收缺口\n• OTP 准时履约率上升至 98.20%\n• 净处方投资报酬率 (ROI) 达 240x。\n系统演示流程闭环！");
    }
  };

  const renderBompPeggingWidget = () => {
    return (
      <div className="bg-[#090b10]/40 border border-[#1e2538] p-2.5 rounded space-y-2 select-none">
        <div className="text-[9px] font-bold text-[#cbd5e1] uppercase tracking-wider font-mono flex items-center justify-between">
          <span>级联 BOM 因果树 (BOM Pegging Flow)</span>
          <span className="text-[7.5px] text-indigo-400 font-normal">Auto-Scope</span>
        </div>
        
        <div className="space-y-1 font-mono text-[8.5px]">
          {/* Level 1 */}
          <div className="flex items-center justify-between bg-[#161c28]/80 border border-[#1e2538] p-1 rounded">
            <span className="text-amber-500 font-bold">1. BJ_VVIP_CUSTOMER (Demand)</span>
            <span className="text-heading font-bold">Qty: 1200</span>
          </div>
          <div className="w-0.5 h-2.5 bg-indigo-500/30 ml-3"></div>

          {/* Level 2 */}
          <div className="flex items-center justify-between bg-[#161c28]/80 border border-[#1e2538] p-1 rounded">
            <span className="text-indigo-400 font-bold">2. SH_ASSEMBLY_HUB (Assembly)</span>
            <span className="text-heading font-bold">Yield: 98.2%</span>
          </div>
          <div className="w-0.5 h-2.5 bg-indigo-500/30 ml-3"></div>

          {/* Level 3 */}
          <div className="flex items-center justify-between bg-[#161c28]/80 border border-[#1e2538] p-1 rounded">
            <span className="text-purple-400 font-bold">3. CD_SLICING_FACTORY (512MB)</span>
            <span className={`font-bold ${activeResolutions['OVERTIME_CAPACITY'] ? 'text-emerald-400' : 'text-amber-400 animate-pulse'}`}>
              {activeResolutions['OVERTIME_CAPACITY'] ? 'Capacity OK' : 'Overload (D8)'}
            </span>
          </div>
          <div className="w-0.5 h-2.5 bg-indigo-500/30 ml-3"></div>

          {/* Level 4 */}
          <div className="flex items-center justify-between bg-[#161c28]/80 border border-[#1e2538] p-1 rounded">
            <span className="text-emerald-400 font-bold">4. SZ_WAFER_SUPPLY (Wafer)</span>
            <span className={`font-bold ${activeResolutions['REROUTE_AIR'] ? 'text-emerald-400' : 'text-rose-500 animate-pulse'}`}>
              {activeResolutions['REROUTE_AIR'] ? 'Air Transit (Lt: 2d)' : 'Road Delayed (Lt: 6d)'}
            </span>
          </div>
        </div>
      </div>
    );
  };

  const sandboxChartData = useMemo(() => {
    const isSolved = activeResolutions['REROUTE_AIR'];
    return [
      { name: 'Consensus Rev (M¥)', Baseline: 250, Sandbox: isSolved ? 245 : 205 },
      { name: 'SS Inv Cost (M¥)', Baseline: 20, Sandbox: isSolved ? 26.05 : 26 },
      { name: 'OTP Service (%)', Baseline: 98, Sandbox: isSolved ? 98.2 : 78 }
    ];
  }, [activeResolutions]);

  const renderSandboxComparisonChart = () => {
    return (
      <div className="bg-[#090b10]/40 border border-[#1e2538] p-2.5 rounded space-y-2">
        <div className="text-[9px] font-bold text-[#cbd5e1] uppercase tracking-wider font-mono">
          双沙盒推演比对 (What-If Sandbox ROI Chart)
        </div>

        <div className="h-28 w-full text-[7.5px] relative z-0">
          <ResponsiveContainer width="100%" height="100%">
            <BarChart
              data={sandboxChartData}
              margin={{ top: 5, right: 5, left: -25, bottom: 5 }}
            >
              <CartesianGrid strokeDasharray="3 3" stroke="#1e2538" />
              <XAxis dataKey="name" stroke="#475569" fontSize={7.5} />
              <YAxis stroke="#475569" fontSize={7.5} />
              <RechartsTooltip contentStyle={{ backgroundColor: '#121620', borderColor: '#1e2538', fontSize: '7.5px' }} />
              <Legend wrapperStyle={{ fontSize: '7.5px' }} />
              <Bar dataKey="Baseline" fill="#475569" radius={[1, 1, 0, 0]} />
              <Bar dataKey="Sandbox" fill="#4f46e5" radius={[1, 1, 0, 0]} />
            </BarChart>
          </ResponsiveContainer>
        </div>
      </div>
    );
  };

  const handleSendChatMessage = (e) => {
    if (e) e.preventDefault();
    if (!chatInput.trim()) return;

    const userMsg = chatInput.trim();
    setCopilotMessages(prev => [
      ...prev,
      {
        sender: 'user',
        text: userMsg,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' })
      }
    ]);
    setChatInput('');

    // Simulated responses
    setTimeout(() => {
      let responseText = '';
      const lower = userMsg.toLowerCase();
      if (lower.includes('run') || lower.includes('mrp') || lower.includes('calculate') || lower.includes('重算')) {
        responseText = '⚡ 收到指令：正在通过 Web API 编译并触发底层连续内存 MRP 算子...\n重算成功，时间消耗: 18ms。DuckDB 内存表已重整！';
        handleRecompileOperators();
      } else if (lower.includes('reroute') || lower.includes('加急') || lower.includes('air')) {
        responseText = '⚡ 收到指令：核准空运加急。已触发 `/api/controltower/resolve` 终极算子。上海封装中继库存平滑。';
        executeResolveAction('REROUTE_AIR');
      } else if (lower.includes('overtime') || lower.includes('加班')) {
        responseText = '⚡ 收到指令：核准过载加班。工作中心效率向上微调，消减排产队列延迟。';
        executeResolveAction('OVERTIME_CAPACITY');
      } else if (lower.includes('upgrade') || lower.includes('升级')) {
        responseText = '⚡ 收到指令：执行规格芯片替代。消耗 512MB 替代缺口晶粒。';
        executeResolveAction('UPGRADE_CHIP');
      } else {
        responseText = '🤖 SCM 专家系统断言：当前网络运行健康度良好。您可以在下方直接点击“一键决策处方”进行 What-If 仿真演练。';
      }

      setCopilotMessages(prev => [
        ...prev,
        {
          sender: 'copilot',
          text: responseText,
          timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' })
        }
      ]);
    }, 800);
  };

  // Global Context state propagated via double-clicks / link drills
  const [contextBoundingBox, setContextBoundingBox] = useState(null);
  const [drilldownData, setDrilldownData] = useState([]);
  const [drilldownLoading, setDrilldownLoading] = useState(false);
  const [selectedNode, setSelectedNode] = useState('Shanghai Hub');

  // Engine calculation mode state
  const [engineMode, setEngineMode] = useState('all'); // 'all' (LBL+DBD), 'lbl' (LBL Only)

  // Sidebar Panel states
  const [isSidebarOpen, setIsSidebarOpen] = useState(true);
  const [activeSidebarTab, setActiveSidebarTab] = useState('tables'); // 'tables' or 'scenarios'
  const [searchTableQuery, setSearchTableQuery] = useState('');
  const [collapsedCategories, setCollapsedCategories] = useState({});

  // Right Click Context Menu state
  const [contextMenu, setContextMenu] = useState(null); // { x, y, tableName }

  // Describe Schema Modal state
  const [schemaModal, setSchemaModal] = useState(null); // { tableName, columns: [...], desc }

  // GIS / EKG map state
  const [timelineDay, setTimelineDay] = useState(8);
  const [activeResolutions, setActiveResolutions] = useState({});
  const [isResolving, setIsResolving] = useState(false);
  const [viewMode, setViewMode] = useState('ekg'); // 'gis' or 'ekg'
  const [hoveredEkgNode, setHoveredEkgNode] = useState(null);

  // Scenario Tree View padlock state
  const [scenarioLocks, setScenarioLocks] = useState({
    baseline: true,
    scenario_a: true,
    scenario_b: false
  });

  // Crosstab Matrix states with Proportional Spread data
  const [crosstabRecords, setCrosstabRecords] = useState([
    { id: 'parent_semi', name: 'Total Semiconductor', isParent: true, parentId: null, site: 'ALL_SITES', isProtected: false, d1: 300, d2: 300, d3: 300, d4: 300, t1: 900, w1: 1200, w2: 1200, w3: 1200, t2: 1000, m1: 4000, m2: 4000 },
    { id: 'part_512', name: '512MB Chip Component', isParent: false, parentId: 'parent_semi', site: 'CD_SLICING_FACTORY', isProtected: false, d1: 150, d2: 150, d3: 150, d4: 150, t1: 450, w1: 600, w2: 600, w3: 600, t2: 500, m1: 2000, m2: 2000 },
    { id: 'part_256', name: '256MB Chip Component (LOCKED CONTRACT)', isParent: false, parentId: 'parent_semi', site: 'SH_ASSEMBLY_HUB', isProtected: true, d1: 100, d2: 100, d3: 100, d4: 100, t1: 300, w1: 400, w2: 400, w3: 400, t2: 333, m1: 1333, m2: 1333 },
    { id: 'part_128', name: '128MB Chip Component', isParent: false, parentId: 'parent_semi', site: 'SZ_WAFER_SUPPLY', isProtected: false, d1: 50, d2: 50, d3: 50, d4: 50, t1: 150, w1: 200, w2: 200, w3: 200, t2: 167, m1: 667, m2: 667 },
    { id: 'parent_mem', name: 'Total Memory', isParent: true, parentId: null, site: 'ALL_SITES', isProtected: false, d1: 200, d2: 200, d3: 200, d4: 200, t1: 600, w1: 800, w2: 800, w3: 800, t2: 700, m1: 2700, m2: 2700 },
    { id: 'part_ram', name: 'RAM SCM Module', isParent: false, parentId: 'parent_mem', site: 'SH_ASSEMBLY_HUB', isProtected: false, d1: 120, d2: 120, d3: 120, d4: 120, t1: 360, w1: 480, w2: 480, w3: 480, t2: 420, m1: 1620, m2: 1620 },
    { id: 'part_cache', name: 'Cache SSD Module', isParent: false, parentId: 'parent_mem', site: 'CD_SLICING_FACTORY', isProtected: false, d1: 80, d2: 80, d3: 80, d4: 80, t1: 240, w1: 320, w2: 320, w3: 320, t2: 280, m1: 1080, m2: 1080 }
  ]);

  // Guardrail limit values
  const [simulatedRecordCount, setSimulatedRecordCount] = useState(15000);
  const [simulatedTabularRows, setSimulatedTabularRows] = useState(3500);
  const [showQbeForm, setShowQbeForm] = useState(false);
  const [qbeFilters, setQbeFilters] = useState({ site: 'ALL', category: 'ALL' });

  // Advanced UI Specification States
  const [activeTabularViewMode, setActiveTabularViewMode] = useState('table'); // 'table' or 'form'
  const [activeRecordIdx, setActiveRecordIdx] = useState(0);
  const [activeQbeFilterSet, setActiveQbeFilterSet] = useState('ALL');
  const [qbeRowFilters, setQbeRowFilters] = useState({});
  const [selectedCell, setSelectedCell] = useState(null); // { type, rowIdx, colId, value }
  const [freezeColumnsCount, setFreezeColumnsCount] = useState(0);
  const [hiddenColumns, setHiddenColumns] = useState(new Set());
  const [activeUOM, setActiveUOM] = useState('Pallet');
  const [activeCurrency, setActiveCurrency] = useState('CNY');
  const [hierarchyPath, setHierarchyPath] = useState('ROOT');
  const [siteFilter, setSiteFilter] = useState('ALL');
  const [showRangePanel, setShowRangePanel] = useState(false);
  const [rangeEditForm, setRangeEditForm] = useState({ op: 'MULTIPLY', val: 1.0 });

  const UOM_FACTORS = { Pallet: 1.0, Kilograms: 25.0, Units: 100.0 };
  const UOM_SUFFIXES = { Pallet: ' (Pallet)', Kilograms: ' (KG)', Units: ' (Units)' };
  const CURRENCY_FACTORS = { CNY: 1.0, USD: 0.14, EUR: 0.13 };
  const CURRENCY_SYMBOLS = { CNY: '¥', USD: '$', EUR: '€' };

  // Guardrail state logic
  const isCrosstabLimitBreached = simulatedRecordCount > 100000;
  const isTabularLimitBreached = simulatedTabularRows > 5000;
  const isEngineHalted = isCrosstabLimitBreached || isTabularLimitBreached;

  useEffect(() => {
    if (isEngineHalted) {
      setShowQbeForm(true);
    }
  }, [isEngineHalted]);

  // Close context menu on global click
  useEffect(() => {
    const handleGlobalClick = () => {
      setContextMenu(null);
    };
    window.addEventListener('click', handleGlobalClick);
    return () => {
      window.removeEventListener('click', handleGlobalClick);
    };
  }, []);

  // Map locations
  const nodes = [
    { name: 'Shenzhen Raw Material', coords: [22.5431, 114.0579], color: '#10b981', label: 'SZ_WAFER_SUPPLY', role: 'supplier' },
    { name: 'Chengdu Factory', coords: [30.5728, 104.0668], color: '#8b5cf6', label: 'CD_SLICING_FACTORY', role: 'factory' },
    { name: 'Shanghai Hub', coords: [31.2304, 121.4737], color: '#4f46e5', label: 'SH_ASSEMBLY_HUB', role: 'hub' },
    { name: 'Beijing Customer', coords: [39.9042, 116.4074], color: '#d97706', label: 'BJ_VVIP_CUSTOMER', role: 'customer' }
  ];
  const szToShCoords = [[22.5431, 114.0579], [31.2304, 121.4737]];
  const cdToShCoords = [[30.5728, 104.0668], [31.2304, 121.4737]];
  const shToBjCoords = [[31.2304, 121.4737], [39.9042, 116.4074]];

  // Load drilldown table from backend API
  const loadDrilldownInfo = async (nodeName) => {
    setDrilldownLoading(true);
    let keyword = 'Wafer';
    if (nodeName.includes('Beijing')) keyword = 'VVIP';
    else if (nodeName.includes('Shanghai')) keyword = '切片';
    else if (nodeName.includes('Chengdu')) keyword = '原料';
    
    try {
      const res = await fetch(`/api/drilldown?node=${encodeURIComponent(keyword)}`);
      const data = await res.json();
      setDrilldownData(data);
    } catch (err) {
      console.error('Error fetching drilldown:', err);
    } finally {
      setDrilldownLoading(false);
    }
  };

  useEffect(() => {
    loadDrilldownInfo(selectedNode);
  }, [selectedNode, currentScenario]);

  // Load dynamic table records from DuckDB generically
  const handleSelectTable = async (tableName) => {
    setDrilldownLoading(true);
    try {
      const res = await fetch(`/api/table?name=${tableName}`);
      const data = await res.json();
      if (data.error) {
        alert(`加载表格失败: ${data.error}`);
      } else {
        setDrilldownData(data);
        
        // Propagate context bounding box for the table
        const tableItem = ENGINE_TABLES.find(t => t.name === tableName);
        setContextBoundingBox({
          scenario: currentScenario,
          filterId: 'ALL_RECORDS',
          siteFilterId: 'ALL_SITES',
          dateBucket: 'All Periods',
          metricName: `Table: ${tableName} (${tableItem ? tableItem.label : '引擎数据表'})`,
          target: '-',
          current: '-',
          constraintState: 'BOUNDED'
        });

        // Automatically switch a companion panel to tabular view
        const ensureTabularVisible = (node) => {
          if (node.type === 'leaf') {
            if (node.view !== 'scorecard') {
              return { ...node, view: 'tabular' };
            }
            return node;
          }
          let updated = false;
          const newChildren = node.children.map(child => {
            if (!updated && child.type === 'leaf' && child.view !== 'scorecard') {
              updated = true;
              return { ...child, view: 'tabular' };
            }
            return child;
          });
          if (updated) return { ...node, children: newChildren };
          return {
            ...node,
            children: node.children.map(child => ensureTabularVisible(child))
          };
        };
        setLayout(prev => ensureTabularVisible(prev));
      }
    } catch (err) {
      console.error('Error fetching table records:', err);
    } finally {
      setDrilldownLoading(false);
    }
  };

  // Describe dynamic table columns schema from DuckDB metadata
  const handleDescribeTableSchema = async (tableName) => {
    try {
      const res = await fetch(`/api/table/schema?name=${tableName}`);
      const data = await res.json();
      if (data.error) {
        alert(`调阅元数据失败: ${data.error}`);
      } else {
        const tableItem = ENGINE_TABLES.find(t => t.name === tableName);
        setSchemaModal({
          tableName,
          columns: data,
          desc: tableItem ? tableItem.desc : 'o9 引擎连续内存运算实体数据表'
        });
      }
    } catch (err) {
      console.error('Error fetching schema:', err);
    }
  };

  // Re-run MRP / DBD Solver calculation via API
  const handleRecompileOperators = async () => {
    setIsResolving(true);
    try {
      const res = await fetch(`/api/run?step=${engineMode}`);
      const data = await res.json();
      
      const alertDiv = document.createElement('div');
      alertDiv.className = "fixed bottom-5 right-5 bg-gradient-to-r from-emerald-950 to-teal-950 border border-emerald-600 text-emerald-200 px-5 py-4 rounded shadow-2xl text-xs z-50 animate-bounce max-w-sm";
      const modeDesc = engineMode === 'lbl' ? '只跑 LBL (净需求下发 & 询单协同)' : '全功能 LBL + DBD 有限产能排程';
      alertDiv.innerHTML = `
        <div class="font-bold mb-1 text-emerald-300 flex items-center">
          <CheckCircle className="w-4 h-4 mr-2 text-emerald-400" />
          后端算子重算成功 (${engineMode === 'lbl' ? 'LBL Only' : 'LBL+DBD'})！
        </div>
        <div>计划计算模式: ${modeDesc} 运行成功，DOD 内存数组已完成更新。</div>
      `;
      document.body.appendChild(alertDiv);
      setTimeout(() => alertDiv.remove(), 5000);
      
      await fetchKpis();
    } catch (err) {
      console.error('Error re-running solver:', err);
    } finally {
      setIsResolving(false);
    }
  };

  // Export drilldownData or schema rows as CSV
  const handleExportCSV = (tableName) => {
    if (drilldownData.length === 0) {
      alert("⚠️ 表格数据为空或尚未加载，请左击表名称加载数据后再执行导出。");
      return;
    }
    try {
      const headers = Object.keys(drilldownData[0]);
      const csvRows = [];
      csvRows.push(headers.join(','));
      for (const row of drilldownData) {
        const values = headers.map(header => {
          const val = row[header];
          return `"${String(val).replace(/"/g, '""')}"`;
        });
        csvRows.push(values.join(','));
      }
      const csvString = csvRows.join('\n');
      const blob = new Blob([csvString], { type: 'text/csv;charset=utf-8;' });
      const link = document.createElement("a");
      const url = URL.createObjectURL(blob);
      link.setAttribute("href", url);
      link.setAttribute("download", `${tableName}_export.csv`);
      link.style.visibility = 'hidden';
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
    } catch (err) {
      console.error(err);
    }
  };

  // Right-click event registration
  const handleTableRightClick = (e, tableName) => {
    e.preventDefault();
    e.stopPropagation();
    setContextMenu({
      x: e.clientX,
      y: e.clientY,
      tableName
    });
  };

  // Execute resolutions on EKG/GIS Map alerts
  const executeResolveAction = async (actionName) => {
    if (isEngineHalted) return;
    setIsResolving(true);
    try {
      const res = await fetch('/api/controltower/resolve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: actionName })
      });
      const data = await res.json();
      
      if (data.status === 'success') {
        const isReset = actionName.startsWith('RESET_');
        const cleanAction = isReset ? actionName.replace('RESET_', '') : actionName;
        const actionLabel = cleanAction === 'REROUTE_AIR' ? '空运加急' : (cleanAction === 'OVERTIME_CAPACITY' ? '核准过载加班' : '升级芯片');

        setActiveResolutions(prev => {
          const updated = { ...prev };
          if (isReset) {
            delete updated[cleanAction];
          } else {
            updated[actionName] = true;
          }
          return updated;
        });

        // Log to Copilot
        logToCopilot(`⚡ 已执行处方决策: [${actionLabel}] -> ${isReset ? '已撤销' : '部署成功'}\n说明: ${data.message}`);

        // Bubble system alert
        const alertDiv = document.createElement('div');
        alertDiv.className = "fixed bottom-5 right-5 bg-gradient-to-r from-purple-950 to-indigo-950 border border-purple-600 text-purple-200 px-5 py-4 rounded shadow-2xl text-xs z-50 animate-bounce max-w-sm";
        alertDiv.innerHTML = `
          <div class="font-bold mb-1 text-purple-300 flex items-center">
            <span class="w-2 h-2 bg-emerald-500 rounded-full mr-2 animate-ping"></span>
            白盒沙盘重算成功！
          </div>
          <div>${data.message}</div>
        `;
        document.body.appendChild(alertDiv);
        setTimeout(() => alertDiv.remove(), 5000);

        await loadDrilldownInfo(selectedNode);
        await fetchKpis();
      } else {
        alert(`执行解耦重构方案失败: ${data.message}`);
      }
    } catch (err) {
      console.error('Error executing resolve:', err);
    } finally {
      setIsResolving(false);
    }
  };

  // Split Panel Action logic
  const handleSplitPanel = (path, orientation) => {
    const updateTree = (node, path) => {
      if (path.length === 0) {
        return {
          type: 'split',
          orientation: orientation,
          children: [
            { type: 'leaf', view: node.view, zoom: 1 },
            { type: 'leaf', view: 'help', zoom: 1 }
          ]
        };
      }
      const [head, ...tail] = path;
      const newChildren = [...node.children];
      newChildren[head] = updateTree(node.children[head], tail);
      return { ...node, children: newChildren };
    };
    setLayout(prev => updateTree(prev, path));
  };

  const handleClosePanePanel = (path) => {
    if (path.length === 0) return;
    const updateTree = (node, path) => {
      if (path.length === 1) {
        const siblingIndex = path[0] === 0 ? 1 : 0;
        return node.children[siblingIndex];
      }
      const [head, ...tail] = path;
      const newChildren = [...node.children];
      newChildren[head] = updateTree(node.children[head], tail);
      return { ...node, children: newChildren };
    };
    setLayout(prev => updateTree(prev, path));
  };

  const handleChangeViewPanel = (path, newView) => {
    const updateTree = (node, path) => {
      if (path.length === 0) {
        return { ...node, view: newView };
      }
      const [head, ...tail] = path;
      const newChildren = [...node.children];
      newChildren[head] = updateTree(node.children[head], tail);
      return { ...node, children: newChildren };
    };
    setLayout(prev => updateTree(prev, path));
  };

  const handleZoomPanel = (path, factor) => {
    const updateTree = (node, path) => {
      if (path.length === 0) {
        return { ...node, zoom: Math.max(0.5, Math.min(2.0, (node.zoom || 1) + factor)) };
      }
      const [head, ...tail] = path;
      const newChildren = [...node.children];
      newChildren[head] = updateTree(node.children[head], tail);
      return { ...node, children: newChildren };
    };
    setLayout(prev => updateTree(prev, path));
  };

  const handleZoomAllPanels = (factor) => {
    const updateTree = (node) => {
      if (node.type === 'leaf') {
        return { ...node, zoom: Math.max(0.5, Math.min(2.0, (node.zoom || 1) + factor)) };
      }
      return {
        ...node,
        children: node.children.map(child => updateTree(child))
      };
    };
    setLayout(prev => updateTree(prev));
  };

  // Toggle Scenario Tree padlock states
  const toggleLockState = (scCode, e) => {
    e.stopPropagation(); // Avoid switching scenario when clicking lock icon
    setScenarioLocks(prev => ({
      ...prev,
      [scCode]: !prev[scCode]
    }));
  };

  // Proportional disaggregation spread engine
  const handleSpreadMutationSpread = (rowId, colId, rawVal) => {
    if (isEngineHalted) return;
    if (isSpreadingRef.current) return;
    isSpreadingRef.current = true;

    try {
      const targetVal = parseFloat(rawVal);
      if (isNaN(targetVal)) return;

      const row = crosstabRecords.find(r => r.id === rowId);
      if (!row) return;

      if (row.isParent) {
        // Modify parent - trigger spread down
        const children = crosstabRecords.filter(c => c.parentId === rowId);
        const protectedRows = children.filter(c => c.isProtected);
        const unprotectedRows = children.filter(c => !c.isProtected);

        const sumProtected = protectedRows.reduce((sum, c) => sum + (c[colId] || 0), 0);
        const sumUnprotected = unprotectedRows.reduce((sum, c) => sum + (c[colId] || 0), 0);

        const remainToSpread = targetVal - sumProtected;
        if (remainToSpread < 0) {
          alert("🚨 错误: 分摊数值低于受合同保护记录的供需总额！");
          return;
        }

        const updated = crosstabRecords.map(r => {
          if (r.parentId === rowId) {
            if (r.isProtected) return r;
            const originalWeight = sumUnprotected > 0 ? (r[colId] || 0) / sumUnprotected : (1 / unprotectedRows.length);
            return {
              ...r,
              [colId]: Math.round(remainToSpread * originalWeight * 10) / 10,
              isDirty: true
            };
          }
          if (r.id === rowId) {
            return { ...r, [colId]: targetVal, isDirty: true };
          }
          return r;
        });
        setCrosstabRecords(updated);
      } else {
        // Modify child row
        const updated = crosstabRecords.map(r => {
          if (r.id === rowId) {
            return { ...r, [colId]: targetVal, isDirty: true };
          }
          return r;
        });

        // Update parent summary
        const parent = updated.find(p => p.id === row.parentId);
        if (parent) {
          const parentChildren = updated.filter(c => c.parentId === parent.id);
          const newParentSum = parentChildren.reduce((sum, c) => sum + (c[colId] || 0), 0);
          const finalRecords = updated.map(r => {
            if (r.id === parent.id) {
              return { ...r, [colId]: newParentSum, isDirty: true };
            }
            return r;
          });
          setCrosstabRecords(finalRecords);
        } else {
          setCrosstabRecords(updated);
        }
      }
    } finally {
      isSpreadingRef.current = false;
    }
  };

  // Apply QBE Filters to drop record counts below guardrails
  const applyQbeFilters = () => {
    setSimulatedRecordCount(15000); // Set to safe level below MaxSourceRecord
    setSimulatedTabularRows(3500);  // Set to safe level below MaxRow
    setShowQbeForm(false);
  };

  // Context Propagation Link handler (Scorecard metrics click)
  const onScorecardDrilldownClick = (metric, scenario, targetVal, currentVal) => {
    const boundingBox = {
      scenario: scenario || currentScenario,
      filterId: 'BJ_NORMAL_CUSTOMER',
      siteFilterId: selectedNode === 'Beijing Customer' ? 'ALL_SITES' : 'SH_ASSEMBLY_HUB',
      dateBucket: 'Week 1 (Transition)',
      metricName: metric,
      target: targetVal,
      current: currentVal,
      constraintState: currentVal < targetVal ? 'CRITICAL_BREACH' : 'BOUNDED'
    };
    setContextBoundingBox(boundingBox);
    loadDrilldownInfo(selectedNode);

    // Auto-propagate layout: switch companion pane view to 'tabular' worksheet detail
    const ensureTabularVisible = (node) => {
      if (node.type === 'leaf') {
        if (node.view !== 'scorecard') {
          return { ...node, view: 'tabular' };
        }
        return node;
      }
      let updated = false;
      const newChildren = node.children.map(child => {
        if (!updated && child.type === 'leaf' && child.view !== 'scorecard') {
          updated = true;
          return { ...child, view: 'tabular' };
        }
        return child;
      });
      if (updated) return { ...node, children: newChildren };
      
      return {
        ...node,
        children: node.children.map(child => ensureTabularVisible(child))
      };
    };
    setLayout(prev => ensureTabularVisible(prev));
  };

  // Dynamic statistics calculations from active cell data
  const getCrosstabStatsStats = () => {
    let vals = [];
    crosstabRecords.forEach(r => {
      if (!r.isParent) {
        const cols = ['d1', 'd2', 'd3', 'd4', 't1', 'w1', 'w2', 'w3', 't2', 'm1', 'm2'];
        cols.forEach(c => {
          if (r[c] !== undefined) vals.push(r[c]);
        });
      }
    });
    if (vals.length === 0) return { avg: 0, count: 0, min: 0, max: 0, sum: 0 };
    const sum = vals.reduce((s, v) => s + v, 0);
    const count = vals.length;
    const avg = sum / count;
    const min = Math.min(...vals);
    const max = Math.max(...vals);
    return {
      sum: Math.round(sum * 10) / 10,
      count,
      avg: Math.round(avg * 10) / 10,
      min,
      max
    };
  };
  const stats = useMemo(() => getCrosstabStatsStats(), [crosstabRecords]);

  const getColType = (colName) => {
    const name = colName.toLowerCase();
    if (name.includes('revenue') || name.includes('cost') || name.includes('price')) return 'Money';
    if (name.includes('qty') || name.includes('day') || name.includes('hour') || name.includes('priority') || name.includes('batches') || name.includes('lead_time') || name.includes('level') || name.includes('ratio') || name.includes('available_') || name.includes('consumed_') || name.includes('limit')) return 'Quantity';
    if (name.includes('date') || name.includes('time') || name.includes('day') && colName.toUpperCase().includes('DATE')) return 'DateTime';
    if (name.includes('is_') || name.includes('lock') || name.includes('round_to_')) return 'Boolean';
    if (name === '!' || name.includes('urgency')) return 'UrgencyMarker';
    return 'String';
  };

  const formatCellValue = (val, colName) => {
    if (val === undefined || val === null) return '';
    const type = getColType(colName);
    if (type === 'Quantity') {
      const scaled = parseFloat(val) * (UOM_FACTORS[activeUOM] || 1);
      if (isNaN(scaled)) return String(val);
      return (scaled % 1 === 0 ? scaled.toFixed(0) : scaled.toFixed(2));
    }
    if (type === 'Money') {
      const scaled = parseFloat(val) * (CURRENCY_FACTORS[activeCurrency] || 1);
      if (isNaN(scaled)) return String(val);
      const symbol = CURRENCY_SYMBOLS[activeCurrency] || '¥';
      return `${symbol}${scaled.toFixed(2)}`;
    }
    if (type === 'Boolean') {
      return val ? 'Y' : 'N';
    }
    return String(val);
  };

  const parseQbeFilter = (cellValue, queryText, isNumeric) => {
    if (!queryText) return true;
    let query = queryText.trim();
    
    // Delimiter protection
    if (isNumeric) {
      query = query.replace(/,/g, '');
    }

    if (query === '=!') {
      return cellValue === '!' || cellValue === true || cellValue === 1;
    }
    if (query === '!!') {
      return !cellValue || cellValue === '';
    }

    const parseDateOffset = (str) => {
      const today = new Date("2026-05-29");
      if (str.toLowerCase() === 'today') return today;
      if (str.toLowerCase() === 'past') return new Date("1970-01-01");
      if (str.toLowerCase() === 'future') return new Date("2100-01-01");
      
      const match = str.toLowerCase().match(/today\s*([+-])\s*(\d+)\s*workday/);
      if (match) {
        const sign = match[1] === '+' ? 1 : -1;
        const days = parseInt(match[2]);
        const resultDate = new Date(today);
        resultDate.setDate(resultDate.getDate() + sign * days);
        return resultDate;
      }
      return new Date(str);
    };

    if (query.includes('..')) {
      const parts = query.split('..');
      const start = parts[0].trim();
      const end = parts[1].trim();
      
      if (isNumeric) {
        const val = parseFloat(cellValue);
        const sVal = parseFloat(start);
        const eVal = parseFloat(end);
        return val >= sVal && val <= eVal;
      } else {
        const valDate = parseDateOffset(cellValue);
        const sDate = parseDateOffset(start);
        const eDate = parseDateOffset(end);
        return valDate >= sDate && valDate <= eDate;
      }
    }

    if (query.includes(',')) {
      const vector = query.split(',').map(item => item.trim().replace(/^['"]|['"]$/g, ''));
      return vector.some(v => String(cellValue).toLowerCase() === v.toLowerCase());
    }

    if (query.startsWith('=')) {
      const target = query.substring(1).trim().replace(/^['"]|['"]$/g, '');
      return String(cellValue).toLowerCase() === target.toLowerCase();
    }

    if (query.startsWith('<>')) {
      const target = query.substring(2).trim().replace(/^['"]|['"]$/g, '');
      return String(cellValue).toLowerCase() !== target.toLowerCase();
    }

    if (query.startsWith('!')) {
      const target = query.substring(1).trim().replace(/^['"]|['"]$/g, '');
      return !String(cellValue).toLowerCase().includes(target.toLowerCase());
    }

    if (query.toUpperCase().startsWith('LIKE')) {
      let target = query.substring(4).trim().replace(/^['"]|['"]$/g, '');
      const regexStr = '^' + target.replace(/\*/g, '.*').replace(/\?/g, '.') + '$';
      try {
        const regex = new RegExp(regexStr, 'i');
        return regex.test(String(cellValue));
      } catch(e) {
        return String(cellValue).toLowerCase().includes(target.toLowerCase());
      }
    }

    return String(cellValue).toLowerCase().includes(query.toLowerCase());
  };

  const filteredData = useMemo(() => {
    const rawData = drilldownData.length > 0 ? drilldownData : [
      { order_code: 'Order1', part_code: 'PART_512', site: 'CD_SLICING_FACTORY', qty: 1200, day: 8, priority: 1, type: 'VVIP Safety Stock Protected' },
      { order_code: 'Order2', part_code: 'PART_256', site: 'SH_ASSEMBLY_HUB', qty: 800, day: 15, priority: 2, type: 'Normal Sourcing Policy' },
      { order_code: 'Order3', part_code: 'PART_128', site: 'SZ_WAFER_SUPPLY', qty: 600, day: 22, priority: 3, type: 'Normal Alternative Match' },
      { order_code: 'Order4', part_code: 'PART_512', site: 'SH_ASSEMBLY_HUB', qty: 450, day: 8, priority: 1, type: 'VVIP Allocation' },
      { order_code: 'Order5', part_code: 'PART_256', site: 'CD_SLICING_FACTORY', qty: 1000, day: 10, priority: 2, type: 'Normal Sourcing Policy' }
    ];

    return rawData.filter(row => {
      // Apply QBE filters
      for (const col of Object.keys(row)) {
        const query = qbeRowFilters[col];
        if (query) {
          const type = getColType(col);
          const isNumeric = type === 'Quantity' || type === 'Money';
          if (!parseQbeFilter(row[col], query, isNumeric)) {
            return false;
          }
        }
      }
      // Apply global Site filter
      if (siteFilter !== 'ALL' && row.site && String(row.site).toLowerCase() !== siteFilter.toLowerCase()) {
        return false;
      }
      return true;
    });
  }, [drilldownData, qbeRowFilters, siteFilter]);



  // Menu action dispatcher
  const triggerMenuAction = async (action) => {
    const alertDiv = (title, message, isWarning = false) => {
      const div = document.createElement('div');
      div.className = `fixed bottom-5 right-5 z-50 animate-bounce max-w-sm px-5 py-4 rounded border shadow-2xl text-xs ${
        isWarning 
          ? 'bg-rose-950 border-rose-600 text-rose-200' 
          : 'bg-[#121620] border-[#1e2538] text-[#cbd5e1]'
      }`;
      div.innerHTML = `
        <div class="font-bold mb-1 flex items-center ${isWarning ? 'text-rose-400' : 'text-indigo-400'}">
          <span class="w-2 h-2 rounded-full mr-2 ${isWarning ? 'bg-rose-500' : 'bg-indigo-500'}"></span>
          ${title}
        </div>
        <div>${message}</div>
      `;
      document.body.appendChild(div);
      setTimeout(() => div.remove(), 4000);
    };

    switch (action) {
      case 'NEW_WORKBOOK':
        alertDiv("资源初始化", "新建工作簿成功，已加载 o9 Author 预设耗散结构环境。");
        break;
      case 'NEW_WORKSHEET':
        alertDiv("资源初始化", "新建工作表成功，内存二维网格重构完毕。");
        break;
      case 'NEW_FILTER':
        alertDiv("资源初始化", "新建过滤器模板就绪，QBE 语法编译器重置。");
        break;
      case 'SAVE_DATA':
        alertDiv("内存增量提交 (Save Data)", "已提交内存增量修改，重新编译底层连续内存算子...");
        await handleRecompileOperators();
        break;
      case 'PRINT':
        alertDiv("PDF 打印预览", `正在渲染 PDF 打印流... (AutoText: Page 1 of 1 | Company: o9 Antigravity | Scenario: ${currentScenario})`);
        window.print();
        break;
      case 'COPY':
        if (selectedCell) {
          const raw = String(selectedCell.value);
          navigator.clipboard.writeText(raw);
          alertDiv("剪贴板操作 (Copy)", `已复制原始数值: "${raw}"`);
        } else {
          alertDiv("剪贴板操作 (Copy)", "未选中任何单元格！", true);
        }
        break;
      case 'PASTE':
        try {
          const text = await navigator.clipboard.readText();
          if (selectedCell) {
            if (selectedCell.type === 'crosstab') {
              handleSpreadMutationSpread(selectedCell.rowId, selectedCell.colId, text);
              alertDiv("剪贴板操作 (Paste)", `已在 Crosstab 单元格粘贴数值: "${text}"，触发比例分摊`);
            } else if (selectedCell.type === 'tabular') {
              const updated = [...drilldownData];
              const row = { ...updated[selectedCell.rowIdx] };
              const type = getColType(selectedCell.colId);
              let val = text;
              if (type === 'Quantity' || type === 'Money') {
                const factor = type === 'Quantity' ? UOM_FACTORS[activeUOM] : CURRENCY_FACTORS[activeCurrency];
                val = parseFloat(text) / factor;
                if (isNaN(val)) val = text;
              }
              row[selectedCell.colId] = val;
              updated[selectedCell.rowIdx] = row;
              setDrilldownData(updated);
              alertDiv("剪贴板操作 (Paste)", `已在 Tabular 单元格粘贴并反算实际值: "${val}"`);
            }
          } else {
            alertDiv("剪贴板操作 (Paste)", "未选择目标单元格！", true);
          }
        } catch (err) {
          alertDiv("剪贴板操作 (Paste)", "无法读取系统剪贴板！", true);
        }
        break;
      case 'INSERT_RECORD':
        if (drilldownData.length > 0) {
          const template = { ...drilldownData[0] };
          Object.keys(template).forEach(k => {
            template[k] = getColType(k) === 'Quantity' || getColType(k) === 'Money' ? 0 : '';
          });
          setDrilldownData([template, ...drilldownData]);
          alertDiv("行级生存期 (Insert)", "成功插入一条新空白记录，处于内存编辑状态。");
        }
        break;
      case 'DELETE_RECORD':
        if (selectedCell && selectedCell.type === 'tabular') {
          const idx = selectedCell.rowIdx;
          const targetRecord = drilldownData[idx];
          const confirmDelete = window.confirm(`⚠️ [CASCADING DELETE WARNING]\n检测到删除记录将级联剥离 DuckDB 内存关联的 2 个下级工单及 5 条供需配额！\n是否强制执行物理级联删除？`);
          if (confirmDelete) {
            const updated = drilldownData.filter((_, i) => i !== idx);
            setDrilldownData(updated);
            setSelectedCell(null);
            alertDiv("行级生存期 (Delete)", "级联删除校验通过，已将记录物理剥离。");
          }
        } else {
          alertDiv("级联删除校验", "未选中任何 Tabular 行记录，无法删除！", true);
        }
        break;
      case 'COPY_TO_FILTER':
        if (selectedCell) {
          const val = String(selectedCell.value);
          setQbeRowFilters(prev => ({
            ...prev,
            [selectedCell.colId]: `=${val}`
          }));
          alertDiv("编译过滤器", `已将选中值 "${val}" 编译为静态列过滤器。`);
        } else {
          alertDiv("编译过滤器", "没有选中的列或行！", true);
        }
        break;
      case 'REFRESH':
        alertDiv("刷新计算", "已发起 DuckDB 连续内存大盘刷新...");
        await loadDrilldownInfo(selectedNode);
        await fetchKpis();
        break;
      default:
        break;
    }
  };

  const exportDataFormat = (format) => {
    if (drilldownData.length === 0) {
      alert("⚠️ 表格数据为空，请加载表格后再导出。");
      return;
    }
    const headers = Object.keys(drilldownData[0]);
    let content = '';
    let mimeType;
    let ext;
    
    if (format === 'TAB') {
      content = headers.join('\t') + '\n';
      drilldownData.forEach(row => {
        content += headers.map(h => String(row[h])).join('\t') + '\n';
      });
      ext = 'tab';
      mimeType = 'text/tab-separated-values;charset=utf-8;';
    } else if (format === 'XML') {
      content = '<?xml version="1.0" encoding="UTF-8"?>\n<worksheet>\n';
      drilldownData.forEach(row => {
        content += '  <record>\n';
        headers.forEach(h => {
          content += `    <${h}>${String(row[h]).replace(/&/g, '&amp;').replace(/</g, '&lt;')}</${h}>\n`;
        });
        content += '  </record>\n';
      });
      content += '</worksheet>';
      ext = 'xml';
      mimeType = 'application/xml;charset=utf-8;';
    } else if (format === 'PDF') {
      content = `%PDF-1.4\n1 0 obj\n<<\n/Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n2 0 obj\n<<\n/Type /Pages\n/Kids [3 0 R]\n/Count 1\n>>\nendobj\n3 0 obj\n<<\n/Type /Page\n/Parent 2 0 R\n/Resources << >>\n/Contents 4 0 R\n>>\nendobj\n4 0 obj\n<< /Length 100 >>\nstream\nBT\n/F1 12 Tf\n50 700 Td\n(o9 Control Tower Export PDF) Tj\n0 -20 Td\n(Scenario: ${currentScenario}) Tj\n0 -20 Td\n(Rows count: ${drilldownData.length}) Tj\nET\nendstream\nendobj\nxref\n0 5\n0000000000 65535 f\n0000000009 00000 n\n0000000058 00000 n\n0000000115 00000 n\n0000000194 00000 n\ntrailer\n<<\n/Size 5\n/Root 1 0 R\n>>\nstartxref\n345\n%%EOF`;
      ext = 'pdf';
      mimeType = 'application/pdf;';
    } else {
      content = '<table><thead><tr>' + headers.map(h => `<th>${h}</th>`).join('') + '</tr></thead><tbody>';
      drilldownData.forEach(row => {
        content += '<tr>' + headers.map(h => `<td>${row[h]}</td>`).join('') + '</tr>';
      });
      content += '</tbody></table>';
      ext = 'xls';
      mimeType = 'application/vnd.ms-excel;charset=utf-8;';
    }
    
    const blob = new Blob([content], { type: mimeType });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = `ct_export_${Date.now()}.${ext}`;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  };

  const toggleTabularViewMode = () => {
    setActiveTabularViewMode(prev => prev === 'table' ? 'form' : 'table');
  };

  const toggleFreezeColumn = () => {
    if (selectedCell && selectedCell.colId) {
      if (selectedCell.type === 'tabular') {
        const idx = Object.keys(drilldownData[0] || {}).indexOf(selectedCell.colId);
        if (idx !== -1) {
          setFreezeColumnsCount(idx + 1);
          return;
        }
      }
    }
    setFreezeColumnsCount(prev => prev > 0 ? 0 : 2);
  };

  // Keyboard shortcut listener
  useEffect(() => {
    const handleShortcuts = (e) => {
      const tagName = document.activeElement.tagName;
      if (tagName === 'INPUT' || tagName === 'TEXTAREA') {
        if (e.ctrlKey && e.key.toLowerCase() === 's') {
          e.preventDefault();
          triggerMenuAction('SAVE_DATA');
        }
        return;
      }
      
      if (e.ctrlKey && e.key.toLowerCase() === 's') {
        e.preventDefault();
        triggerMenuAction('SAVE_DATA');
      } else if (e.ctrlKey && e.key.toLowerCase() === 'c') {
        e.preventDefault();
        triggerMenuAction('COPY');
      } else if (e.ctrlKey && e.key.toLowerCase() === 'v') {
        e.preventDefault();
        triggerMenuAction('PASTE');
      } else if (e.ctrlKey && e.key.toLowerCase() === 'r') {
        e.preventDefault();
        toggleFreezeColumn();
      } else if (e.key === 'Insert') {
        e.preventDefault();
        triggerMenuAction('INSERT_RECORD');
      } else if (e.key === 'Delete') {
        e.preventDefault();
        triggerMenuAction('DELETE_RECORD');
      }
    };
    window.addEventListener('keydown', handleShortcuts);
    return () => window.removeEventListener('keydown', handleShortcuts);
  }, [drilldownData, activeTabularViewMode, selectedCell]);

  const resetWorkbookConfig = () => {
    setFreezeColumnsCount(0);
    setHiddenColumns(new Set());
    setQbeRowFilters({});
    setActiveUOM('Pallet');
    setActiveCurrency('CNY');
    setHierarchyPath('ROOT');
    setSiteFilter('ALL');
    setActiveQbeFilterSet('ALL');
  };

  const toggleUnhideColumns = () => {
    if (selectedCell && selectedCell.colId) {
      setHiddenColumns(prev => {
        const next = new Set(prev);
        if (next.has(selectedCell.colId)) {
          next.delete(selectedCell.colId);
        } else {
          const totalCols = Object.keys(drilldownData[0] || {}).length;
          if (next.size + 1 < totalCols) {
            next.add(selectedCell.colId);
          } else {
            alert("⚠️ 安全熔断：工作表必须至少留有一列可见！");
          }
        }
        return next;
      });
    } else {
      setHiddenColumns(new Set());
    }
  };

  const triggerSort = (direction) => {
    if (activeTabularViewMode === 'form') {
      alert("⚠️ Form View 状态下自动禁用快速排序！");
      return;
    }
    if (selectedCell && selectedCell.colId) {
      const col = selectedCell.colId;
      const sorted = [...drilldownData].sort((a, b) => {
        const type = getColType(col);
        const aVal = a[col];
        const bVal = b[col];
        if (type === 'Quantity' || type === 'Money') {
          return direction === 'ASC' ? parseFloat(aVal) - parseFloat(bVal) : parseFloat(bVal) - parseFloat(aVal);
        }
        return direction === 'ASC' 
          ? String(aVal).localeCompare(String(bVal)) 
          : String(bVal).localeCompare(String(aVal));
      });
      setDrilldownData(sorted);
    } else {
      alert("⚠️ 请先单击选中欲排序目标列的任意单元格。");
    }
  };

  const calculateAutoStats = () => {
    if (selectedCell) {
      alert(`📊 自动统计选中单元格:\n列: ${selectedCell.colId}\n数值: ${selectedCell.value}`);
    } else {
      alert(`📊 自动大盘数据统计:\n数量 Average: ${stats.avg}\n数量 Sum: ${stats.sum}\n选中 Count: ${stats.count}`);
    }
  };

  const applyPresetFilter = (preset) => {
    setActiveQbeFilterSet(preset);
    if (preset === 'ALL') {
      setQbeRowFilters({});
    } else if (preset === 'HIGH_QTY') {
      const filterableCol = Object.keys(drilldownData[0] || {}).find(k => getColType(k) === 'Quantity');
      if (filterableCol) {
        setQbeRowFilters({ [filterableCol]: '1000..150000' });
      }
    } else if (preset === 'SHORTAGE') {
      const filterableCol = Object.keys(drilldownData[0] || {}).find(k => k.toLowerCase().includes('shortage') || k.toLowerCase().includes('avail'));
      if (filterableCol) {
        setQbeRowFilters({ [filterableCol]: '<>0' });
      }
    }
  };

  const changeUOMSelector = (uom) => {
    setActiveUOM(uom);
  };

  const changeCurrencySelector = (cur) => {
    setActiveCurrency(cur);
  };

  const applyRangeEditing = (op, valStr) => {
    const val = parseFloat(valStr);
    if (isNaN(val)) return;
    if (selectedCell && selectedCell.colId) {
      const col = selectedCell.colId;
      const type = getColType(col);
      if (type !== 'Quantity' && type !== 'Money') {
        alert("⚠️ 只能对数值或金额类型列进行区间数学运算！");
        return;
      }
      
      const updated = drilldownData.map(row => {
        let current = parseFloat(row[col]) || 0.0;
        if (op === 'ADD') current += val;
        else if (op === 'SUB') current -= val;
        else if (op === 'MULTIPLY') current *= val;
        else if (op === 'DIVIDE') {
          if (val === 0) current = NaN;
          else current /= val;
        }
        return {
          ...row,
          [col]: isNaN(current) ? 'NaN' : current
        };
      });
      setDrilldownData(updated);
      setShowRangePanel(false);
    } else {
      alert("⚠️ 请先选择需要进行区间运算的目标列！");
    }
  };

  // Left Sidebar Category collapsible toggle
  const toggleCategoryCollapse = (catName) => {
    setCollapsedCategories(prev => ({
      ...prev,
      [catName]: !prev[catName]
    }));
  };

  // --- Left Explorer Sidebar View ---
  const renderTablesSidebar = () => {
    // Filter categories and tables based on query
    const filteredTables = ENGINE_TABLES.filter(t => 
      t.name.toLowerCase().includes(searchTableQuery.toLowerCase()) || 
      t.label.toLowerCase().includes(searchTableQuery.toLowerCase()) || 
      t.desc.toLowerCase().includes(searchTableQuery.toLowerCase())
    );

    // Group tables by categories
    const categories = ['物料与主数据', '需求与计划', '计划工单与匹配', '生产与排产', '联副产品', '协同与大盘'];

    return (
      <div className="flex-1 flex flex-col overflow-hidden min-h-0 space-y-2">
        {/* Table Search Input */}
        <div className="relative">
          <input 
            type="text" 
            placeholder="搜索引擎数据表..." 
            value={searchTableQuery}
            onChange={(e) => setSearchTableQuery(e.target.value)}
            className="w-full bg-[#090b10] border border-[#1e2538] text-heading rounded px-2.5 py-1.5 pl-8 text-[9px] outline-none focus:border-indigo-500 placeholder-muted"
          />
          <Search className="w-3.5 h-3.5 text-muted absolute left-2.5 top-2.2" />
        </div>

        {/* Scrollable Tables List */}
        <div className="flex-1 overflow-y-auto space-y-2 custom-fine-scrollbar pr-1 select-none">
          {categories.map(cat => {
            const catTables = filteredTables.filter(t => t.category === cat);
            if (catTables.length === 0) return null;
            const isCollapsed = collapsedCategories[cat];

            return (
              <div key={cat} className="space-y-1">
                {/* Category Header */}
                <button 
                  onClick={() => toggleCategoryCollapse(cat)}
                  className="w-full flex items-center justify-between py-1 px-1.5 bg-[#161c28]/50 hover:bg-[#161c28] border border-[#1e2538]/40 rounded text-[9.5px] font-bold text-heading"
                >
                  <span className="flex items-center space-x-1.5">
                    {isCollapsed ? <ChevronRight className="w-3.5 h-3.5 text-indigo-400" /> : <ChevronDown className="w-3.5 h-3.5 text-indigo-400" />}
                    <span>{cat}</span>
                  </span>
                  <span className="text-[8px] bg-slate-900 border border-[#1e2538] px-1 rounded text-muted font-normal">{catTables.length}</span>
                </button>

                {/* Category Body tables list */}
                {!isCollapsed && (
                  <div className="pl-2 space-y-0.5">
                    {catTables.map(t => (
                      <div 
                        key={t.name}
                        onClick={() => handleSelectTable(t.name)}
                        onContextMenu={(e) => handleTableRightClick(e, t.name)}
                        className="flex items-center justify-between p-1.5 rounded hover:bg-[#1c2335]/40 cursor-pointer border border-transparent hover:border-[#1e2538]/50 group transition-all"
                        title={`${t.label}\n${t.desc}\n双击右键：查看相关算子操作`}
                      >
                        <div className="flex items-center space-x-1.5 min-w-0">
                          <Database className="w-3.5 h-3.5 text-indigo-500 flex-shrink-0 group-hover:text-indigo-400" />
                          <div className="flex flex-col min-w-0">
                            <span className="text-[9.5px] font-mono text-[#cbd5e1] font-bold truncate">{t.name}</span>
                            <span className="text-[8px] text-muted truncate">{t.label}</span>
                          </div>
                        </div>
                        <ChevronRight className="w-3 h-3 text-muted/30 group-hover:text-indigo-400 opacity-0 group-hover:opacity-100 transition-all" />
                      </div>
                    ))}
                  </div>
                )}
              </div>
            );
          })}
        </div>
      </div>
    );
  };

  const renderScenariosSidebar = () => {
    const handleCreateNewSandbox = () => {
      const code = prompt("请输入新 What-If 沙箱标识 (仅限英文字母/数字/下划线, 如 sandbox_overtime):", `sandbox_${Date.now().toString().slice(-4)}`);
      if (!code) return;
      const name = prompt("请输入新沙箱描述名称:", "高频并发What-If演练沙箱");
      if (!name) return;
      createScenario(code.trim(), name.trim());
    };

    return (
      <div className="flex-1 flex flex-col h-full overflow-hidden select-none">
        
        {/* 沙箱操作按钮组 */}
        <div className="p-2 border-b border-[#1e2538] bg-[#161c28]/60 flex flex-col space-y-1.5">
          <button
            onClick={handleCreateNewSandbox}
            className="w-full py-1.5 px-2 bg-indigo-600 hover:bg-indigo-500 text-white rounded text-[10px] font-bold transition-all flex items-center justify-center cursor-pointer shadow"
          >
            <FolderOpen className="w-3.5 h-3.5 mr-1" />
            + 新建 What-If 仿真沙箱
          </button>

          {currentScenario !== 'baseline' && (
            <button
              onClick={() => {
                if (window.confirm(`⚠️ 确认将沙箱 [${currentScenario.toUpperCase()}] 的 DuckDB 数据库文件物理原子覆盖覆盖至生产主库 (ipc.db)？`)) {
                  mergeScenario(currentScenario);
                }
              }}
              className="w-full py-1.5 px-2 bg-emerald-600 hover:bg-emerald-500 text-white rounded text-[10px] font-bold transition-all flex items-center justify-center cursor-pointer shadow animate-pulse"
            >
              <CheckCircle className="w-3.5 h-3.5 mr-1" />
              一键合并覆盖生产主库 (Merge to Baseline)
            </button>
          )}
        </div>

        {/* 动态沙箱场景列表 */}
        <div className="flex-1 overflow-y-auto custom-fine-scrollbar p-2 space-y-2">
          {scenarios.map((sc, idx) => {
            const isActive = currentScenario === sc.scenario_code;
            const isBaseline = sc.scenario_code === 'baseline';

            return (
              <div 
                key={sc.scenario_code}
                onClick={() => switchScenario(sc.scenario_code)}
                className={`p-2 rounded border cursor-pointer transition-all flex justify-between items-center ${
                  isActive 
                    ? 'bg-indigo-950/60 border-indigo-500 text-indigo-200 shadow-md' 
                    : 'bg-[#090b10] border-[#1e2538] text-muted hover:border-slate-700 hover:text-heading'
                }`}
              >
                <div className="flex items-center space-x-2 min-w-0">
                  <div className={`w-6 h-6 rounded-full flex items-center justify-center flex-shrink-0 text-xs ${
                    isBaseline ? 'bg-amber-950/80 text-amber-400 border border-amber-800' : 'bg-indigo-900/60 text-indigo-300 border border-indigo-700'
                  }`}>
                    {isBaseline ? '★' : `#${idx}`}
                  </div>
                  <div className="flex flex-col min-w-0">
                    <span className="font-bold text-[10px] text-heading truncate">{sc.scenario_name || sc.scenario_code}</span>
                    <span className="text-[8px] font-mono text-muted truncate">DB: sandbox_{sc.scenario_code}.db</span>
                  </div>
                </div>

                <div className="flex items-center space-x-1">
                  {isActive && <span className="text-[8px] bg-indigo-600 text-white px-1.5 py-0.2 rounded font-bold">Active</span>}
                  {!isBaseline && (
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        if (window.confirm(`确认删除沙箱 [${sc.scenario_code}] 物理库？`)) {
                          deleteScenario(sc.scenario_code);
                        }
                      }}
                      className="text-[9px] text-rose-400 hover:text-rose-300 p-0.5"
                      title="删除沙箱"
                    >
                      ✕
                    </button>
                  )}
                </div>
              </div>
            );
          })}
        </div>
      </div>
    );
  };

  // --- Workspace Split Layout Renderer ---
  const renderLayout = (node, path) => {
    if (!node) return null;

    if (node.type === 'split') {
      const isHorizontal = node.orientation === 'horizontal';
      return (
        <div className={`flex ${isHorizontal ? 'flex-row' : 'flex-col'} w-full h-full gap-2.5 overflow-hidden`}>
          {node.children.map((child, idx) => (
            <div key={idx} className="flex-1 min-w-0 min-h-0 relative">
              {renderLayout(child, [...path, idx])}
            </div>
          ))}
        </div>
      );
    }

    if (node.type === 'leaf') {
      const viewContent = (() => {
        switch (node.view) {
          case 'ekg':
            return renderEkgView();
          case 'scorecard':
            return renderScorecardMatrixView();
          case 'crosstab':
            return renderCrosstabView();
          case 'tabular':
            return renderTabularView();
          case 'scenarios':
            return renderScenarioTreeView();
          case 'help':
          default:
            return renderHelpView();
        }
      })();

      const isEkg = node.view === 'ekg';
      const zoomFactor = isEkg ? 1 : (node.zoom || 1);

      return (
        <div className="w-full h-full bg-[#121620] border border-[#1e2538] rounded flex flex-col overflow-hidden shadow-lg hover:border-indigo-500/30 transition-all duration-300 relative">
          {/* Panel Toolbar */}
          <div className="flex items-center justify-between px-3 py-1.5 bg-[#161c28] border-b border-[#1e2538] text-[9.5px] select-none">
            <div className="flex items-center space-x-2">
              <select
                value={node.view}
                onChange={(e) => handleChangeViewPanel(path, e.target.value)}
                className="bg-[#090b10] border border-[#1e2538] text-[#cbd5e1] text-[9.5px] px-1.5 py-0.5 rounded outline-none focus:border-indigo-500 cursor-pointer font-bold"
              >
                <option value="ekg">🗺️ o9 EKG 拓扑地图</option>
                <option value="scorecard">📊 差异对比矩阵</option>
                <option value="crosstab">🧮 时序交叉矩阵</option>
                <option value="tabular">📋 详细数据工作表</option>
                <option value="scenarios">🌲 增量场景拓扑树</option>
                <option value="help">📖 求解器公理说明书</option>
              </select>
              {!isEkg && (
                <span className="text-muted font-mono text-[8.5px]">Zoom: {Math.round(zoomFactor * 100)}%</span>
              )}
            </div>

            <div className="flex items-center space-x-1">
              {!isEkg && (
                <>
                  <button
                    onClick={() => handleZoomPanel(path, 0.1)}
                    className="p-1 text-muted hover:text-[#f8fafc] hover:bg-[#121620] rounded transition-all"
                    title="放大 (Zoom In)"
                  >
                    <ZoomIn className="w-3.5 h-3.5" />
                  </button>
                  <button
                    onClick={() => handleZoomPanel(path, -0.1)}
                    className="p-1 text-muted hover:text-[#f8fafc] hover:bg-[#121620] rounded transition-all"
                    title="缩小 (Zoom Out)"
                  >
                    <ZoomOut className="w-3.5 h-3.5" />
                  </button>
                </>
              )}
              <button
                onClick={() => handleSplitPanel(path, 'horizontal')}
                className="p-1 text-muted hover:text-[#f8fafc] hover:bg-[#121620] rounded transition-all"
                title="左右分屏"
              >
                <Columns className="w-3.5 h-3.5" />
              </button>
              <button
                onClick={() => handleSplitPanel(path, 'vertical')}
                className="p-1 text-muted hover:text-[#f8fafc] hover:bg-[#121620] rounded transition-all"
                title="上下分屏"
              >
                <Rows className="w-3.5 h-3.5" />
              </button>
              {path.length > 0 && (
                <button
                  onClick={() => handleClosePanePanel(path)}
                  className="p-1 text-rose-400 hover:text-rose-300 hover:bg-rose-950/20 rounded ml-1 border border-transparent hover:border-rose-900/40 transition-all"
                  title="关闭窗格"
                >
                  <X className="w-3.5 h-3.5" />
                </button>
              )}
            </div>
          </div>

          {/* Panel Content Area */}
          <div className="flex-1 overflow-auto min-h-0 relative custom-fine-scrollbar">
            <div
              style={{
                transform: `scale(${zoomFactor})`,
                transformOrigin: 'top left',
                width: `${100 / zoomFactor}%`,
                height: `${100 / zoomFactor}%`
              }}
              className="absolute inset-0"
            >
              {viewContent}
            </div>
          </div>
        </div>
      );
    }

    return null;
  };

  // --- Sub-View Renders ---

  // 1. EKG Topology & Map View
  const renderEkgView = () => {
    return (
      <div className="w-full h-full flex flex-col p-2.5 space-y-2.5 overflow-hidden">
        <div className="flex justify-between items-center border-b border-[#1e2538] pb-1.5 select-none">
          <div className="text-[10px] font-bold text-heading flex items-center">
            <Map className="w-3.5 h-3.5 text-indigo-500 mr-1" />
            Supply Chain GIS Map & EKG Graph
          </div>
          <div className="flex space-x-1 bg-[#090b10] border border-[#1e2538] p-0.5 rounded text-[8.5px]">
            <button 
              onClick={() => setViewMode('gis')} 
              className={`px-2 py-0.5 rounded transition-all font-bold ${viewMode === 'gis' ? 'bg-indigo-600 text-white' : 'text-muted'}`}
            >
              GIS 地面映射
            </button>
            <button 
              onClick={() => setViewMode('ekg')} 
              className={`px-2 py-0.5 rounded transition-all font-bold ${viewMode === 'ekg' ? 'bg-indigo-600 text-white' : 'text-muted'}`}
            >
              o9 EKG 拓扑
            </button>
          </div>
        </div>

        <div className="flex-1 border border-[#1e2538] rounded overflow-hidden relative bg-[#090b10] min-h-[160px] z-0">
          {viewMode === 'gis' ? (
            <MapContainer 
              center={[31.2304, 114.0579]} 
              zoom={4} 
              zoomControl={false}
              style={{ height: '100%', width: '100%' }}
            >
              <TileLayer
                attribution='&copy; CARTO'
                url="https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png"
              />
              {nodes.map(n => (
                <Marker
                  key={n.name}
                  position={n.coords}
                  icon={createCustomIcon(n.color, n.label)}
                  eventHandlers={{
                    click: () => setSelectedNode(n.name)
                  }}
                />
              ))}
              <Polyline
                positions={szToShCoords}
                color={activeResolutions['REROUTE_AIR'] ? '#10b981' : '#d97706'}
                weight={2}
                dashArray={activeResolutions['REROUTE_AIR'] ? "3, 3" : "6, 6"}
              />
              <Polyline positions={cdToShCoords} color="#8b5cf6" weight={1.5} />
              <Polyline positions={shToBjCoords} color="#4f46e5" weight={2} />
            </MapContainer>
          ) : (
            <div className="w-full h-full relative flex items-center justify-between px-6 bg-radial from-[#0c101a] to-[#05060b] min-h-[150px]">
              {/* EKG Nodes connection SVG layer */}
              <svg className="absolute inset-0 w-full h-full pointer-events-none z-10">
                <path d="M 80 80 L 160 80" className={`ekg-flow-line ${activeResolutions['REROUTE_AIR'] ? 'ekg-flow-vvip' : 'ekg-flow-boundary'}`} style={{ strokeWidth: 1.5 }} />
                <path d="M 220 80 L 320 80" className={`ekg-flow-line ${activeResolutions['OVERTIME_CAPACITY'] ? 'ekg-flow-vvip' : 'ekg-flow-active'}`} style={{ strokeWidth: 2 }} />
                <path d="M 380 80 Q 420 50 480 40" className="ekg-flow-line ekg-flow-active" style={{ strokeWidth: 2 }} />
                <path d="M 380 80 Q 420 110 480 120" className={`ekg-flow-line ${activeResolutions['UPGRADE_CHIP'] ? 'ekg-flow-vvip' : 'ekg-flow-decouple'}`} style={{ strokeWidth: 1.5 }} />
              </svg>

              {/* SZ supplier */}
              <div 
                onClick={() => setSelectedNode('Shenzhen Raw Material')}
                className={`w-28 bg-[#121620]/90 border rounded p-1.5 z-20 cursor-pointer text-left transition-all hover:scale-102 ${
                  selectedNode === 'Shenzhen Raw Material' ? 'border-[#10b981] shadow' : 'border-[#1e2538]'
                }`}
              >
                <div className="text-[8px] text-[#10b981] font-bold">RAW SUPPLY</div>
                <div className="text-[10px] font-bold text-heading truncate">SZ_WAFER_SUPPLY</div>
              </div>

              {/* CD factory */}
              <div 
                onClick={() => setSelectedNode('Chengdu Factory')}
                className={`w-28 bg-[#121620]/90 border rounded p-1.5 z-20 cursor-pointer text-left transition-all hover:scale-102 ${
                  selectedNode === 'Chengdu Factory' ? 'border-[#8b5cf6] shadow' : 'border-[#1e2538]'
                }`}
              >
                <div className="text-[8px] text-[#8b5cf6] font-bold">FACTORY</div>
                <div className="text-[10px] font-bold text-heading truncate">CD_SLICING_FAC</div>
              </div>

              {/* SH Hub */}
              <div 
                onClick={() => setSelectedNode('Shanghai Hub')}
                className={`w-28 bg-[#121620]/90 border rounded p-1.5 z-20 cursor-pointer text-left transition-all hover:scale-102 ${
                  selectedNode === 'Shanghai Hub' ? 'border-[#4f46e5] shadow' : 'border-[#1e2538]'
                }`}
              >
                <div className="text-[8px] text-[#4f46e5] font-bold">DECOUPLING</div>
                <div className="text-[10px] font-bold text-heading truncate">SH_ASSEMBLY_HUB</div>
              </div>

              {/* BJ client */}
              <div 
                onClick={() => setSelectedNode('Beijing Customer')}
                className={`w-28 bg-[#121620]/90 border rounded p-1.5 z-20 cursor-pointer text-left transition-all hover:scale-102 ${
                  selectedNode === 'Beijing Customer' ? 'border-[#d97706] shadow' : 'border-[#1e2538]'
                }`}
              >
                <div className="text-[8px] text-[#d97706] font-bold">VVIP CUSTOMER</div>
                <div className="text-[10px] font-bold text-heading truncate">BJ_VVIP_CUSTOMER</div>
              </div>
            </div>
          )}
        </div>
        <div className="flex items-center justify-between text-[8px] text-muted font-mono select-none px-1 border-t border-[#1e2538]/30 pt-1.5 flex-shrink-0">
          <span>Active Nodes: 4 | Links: 3 | Topology Status: SECURE</span>
          <span className="flex items-center"><span className="w-1.5 h-1.5 bg-emerald-500 rounded-full mr-1 animate-pulse"></span>DuckDB Connected</span>
        </div>
      </div>
    );
  };

  // 2. Scenario Tree view with incremental delta tracking and padlocks
  const renderScenarioTreeView = () => {
    return (
      <div className="w-full h-full p-3 flex flex-col space-y-3">
        <div className="border-b border-[#1e2538] pb-1.5 flex items-center justify-between">
          <div className="text-[10px] font-bold text-heading flex items-center">
            <FolderOpen className="w-4 h-4 text-indigo-500 mr-1.5" />
            增量拓扑场景树 (Scenario Tree Delta Topology)
          </div>
        </div>

        <div className="flex-1 bg-[#090b10]/60 border border-[#1e2538] rounded p-4 flex flex-col space-y-4">
          
          {/* Node 1: Root */}
          <div className="flex items-start space-x-3">
            <div className="flex flex-col items-center">
              <div 
                onClick={() => switchScenario('baseline')}
                className={`w-8 h-8 rounded-full flex items-center justify-center cursor-pointer transition-all border ${
                  currentScenario === 'baseline' ? 'bg-indigo-950 border-indigo-500 text-indigo-400 shadow' : 'bg-[#121620] border-[#1e2538] text-muted'
                }`}
              >
                <Database className="w-4 h-4" />
              </div>
              <div className="w-0.5 h-10 bg-[#1e2538]"></div>
            </div>
            
            <div className="flex-1 bg-[#121620]/80 border border-[#1e2538] p-2 rounded text-[10px] flex justify-between items-center">
              <div>
                <h5 className="font-bold text-[#f8fafc] flex items-center">
                  根节点: Enterprise Data Mirror (Approved Baseline)
                  <Lock className="w-3 h-3 text-rose-500 ml-1.5 cursor-pointer" title="ERP 审批刚性锁 - 全局只读" />
                </h5>
                <p className="text-[8.5px] text-muted mt-0.5">主数据集成数据镜像流控层，锁定不可改</p>
              </div>
              <div className="text-right">
                <span className="text-[9px] bg-emerald-950/60 text-emerald-400 border border-emerald-900/50 px-1.5 py-0.5 rounded font-mono font-bold">Delta: 0.00%</span>
                <div className="text-[8px] text-muted mt-1">只读主线</div>
              </div>
            </div>
          </div>

          {/* Node 2: Scenario A */}
          <div className="flex items-start space-x-3 pl-8">
            <div className="flex flex-col items-center">
              <div 
                onClick={() => switchScenario('scenario_a')}
                className={`w-8 h-8 rounded-full flex items-center justify-center cursor-pointer transition-all border ${
                  currentScenario === 'scenario_a' ? 'bg-indigo-950 border-indigo-500 text-indigo-400 shadow' : 'bg-[#121620] border-[#1e2538] text-muted'
                }`}
              >
                <Key className="w-4 h-4 text-amber-500" />
              </div>
              <div className="w-0.5 h-16 bg-[#1e2538]"></div>
            </div>
            
            <div className="flex-1 bg-[#121620]/80 border border-[#1e2538] p-2 rounded text-[10px] flex justify-between items-center">
              <div>
                <h5 className="font-bold text-[#f8fafc] flex items-center">
                  中继层: High Demand Simulation (Scenario A)
                  <span onClick={(e) => toggleLockState('scenario_a', e)} className="ml-1.5 cursor-pointer">
                    {scenarioLocks.scenario_a ? (
                      <Lock className="w-3 h-3 text-rose-500" title="已加锁" />
                    ) : (
                      <Unlock className="w-3 h-3 text-emerald-400" title="未加锁" />
                    )}
                  </span>
                </h5>
                <p className="text-[8.5px] text-muted mt-0.5">高需求与产能瓶颈沙盒。未变动行由指针向上穿透继承</p>
              </div>
              <div className="text-right">
                <span className="text-[9px] bg-indigo-950/60 text-indigo-400 border border-indigo-900/50 px-1.5 py-0.5 rounded font-mono font-bold">Delta: 15.4%</span>
                <div className="text-[8px] text-muted mt-1">DOD 增量继承</div>
              </div>
            </div>
          </div>

          {/* Node 3: Scenario B */}
          <div className="flex items-start space-x-3 pl-16">
            <div className="flex flex-col items-center">
              <div 
                onClick={() => switchScenario('scenario_b')}
                className={`w-8 h-8 rounded-full flex items-center justify-center cursor-pointer transition-all border ${
                  currentScenario === 'scenario_b' ? 'bg-indigo-950 border-indigo-500 text-indigo-400 shadow-lg' : 'bg-[#121620] border-[#1e2538] text-muted'
                }`}
              >
                <Unlock className="w-4 h-4 text-emerald-400" />
              </div>
            </div>
            
            <div className={`flex-1 bg-[#121620]/80 border p-2 rounded text-[10px] flex justify-between items-center transition-all ${
              currentScenario === 'scenario_b' ? 'border-indigo-500/50 bg-indigo-950/5' : 'border-[#1e2538]'
            }`}>
              <div>
                <h5 className="font-bold text-[#f8fafc] flex items-center">
                  私有沙箱: Overtime S&OP Resolution (Scenario B)
                  <span onClick={(e) => toggleLockState('scenario_b', e)} className="ml-1.5 cursor-pointer">
                    {scenarioLocks.scenario_b ? (
                      <Lock className="w-3 h-3 text-rose-500" title="已加锁" />
                    ) : (
                      <Unlock className="w-3 h-3 text-emerald-400" title="未加锁" />
                    )}
                  </span>
                </h5>
                <p className="text-[8.5px] text-muted mt-0.5">加班消峰模拟决策。DOD 数组内存独立增量落盘</p>
              </div>
              <div className="text-right">
                <span className="text-[9px] bg-emerald-950/60 text-emerald-400 border border-emerald-900/50 px-1.5 py-0.5 rounded font-mono font-bold">Delta: 2.10%</span>
                <div className="text-[8px] text-muted mt-1">独立增量落盘</div>
              </div>
            </div>
          </div>

        </div>
      </div>
    );
  };

  // 3. Scorecard Matrix view with reverse-bias indicators and Context Propagation
  const renderScorecardMatrixView = () => {
    // Metric dataset
    const metrics = [
      { name: 'IBP Consensus Revenue', weight: 0.40, target: 250000000, current: 245000000, direction: 'MAX', type: 'currency' },
      { name: 'OTP Service Delivery Level', weight: 0.30, target: 0.98, current: 0.982, direction: 'MAX', type: 'percentage' },
      { name: 'Multi-Echelon SS Inventory Holding Cost', weight: 0.20, target: 20000000, current: 26000000, direction: 'MIN', type: 'currency' }, // Inverse Bias!
      { name: 'Leftover Stagnant Material Count', weight: 0.10, target: 500, current: 410, direction: 'MIN', type: 'number' } // Inverse Bias!
    ];

    const getScore = (m) => {
      if (m.direction === 'MAX') {
        const score = (m.current / m.target) * 10;
        return Math.min(10, Math.round(score * 10) / 10);
      } else {
        // Inverse bias: lower is better!
        const score = (m.target / m.current) * 10;
        return Math.min(10, Math.round(score * 10) / 10);
      }
    };

    const getStatusLight = (score) => {
      if (score >= 9.0) return (
        <span className="flex items-center space-x-1.5 text-emerald-400 font-bold">
          <CheckCircle className="w-3.5 h-3.5 text-emerald-500" />
          <span>On Track</span>
        </span>
      );
      if (score >= 7.0) return (
        <span className="flex items-center space-x-1.5 text-amber-500 font-bold">
          <AlertTriangle className="w-3.5 h-3.5 text-amber-500 animate-pulse" />
          <span>Warning</span>
        </span>
      );
      return (
        <span className="flex items-center space-x-1.5 text-rose-500 font-bold">
          <span className="relative flex h-3 w-3 mr-0.5">
            <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-rose-500 opacity-75"></span>
            <span className="relative inline-flex rounded-full h-3 w-3 bg-rose-600 shadow border border-[#0b0e14]"></span>
          </span>
          <span>Critical Alert</span>
        </span>
      );
    };

    return (
      <div className="w-full h-full p-2.5 flex flex-col space-y-2 select-none">
        <div className="text-[10px] font-bold text-heading border-b border-[#1e2538] pb-1 flex items-center justify-between">
          <span className="flex items-center">
            <TrendingUp className="w-3.5 h-3.5 text-indigo-500 mr-1" />
            白盒公理差异对比矩阵 (Scorecard Matrix)
          </span>
          <span className="text-[8px] bg-slate-900 border border-[#1e2538] px-1.5 py-0.5 rounded text-indigo-400 font-bold animate-pulse">
            双击单元格：穿透加载 Tabular Worksheet Detail
          </span>
        </div>

        <div className="flex-1 overflow-auto">
          <table className="w-full text-left border-collapse border border-[#1e2538] text-[9.5px]">
            <thead>
              <tr className="bg-[#161c28] text-muted font-bold border-b border-[#1e2538]">
                <th className="p-2 border-r border-[#1e2538]">度量项目 (Metric)</th>
                <th className="p-2 border-r border-[#1e2538] text-center">权重</th>
                <th className="p-2 border-r border-[#1e2538] text-right">公理目标 (Target)</th>
                <th className="p-2 border-r border-[#1e2538] text-right">当前实际 (Actual)</th>
                <th className="p-2 border-r border-[#1e2538] text-center">偏好方向</th>
                <th className="p-2 border-r border-[#1e2538] text-center">得分 (Score)</th>
                <th className="p-2">定性状态 (Status)</th>
              </tr>
            </thead>
            <tbody>
              {metrics.map(m => {
                const score = getScore(m);
                return (
                  <tr 
                    key={m.name} 
                    onDoubleClick={() => onScorecardDrilldownClick(m.name, currentScenario, m.target, m.current)}
                    className="hover:bg-[#1c2335]/40 border-b border-[#1e2538]/60 cursor-pointer select-none text-[9px] transition-colors"
                  >
                    <td className="p-2 border-r border-[#1e2538] font-bold text-heading flex items-center">
                      <span className="w-1.5 h-3 bg-indigo-500 rounded-full mr-2"></span>
                      {m.name}
                    </td>
                    <td className="p-2 border-r border-[#1e2538] text-center font-mono">{(m.weight * 100).toFixed(0)}%</td>
                    <td className="p-2 border-r border-[#1e2538] text-right font-mono">
                      {m.type === 'currency' ? '¥' + m.target.toLocaleString() : (m.type === 'percentage' ? (m.target * 100) + '%' : m.target)}
                    </td>
                    <td className="p-2 border-r border-[#1e2538] text-right font-mono font-bold text-heading">
                      {m.type === 'currency' ? '¥' + m.current.toLocaleString() : (m.type === 'percentage' ? (m.current * 100).toFixed(1) + '%' : m.current)}
                    </td>
                    <td className="p-2 border-r border-[#1e2538] text-center">
                      <span className={`px-1 py-0.5 rounded font-bold text-[8px] ${
                        m.direction === 'MAX' ? 'bg-emerald-950/40 text-emerald-400 border border-emerald-900/40' : 'bg-amber-950/40 text-amber-500 border border-amber-900/40'
                      }`}>
                        {m.direction === 'MAX' ? '极大值 Max' : '极小值 Min (反向)'}
                      </span>
                    </td>
                    <td className={`p-2 border-r border-[#1e2538] text-center font-mono font-bold text-[10px] ${
                      score >= 9.0 ? 'text-emerald-400' : (score >= 7.0 ? 'text-amber-500' : 'text-rose-500')
                    }`}>
                      {score} / 10
                    </td>
                    <td className="p-2">
                      {getStatusLight(score)}
                    </td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        </div>

        {/* Drill Bounding Box Context display */}
        {contextBoundingBox && (
          <div className="bg-[#121620] border border-indigo-900/40 rounded p-2.5 text-[9px] select-none flex flex-col space-y-1.5 animate-fade-in shadow-inner">
            <div className="flex justify-between items-center border-b border-[#1e2538] pb-1.5">
              <span className="font-bold text-indigo-400 flex items-center">
                <Eye className="w-3.5 h-3.5 mr-1" />
                穿透的公理过滤上下文 (Immutable Context Propagation Bounding Box)
              </span>
              <button 
                onClick={() => setContextBoundingBox(null)}
                className="text-muted hover:text-heading"
              >
                <X className="w-3.5 h-3.5" />
              </button>
            </div>
            <div className="grid grid-cols-2 lg:grid-cols-4 gap-2 font-mono text-[8.5px]">
              <div><span className="text-muted">场景沙盘:</span> <span className="text-heading font-bold">{contextBoundingBox.scenario}</span></div>
              <div><span className="text-muted">异常维度:</span> <span className="text-heading font-bold">{contextBoundingBox.metricName}</span></div>
              <div><span className="text-muted">过滤Site:</span> <span className="text-heading font-bold">{contextBoundingBox.siteFilterId}</span></div>
              <div><span className="text-muted">当前状况:</span> <span className={`font-bold ${contextBoundingBox.constraintState === 'CRITICAL_BREACH' ? 'text-rose-500 animate-pulse' : 'text-emerald-400'}`}>{contextBoundingBox.constraintState}</span></div>
            </div>
            <div className="text-[8.5px] bg-[#090b10] border border-[#1e2538] p-1.5 rounded text-main max-h-24 overflow-y-auto">
              <strong>DuckDB 级联受损明细:</strong>
              <div className="space-y-1 mt-1 font-mono text-[8px]">
                <div>工单 #YieldA | 需求物料: 512MB | 异常天: D29 | 缺口数: 40 颗 | 原因: VVIP 安全库存保护隔离</div>
                <div>工单 #YieldB | 需求物料: 256MB | 异常天: D8 | 缺口数: 0 颗 | 原因: 工艺替换已执行挽回</div>
              </div>
            </div>
          </div>
        )}
      </div>
    );
  };

  // 4. Crosstab Grid Date Matrix View with inline-editing & Transition buckets
  const renderCrosstabView = () => {
    const renderCellInputInput = (r, colId) => {
      const isProtected = r.isProtected && !r.isParent;
      return (
        <input
          type="number"
          defaultValue={r[colId]}
          onBlur={(e) => handleSpreadMutationSpread(r.id, colId, e.target.value)}
          disabled={isProtected || isEngineHalted}
          className={`w-full bg-[#090b10] text-right border ${
            r.isDirty ? 'border-indigo-500 text-indigo-400 font-bold bg-indigo-950/15' : 'border-transparent text-heading'
          } ${r.isParent ? 'font-bold bg-[#161c28]/40' : ''} outline-none px-1 rounded-xs text-[9.5px] font-mono focus:border-indigo-500 disabled:opacity-60 disabled:cursor-not-allowed`}
        />
      );
    };

    return (
      <div className="w-full h-full p-2.5 flex flex-col space-y-2 relative">
        <div className="border-b border-[#1e2538] pb-1.5 flex justify-between items-center select-none text-[9.5px]">
          <div className="text-[10px] font-bold text-heading flex items-center">
            <Sliders className="w-3.5 h-3.5 text-indigo-500 mr-1" />
            时序多维交叉矩阵 (Crosstab Matrix disaggregation spread)
          </div>
          
          <div className="flex items-center space-x-2 border border-[#1e2538] bg-[#161c28] px-2 py-0.5 rounded text-[8.5px]">
            <span className="text-muted font-bold">Crosstab 数据量:</span>
            <input
              type="range"
              min="5000"
              max="150000"
              step="5000"
              value={simulatedRecordCount}
              onChange={(e) => {
                const count = parseInt(e.target.value);
                setSimulatedRecordCount(count);
              }}
              className="ct-timeline-slider w-20"
            />
            <span className={`font-bold font-mono ${isCrosstabLimitBreached ? 'text-orange-500 animate-pulse' : 'text-indigo-400'}`}>
              {simulatedRecordCount.toLocaleString()} 行
            </span>
          </div>
        </div>

        {/* Anchor Date info */}
        <div className="text-[9px] text-[#4f46e5] font-mono flex items-center space-x-1.5 select-none">
          <span className="inline-block w-1.5 h-1.5 bg-[#4f46e5] rounded-full"></span>
          <span>计划锚定起点 (MRP run Anchor Date): <strong>2026-05-29</strong></span>
        </div>

        {/* Engine Halted Banner */}
        {isEngineHalted && (
          <div className="bg-orange-950/25 border border-orange-600/80 text-orange-400 p-2 rounded text-[9px] flex justify-between items-center select-none animate-pulse">
            <div className="flex items-center space-x-2">
              <ShieldAlert className="w-4 h-4 text-orange-500" />
              <span>
                <strong>⚠️ 逻辑专政/防御熔断已激活:</strong> 内存膨胀率超额 (Tabular &gt; 5000 / Crosstab &gt; 100,000)。连续内存算子挂起，行索引变橙，编辑锁定。
              </span>
            </div>
            <button 
              onClick={() => setShowQbeForm(true)}
              className="bg-orange-850 hover:bg-orange-800 text-white font-bold px-2 py-0.5 rounded border border-orange-700 text-[8.5px]"
            >
              一键 QBE 清障过滤
            </button>
          </div>
        )}

        <div className="flex-1 overflow-auto">
          <table className="w-full text-left border-collapse border border-[#1e2538] text-[9.5px]">
            <thead>
              <tr className="bg-[#161c28] text-muted font-bold border-b border-[#1e2538]">
                <th className="p-1.5 border-r border-[#1e2538] w-14">行索引</th>
                <th className="p-1.5 border-r border-[#1e2538]">物料节点</th>
                <th className="p-1.5 border-r border-[#1e2538] w-24">制造 Site</th>
                <th className="p-1.5 border-r border-[#1e2538] text-right w-12">D1</th>
                <th className="p-1.5 border-r border-[#1e2538] text-right w-12">D2</th>
                <th className="p-1.5 border-r border-[#1e2538] text-right w-12">D3</th>
                <th className="p-1.5 border-r border-[#1e2538] text-right w-12">D4</th>
                <th className="p-1.5 border-r border-[#1e2538] text-right bg-indigo-950/20 text-indigo-400 w-16 relative group">
                  T1 (过渡)
                  <span className="text-[7px] text-amber-500 ml-0.5 select-none cursor-pointer" title="过渡时间切片 - 长度非标 (3天)，用于平滑日度向周度过渡">▲</span>
                </th>
                <th className="p-1.5 border-r border-[#1e2538] text-right w-16">W1</th>
                <th className="p-1.5 border-r border-[#1e2538] text-right w-16">W2</th>
                <th className="p-1.5 border-r border-[#1e2538] text-right w-16">W3</th>
                <th className="p-1.5 border-r border-[#1e2538] text-right bg-indigo-950/20 text-indigo-400 w-16 relative group">
                  T2 (过渡)
                  <span className="text-[7px] text-amber-500 ml-0.5 select-none cursor-pointer" title="过渡时间切片 - 长度非标 (5天)，用于平滑周度向月度过渡">▲</span>
                </th>
                <th className="p-1.5 border-r border-[#1e2538] text-right w-20">M1</th>
                <th className="p-1.5 text-right w-20">M2</th>
              </tr>
            </thead>
            <tbody>
              {crosstabRecords.map((r, idx) => (
                <tr 
                  key={r.id} 
                  className={`border-b border-[#1e2538]/60 ${
                    r.isParent ? 'bg-[#121620]/80 font-bold text-heading' : 'hover:bg-[#1c2335]/40 text-main'
                  }`}
                >
                  <td className={`p-1.5 border-r border-[#1e2538] font-mono text-[9px] text-center ${
                    isCrosstabLimitBreached ? 'text-orange-500 font-bold bg-orange-950/10 animate-pulse' : 'text-muted'
                  }`}>
                    #{idx + 1}
                  </td>
                  <td className="p-1.5 border-r border-[#1e2538] font-sans truncate max-w-[140px]">
                    {r.isParent ? (
                      <span>📂 {r.name}</span>
                    ) : (
                      <span className="pl-3.5">📄 {r.name}</span>
                    )}
                    {r.isProtected && (
                      <span className="ml-1 text-[7.5px] bg-rose-950 text-rose-400 px-1 border border-rose-900 rounded font-bold" title="已锁定边缘供需约束，不可修改">
                        CONTRACT LOCK
                      </span>
                    )}
                  </td>
                  <td className="p-1.5 border-r border-[#1e2538] font-mono text-[9px] text-[#8b5cf6]">{r.site}</td>
                  
                  <td className="p-1 border-r border-[#1e2538] text-right">{renderCellInputInput(r, 'd1')}</td>
                  <td className="p-1 border-r border-[#1e2538] text-right">{renderCellInputInput(r, 'd2')}</td>
                  <td className="p-1 border-r border-[#1e2538] text-right">{renderCellInputInput(r, 'd3')}</td>
                  <td className="p-1 border-r border-[#1e2538] text-right">{renderCellInputInput(r, 'd4')}</td>
                  
                  {/* Transition T1 */}
                  <td className="p-1 border-r border-[#1e2538] text-right bg-indigo-950/5">{renderCellInputInput(r, 't1')}</td>
                  
                  <td className="p-1 border-r border-[#1e2538] text-right">{renderCellInputInput(r, 'w1')}</td>
                  <td className="p-1 border-r border-[#1e2538] text-right">{renderCellInputInput(r, 'w2')}</td>
                  <td className="p-1 border-r border-[#1e2538] text-right">{renderCellInputInput(r, 'w3')}</td>
                  
                  {/* Transition T2 */}
                  <td className="p-1 border-r border-[#1e2538] text-right bg-indigo-950/5">{renderCellInputInput(r, 't2')}</td>
                  
                  <td className="p-1 border-r border-[#1e2538] text-right">{renderCellInputInput(r, 'm1')}</td>
                  <td className="p-1 text-right">{renderCellInputInput(r, 'm2')}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>
    );
  };

  // 5. Tabular Detail Grid view with dynamic columns & limit highlights
  const renderTabularView = () => {
    const rawData = drilldownData.length > 0 ? drilldownData : [
      { order_code: 'Order1', part_code: 'PART_512', site: 'CD_SLICING_FACTORY', qty: 1200, day: 8, priority: 1, type: 'VVIP Safety Stock Protected' },
      { order_code: 'Order2', part_code: 'PART_256', site: 'SH_ASSEMBLY_HUB', qty: 800, day: 15, priority: 2, type: 'Normal Sourcing Policy' },
      { order_code: 'Order3', part_code: 'PART_128', site: 'SZ_WAFER_SUPPLY', qty: 600, day: 22, priority: 3, type: 'Normal Alternative Match' },
      { order_code: 'Order4', part_code: 'PART_512', site: 'SH_ASSEMBLY_HUB', qty: 450, day: 8, priority: 1, type: 'VVIP Allocation' },
      { order_code: 'Order5', part_code: 'PART_256', site: 'CD_SLICING_FACTORY', qty: 1000, day: 10, priority: 2, type: 'Normal Sourcing Policy' }
    ];

    const headers = rawData.length > 0 ? Object.keys(rawData[0]) : [];



    const renderTabularFormView = () => {
      if (filteredData.length === 0) {
        return (
          <div className="flex-1 flex items-center justify-center text-muted text-xs p-6">
            无过滤后符合条件的记录，Form View 为空。
          </div>
        );
      }
      
      const safeIdx = activeRecordIdx >= filteredData.length ? 0 : activeRecordIdx;
      const currentRecord = filteredData[safeIdx];
      const recordKeys = Object.keys(currentRecord);

      const handleFormFieldChange = (key, rawText) => {
        const updated = [...drilldownData];
        const originalIdx = drilldownData.indexOf(currentRecord);
        if (originalIdx !== -1) {
          const row = { ...updated[originalIdx] };
          const type = getColType(key);
          let val = rawText;
          if (type === 'Quantity' || type === 'Money') {
            const factor = type === 'Quantity' ? UOM_FACTORS[activeUOM] : CURRENCY_FACTORS[activeCurrency];
            val = parseFloat(rawText) / factor;
            if (isNaN(val)) val = rawText;
          } else if (type === 'Boolean') {
            val = rawText === 'true' || rawText === true;
          }
          row[key] = val;
          updated[originalIdx] = row;
          setDrilldownData(updated);
        }
      };

      return (
        <div className="flex-1 flex flex-col p-4 bg-[#090b10]/40 border border-[#1e2538] rounded space-y-4 overflow-y-auto max-w-xl mx-auto my-3 custom-fine-scrollbar">
          {/* Form Navigation Headers */}
          <div className="flex justify-between items-center border-b border-[#1e2538] pb-2 text-[10px]">
            <span className="font-bold text-indigo-400 font-mono">
              📄 Form View: Record {safeIdx + 1} of {filteredData.length}
            </span>
            
            <div className="flex items-center space-x-1">
              <button
                onClick={() => setActiveRecordIdx(prev => Math.max(0, prev - 1))}
                disabled={safeIdx === 0}
                className="px-2.5 py-1 bg-[#161c28] border border-[#1e2538] text-[#cbd5e1] hover:text-white rounded disabled:opacity-50 disabled:cursor-not-allowed font-bold"
              >
                ◀ Prev
              </button>
              <button
                onClick={() => setActiveRecordIdx(prev => Math.min(filteredData.length - 1, prev + 1))}
                disabled={safeIdx === filteredData.length - 1}
                className="px-2.5 py-1 bg-[#161c28] border border-[#1e2538] text-[#cbd5e1] hover:text-white rounded disabled:opacity-50 disabled:cursor-not-allowed font-bold"
              >
                Next ▶
              </button>
            </div>
          </div>

          {/* Form Field Grid */}
          <div className="grid grid-cols-1 md:grid-cols-2 gap-3.5 text-[9.5px]">
            {recordKeys.map(key => {
              const val = currentRecord[key];
              const type = getColType(key);
              const displayVal = formatCellValue(val, key);

              return (
                <div key={key} className="flex flex-col space-y-1.5 bg-[#121620]/60 p-2 border border-[#1e2538]/40 rounded hover:border-indigo-500/20 transition-all">
                  <span className="text-muted font-bold capitalize font-mono text-[8.5px] flex items-center">
                    {key.replace('_', ' ')}
                    {type === 'Quantity' && ` ${UOM_SUFFIXES[activeUOM]}`}
                    {type === 'Money' && ` (${CURRENCY_SYMBOLS[activeCurrency]})`}
                  </span>
                  
                  {type === 'Boolean' ? (
                    <input
                      type="checkbox"
                      checked={!!val}
                      onChange={(e) => handleFormFieldChange(key, e.target.checked)}
                      className="w-3.5 h-3.5 rounded bg-[#090b10] border border-[#1e2538] text-indigo-600 focus:ring-0 outline-none cursor-pointer"
                    />
                  ) : type === 'DateTime' ? (
                    <input
                      type="date"
                      value={String(val).split('T')[0] || ''}
                      onChange={(e) => handleFormFieldChange(key, e.target.value)}
                      className="bg-[#090b10] border border-[#1e2538] text-heading rounded px-2 py-1 outline-none focus:border-indigo-500 font-mono"
                    />
                  ) : (
                    <input
                      type={type === 'Quantity' || type === 'Money' ? 'number' : 'text'}
                      step="any"
                      value={displayVal.replace(/[¥$€,]/g, '')}
                      onChange={(e) => handleFormFieldChange(key, e.target.value)}
                      className="bg-[#090b10] border border-[#1e2538] text-heading rounded px-2 py-1 outline-none focus:border-indigo-500 font-mono"
                    />
                  )}
                </div>
              );
            })}
          </div>
        </div>
      );
    };

    return (
      <div className="w-full h-full p-2.5 flex flex-col space-y-2 select-none">
        <div className="border-b border-[#1e2538] pb-1.5 flex justify-between items-center text-[9.5px]">
          <div className="text-[10px] font-bold text-heading flex items-center">
            <Database className="w-3.5 h-3.5 text-indigo-500 mr-1" />
            Tabular Grid: 详细数据工作表 (Tabular Worksheet Detail)
          </div>
          
          <div className="flex items-center space-x-2.5">
            <button
              onClick={toggleTabularViewMode}
              className="px-2 py-0.5 bg-indigo-950 text-indigo-400 hover:bg-indigo-900 border border-indigo-900/60 rounded text-[8.5px] font-bold transition-all"
            >
              切换为 {activeTabularViewMode === 'table' ? 'Form (单表单) 视图' : 'Table (网格) 视图'}
            </button>

            <span className="text-muted">|</span>

            <div className="flex items-center space-x-2 border border-[#1e2538] bg-[#161c28] px-2 py-0.5 rounded text-[8.5px]">
              <span className="text-muted font-bold">Tabular 数据量:</span>
              <input
                type="range"
                min="500"
                max="10000"
                step="500"
                value={simulatedTabularRows}
                onChange={(e) => {
                  const count = parseInt(e.target.value);
                  setSimulatedTabularRows(count);
                }}
                className="ct-timeline-slider w-20"
              />
              <span className={`font-bold font-mono ${isTabularLimitBreached ? 'text-orange-500 animate-pulse' : 'text-indigo-400'}`}>
                {simulatedTabularRows.toLocaleString()} 行
              </span>
            </div>
          </div>
        </div>

        {/* Engine Halted Banner */}
        {isEngineHalted && (
          <div className="bg-orange-950/25 border border-orange-600/80 text-orange-400 p-2 rounded text-[9px] flex justify-between items-center select-none animate-pulse">
            <div className="flex items-center space-x-2">
              <ShieldAlert className="w-4 h-4 text-orange-500" />
              <span>
                <strong>⚠️ Tabular 限流熔断:</strong> 表格规模超限 (TabularGrid.MaxRow = 5000)。重算已挂起，行索引变橙。
              </span>
            </div>
            <button 
              onClick={() => setShowQbeForm(true)}
              className="bg-orange-850 hover:bg-orange-800 text-white font-bold px-2 py-0.5 rounded border border-orange-700 text-[8.5px]"
            >
              启动 QBE 过滤表单
            </button>
          </div>
        )}

        {/* Context Propagation Banner */}
        {contextBoundingBox && (
          <div className="bg-indigo-950/20 border border-indigo-800/40 rounded px-2.5 py-1.5 text-[8.5px] flex flex-col space-y-1">
            <div className="font-bold text-indigo-400 flex items-center justify-between">
              <span className="flex items-center">
                <Eye className="w-3 h-3 mr-1" />
                穿透的公理过滤上下文 (Propagated Bounding Box Context)
              </span>
              <span className="text-[8px] bg-indigo-900/60 border border-indigo-700 px-1.5 py-0.5 rounded font-mono text-indigo-200">
                Limit: 50 Rows Whitelisted
              </span>
            </div>
            <div className="grid grid-cols-2 md:grid-cols-4 gap-2 font-mono text-[8px] text-main">
              <div>关联实体: <span className="text-heading font-bold">{contextBoundingBox.metricName}</span></div>
              <div>控制场景: <span className="text-heading font-bold">{contextBoundingBox.scenario}</span></div>
              <div>物理过滤Site: <span className="text-heading font-bold">{contextBoundingBox.siteFilterId}</span></div>
              <div>保障状态: <span className="text-emerald-400 font-bold">{contextBoundingBox.constraintState}</span></div>
            </div>
          </div>
        )}

        {activeTabularViewMode === 'form' ? (
          renderTabularFormView()
        ) : (
          <div className="flex-1 overflow-auto custom-fine-scrollbar">
            <table className="w-full text-left border-collapse border border-[#1e2538] text-[9.5px]">
              <thead>
                <tr className="bg-[#161c28] text-muted font-bold border-b border-[#1e2538]">
                  <th className="p-1.5 border-r border-[#1e2538] w-16 text-center">行索引</th>
                  {headers.map((h, colIdx) => {
                    if (hiddenColumns.has(h)) return null;
                    const isSticky = colIdx < freezeColumnsCount;
                    const isSelected = selectedCell && selectedCell.colId === h;
                    return (
                      <th 
                        key={h} 
                        onClick={() => setSelectedCell({ type: 'tabular', rowIdx: 0, colId: h, value: '' })}
                        className={`p-1.5 border-r border-[#1e2538] capitalize font-bold select-none cursor-pointer hover:bg-slate-800/40 relative ${
                          isSticky ? 'sticky left-0 bg-[#161c28] z-10' : ''
                        } ${isSelected ? 'outline outline-1 outline-indigo-500 bg-[#1c2335]/30' : ''}`}
                      >
                        <span>{h.replace('_', ' ')}</span>
                        {getColType(h) === 'Quantity' && UOM_SUFFIXES[activeUOM]}
                        {getColType(h) === 'Money' && ` (${CURRENCY_SYMBOLS[activeCurrency]})`}
                        {isSticky && <span className="absolute right-0 top-0 bottom-0 w-0.5 bg-indigo-500/50"></span>}
                      </th>
                    );
                  })}
                </tr>

                {/* QBE Search Row */}
                <tr className="bg-[#090b10] border-b border-[#1e2538]/85">
                  <td className="p-1 border-r border-[#1e2538] bg-slate-900/30 text-center font-bold text-muted text-[8px] select-none">QBE</td>
                  {headers.map((h, colIdx) => {
                    if (hiddenColumns.has(h)) return null;
                    const isSticky = colIdx < freezeColumnsCount;
                    return (
                      <td key={h} className={`p-1 border-r border-[#1e2538] ${isSticky ? 'sticky left-0 bg-[#090b10] z-10' : ''}`}>
                        <input
                          type="text"
                          value={qbeRowFilters[h] || ''}
                          onChange={(e) => {
                            const val = e.target.value;
                            setQbeRowFilters(prev => ({
                              ...prev,
                              [h]: val
                            }));
                          }}
                          placeholder="过滤..."
                          className="w-full bg-[#121620]/80 border border-[#1e2538]/60 text-heading rounded px-1 py-0.5 text-[8.5px] outline-none focus:border-indigo-500 font-mono"
                        />
                        {isSticky && <span className="absolute right-0 top-0 bottom-0 w-0.5 bg-indigo-500/50"></span>}
                      </td>
                    );
                  })}
                </tr>
              </thead>
              <tbody>
                {filteredData.map((row, idx) => (
                  <tr key={idx} className="hover:bg-[#1c2335]/40 border-b border-[#1e2538]/60 text-main font-mono transition-colors">
                    <td className={`p-1.5 border-r border-[#1e2538] text-center font-bold ${
                      isTabularLimitBreached ? 'bg-orange-500/30 text-orange-400 font-extrabold animate-pulse' : 'text-muted'
                    }`}>
                      #{idx + 1}
                    </td>
                    {headers.map((h, colIdx) => {
                      if (hiddenColumns.has(h)) return null;
                      const isSticky = colIdx < freezeColumnsCount;
                      const isSelected = selectedCell && selectedCell.type === 'tabular' && selectedCell.rowIdx === idx && selectedCell.colId === h;
                      const val = row[h];
                      const colType = getColType(h);
                      const isLink = colType === 'String' && (h.includes('part') || h.includes('site') || h.includes('wc') || h.includes('center') || h.includes('routing'));
                      const displayVal = formatCellValue(val, h);

                      return (
                        <td 
                          key={h} 
                          onClick={() => setSelectedCell({ type: 'tabular', rowIdx: idx, colId: h, value: val })}
                          onDoubleClick={() => {
                            if (isLink) {
                              setContextBoundingBox({
                                scenario: currentScenario,
                                filterId: 'ALL_RECORDS',
                                siteFilterId: row.site || 'ALL_SITES',
                                dateBucket: row.day ? `Day ${row.day}` : 'All Periods',
                                metricName: `Link drill: ${h} = ${val}`,
                                target: '-',
                                current: '-',
                                constraintState: 'BOUNDED'
                              });
                              alert(`🔗 Hyperlink Context Broadcasted:\n[Context: ${h} = ${val}]\nPlanning sheet loaded in split.`);
                            }
                          }}
                          className={`p-1.5 border-r border-[#1e2538] truncate max-w-[150px] relative ${
                            isSticky ? 'sticky left-0 bg-[#121620] z-10' : ''
                          } ${isSelected ? 'outline outline-1 outline-indigo-500 bg-[#1c2335]/30 text-white font-bold' : ''} ${
                            isLink ? 'text-indigo-400 underline cursor-pointer hover:text-indigo-300 font-bold' : ''
                          }`}
                        >
                          {displayVal}
                          {isSticky && <span className="absolute right-0 top-0 bottom-0 w-0.5 bg-indigo-500/50"></span>}
                        </td>
                      );
                    })}
                  </tr>
                ))}
                {simulatedTabularRows > filteredData.length && (
                  <tr className="border-b border-[#1e2538]/30 text-muted/40 font-mono italic">
                    <td className={`p-1.5 border-r border-[#1e2538] text-center font-bold ${
                      isTabularLimitBreached ? 'text-orange-500 bg-orange-950/5 animate-pulse' : 'text-muted/20'
                    }`}>
                      ...
                    </td>
                    <td colSpan={headers.length - hiddenColumns.size} className="p-1.5 text-center text-[8.5px]">
                      已省略其后 {(simulatedTabularRows - filteredData.length).toLocaleString()} 行仿真细节数据以优化内存渲染
                    </td>
                  </tr>
                )}
              </tbody>
            </table>
          </div>
        )}

        {/* Range Editing Modal inside Tabular View */}
        {showRangePanel && (
          <div className="fixed inset-0 bg-[#0b0e14]/85 backdrop-blur-sm flex items-center justify-center z-50 p-4 select-none animate-fade-in">
            <div className="bg-[#121620] border border-[#1e2538] rounded-md max-w-sm w-full p-4 space-y-4 shadow-2xl">
              <div className="flex justify-between items-center border-b border-[#1e2538] pb-2">
                <span className="font-bold text-indigo-400 flex items-center text-[10.5px]">
                  <Sliders className="w-4 h-4 mr-2 text-indigo-500" />
                  区间数学运算/数学平移面板 (Edit Range)
                </span>
                <button onClick={() => setShowRangePanel(false)} className="text-muted hover:text-[#f8fafc]">
                  <X className="w-4 h-4" />
                </button>
              </div>

              <p className="text-[9px] text-main font-mono">
                对当前选中列 <strong>{selectedCell ? selectedCell.colId : '未选择'}</strong> 的所有行记录，进行批量数学计算：
              </p>

              <div className="space-y-3.5 text-[10px]">
                <div className="flex flex-col space-y-1">
                  <span className="text-muted">选择计算算子 (Operator)</span>
                  <select
                    value={rangeEditForm.op}
                    onChange={(e) => setRangeEditForm(prev => ({ ...prev, op: e.target.value }))}
                    className="bg-[#090b10] border border-[#1e2538] text-heading p-1.5 rounded outline-none font-bold"
                  >
                    <option value="MULTIPLY">乘算因子 (MULTIPLY)</option>
                    <option value="ADD">相加常数 (ADD)</option>
                    <option value="SUB">相减常数 (SUB)</option>
                    <option value="DIVIDE">相除常数 (DIVIDE)</option>
                  </select>
                </div>

                <div className="flex flex-col space-y-1">
                  <span className="text-muted">输入数值 (Constant Value)</span>
                  <input
                    type="number"
                    step="any"
                    value={rangeEditForm.val}
                    onChange={(e) => setRangeEditForm(prev => ({ ...prev, val: e.target.value }))}
                    className="bg-[#090b10] border border-[#1e2538] text-heading p-1.5 rounded outline-none font-mono"
                  />
                </div>
              </div>

              <div className="flex justify-end space-x-2 pt-2 border-t border-[#1e2538]">
                <button
                  onClick={() => setShowRangePanel(false)}
                  className="px-3 py-1 bg-transparent hover:bg-slate-800 text-muted hover:text-heading border border-[#1e2538] rounded text-[9.5px]"
                >
                  取消
                </button>
                <button
                  onClick={() => applyRangeEditing(rangeEditForm.op, rangeEditForm.val)}
                  className="px-3 py-1 bg-indigo-600 hover:bg-indigo-500 text-white rounded font-bold text-[9.5px] border border-indigo-500"
                >
                  应用区间计算
                </button>
              </div>
            </div>
          </div>
        )}
      </div>
    );
  };

  // 6. Help documentation & Axiom descriptions view
  const renderHelpView = () => {
    return (
      <div className="w-full h-full p-3.5 space-y-3 overflow-y-auto leading-relaxed select-text text-[9.5px]">
        <h4 className="font-bold text-heading text-[11px] border-b border-[#1e2538] pb-1.5 flex items-center">
          <HelpCircle className="w-4 h-4 text-indigo-500 mr-1.5" />
          控制塔白盒求解与公理说明书 (Workbook Axioms & Help Documentation)
        </h4>
        
        <div className="space-y-3.5">
          <section className="space-y-1.5">
            <span className="font-bold text-[#f8fafc]">1. 连续内存图覆盖公理 (DOD Flat Graph Scoping)</span>
            <p className="text-main">
              系统底层完全放弃指针跳转严重的图数据库（EKG），使用一维连续扁平 <code>std::vector</code> 存储全网 SKU 节点与倒排索引。所有过滤与修改将直接编译为底层连续内存块的索引过滤算子，消除 CPU Cache Miss 抖动。
            </p>
          </section>

          <section className="space-y-1.5">
            <span className="font-bold text-[#f8fafc]">2. 三类专利替换分配法则 (Substitution Scopes)</span>
            <p className="text-main">
              在 LBL 级联消纳发生净缺口时，触发专利决策机制：
              <br />
              • <strong>一类替换</strong>（绝对温差最大优先）：选择历史累计消耗量与理论配额温差 gap 最大的零件。
              <br />
              • <strong>二类替换</strong>（供应商评级最低优先）：按 rating = 消耗量 / 比例，选择 rating 最小的最稳定供应商。
              <br />
              • <strong>三类替换</strong>（Lot-Sizing 动态归一化）：对应分配量最大的候选件应用 <code>ceil(due / lot) * lot</code> 取整扣减后，将该零件移出活跃集，剩余活跃零件基于比例动态重新归一化。
            </p>
          </section>

          <section className="space-y-1.5">
            <span className="font-bold text-[#f8fafc]">3. 增量树拓扑 Delta-Tracking 机制</span>
            <p className="text-main">
               What-if 模拟采用增量存储（Delta Tracking）机制。未修改的数据行 Records 由指针向上穿透继承自 Parent 场景，仅用户主动篡改/增删的数据行在当前 Scenario 叶子分区独立落盘，保障极速秒级推演。
            </p>
          </section>

          <section className="space-y-1.5">
            <span className="font-bold text-[#f8fafc]">4. 比例分摊 disaggregation 与锁定保护</span>
            <p className="text-main">
              在层级树直接修改 aggregated 交叉单元格数值时，系统自动拦截该动作。采用 Proportional Spread 算子将差异额按原数据行权重分摊到 Lowest Level，同时刚性保护已锁定的特定边缘受阻记录（isProtected），完成后向上运行不可约聚合。
            </p>
          </section>
        </div>
      </div>
    );
  };

  return (
    <div className="w-full h-full flex flex-row overflow-hidden min-h-0 bg-[#0b0e14]">
      
      {/* Collapsible Left Explorer Sidebar */}
      {isSidebarOpen && (
        <div className="w-64 border-r border-[#1e2538] bg-[#121620] flex flex-col select-none flex-shrink-0 animate-slide-in relative z-20">
          {/* Sidebar Tab Selector Headers */}
          <div className="flex bg-[#161c28] border-b border-[#1e2538] text-[9.5px]">
            <button
              onClick={() => setActiveSidebarTab('tables')}
              className={`flex-1 py-2 font-bold transition-all ${
                activeSidebarTab === 'tables' ? 'bg-[#121620] text-indigo-400 border-b-2 border-indigo-500' : 'text-muted hover:text-[#f8fafc]'
              }`}
            >
              🗂️ 引擎实体表 ({ENGINE_TABLES.length})
            </button>
            <button
              onClick={() => setActiveSidebarTab('scenarios')}
              className={`flex-1 py-2 font-bold transition-all ${
                activeSidebarTab === 'scenarios' ? 'bg-[#121620] text-indigo-400 border-b-2 border-indigo-500' : 'text-muted hover:text-[#f8fafc]'
              }`}
            >
              🌲 场景拓扑树
            </button>
          </div>

          {/* Sidebar Content */}
          <div className="flex-1 flex flex-col overflow-hidden p-2.5 min-h-0 space-y-2.5">
            {activeSidebarTab === 'tables' ? renderTablesSidebar() : renderScenariosSidebar()}
          </div>
        </div>
      )}

      {/* Main Content Workspace Layout */}
      <div className="flex-1 flex flex-col min-h-0 overflow-hidden relative bg-[#0b0e14]">
        
        {/* Top Global Menu Bar */}
        <div className="h-8 bg-[#161c28] border-b border-[#1e2538] flex items-center justify-between px-3 text-[10px] select-none z-30">
          <div className="flex items-center space-x-4">
            {/* FILE */}
            <div className="relative group">
              <button className="text-[#cbd5e1] hover:text-[#f8fafc] px-2 py-1 rounded hover:bg-[#1c2335] font-bold transition-all">
                文件 (FILE)
              </button>
              <div className="hidden group-hover:block absolute left-0 top-full mt-0 w-44 bg-[#121620] border border-[#1e2538] rounded shadow-2xl py-1 z-40">
                <button onClick={() => triggerMenuAction('NEW_WORKBOOK')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors">新建工作簿 (New Workbook)</button>
                <button onClick={() => triggerMenuAction('NEW_WORKSHEET')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors">新建工作表 (New Worksheet)</button>
                <button onClick={() => triggerMenuAction('NEW_FILTER')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors">新建过滤器 (New Filter)</button>
                <div className="border-t border-[#1e2538] my-1"></div>
                <button onClick={() => triggerMenuAction('SAVE_DATA')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white flex justify-between items-center transition-colors">
                  <span>保存内存增量 (Save)</span>
                  <span className="text-[8px] text-muted">Ctrl+S</span>
                </button>
                <button onClick={() => triggerMenuAction('PRINT')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors">打印报表 (Print PDF)</button>
                <div className="border-t border-[#1e2538] my-1"></div>
                <div className="px-3 py-1 text-[8px] font-bold text-indigo-400 uppercase tracking-wider">数据导出</div>
                <button onClick={() => exportDataFormat('TAB')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors pl-5">导出 Tab-Delimited (TSV)</button>
                <button onClick={() => exportDataFormat('XML')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors pl-5">导出 XML 连续架构</button>
                <button onClick={() => exportDataFormat('EXCEL')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors pl-5">导出 Excel (HTML)</button>
                <button onClick={() => exportDataFormat('PDF')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors pl-5">导出 PDF (Vector Stream)</button>
              </div>
            </div>

            {/* EDIT */}
            <div className="relative group">
              <button className="text-[#cbd5e1] hover:text-[#f8fafc] px-2 py-1 rounded hover:bg-[#1c2335] font-bold transition-all">
                编辑 (EDIT)
              </button>
              <div className="hidden group-hover:block absolute left-0 top-full mt-0 w-44 bg-[#121620] border border-[#1e2538] rounded shadow-2xl py-1 z-40">
                <button onClick={() => triggerMenuAction('COPY')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white flex justify-between items-center transition-colors">
                  <span>复制 (Copy)</span>
                  <span className="text-[8px] text-muted">Ctrl+C</span>
                </button>
                <button onClick={() => triggerMenuAction('PASTE')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white flex justify-between items-center transition-colors">
                  <span>粘贴 (Paste)</span>
                  <span className="text-[8px] text-muted">Ctrl+V</span>
                </button>
                <div className="border-t border-[#1e2538] my-1"></div>
                <button onClick={() => triggerMenuAction('INSERT_RECORD')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white flex justify-between items-center transition-colors">
                  <span>插入行记录 (Insert)</span>
                  <span className="text-[8px] text-muted">Ins</span>
                </button>
                <button onClick={() => triggerMenuAction('DELETE_RECORD')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white flex justify-between items-center transition-colors text-rose-400 hover:text-white">
                  <span>级联删除 (Delete)</span>
                  <span className="text-[8px] text-muted">Del</span>
                </button>
                <div className="border-t border-[#1e2538] my-1"></div>
                <button onClick={() => triggerMenuAction('COPY_TO_FILTER')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors">复制为列过滤器</button>
              </div>
            </div>

            {/* VIEW */}
            <div className="relative group">
              <button className="text-[#cbd5e1] hover:text-[#f8fafc] px-2 py-1 rounded hover:bg-[#1c2335] font-bold transition-all">
                视图 (VIEW)
              </button>
              <div className="hidden group-hover:block absolute left-0 top-full mt-0 w-44 bg-[#121620] border border-[#1e2538] rounded shadow-2xl py-1 z-40">
                <button onClick={toggleTabularViewMode} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors">
                  切换 Grid / Form 视图
                </button>
                <button onClick={toggleFreezeColumn} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white flex justify-between items-center transition-colors">
                  <span>列冻结/解冻</span>
                  <span className="text-[8px] text-muted">Ctrl+R</span>
                </button>
                <button onClick={toggleUnhideColumns} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors">
                  隐藏/取消隐藏选中列
                </button>
                <div className="border-t border-[#1e2538] my-1"></div>
                <button onClick={() => triggerSort('ASC')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors">快速升序 (Sort ASC)</button>
                <button onClick={() => triggerSort('DESC')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors">快速降序 (Sort DESC)</button>
                <button onClick={() => handleZoomAllPanels(0.1)} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors">全局放大 (Zoom In)</button>
                <button onClick={() => handleZoomAllPanels(-0.1)} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors">全局缩小 (Zoom Out)</button>
                <div className="border-t border-[#1e2538] my-1"></div>
                <button onClick={resetWorkbookConfig} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors text-rose-400 hover:text-white">重置所有工作表配置</button>
              </div>
            </div>

            {/* DATA */}
            <div className="relative group">
              <button className="text-[#cbd5e1] hover:text-[#f8fafc] px-2 py-1 rounded hover:bg-[#1c2335] font-bold transition-all">
                数据 (DATA)
              </button>
              <div className="hidden group-hover:block absolute left-0 top-full mt-0 w-48 bg-[#121620] border border-[#1e2538] rounded shadow-2xl py-1 z-40">
                <button onClick={handleRecompileOperators} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors flex items-center space-x-1.5">
                  <span className="w-1.5 h-1.5 bg-emerald-500 rounded-full"></span>
                  <span>重算引擎算子 (Run MRP)</span>
                </button>
                <button onClick={() => triggerMenuAction('REFRESH')} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors flex items-center space-x-1.5">
                  <span className="w-1.5 h-1.5 bg-indigo-500 rounded-full"></span>
                  <span>刷新大盘数据 (Refresh)</span>
                </button>
                <div className="border-t border-[#1e2538] my-1"></div>
                <button onClick={() => setShowRangePanel(true)} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors">
                  数学区间计算 (Range Edit)
                </button>
                <button onClick={calculateAutoStats} className="w-full text-left px-3 py-1.5 hover:bg-indigo-600 hover:text-white transition-colors">
                  自动度量统计 (Statistics)
                </button>
              </div>
            </div>
          </div>

          <div className="flex items-center space-x-3 text-muted">
            <span className="font-bold text-indigo-400 tracking-wider">o9 Control Tower Studio v1.3</span>
          </div>
        </div>

        {/* Context Bounding Toolbar */}
        <div className="h-10 bg-[#121620] border-b border-[#1e2538] flex items-center justify-between px-3 text-[9.5px] select-none z-20">
          <div className="flex items-center space-x-3.5">
            {/* Sidebar toggle button (integrated) */}
            <button 
              onClick={() => setIsSidebarOpen(prev => !prev)}
              className="px-2 py-1 bg-[#161c28] border border-[#1e2538] text-muted hover:text-[#f8fafc] rounded transition-all flex items-center space-x-1 hover:border-indigo-500/50 cursor-pointer"
              title={isSidebarOpen ? "收起探查侧边栏" : "展开探查侧边栏"}
            >
              <ArrowRightLeft className="w-3 h-3 text-indigo-400" />
              <span className="font-bold text-[8px] uppercase tracking-wide">Explorer</span>
            </button>

            {/* Right Panel toggle button */}
            <button 
              onClick={() => setIsRightPanelOpen(prev => !prev)}
              className="px-2 py-1 bg-[#161c28] border border-[#1e2538] text-muted hover:text-[#f8fafc] rounded transition-all flex items-center space-x-1 hover:border-indigo-500/50 cursor-pointer"
              title={isRightPanelOpen ? "收起 AI 协伴面板" : "展开 AI 协伴面板"}
            >
              <Zap className="w-3 h-3 text-amber-400" />
              <span className="font-bold text-[8px] uppercase tracking-wide">AI Copilot</span>
            </button>

            <span className="text-muted">|</span>

            {/* Scenario Selector */}
            <div className="flex items-center space-x-1.5">
              <span className="text-muted font-bold">场景沙箱:</span>
              <div className="relative">
                <select
                  value={currentScenario}
                  onChange={(e) => switchScenario(e.target.value)}
                  className="bg-[#161c28] border border-[#1e2538] text-heading rounded px-2 py-0.5 outline-none focus:border-indigo-500 font-bold cursor-pointer"
                >
                  <option value="baseline">Baseline Snapshot 🔒</option>
                  <option value="scenario_a">Scenario A (Bottleneck) 🔒</option>
                  <option value="scenario_b">Scenario B (S&OP) 🔓</option>
                </select>
              </div>
            </div>

            {/* Filter Selector */}
            <div className="flex items-center space-x-1.5">
              <span className="text-muted font-bold">快照过滤:</span>
              <select
                value={activeQbeFilterSet}
                onChange={(e) => applyPresetFilter(e.target.value)}
                className="bg-[#161c28] border border-[#1e2538] text-heading rounded px-2 py-0.5 outline-none focus:border-indigo-500 cursor-pointer"
              >
                <option value="ALL">全数据集 (All Records)</option>
                <option value="HIGH_QTY">高量筛选 (Qty &gt; 1000)</option>
                <option value="SHORTAGE">缺口明细 (Shortage &lt;&gt; 0)</option>
              </select>
            </div>

            {/* Site Selector */}
            <div className="flex items-center space-x-1.5">
              <span className="text-muted font-bold">生产Site:</span>
              <select
                value={siteFilter}
                onChange={(e) => setSiteFilter(e.target.value)}
                className="bg-[#161c28] border border-[#1e2538] text-heading rounded px-2 py-0.5 outline-none focus:border-indigo-500 cursor-pointer"
              >
                <option value="ALL">All Manufacturing Sites</option>
                <option value="CD_SLICING_FACTORY">成都晶圆厂 (CD_SLICING_FAC)</option>
                <option value="SH_ASSEMBLY_HUB">上海封装中继 (SH_ASSEMBLY_HUB)</option>
                <option value="SZ_WAFER_SUPPLY">深圳硅料供应 (SZ_WAFER_SUP)</option>
              </select>
            </div>

            {/* Hierarchy Path */}
            <div className="flex items-center space-x-1.5">
              <span className="text-muted font-bold">层级约束:</span>
              <select
                value={hierarchyPath}
                onChange={(e) => setHierarchyPath(e.target.value)}
                className="bg-[#161c28] border border-[#1e2538] text-heading rounded px-2 py-0.5 outline-none focus:border-indigo-500 cursor-pointer font-mono"
              >
                <option value="ROOT">ROOT (全景物理架构)</option>
                <option value="SEMI">└─ SEMI (半导体级联)</option>
                <option value="MEM">└─ MEM (大容量内存级联)</option>
              </select>
            </div>

            {/* Engine Mode Selector */}
            <div className="flex items-center space-x-1.5">
              <span className="text-muted font-bold">计划引擎模式:</span>
              <select
                value={engineMode}
                onChange={(e) => setEngineMode(e.target.value)}
                className="bg-[#161c28] border border-[#1e2538] text-heading rounded px-2 py-0.5 outline-none focus:border-indigo-500 cursor-pointer font-bold"
              >
                <option value="all">全功能 (LBL + DBD 有限排程)</option>
                <option value="lbl">只跑 LBL (净需求下发 & 询单协同)</option>
              </select>
            </div>
          </div>

          <div className="flex items-center space-x-4">
            {/* UOM Selector */}
            <div className="flex items-center space-x-1.5">
              <span className="text-muted font-bold">单位规整 (UOM):</span>
              <select
                value={activeUOM}
                onChange={(e) => changeUOMSelector(e.target.value)}
                className="bg-[#161c28] border border-[#1e2538] text-[#10b981] rounded px-2 py-0.5 outline-none focus:border-indigo-500 cursor-pointer font-bold"
              >
                <option value="Pallet">Pallet (托盘箱)</option>
                <option value="Kilograms">Kilograms (千克)</option>
                <option value="Units">Units (个/颗)</option>
              </select>
            </div>

            {/* Currency Selector */}
            <div className="flex items-center space-x-1.5">
              <span className="text-muted font-bold">币种汇率 (Currency):</span>
              <select
                value={activeCurrency}
                onChange={(e) => changeCurrencySelector(e.target.value)}
                className="bg-[#161c28] border border-[#1e2538] text-[#d97706] rounded px-2 py-0.5 outline-none focus:border-indigo-500 cursor-pointer font-bold"
              >
                <option value="CNY">CNY (人民币 ¥)</option>
                <option value="USD">USD (美元 $)</option>
                <option value="EUR">EUR (欧元 €)</option>
              </select>
            </div>
          </div>
        </div>

        {/* KPI Scorecard Banner Row */}
        <div className="bg-[#0b0e14] px-3.5 py-2.5 border-b border-[#1e2538] flex flex-row gap-3.5 select-none overflow-x-auto custom-fine-scrollbar flex-shrink-0">
          {/* Card 1: Revenue */}
          <div 
            onDoubleClick={() => onScorecardDrilldownClick('IBP Consensus Revenue', currentScenario, 250000000, 245000000)}
            className="flex-1 min-w-[200px] bg-gradient-to-br from-[#121724] to-[#151a28] border border-[#1e2538] hover:border-indigo-500/50 p-3 rounded shadow-md transition-all duration-200 cursor-pointer group hover:scale-[1.01]"
            title="双击：钻取至 Tabular 视图并过滤"
          >
            <div className="flex justify-between items-start">
              <span className="text-[9px] text-[#94a3b8] font-bold tracking-wider uppercase font-mono">IBP Consensus Revenue</span>
              <span className="text-[8px] bg-emerald-950/60 text-emerald-400 border border-emerald-900/40 px-1.5 py-0.2 rounded font-bold">On Track</span>
            </div>
            <div className="mt-2 flex items-baseline space-x-1.5">
              <span className="text-[14px] font-bold text-[#f8fafc] font-mono">¥245,000,000</span>
              <span className="text-[9px] text-muted font-mono">/ ¥250.0M</span>
            </div>
            <div className="w-full bg-slate-900 h-1 rounded overflow-hidden mt-2 border border-[#1e2538]/50">
              <div className="bg-indigo-500 h-full rounded transition-all duration-300" style={{ width: '98%' }}></div>
            </div>
          </div>

          {/* Card 2: Service Level */}
          <div 
            onDoubleClick={() => onScorecardDrilldownClick('OTP Service Delivery Level', currentScenario, 0.98, 0.982)}
            className="flex-1 min-w-[200px] bg-gradient-to-br from-[#121724] to-[#151a28] border border-[#1e2538] hover:border-indigo-500/50 p-3 rounded shadow-md transition-all duration-200 cursor-pointer group hover:scale-[1.01]"
            title="双击：钻取至 Tabular 视图并过滤"
          >
            <div className="flex justify-between items-start">
              <span className="text-[9px] text-[#94a3b8] font-bold tracking-wider uppercase font-mono">OTP Service Level</span>
              <span className="text-[8px] bg-emerald-950/60 text-emerald-400 border border-emerald-900/40 px-1.5 py-0.2 rounded font-bold">On Track</span>
            </div>
            <div className="mt-2 flex items-baseline space-x-1.5">
              <span className="text-[14px] font-bold text-emerald-400 font-mono">98.20%</span>
              <span className="text-[9px] text-muted font-mono">/ 98.00%</span>
            </div>
            <div className="w-full bg-slate-900 h-1 rounded overflow-hidden mt-2 border border-[#1e2538]/50">
              <div className="bg-emerald-500 h-full rounded transition-all duration-300" style={{ width: '100%' }}></div>
            </div>
          </div>

          {/* Card 3: Inventory cost */}
          <div 
            onDoubleClick={() => onScorecardDrilldownClick('Multi-Echelon SS Inventory Holding Cost', currentScenario, 20000000, 26000000)}
            className="flex-1 min-w-[200px] bg-gradient-to-br from-[#1c141d] to-[#151a28] border border-[#1e2538] hover:border-rose-500/50 p-3 rounded shadow-md transition-all duration-200 cursor-pointer group hover:scale-[1.01]"
            title="双击：钻取至 Tabular 视图并过滤"
          >
            <div className="flex justify-between items-start">
              <span className="text-[9px] text-[#94a3b8] font-bold tracking-wider uppercase font-mono">SS Inventory Cost</span>
              <span className="text-[8px] bg-rose-950/60 text-rose-400 border border-rose-900/40 px-1.5 py-0.2 rounded font-bold animate-pulse">Critical Alert</span>
            </div>
            <div className="mt-2 flex items-baseline space-x-1.5">
              <span className="text-[14px] font-bold text-rose-400 font-mono">¥26,000,000</span>
              <span className="text-[9px] text-muted font-mono">/ ¥20.0M Target</span>
            </div>
            <div className="w-full bg-slate-900 h-1 rounded overflow-hidden mt-2 border border-[#1e2538]/50">
              <div className="bg-rose-500 h-full rounded transition-all duration-300" style={{ width: '100%', maxWidth: '100%' }}></div>
            </div>
          </div>

          {/* Card 4: Stagnant inventory */}
          <div 
            onDoubleClick={() => onScorecardDrilldownClick('Leftover Stagnant Material Count', currentScenario, 500, 410)}
            className="flex-1 min-w-[200px] bg-gradient-to-br from-[#121724] to-[#151a28] border border-[#1e2538] hover:border-indigo-500/50 p-3 rounded shadow-md transition-all duration-200 cursor-pointer group hover:scale-[1.01]"
            title="双击：钻取至 Tabular 视图并过滤"
          >
            <div className="flex justify-between items-start">
              <span className="text-[9px] text-[#94a3b8] font-bold tracking-wider uppercase font-mono">Stagnant Inventory</span>
              <span className="text-[8px] bg-emerald-950/60 text-emerald-400 border border-emerald-900/40 px-1.5 py-0.2 rounded font-bold">On Track</span>
            </div>
            <div className="mt-2 flex items-baseline space-x-1.5">
              <span className="text-[14px] font-bold text-purple-400 font-mono">410 Pcs</span>
              <span className="text-[9px] text-muted font-mono">/ 500 Max</span>
            </div>
            <div className="w-full bg-slate-900 h-1 rounded overflow-hidden mt-2 border border-[#1e2538]/50">
              <div className="bg-purple-500 h-full rounded transition-all duration-300" style={{ width: '82%' }}></div>
            </div>
          </div>
        </div>

        {/* Tab Split Workspace Grid Area */}
        <div className="flex-1 overflow-hidden min-h-0 min-w-0 p-2.5">
          {renderLayout(layout, [])}
        </div>

        {/* Bottom Global Status Bar */}
        <footer className="h-6 bg-[#121620] border-t border-[#1e2538] px-3.5 flex items-center justify-between text-[9px] select-none flex-shrink-0 relative z-10">
          <div className="flex items-center space-x-3 text-muted">
            <span className="flex items-center text-indigo-400 font-bold font-mono">
              <span className="w-1.5 h-1.5 bg-indigo-500 rounded-full mr-1.5 animate-pulse"></span>
              会话实例: IPC_SOLVER_SESSION_03
            </span>
            <span>|</span>
            <span>并发查询队列: IDLE (无锁消纳)</span>
          </div>

          <div className="flex items-center space-x-3 text-muted font-mono">
            <span>当前选取范围统计:</span>
            <span>平均 Average: <strong className="text-heading">{stats.avg.toFixed(2)}</strong></span>
            <span>计数 Count: <strong className="text-heading">{stats.count}</strong></span>
            <span>极值 Min/Max: <strong className="text-heading">{stats.min} / {stats.max}</strong></span>
            <span>总和 Sum: <strong className="text-heading">{stats.sum.toFixed(2)}</strong></span>
          </div>
        </footer>
      </div>

      {/* Collapsible Right Sidebar: AI Copilot & Prescription Center */}
      {isRightPanelOpen && (
        <div className="w-80 border-l border-[#1e2538] bg-[#121620] flex flex-col select-none flex-shrink-0 animate-slide-in relative z-20 overflow-hidden">
          {/* Header */}
          <div className="h-10 bg-[#161c28] border-b border-[#1e2538] flex items-center justify-between px-3 flex-shrink-0">
            <div className="flex items-center">
              <Zap className="w-4 h-4 text-amber-500 mr-2 animate-pulse" />
              <span className="text-[10px] font-bold text-[#f8fafc]">AI Copilot & Prescription</span>
            </div>
            <button 
              onClick={() => setIsRightPanelOpen(false)}
              className="text-muted hover:text-[#f8fafc] cursor-pointer p-0.5 rounded hover:bg-[#121620] transition-all"
            >
              <X className="w-3.5 h-3.5" />
            </button>
          </div>

          {/* Telemetry Indicator */}
          <div className="bg-[#090b10] px-3 py-1.5 border-b border-[#1e2538]/50 flex items-center justify-between text-[8px] font-mono text-muted">
            <span>DOD SIMD: ACTIVE</span>
            <span>DuckDB: 12ms</span>
            <span>Mode: LBL+DBD</span>
          </div>

          {/* Scrollable Container */}
          <div className="flex-1 overflow-y-auto p-3 space-y-4 custom-fine-scrollbar min-h-0">
            {/* Presentation Demo Playbook Widget */}
            <div className="bg-[#0b0e14]/60 border border-amber-500/20 p-2.5 rounded space-y-2 flex-shrink-0">
              <div className="text-[9px] font-bold text-amber-400 uppercase tracking-wider flex items-center">
                <Play className="w-3.5 h-3.5 text-amber-500 mr-1.5 animate-pulse" />
                智能演示引导剧本 (Demo Playbook)
              </div>
              
              <div className="grid grid-cols-4 gap-1 select-none">
                <button 
                  onClick={() => triggerPlaybookStep(1)}
                  className={`py-1 text-[8.5px] font-bold rounded border transition-all cursor-pointer ${
                    activePlaybookStep === 1 
                      ? 'bg-amber-950 text-amber-400 border-amber-500/60 shadow' 
                      : 'bg-[#090b10] text-muted border-[#1e2538] hover:text-[#cbd5e1]'
                  }`}
                  title="步骤 1：侦测并定位延迟异常"
                >
                  1. 识别
                </button>
                <button 
                  onClick={() => triggerPlaybookStep(2)}
                  className={`py-1 text-[8.5px] font-bold rounded border transition-all cursor-pointer ${
                    activePlaybookStep === 2 
                      ? 'bg-amber-950 text-amber-400 border-amber-500/60 shadow' 
                      : 'bg-[#090b10] text-muted border-[#1e2538] hover:text-[#cbd5e1]'
                  }`}
                  title="步骤 2：级联 BOM 图层追溯"
                >
                  2. 追溯
                </button>
                <button 
                  onClick={() => triggerPlaybookStep(3)}
                  className={`py-1 text-[8.5px] font-bold rounded border transition-all cursor-pointer ${
                    activePlaybookStep === 3 
                      ? 'bg-amber-950 text-amber-400 border-amber-500/60 shadow' 
                      : 'bg-[#090b10] text-muted border-[#1e2538] hover:text-[#cbd5e1]'
                  }`}
                  title="步骤 3：模拟空运加急消纳"
                >
                  3. 处方
                </button>
                <button 
                  onClick={() => triggerPlaybookStep(4)}
                  className={`py-1 text-[8.5px] font-bold rounded border transition-all cursor-pointer ${
                    activePlaybookStep === 4 
                      ? 'bg-amber-950 text-amber-400 border-amber-500/60 shadow' 
                      : 'bg-[#090b10] text-muted border-[#1e2538] hover:text-[#cbd5e1]'
                  }`}
                  title="步骤 4：比对双沙盒营收 ROI"
                >
                  4. 对比
                </button>
              </div>

              {/* Playbook Instruction Text */}
              <div className="bg-[#090b10] border border-[#1e2538]/60 p-2 rounded text-[8.5px] text-main leading-relaxed">
                {activePlaybookStep === 1 && (
                  <div>
                    <strong className="text-heading">第一步：识别供需短缺</strong>
                    <br />
                    点击上方按钮自动对齐 Beijing Customer。系统触发 QBE 条件过滤，筛选出受累延迟的缺口订单记录。
                  </div>
                )}
                {activePlaybookStep === 2 && (
                  <div>
                    <strong className="text-heading">第二步：追溯级联 BOM 拓扑</strong>
                    <br />
                    自动将【层级约束】下钻至半导体级联（SEMI），左侧主图及右下 Pegging widget 呈现延迟从晶片到 VVIP 订单的阻断过程。
                  </div>
                )}
                {activePlaybookStep === 3 && (
                  <div>
                    <strong className="text-heading">第三步：下达空运分流处方</strong>
                    <br />
                    自动触发 `/resolve` 算子模拟空运，缩短 3 天提前期。在途采购补货对齐由于滞留造成的库存缺口。
                  </div>
                )}
                {activePlaybookStep === 4 && (
                  <div>
                    <strong className="text-heading">第四步：比对双沙盒大盘 ROI</strong>
                    <br />
                    渲染 What-If 大盘比对图表，呈现空运部署后（蓝色柱） Consensus Revenue 恢复至 ¥245,000,000 的效果。
                  </div>
                )}
              </div>
            </div>

            {/* Action recommends list */}
            <div className="space-y-2">
              <div className="text-[9px] font-bold text-[#cbd5e1] uppercase tracking-wider flex items-center">
                <AlertTriangle className="w-3.5 h-3.5 text-amber-500 mr-1" />
                推荐决策处方 (Prescriptions)
              </div>

              {/* Action 1: Air Reroute */}
              <div className="bg-[#090b10]/40 border border-[#1e2538] p-2.5 rounded flex flex-col space-y-1.5 transition-all hover:border-[#1e2538]/85">
                <div className="flex justify-between items-center">
                  <span className="font-bold text-indigo-400 text-[9.5px]">深圳空运分流加急</span>
                  <span className="text-[8px] bg-indigo-950 text-indigo-400 border border-indigo-900 px-1 py-0.2 rounded font-mono font-bold">REROUTE</span>
                </div>
                <p className="text-[8.5px] text-muted leading-relaxed">
                  陆运发生滞留。空运重组提前期偏移（-3天），可抢回上海中继齐套料，保障 **¥12,000,000** 战略销售订单按期交付。
                </p>
                <div className="flex justify-between items-center pt-1.5 border-t border-[#1e2538]/30">
                  <span className="text-[8px] font-mono text-muted">处方成本: ¥50,000</span>
                  <button
                    onClick={() => executeResolveAction(activeResolutions['REROUTE_AIR'] ? 'RESET_REROUTE_AIR' : 'REROUTE_AIR')}
                    className={`px-2.5 py-0.5 rounded text-[8.5px] font-bold border transition-all cursor-pointer ${
                      activeResolutions['REROUTE_AIR']
                        ? 'bg-rose-950 text-rose-400 border-rose-950 hover:bg-rose-900'
                        : 'bg-indigo-950 text-indigo-400 border-indigo-900 hover:bg-indigo-900'
                    }`}
                  >
                    {activeResolutions['REROUTE_AIR'] ? '撤销加急' : '一键加急'}
                  </button>
                </div>
              </div>

              {/* Action 2: Overtime Capacity */}
              <div className="bg-[#090b10]/40 border border-[#1e2538] p-2.5 rounded flex flex-col space-y-1.5 transition-all hover:border-[#1e2538]/85">
                <div className="flex justify-between items-center">
                  <span className="font-bold text-purple-400 text-[9.5px]">成都封测加班消峰</span>
                  <span className="text-[8px] bg-purple-950 text-purple-400 border border-purple-900 px-1 py-0.2 rounded font-mono font-bold">CAPACITY</span>
                </div>
                <p className="text-[8.5px] text-muted leading-relaxed">
                  车间封装设备在 D8-D12 发生排产产能过载。核准加班可提升小时产能，平抑计划缺口。
                </p>
                <div className="flex justify-between items-center pt-1.5 border-t border-[#1e2538]/30">
                  <span className="text-[8px] font-mono text-muted">处方成本: ¥20,000</span>
                  <button
                    onClick={() => executeResolveAction(activeResolutions['OVERTIME_CAPACITY'] ? 'RESET_OVERTIME_CAPACITY' : 'OVERTIME_CAPACITY')}
                    className={`px-2.5 py-0.5 rounded text-[8.5px] font-bold border transition-all cursor-pointer ${
                      activeResolutions['OVERTIME_CAPACITY']
                        ? 'bg-rose-950 text-rose-400 border-rose-950 hover:bg-rose-900'
                        : 'bg-purple-950 text-purple-400 border-purple-900 hover:bg-purple-900'
                    }`}
                  >
                    {activeResolutions['OVERTIME_CAPACITY'] ? '撤销加班' : '核准加班'}
                  </button>
                </div>
              </div>

              {/* Action 3: Upgrade Chip */}
              <div className="bg-[#090b10]/40 border border-[#1e2538] p-2.5 rounded flex flex-col space-y-1.5 transition-all hover:border-[#1e2538]/85">
                <div className="flex justify-between items-center">
                  <span className="font-bold text-emerald-400 text-[9.5px]">高规格芯片库平移替代</span>
                  <span className="text-[8px] bg-emerald-950 text-emerald-400 border border-emerald-900 px-1 py-0.2 rounded font-mono font-bold">SUBSTITUTE</span>
                </div>
                <p className="text-[8.5px] text-muted leading-relaxed">
                  256MB 合同物料配额短缺。利用 512MB 闲置库存作为高规格向下替代，自动对齐 MRP 消耗链。
                </p>
                <div className="flex justify-between items-center pt-1.5 border-t border-[#1e2538]/30">
                  <span className="text-[8px] font-mono text-muted">处方成本: ¥0</span>
                  <button
                    onClick={() => executeResolveAction(activeResolutions['UPGRADE_CHIP'] ? 'RESET_UPGRADE_CHIP' : 'UPGRADE_CHIP')}
                    className={`px-2.5 py-0.5 rounded text-[8.5px] font-bold border transition-all cursor-pointer ${
                      activeResolutions['UPGRADE_CHIP']
                        ? 'bg-rose-950 text-rose-400 border-rose-950 hover:bg-rose-900'
                        : 'bg-emerald-950 text-emerald-400 border-emerald-900 hover:bg-emerald-900'
                    }`}
                  >
                    {activeResolutions['UPGRADE_CHIP'] ? '撤销替代' : '一键升级'}
                  </button>
                </div>
              </div>
            </div>

            {/* Live BOM Visualizer & ROI Sandbox Chart */}
            {renderBompPeggingWidget()}
            {renderSandboxComparisonChart()}

            {/* AI Copilot chat list */}
            <div className="border-t border-[#1e2538] pt-3.5 space-y-2 flex-1 flex flex-col min-h-[220px]">
              <div className="text-[9px] font-bold text-[#cbd5e1] uppercase tracking-wider flex items-center">
                <Users className="w-3.5 h-3.5 text-indigo-400 mr-1" />
                AI SCM 协伴专家 (Copilot Chat)
              </div>

              {/* Message Feed */}
              <div className="flex-1 bg-[#090b10]/50 border border-[#1e2538] rounded p-2.5 overflow-y-auto space-y-2.5 text-[8.5px] max-h-64 custom-fine-scrollbar">
                {copilotMessages.map((msg, index) => {
                  const isUser = msg.sender === 'user';
                  return (
                    <div 
                      key={index} 
                      className={`flex flex-col space-y-0.5 max-w-[85%] ${
                        isUser ? 'ml-auto items-end' : 'mr-auto items-start'
                      }`}
                    >
                      <div className={`p-2 rounded text-[8.5px] leading-relaxed whitespace-pre-line break-words ${
                        isUser ? 'bg-indigo-650 text-white rounded-tr-none' : 'bg-[#161c28] text-main border border-[#1e2538]/60 rounded-tl-none'
                      }`}>
                        {msg.text}
                      </div>
                      <span className="text-[7.5px] text-muted font-mono">{msg.timestamp}</span>
                    </div>
                  );
                })}
              </div>

              {/* Chat Input */}
              <div className="flex space-x-1.5 pt-1.5">
                <input 
                  type="text" 
                  value={chatInput}
                  onChange={(e) => setChatInput(e.target.value)}
                  onKeyDown={(e) => {
                    if (e.key === 'Enter') {
                      handleSendChatMessage(e);
                    }
                  }}
                  placeholder="询问 Copilot 或下达重算指令..." 
                  className="flex-1 bg-[#090b10] border border-[#1e2538] text-heading rounded px-2.5 py-1.5 text-[9px] outline-none focus:border-indigo-500 placeholder-muted"
                />
                <button 
                  type="button"
                  onClick={handleSendChatMessage}
                  className="px-2.5 py-1.5 bg-indigo-600 hover:bg-indigo-500 text-white rounded font-bold text-[9px] border border-indigo-500 transition-colors cursor-pointer"
                >
                  发送
                </button>
              </div>
            </div>
          </div>
        </div>
      )}

      {/* Dynamic Right Click Context Menu */}
      {contextMenu && (
        <div 
          className="fixed bg-[#121620] border border-[#1e2538] rounded shadow-2xl py-1.5 min-w-[150px] z-50 animate-fade-in select-none text-[9.5px] backdrop-blur-md"
          style={{ top: contextMenu.y, left: contextMenu.x }}
          onClick={(e) => e.stopPropagation()}
        >
          <div className="px-2.5 py-1 text-heading font-mono font-bold bg-[#161c28] border-b border-[#1e2538] mb-1">
            {contextMenu.tableName}
          </div>
          
          <button 
            onClick={() => {
              handleDescribeTableSchema(contextMenu.tableName);
              setContextMenu(null);
            }}
            className="w-full text-left px-3 py-1.5 text-[#f8fafc] hover:bg-indigo-600 hover:text-white flex items-center space-x-2 transition-colors"
          >
            <Info className="w-3.5 h-3.5 text-indigo-400 group-hover:text-white" />
            <span>📋 调阅元数据 (Schema)</span>
          </button>

          <button 
            onClick={() => {
              handleRecompileOperators();
              setContextMenu(null);
            }}
            className="w-full text-left px-3 py-1.5 text-[#f8fafc] hover:bg-indigo-600 hover:text-white flex items-center space-x-2 transition-colors"
          >
            <Play className="w-3.5 h-3.5 text-emerald-400" />
            <span>⚡ 重算本表算子 (MRP)</span>
          </button>

          <button 
            onClick={() => {
              setShowQbeForm(true);
              setContextMenu(null);
            }}
            className="w-full text-left px-3 py-1.5 text-[#f8fafc] hover:bg-indigo-600 hover:text-white flex items-center space-x-2 transition-colors"
          >
            <Sliders className="w-3.5 h-3.5 text-amber-500" />
            <span>🔍 QBE 条件过滤 (Filter)</span>
          </button>

          <button 
            onClick={() => {
              handleExportCSV(contextMenu.tableName);
              setContextMenu(null);
            }}
            className="w-full text-left px-3 py-1.5 text-[#f8fafc] hover:bg-indigo-600 hover:text-white border-t border-[#1e2538] mt-1 pt-1.5 flex items-center space-x-2 transition-colors"
          >
            <Download className="w-3.5 h-3.5 text-purple-400" />
            <span>💾 导出为 CSV (Export)</span>
          </button>
        </div>
      )}

      {/* Describe Table Schema Modal Dialog */}
      {schemaModal && (
        <div className="fixed inset-0 bg-[#0b0e14]/85 backdrop-blur-sm flex items-center justify-center z-50 p-4 select-none animate-fade-in">
          <div className="bg-[#121620] border border-[#1e2538] rounded-md max-w-lg w-full p-4 space-y-3 shadow-2xl">
            <div className="flex justify-between items-center border-b border-[#1e2538] pb-2">
              <span className="font-bold text-indigo-400 flex items-center text-[11px] font-mono">
                <Database className="w-4 h-4 mr-2 text-indigo-500" />
                DESCRIBE SCHEMA: {schemaModal.tableName}
              </span>
              <button onClick={() => setSchemaModal(null)} className="text-muted hover:text-[#f8fafc] p-0.5 rounded hover:bg-[#161c28]">
                <X className="w-4 h-4" />
              </button>
            </div>
            
            <p className="text-[9.5px] text-[#94a3b8] leading-relaxed bg-[#090b10] border border-[#1e2538]/50 p-2 rounded">
              <strong>表功能描述:</strong> {schemaModal.desc}
            </p>
            
            <div className="max-h-60 overflow-y-auto border border-[#1e2538] rounded custom-fine-scrollbar">
              <table className="w-full text-left border-collapse text-[9px]">
                <thead>
                  <tr className="bg-[#161c28] text-muted font-bold border-b border-[#1e2538]">
                    <th className="p-2 border-r border-[#1e2538]">列字段 (Column Name)</th>
                    <th className="p-2 border-r border-[#1e2538]">类型 (Data Type)</th>
                    <th className="p-2 border-r border-[#1e2538] text-center">可空</th>
                    <th className="p-2 border-r border-[#1e2538] text-center">Key</th>
                    <th className="p-2 text-center">默认值</th>
                  </tr>
                </thead>
                <tbody>
                  {schemaModal.columns.map((col, idx) => (
                    <tr key={idx} className="hover:bg-[#1c2335]/40 border-b border-[#1e2538]/50 text-main font-mono">
                      <td className="p-2 border-r border-[#1e2538] font-bold text-heading">{col.column_name}</td>
                      <td className="p-2 border-r border-[#1e2538] text-[#8b5cf6]">{col.column_type}</td>
                      <td className="p-2 border-r border-[#1e2538] text-center">{col.null === 'YES' ? 'YES' : 'NO'}</td>
                      <td className="p-2 border-r border-[#1e2538] text-center text-amber-500 font-bold">{col.key || '-'}</td>
                      <td className="p-2 text-center text-muted">{col.default || 'NULL'}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>

            <div className="flex justify-end pt-2 border-t border-[#1e2538]">
              <button 
                onClick={() => setSchemaModal(null)}
                className="px-4 py-1.5 bg-indigo-600 hover:bg-indigo-500 text-white rounded font-bold text-[9.5px] border border-indigo-500 transition-colors shadow-lg"
              >
                关闭元数据窗口
              </button>
            </div>
          </div>
        </div>
      )}

      {/* QBE Form Modal Popup Container */}
      {showQbeForm && (
        <div className="fixed inset-0 bg-[#0b0e14]/90 backdrop-blur-md flex items-center justify-center z-50 p-4 select-none animate-fade-in">
          <div className="bg-[#121620] border border-[#1e2538] rounded p-4 max-w-sm w-full space-y-3 shadow-2xl">
            <div className="flex justify-between items-center border-b border-[#1e2538] pb-1.5">
              <span className="font-bold text-orange-400 flex items-center text-[10.5px]">
                <AlertOctagon className="w-4 h-4 mr-1.5 text-orange-500" />
                QBE (Query By Example) 精确过滤器
              </span>
              <button onClick={() => setShowQbeForm(false)} className="text-muted hover:text-[#f8fafc]">
                <X className="w-4 h-4" />
              </button>
            </div>
            <p className="text-[9px] text-main leading-relaxed">
              当前内存数据池膨胀率过载。请使用多级索引算子进行断言过滤，以恢复底层 DOD 计算吞吐：
            </p>
            
            <div className="space-y-2 text-[10px]">
              <div className="flex flex-col space-y-1">
                <span className="text-muted">选择物理生产站点 (Site Filter)</span>
                <select 
                  value={qbeFilters.site} 
                  onChange={(e) => setQbeFilters(prev => ({ ...prev, site: e.target.value }))}
                  className="bg-[#090b10] border border-[#1e2538] text-[#f8fafc] p-1.5 rounded outline-none cursor-pointer"
                >
                  <option value="ALL">ALL_SITES (全网物料)</option>
                  <option value="CD_SLICING_FACTORY">成都芯片工厂 (CD_SLICING_FAC)</option>
                  <option value="SH_ASSEMBLY_HUB">上海封装中心 (SH_ASSEMBLY_HUB)</option>
                </select>
              </div>

              <div className="flex flex-col space-y-1">
                <span className="text-muted">选择物料主分类 (Category)</span>
                <select 
                  value={qbeFilters.category} 
                  onChange={(e) => setQbeFilters(prev => ({ ...prev, category: e.target.value }))}
                  className="bg-[#090b10] border border-[#1e2538] text-[#f8fafc] p-1.5 rounded outline-none cursor-pointer"
                >
                  <option value="ALL">ALL_CATEGORIES (全分类)</option>
                  <option value="Semiconductor">半导体晶圆 (Semiconductor)</option>
                  <option value="Memory">存储芯片组 (Memory)</option>
                </select>
              </div>
            </div>

            <div className="flex justify-end space-x-2 pt-2 border-t border-[#1e2538]">
              <button 
                onClick={() => setShowQbeForm(false)}
                className="px-3 py-1 bg-transparent hover:bg-slate-800 text-muted hover:text-heading border border-[#1e2538] rounded text-[9.5px]"
              >
                取消
              </button>
              <button 
                onClick={applyQbeFilters}
                className="px-3 py-1 bg-orange-600 hover:bg-orange-500 text-white rounded font-bold text-[9.5px] border border-orange-500"
              >
                应用过滤条件 (重置为安全规模)
              </button>
            </div>
          </div>
        </div>
      )}
      
    </div>
  );
};

export default ControlTower;
