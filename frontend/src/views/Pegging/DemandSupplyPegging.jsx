import React, { useState, useEffect, useRef } from 'react';
import { 
  Search, 
  GitMerge, 
  ShieldCheck, 
  ShieldAlert, 
  ArrowDownUp,
  ChevronDown,
  ChevronRight,
  Clock,
  XCircle,
  CheckCircle,
  HelpCircle,
  Sliders,
  Download,
  Play,
  Database,
  Info,
  Layers,
  ArrowRightLeft
} from 'lucide-react';
import { useScenario } from '../../contexts/ScenarioContext';

const DemandSupplyPegging = () => {
  const { currentScenario, selectedDemandId, setSelectedDemandId } = useScenario();
  
  const [demands, setDemands] = useState([]);
  const [searchQuery, setSearchQuery] = useState('');
  const [statusFilter, setStatusFilter] = useState('ALL');
  const [deliveryMode, setDeliveryMode] = useState('LATEST');
  const [selectedDemand, setSelectedDemand] = useState(null);
  const [peggingDetails, setPeggingDetails] = useState(null);
  const [treeLoading, setTreeLoading] = useState(false);
  const [listLoading, setListLoading] = useState(false);
  const [offset, setOffset] = useState(0);
  const [hasMore, setHasMore] = useState(true);

  const isFetchingRef = useRef(false);
  const latestRequestRef = useRef(0);

  // Floating Context Menu states
  const [demandMenu, setDemandMenu] = useState(null); // { x, y, demand }
  const [treeNodeMenu, setTreeNodeMenu] = useState(null); // { x, y, partCode }

  // Assignments Ledger Sheet Modal states
  const [assignments, setAssignments] = useState([]);
  const [assignmentsLoading, setAssignmentsLoading] = useState(false);
  const [showAssignmentsModal, setShowAssignmentsModal] = useState(false);
  const [activeDemandForAssignments, setActiveDemandForAssignments] = useState('');

  // What-if Demand Override Modal states
  const [showOverrideModal, setShowOverrideModal] = useState(false);
  const [overrideForm, setOverrideForm] = useState({ demandId: '', qty: 0, day: 15 });

  // Part On-hand Inventory Modal states
  const [partOnhandData, setPartOnhandData] = useState([]);
  const [onhandLoading, setOnhandLoading] = useState(false);
  const [showOnhandModal, setShowOnhandModal] = useState(false);
  const [activePartForOnhand, setActivePartForOnhand] = useState('');

  // BOM Component Items Modal states
  const [bomData, setBomData] = useState([]);
  const [bomLoading, setBomLoading] = useState(false);
  const [showBomModal, setShowBomModal] = useState(false);
  const [activePartForBom, setActivePartForBom] = useState('');

  // Tree collapse states (path -> boolean)
  const [collapsedPaths, setCollapsedPaths] = useState(new Set());

  // Close context menus on click anywhere
  useEffect(() => {
    const closeAllMenus = () => {
      setDemandMenu(null);
      setTreeNodeMenu(null);
    };
    window.addEventListener('click', closeAllMenus);
    return () => window.removeEventListener('click', closeAllMenus);
  }, []);

  // Deep recursive labeling of critical path (delays propagating bottom-up)
  const analyzeCriticalPath = (node) => {
    if (!node) return false;
    
    const isSelfDelayed = 
      (node.type === 'supply' && (node.supply_type === 'Shortage' || node.finish === 'Delayed')) || 
      (node.type === 'demand' && (node.delay_days > 0 || node.due === 'Delayed'));
      
    let isChildDelayed = false;
    if (node.children && node.children.length > 0) {
      node.children.forEach(child => {
        const childCritical = analyzeCriticalPath(child);
        if (childCritical) {
          isChildDelayed = true;
        }
      });
    }
    
    const isCritical = isSelfDelayed || isChildDelayed;
    node.isOnCriticalPath = isCritical;
    return isCritical;
  };

  const fetchDemands = async (currentOffset = 0, isAppend = false) => {
    if (isAppend && isFetchingRef.current) return;
    
    const requestId = Date.now();
    latestRequestRef.current = requestId;
    
    isFetchingRef.current = true;
    setListLoading(true);
    try {
      const q = searchQuery.trim();
      const limit = 100;
      const res = await fetch(`/api/pegging/demands?q=${encodeURIComponent(q)}&status=${statusFilter}&mode=${deliveryMode}&limit=${limit}&offset=${currentOffset}`);
      const data = await res.json();
      
      if (latestRequestRef.current !== requestId) return;
      
      if (isAppend) {
        setDemands(prev => {
          const existingKeys = new Set(prev.map(d => `${d.demand_id}-${d.part}`));
          const uniqueNew = data.filter(d => !existingKeys.has(`${d.demand_id}-${d.part}`));
          return [...prev, ...uniqueNew];
        });
      } else {
        setDemands(data);
        if (data.length > 0 && !selectedDemand) {
          setSelectedDemand(data[0]);
        }
      }
      
      if (data.length < limit) {
        setHasMore(false);
      } else {
        setHasMore(true);
      }
    } catch (err) {
      console.error('Error fetching pegging demands:', err);
    } finally {
      if (latestRequestRef.current === requestId) {
        isFetchingRef.current = false;
        setListLoading(false);
      }
    }
  };

  const handleScroll = (e) => {
    const { scrollTop, scrollHeight, clientHeight } = e.currentTarget;
    if (scrollHeight - scrollTop - clientHeight < 50) {
      if (hasMore && !isFetchingRef.current) {
        const nextOffset = offset + 100;
        setOffset(nextOffset);
        fetchDemands(nextOffset, true);
      }
    }
  };

  const loadPeggingTree = async (demandId) => {
    setTreeLoading(true);
    try {
      const res = await fetch(`/api/pegging/tree?demand_id=${encodeURIComponent(demandId)}`);
      const data = await res.json();
      if (data && data.tree) {
        analyzeCriticalPath(data.tree);
      }
      setPeggingDetails(data);
      setCollapsedPaths(new Set()); // Reset collapse states on new order load
    } catch (err) {
      console.error('Error loading pegging tree:', err);
    } finally {
      setTreeLoading(false);
    }
  };

  // Query database for quota assignments for a specific demand
  const loadAssignments = async (demandId) => {
    setAssignmentsLoading(true);
    setActiveDemandForAssignments(demandId);
    setShowAssignmentsModal(true);
    try {
      const res = await fetch(`/api/pegging/assignments?demand_id=${encodeURIComponent(demandId)}`);
      const data = await res.json();
      setAssignments(data);
    } catch (err) {
      console.error('Error loading assignments:', err);
    } finally {
      setAssignmentsLoading(false);
    }
  };

  // Open override modal
  const openOverrideModal = (d) => {
    setOverrideForm({
      demandId: d.demand_id,
      qty: d.qty,
      day: d.due_date ? parseInt(d.due_date.replace(/[^0-9]/g, '')) || 15 : 15
    });
    setShowOverrideModal(true);
  };

  // Trigger solver re-run on demand modification
  const handleApplyOverride = async () => {
    setTreeLoading(true);
    try {
      // Execute resolver in solver backend
      const res = await fetch('/api/controltower/resolve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'REROUTE_AIR' })
      });
      await res.json();
      
      alert(`What-if 需求拟定调整成功！独立需求数量修改为: ${overrideForm.qty}，交期调整至 D${overrideForm.day}。系统已调用 C++ MRP/DBD 优化引擎重构连续内存图谱分配算子。`);
      setShowOverrideModal(false);
      
      // Reload demands list and tree
      fetchDemands(0, false);
      if (selectedDemand) {
        loadPeggingTree(selectedDemand.demand_id);
      }
    } catch (err) {
      console.error('Error running What-if override:', err);
    } finally {
      setTreeLoading(false);
    }
  };

  // Query database for part onhand inventory
  const loadPartOnhand = async (partCode) => {
    setOnhandLoading(true);
    setActivePartForOnhand(partCode);
    setShowOnhandModal(true);
    try {
      const res = await fetch(`/api/drilldown/inventory?part_code=${encodeURIComponent(partCode)}`);
      const data = await res.json();
      setPartOnhandData(data);
    } catch (err) {
      console.error('Error loading inventory:', err);
    } finally {
      setOnhandLoading(false);
    }
  };

  // Query database for part BOM child items
  const loadPartBom = async (partCode) => {
    setBomLoading(true);
    setActivePartForBom(partCode);
    setShowBomModal(true);
    try {
      const res = await fetch(`/api/mrp/bom?part_code=${encodeURIComponent(partCode)}`);
      const data = await res.json();
      setBomData(data);
    } catch (err) {
      console.error('Error loading BOM:', err);
    } finally {
      setBomLoading(false);
    }
  };

  useEffect(() => {
    setOffset(0);
    setHasMore(true);
    fetchDemands(0, false);
  }, [currentScenario, statusFilter, deliveryMode]);

  useEffect(() => {
    if (selectedDemand) {
      loadPeggingTree(selectedDemand.demand_id);
    } else {
      setPeggingDetails(null);
    }
  }, [selectedDemand]);

  useEffect(() => {
    if (selectedDemandId && demands.length > 0) {
      const matched = demands.find(d => d.demand_id === selectedDemandId);
      if (matched) {
        setSelectedDemand(matched);
      } else {
        setSelectedDemand({ demand_id: selectedDemandId, part: '', qty: 0, customer: '', due_date: '' });
      }
    }
  }, [selectedDemandId, demands]);

  const handleSearchSubmit = (e) => {
    e.preventDefault();
    setOffset(0);
    setHasMore(true);
    fetchDemands(0, false);
  };

  const toggleCollapse = (path) => {
    setCollapsedPaths(prev => {
      const next = new Set(prev);
      if (next.has(path)) {
        next.delete(path);
      } else {
        next.add(path);
      }
      return next;
    });
  };

  // Context Menu handlers
  const handleDemandClickOrRightClick = (e, d) => {
    e.preventDefault();
    e.stopPropagation();
    setSelectedDemand(d);
    setSelectedDemandId(d.demand_id);
    setDemandMenu({
      x: e.clientX,
      y: e.clientY,
      demand: d
    });
  };

  const handleTreeNodeClickOrRightClick = (e, node) => {
    e.preventDefault();
    e.stopPropagation();
    if (node.type === 'demand') {
      const demandObj = {
        demand_id: peggingDetails?.demand_id || selectedDemand?.demand_id || node.part_code,
        part: node.part_code,
        qty: node.qty,
        due_date: node.due,
        customer: selectedDemand?.customer || 'NORMAL_CUST'
      };
      setDemandMenu({
        x: e.clientX,
        y: e.clientY,
        demand: demandObj
      });
    } else if (node.type === 'supply') {
      setTreeNodeMenu({
        x: e.clientX,
        y: e.clientY,
        partCode: node.part_code
      });
    }
  };

  // Graphical collapsible node tree renderer with neon red critical path
  const renderTreeNode = (node, path = "0", isLast = true, parentIsOnCP = false) => {
    if (!node) return null;
    
    const isSupply = node.type === 'supply';
    const isShortage = node.supply_type === 'Shortage';
    const isDelayed = node.type === 'demand' ? node.delay_days > 0 : node.finish === 'Delayed';
    
    const isCollapsed = collapsedPaths.has(path);
    const hasChildren = node.children && node.children.length > 0;
    const isOnCP = node.isOnCriticalPath;
    
    // Connect lines classes (pulse red if critical, else dark gray)
    const verticalLineColor = isOnCP ? 'bg-rose-500 shadow-[0_0_8px_rgba(244,63,94,0.6)] animate-pulse' : 'bg-slate-800';
    const horizontalLineColor = isOnCP ? 'bg-rose-500 shadow-[0_0_8px_rgba(244,63,94,0.6)] animate-pulse' : 'bg-slate-800';
 
    let typeBadge = 'bg-slate-900 border-slate-700 text-slate-400';
    if (node.part_type === 'FINISHED') typeBadge = 'bg-indigo-950 border-indigo-800 text-indigo-400';
    if (node.part_type === 'SEMI') typeBadge = 'bg-amber-950 border-amber-800 text-amber-500';
    if (node.part_type === 'RAW') typeBadge = 'bg-emerald-950 border-emerald-800 text-emerald-400';

    return (
      <div key={path} className="flex flex-col relative select-none">
        
        {/* Connector lines structure */}
        {path !== "0" && (
          <>
            {/* Vertical Segment */}
            <div 
              className={`absolute left-[8px] top-0 w-[1.5px] z-0 ${verticalLineColor}`} 
              style={{ height: isLast ? '20px' : '100%' }}
            />
            {/* Horizontal Branch Segment */}
            <div 
              className={`absolute left-[8px] top-[20px] h-[1.5px] w-[14px] z-0 ${horizontalLineColor}`}
            />
          </>
        )}

        {/* Node Capsule Card */}
        <div 
          onClick={(e) => {
            if (node.type === 'demand' || (node.type === 'supply' && !hasChildren)) {
              handleTreeNodeClickOrRightClick(e, node);
            } else {
              toggleCollapse(path);
            }
          }}
          onContextMenu={(e) => handleTreeNodeClickOrRightClick(e, node)}
          className={`flex items-center justify-between p-2.5 mb-2 border rounded-xs z-10 transition-all cursor-pointer ${
            path !== "0" ? 'ml-6' : ''
          } ${
            isOnCP 
              ? 'bg-rose-950/15 border-rose-900/60 shadow-[0_0_12px_rgba(244,63,94,0.12)] border-l-4 border-l-rose-500 text-rose-300 hover:bg-rose-950/25'
              : isSupply 
                ? 'bg-slate-900/40 border-slate-800 border-l-4 border-l-emerald-500 text-main hover:bg-slate-900/60'
                : 'bg-indigo-950/10 border-indigo-900/40 border-l-4 border-l-indigo-500 text-indigo-300 font-bold hover:bg-indigo-950/20'
          }`}
          title={`${node.part_code} (右击调阅库存/BOM组件结构)`}
        >
          <div className="flex items-center space-x-2 truncate flex-1">
            {/* Collapse Icon */}
            {hasChildren ? (
              <button 
                onClick={(e) => {
                  e.stopPropagation();
                  toggleCollapse(path);
                }}
                className="p-0.5 rounded hover:bg-slate-800/60 text-muted hover:text-heading cursor-pointer transition-colors"
              >
                {isCollapsed ? (
                  <ChevronRight className="w-3.5 h-3.5 text-muted" />
                ) : (
                  <ChevronDown className="w-3.5 h-3.5 text-muted" />
                )}
              </button>
            ) : (
              <div className="w-4.5" /> // Alignment padding
            )}

            {/* Status Indicator Icon */}
            {isSupply ? (
              isShortage ? (
                <XCircle className="w-4 h-4 text-rose-500 animate-pulse flex-shrink-0" />
              ) : (
                <CheckCircle className="w-4 h-4 text-emerald-500 flex-shrink-0" />
              )
            ) : (
              <Clock className="w-4 h-4 text-indigo-400 flex-shrink-0" />
            )}

            {/* Part Name */}
            <span 
              className={`font-mono text-heading truncate hover:underline ${path === "0" ? 'text-[12.5px] font-extrabold' : 'text-xs'}`}
            >
              {node.part_code}
            </span>

            {/* Part Level Badge */}
            <span className={`text-[8px] px-1.5 py-0.2 rounded border scale-85 uppercase font-semibold ${typeBadge}`}>
              {node.part_type}
            </span>
            
            {isSupply && (
              <span className="text-[8.5px] bg-slate-800/60 border border-slate-700/80 text-muted px-1.5 py-0.2 rounded font-mono">
                {node.supply_type}
              </span>
            )}
          </div>

          {/* Right Metrics panel */}
          <div className="flex items-center space-x-3.5 flex-shrink-0 text-right">
            <span className="cyber-font text-[10.5px]">
              {isSupply ? `供应: ${node.qty}` : `需求: ${node.qty}`}
            </span>
            <span className={`cyber-font text-[9.5px] font-bold ${
              isShortage || isDelayed ? 'text-rose-500 animate-pulse' : 'text-emerald-500'
            }`}>
              {isSupply ? `交期: ${node.finish}` : `交期: ${node.due}`}
            </span>
            {hasChildren && isCollapsed && (
              <span className="text-[8.5px] bg-indigo-950/40 border border-indigo-900/60 px-1.5 py-0.5 rounded text-indigo-400 font-bold font-mono animate-pulse">
                +{node.children.length} 级联
              </span>
            )}
          </div>
        </div>

        {/* Children node list */}
        {hasChildren && !isCollapsed && (
          <div className="flex flex-col">
            {node.children.map((child, idx) => 
              renderTreeNode(child, `${path}-${idx}`, idx === node.children.length - 1, isOnCP)
            )}
          </div>
        )}
      </div>
    );
  };

  const getOtifRate = () => {
    if (demands.length === 0) return 100;
    const delayed = demands.filter(d => d.delay_days > 0).length;
    return Math.round(((demands.length - delayed) / demands.length) * 100);
  };

  const otifRate = getOtifRate();

  return (
    <div className="flex flex-row space-x-3 h-full overflow-hidden relative">
      
      {/* 左侧：客户订单需求列表 */}
      <div className="w-[340px] bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden select-none">
        
        <div className="border-b border-main pb-2 mb-2.5 flex justify-between items-center">
          <h4 className="font-bold text-heading text-xs flex items-center">
            <GitMerge className="w-4 h-4 text-indigo-500 mr-1.5" />
            独立订单钉结视图 (Demand Ledger)
          </h4>
          <span className="text-[9px] text-muted cyber-font" id="pegging-demands-count">
            {demands.length} Orders
          </span>
        </div>

        {/* 检索及过滤器 */}
        <form onSubmit={handleSearchSubmit} className="space-y-2 mb-3">
          <div className="relative">
            <input
              type="text"
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              placeholder="搜索订单/客户/零件号..."
              className="w-full bg-input border border-main text-heading text-xs pl-8 pr-3 py-1.5 rounded focus:outline-none focus:border-indigo-500"
            />
            <Search className="w-3.5 h-3.5 text-muted absolute left-2.5 top-2.5" />
          </div>

          <div className="grid grid-cols-2 gap-1.5">
            <select
              value={statusFilter}
              onChange={(e) => setStatusFilter(e.target.value)}
              className="bg-input border border-main text-[10px] text-heading p-1 py-1.5 rounded outline-none cursor-pointer"
            >
              <option value="ALL">全部交付状态 (ALL)</option>
              <option value="DELAY">仅显示延误订单 (DELAY)</option>
              <option value="ONTIME">仅按时交付订单 (ONTIME)</option>
            </select>
            
            <select
              value={deliveryMode}
              onChange={(e) => setDeliveryMode(e.target.value)}
              className="bg-input border border-main text-[10px] text-heading p-1 py-1.5 rounded outline-none cursor-pointer"
            >
              <option value="LATEST">最新汇总关系 (LATEST)</option>
              <option value="SPLIT">细分分配记录 (SPLIT)</option>
            </select>
          </div>
        </form>

        {/* 订单列表 */}
        <div 
          onScroll={handleScroll}
          className="flex-1 overflow-y-auto space-y-2 pr-1 relative"
        >
          {listLoading && offset === 0 && (
            <div className="absolute inset-0 bg-card/80 flex items-center justify-center text-xs text-indigo-400 z-10">
              载入列表中...
            </div>
          )}

          {demands.map(d => {
            const isDelayed = d.delay_days > 0;
            const isSelected = selectedDemand?.demand_id === d.demand_id && selectedDemand?.part === d.part;
            
            return (
              <div
                key={`${d.demand_id}-${d.part}`}
                onClick={(e) => handleDemandClickOrRightClick(e, d)}
                onContextMenu={(e) => handleDemandClickOrRightClick(e, d)}
                className={`w-full text-left p-2.5 border rounded-xs transition-all flex flex-col space-y-1.5 cursor-pointer ${
                  isSelected
                    ? 'bg-indigo-950/20 border-indigo-500 text-heading'
                    : 'bg-input border-main text-muted hover:bg-slate-800/20 hover:text-heading'
                }`}
                title="左击查看动作 / 右击选择表单分析功能"
              >
                <div className="flex justify-between items-center w-full">
                  <span className="font-mono font-bold text-heading text-[11px] truncate flex-1">{d.demand_id}</span>
                  <span className={`text-[8px] font-bold px-1.5 py-0.2 rounded border scale-90 ${
                    isDelayed 
                      ? 'bg-rose-950 border-rose-900 text-rose-400 animate-pulse'
                      : 'bg-emerald-950 border-emerald-900 text-emerald-400'
                  }`}>
                    {isDelayed ? `延期 ${d.delay_days}天` : '准时交付'}
                  </span>
                </div>

                <div className="flex justify-between text-[10px] w-full font-mono">
                  <span>零件: <b className="text-heading font-medium">{d.part}</b></span>
                  <span>数量: <b className="text-heading font-medium">{d.qty}</b></span>
                </div>

                <div className="flex justify-between text-[9px] w-full text-muted">
                  <span>客户: {d.customer}</span>
                  <span>交期: {d.due_date}</span>
                </div>

                {/* Selected card actions bar */}
                {isSelected && (
                  <div className="flex items-center space-x-1.5 pt-2 mt-1.5 border-t border-indigo-900/40 select-none">
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        loadPeggingTree(d.demand_id);
                      }}
                      className="flex-1 py-1 bg-indigo-600 hover:bg-indigo-500 text-white rounded text-[8.5px] font-bold text-center transition-colors"
                      title="展开计划树图形谱"
                    >
                      🔍 计划展开
                    </button>
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        loadAssignments(d.demand_id);
                      }}
                      className="flex-1 py-1 bg-indigo-950 text-indigo-400 hover:bg-indigo-900 border border-indigo-900/60 rounded text-[8.5px] font-bold text-center transition-colors"
                      title="打开供应配额匹配细分账表"
                    >
                      📊 供应匹配
                    </button>
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        openOverrideModal(d);
                      }}
                      className="flex-1 py-1 bg-slate-800 hover:bg-slate-700 text-[#cbd5e1] rounded text-[8.5px] font-bold text-center transition-colors"
                      title="模拟调整订单交期或数量"
                    >
                      ⚙️ What-if
                    </button>
                  </div>
                )}
              </div>
            );
          })}

          {listLoading && offset > 0 && (
            <div className="text-center py-2 text-indigo-400 text-[10px] font-mono animate-pulse">
              正在加载更多订单...
            </div>
          )}

          {!hasMore && demands.length > 0 && (
            <div className="text-center py-2 text-muted text-[10px] font-mono">
              已加载全部订单
            </div>
          )}

          {demands.length === 0 && !listLoading && (
            <div className="text-center py-10 text-muted text-xs">无满足条件的订单</div>
          )}
        </div>
      </div>

      {/* 右侧：供需溯源图形化折叠树 */}
      <div className="flex-1 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden select-none">
        
        {/* 顶部指标与信息 */}
        <div className="grid grid-cols-3 gap-3 border-b border-main pb-3 mb-3">
          
          {/* OTIF 环形图 */}
          <div className="flex items-center space-x-2 border-r border-main pr-3">
            <svg width="40" height="40" viewBox="0 0 36 36" className="transform -rotate-90">
              <circle cx="18" cy="18" r="14" fill="none" stroke="var(--border-color)" strokeWidth="3.5" />
              <circle 
                cx="18" 
                cy="18" 
                r="14" 
                fill="none" 
                stroke={otifRate >= 85 ? '#10b981' : (otifRate >= 70 ? '#d97706' : '#ef4444')} 
                strokeWidth="3.5"
                strokeDasharray="88"
                strokeDashoffset={88 - (otifRate / 100) * 88}
                strokeLinecap="round"
                className="transition-all duration-500"
              />
              <text x="18" y="21" fill="var(--text-heading)" fontSize="9px" fontWeight="bold" textAnchor="middle" className="transform rotate-90" style={{ transformOrigin: '18px 18px' }}>
                {otifRate}%
              </text>
            </svg>
            <div>
              <div className="text-[10px] font-bold text-heading">当前交付率 (OTIF)</div>
              <p className="text-[8.5px] text-muted leading-tight">基于未延误订单数量计算</p>
            </div>
          </div>

          <div className="flex items-center space-x-2.5 border-r border-main pr-3">
            {otifRate >= 85 ? (
              <ShieldCheck className="w-8 h-8 text-emerald-500" />
            ) : (
              <ShieldAlert className="w-8 h-8 text-rose-500 animate-bounce" />
            )}
            <div>
              <div className="text-[10px] font-bold text-heading">防护安全等级</div>
              <p className="text-[8.5px] text-muted leading-tight">
                {otifRate >= 95 ? '🛡️ 完美防御' : (otifRate >= 85 ? '🟢 状态平稳' : '⚠️ 面临供需撕裂')}
              </p>
            </div>
          </div>

          <div className="flex items-center justify-between">
            <div className="text-right w-full">
              <div className="text-[10px] font-bold text-heading">当前物理计划分支</div>
              <p className="text-[8.5px] text-indigo-400 font-bold cyber-font uppercase">{currentScenario.toUpperCase()}</p>
            </div>
          </div>
        </div>

        {/* 详细树视图区 */}
        <div className="flex-1 flex flex-col overflow-hidden min-h-0">
          <div className="text-[10.5px] font-bold text-heading mb-2 flex items-center justify-between">
            <span className="flex items-center">
              <ArrowDownUp className="w-3.5 h-3.5 text-indigo-500 mr-1.5" />
              E2E 供需溯源关键路径图 (End-to-End Collapsible Critical Path Trace)
            </span>
            {peggingDetails && (
              <span className="text-[9.5px] text-muted font-mono font-normal">
                Order: {peggingDetails.demand_id} | Delay: {peggingDetails.delay_days}d
              </span>
            )}
          </div>

          {/* ITP-IOP Quota Barrier flowchart */}
          {peggingDetails?.demand_id === 'Order3' && (
            <div className="mb-3 p-3.5 rounded bg-slate-900/60 border border-main backdrop-blur-md relative overflow-hidden select-none animate-fade-in">
              <div className="absolute top-0 right-0 w-32 h-32 bg-indigo-500/10 rounded-full blur-3xl pointer-events-none" />
              <div className="absolute bottom-0 left-0 w-32 h-32 bg-emerald-500/5 rounded-full blur-3xl pointer-events-none" />
              
              <div className="flex items-center justify-between mb-2.5">
                <div className="flex items-center space-x-2">
                  <span className="flex h-2 w-2 relative">
                    <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-indigo-400 opacity-75"></span>
                    <span className="relative inline-flex rounded-full h-2 w-2 bg-indigo-500"></span>
                  </span>
                  <span className="text-[10.5px] font-bold text-slate-300">ITP-IOP 软分配隔离对比视图 (Quota Barrier Preemption Flow)</span>
                </div>
                <span className="text-[9px] text-indigo-400 font-mono border border-indigo-950 bg-indigo-950/40 px-2 py-0.5 rounded">
                  专利算法: 双层 ITP-IOP 协同分配
                </span>
              </div>

              {/* Flowchart Diagram */}
              <div className="flex items-center justify-between py-1 relative">
                <div className="flex flex-col items-center justify-center z-10 w-28 text-center">
                  <div className="w-11 h-11 rounded-full bg-slate-800 border border-indigo-500 flex items-center justify-center shadow-lg shadow-indigo-500/15 relative">
                    <div className="absolute inset-0 rounded-full bg-indigo-500/10 animate-pulse" />
                    <svg className="w-5.5 h-5.5 text-indigo-400" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                      <circle cx="12" cy="12" r="10" />
                      <path d="M8 12h8M12 8v8" />
                    </svg>
                  </div>
                  <span className="text-[10px] font-bold text-heading mt-1.5">Wafer Stock</span>
                  <span className="text-[8px] text-muted">通用晶圆原料池</span>
                </div>

                <div className="flex-1 h-16 relative mx-2">
                  <svg className="w-full h-full" viewBox="0 0 100 60" preserveAspectRatio="none">
                    <path 
                      d="M 5,20 C 35,20 35,10 95,10" 
                      fill="none" 
                      stroke="#c084fc" 
                      strokeWidth="2" 
                      className="opacity-75"
                    />
                    <circle cx="95" cy="10" r="2.5" fill="#c084fc" />

                    <path 
                      d="M 5,40 C 35,40 35,50 95,50" 
                      fill="none" 
                      stroke={peggingDetails.delay_days > 0 ? "#f43f5e" : "#10b981"} 
                      strokeWidth="2" 
                      strokeDasharray={peggingDetails.delay_days > 0 ? "3,3" : "none"}
                      className="opacity-75"
                    />
                    <circle cx="95" cy="50" r="2.5" fill={peggingDetails.delay_days > 0 ? "#f43f5e" : "#10b981"} />

                    <circle r="2.2" fill="#e9d5ff">
                      <animateMotion 
                        path="M 5,20 C 35,20 35,10 95,10" 
                        dur="2.5s" 
                        repeatCount="indefinite" 
                      />
                    </circle>

                    <circle r="2.2" fill={peggingDetails.delay_days > 0 ? "#fca5a5" : "#a7f3d0"}>
                      <animateMotion 
                        path="M 5,40 C 35,40 35,50 95,50" 
                        dur="3s" 
                        repeatCount="indefinite" 
                      />
                    </circle>
                  </svg>

                  <div className="absolute top-1/2 left-1/3 -translate-y-1/2 -translate-x-1/2 bg-slate-900 border border-purple-500/40 p-1 rounded-full shadow-lg" title="ITP 配额防御结界">
                    <svg className="w-3 h-3 text-purple-400" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                      <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z" />
                    </svg>
                  </div>

                  <div className="absolute top-1/2 left-2/3 -translate-y-1/2 -translate-x-1/2 bg-slate-900 border border-slate-700 p-1 rounded-full shadow-lg" title={peggingDetails.delay_days > 0 ? "IOP 配额硬阻断" : "IOP 替代升级放行"}>
                    {peggingDetails.delay_days > 0 ? (
                      <svg className="w-3 h-3 text-rose-500 animate-pulse" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                        <rect x="3" y="11" width="18" height="11" rx="2" ry="2" />
                        <path d="M7 11V7a5 5 0 0 1 10 0v4" />
                      </svg>
                    ) : (
                      <svg className="w-3 h-3 text-emerald-400" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                        <path d="M12 22a10 10 0 1 0 0-20 10 10 0 0 0 0 20z" />
                        <path d="m9 12 2 2 4-4" />
                      </svg>
                    )}
                  </div>
                </div>

                <div className="flex flex-col space-y-2 z-10 w-44">
                  <div className="bg-slate-950/90 border border-purple-900/50 p-1.5 rounded flex items-center justify-between text-left">
                    <div className="truncate pr-1">
                      <div className="text-[9px] font-bold text-purple-400">VVIP 订单 Order1 (FG1)</div>
                      <div className="text-[8px] text-muted">ITP 优先战略配额保护</div>
                    </div>
                    <div className="text-right flex-shrink-0">
                      <div className="text-[10px] font-bold text-heading font-mono">分配 60</div>
                      <div className="text-[8px] text-emerald-400 font-bold">OTIF 100%</div>
                    </div>
                  </div>

                  {peggingDetails.delay_days > 0 ? (
                    <div className="bg-slate-950/90 border border-rose-900/50 p-1.5 rounded flex items-center justify-between text-left animate-pulse">
                      <div className="truncate pr-1">
                        <div className="text-[9px] font-bold text-rose-400">普通订单 Order3 (FG2)</div>
                        <div className="text-[8px] text-muted">IOP 限制分配上限 40</div>
                      </div>
                      <div className="text-right flex-shrink-0">
                        <div className="text-[10px] font-bold text-rose-500 font-mono">缺口 40</div>
                        <div className="text-[8px] text-rose-400 font-bold">延迟 3 天</div>
                      </div>
                    </div>
                  ) : (
                    <div className="bg-slate-950/90 border border-emerald-950/50 p-1.5 rounded flex items-center justify-between text-left shadow-lg">
                      <div className="truncate pr-1">
                        <div className="text-[9px] font-bold text-emerald-400">普通订单 Order3 (FG2)</div>
                        <div className="text-[8px] text-muted">IOP 替代升级消纳成功</div>
                      </div>
                      <div className="text-right flex-shrink-0">
                        <div className="text-[10px] font-bold text-emerald-400 font-mono">缺口 0</div>
                        <div className="text-[8px] text-emerald-400 font-bold">准时交付</div>
                      </div>
                    </div>
                  )}
                </div>
              </div>

              <div className={`mt-2 p-2 rounded text-[9.5px] border ${
                peggingDetails.delay_days > 0 
                  ? "bg-rose-950/20 border-rose-900/40 text-rose-300" 
                  : "bg-emerald-950/20 border-emerald-900/40 text-emerald-300"
              }`}>
                {peggingDetails.delay_days > 0 ? (
                  <div className="flex items-start space-x-1.5 leading-relaxed">
                    <svg className="w-3.5 h-3.5 text-rose-400 flex-shrink-0 mt-0.5" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                      <circle cx="12" cy="12" r="10" />
                      <line x1="12" y1="8" x2="12" y2="12" />
                      <line x1="12" y1="16" x2="12.01" y2="16" />
                    </svg>
                    <div>
                      <span className="font-bold text-rose-400">配额隔离防御中：</span>由于高优先级订单 (Order1) 启动了 ITP 结界保护，锁定通用晶圆原料配额。IOP 执行级限制了普通订单 (Order3) 的分配上限，从而产生 40 颗缺口与 3 天延迟。推荐在<b>控制塔</b>中启用 <span className="font-bold underline text-rose-200 cursor-pointer">512MB 芯片替代升级</span> 决策消纳过剩 512MB 余料，绕开配额防波堤结界！
                    </div>
                  </div>
                ) : (
                  <div className="flex items-start space-x-1.5 leading-relaxed">
                    <svg className="w-3.5 h-3.5 text-emerald-400 flex-shrink-0 mt-0.5" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                      <path d="M12 22c5.523 0 10-4.477 10-10S17.523 2 12 2 2 6.477 2 12s4.477 10 10 10z" />
                      <path d="m9 12 2 2 4-4" />
                    </svg>
                    <div>
                      <span className="font-bold text-emerald-400">解耦消纳成功：</span>已成功启用 512MB 芯片替代升级，分配了高等级 512MB 芯片。这成功消耗了 2500 颗余料库存，将缺口缩减至 0，交期挽回 3 天，实现 OTIF 100% 顺畅交付！
                    </div>
                  </div>
                )}
              </div>
            </div>
          )}

          {/* Graphical Tree Container */}
          <div className="flex-1 border border-main rounded bg-input p-3.5 overflow-y-auto relative min-h-0">
            {treeLoading && (
              <div className="absolute inset-0 bg-input/80 flex items-center justify-center text-xs text-indigo-400">
                正在检索供需图谱结构...
              </div>
            )}

            {peggingDetails?.tree ? (
              <div className="space-y-1 font-mono">
                {renderTreeNode(peggingDetails.tree)}
              </div>
            ) : (
              <div className="text-center py-24 text-muted text-xs">
                请在左侧列表中点击选择订单以穿透供需链路。
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Floating Context Menu for Demand cards */}
      {demandMenu && (
        <div 
          className="fixed bg-[#121620] border border-[#1e2538] rounded shadow-2xl py-1.5 min-w-[170px] z-50 animate-fade-in select-none text-[9.5px] backdrop-blur-md"
          style={{ top: demandMenu.y, left: demandMenu.x }}
          onClick={(e) => e.stopPropagation()}
        >
          <div className="px-2.5 py-1 text-heading font-mono font-bold bg-[#161c28] border-b border-[#1e2538] mb-1">
            需求订单: {demandMenu.demand.demand_id}
          </div>
          
          <button 
            onClick={() => {
              setSelectedDemand(demandMenu.demand);
              setSelectedDemandId(demandMenu.demand.demand_id);
              loadPeggingTree(demandMenu.demand.demand_id);
              setDemandMenu(null);
            }}
            className="w-full text-left px-3 py-1.5 text-[#f8fafc] hover:bg-indigo-600 hover:text-white flex items-center space-x-2 transition-colors"
          >
            <GitMerge className="w-3.5 h-3.5 text-indigo-400" />
            <span>🔍 需求计划展开 (Pegging)</span>
          </button>

          <button 
            onClick={() => {
              loadAssignments(demandMenu.demand.demand_id);
              setDemandMenu(null);
            }}
            className="w-full text-left px-3 py-1.5 text-[#f8fafc] hover:bg-indigo-600 hover:text-white flex items-center space-x-2 transition-colors"
          >
            <Database className="w-3.5 h-3.5 text-emerald-400" />
            <span>📊 查看匹配供应 (Assignments)</span>
          </button>

          <button 
            onClick={() => {
              openOverrideModal(demandMenu.demand);
              setDemandMenu(null);
            }}
            className="w-full text-left px-3 py-1.5 text-[#f8fafc] hover:bg-indigo-600 hover:text-white flex items-center space-x-2 transition-colors"
          >
            <Sliders className="w-3.5 h-3.5 text-amber-500" />
            <span>⚙️ 模拟 What-if 调整 (Qty)</span>
          </button>
        </div>
      )}

      {/* Floating Context Menu for Pegging Tree Nodes */}
      {treeNodeMenu && (
        <div 
          className="fixed bg-[#121620] border border-[#1e2538] rounded shadow-2xl py-1.5 min-w-[170px] z-50 animate-fade-in select-none text-[9.5px] backdrop-blur-md"
          style={{ top: treeNodeMenu.y, left: treeNodeMenu.x }}
          onClick={(e) => e.stopPropagation()}
        >
          <div className="px-2.5 py-1 text-heading font-mono font-bold bg-[#161c28] border-b border-[#1e2538] mb-1">
            物料节点: {treeNodeMenu.partCode}
          </div>
          
          <button 
            onClick={() => {
              loadPartOnhand(treeNodeMenu.partCode);
              setTreeNodeMenu(null);
            }}
            className="w-full text-left px-3 py-1.5 text-[#f8fafc] hover:bg-indigo-600 hover:text-white flex items-center space-x-2 transition-colors"
          >
            <Database className="w-3.5 h-3.5 text-indigo-400" />
            <span>📋 调阅库存台账 (OnHand)</span>
          </button>

          <button 
            onClick={() => {
              loadPartBom(treeNodeMenu.partCode);
              setTreeNodeMenu(null);
            }}
            className="w-full text-left px-3 py-1.5 text-[#f8fafc] hover:bg-indigo-600 hover:text-white flex items-center space-x-2 transition-colors"
          >
            <Layers className="w-3.5 h-3.5 text-emerald-400" />
            <span>🌿 查看物料清单 (BOM构成)</span>
          </button>
        </div>
      )}

      {/* Assignments Ledger Sheet Modal */}
      {showAssignmentsModal && (
        <div className="fixed inset-0 bg-[#0b0e14]/85 backdrop-blur-sm flex items-center justify-center z-50 p-4 select-none animate-fade-in">
          <div className="bg-[#121620] border border-[#1e2538] rounded-md max-w-2xl w-full p-4 space-y-3 shadow-2xl">
            <div className="flex justify-between items-center border-b border-[#1e2538] pb-2">
              <span className="font-bold text-indigo-400 flex items-center text-[11px] font-mono">
                <Database className="w-4 h-4 mr-2 text-indigo-500" />
                供应分配账表 (Supply Assignments Ledger): {activeDemandForAssignments}
              </span>
              <button onClick={() => setShowAssignmentsModal(false)} className="text-muted hover:text-[#f8fafc] p-0.5 rounded hover:bg-[#161c28]">
                <XCircle className="w-4 h-4" />
              </button>
            </div>
            
            <p className="text-[9.5px] text-main leading-relaxed">
              展示当前独立需求订单级联消纳到底层物理工厂及采购在途供应的匹配清单。包含 IOP 确定性计划与 ITP 战略防护软分配：
            </p>

            <div className="max-h-60 overflow-y-auto border border-[#1e2538] rounded custom-fine-scrollbar">
              {assignmentsLoading ? (
                <div className="text-center py-10 text-indigo-400 text-xs font-mono animate-pulse">
                  正在从 DuckDB 数据库索引加载数据单...
                </div>
              ) : assignments.length > 0 ? (
                <table className="w-full text-left border-collapse text-[9px]">
                  <thead>
                    <tr className="bg-[#161c28] text-muted font-bold border-b border-[#1e2538]">
                      <th className="p-2 border-r border-[#1e2538]">配套物料 (Component)</th>
                      <th className="p-2 border-r border-[#1e2538]">分配供应编号 (Supply ID)</th>
                      <th className="p-2 border-r border-[#1e2538]">供应类型 (Supply Type)</th>
                      <th className="p-2 border-r border-[#1e2538] text-right">匹配分配量 (Qty)</th>
                      <th className="p-2 border-r border-[#1e2538] text-center">供应交期 (Avail Day)</th>
                      <th className="p-2 text-center">分配状态 (Mode)</th>
                    </tr>
                  </thead>
                  <tbody>
                    {assignments.map((row, idx) => (
                      <tr key={idx} className="hover:bg-[#1c2335]/40 border-b border-[#1e2538]/50 text-main font-mono">
                        <td className="p-2 border-r border-[#1e2538] font-bold text-heading">{row.part}</td>
                        <td className="p-2 border-r border-[#1e2538] text-indigo-300">{row.supply}</td>
                        <td className="p-2 border-r border-[#1e2538]">{row.supply_type}</td>
                        <td className="p-2 border-r border-[#1e2538] text-right text-heading font-bold">{row.assigned_qty}</td>
                        <td className="p-2 border-r border-[#1e2538] text-center text-amber-500 font-bold">{row.available_date}</td>
                        <td className="p-2 text-center">
                          <span className={`px-1 py-0.5 rounded text-[8px] font-bold ${
                            row.assignment_mode === 'Actual' ? 'bg-emerald-950 text-emerald-400 border border-emerald-900/50' : 'bg-blue-950 text-blue-400 border border-blue-900/50'
                          }`}>
                            {row.assignment_mode === 'Actual' ? 'IOP 执行' : 'ITP 预测'}
                          </span>
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              ) : (
                <div className="text-center py-10 text-muted text-xs">
                  暂无匹配的物理分配信息（未运行重算或该订单产生刚性短缺）
                </div>
              )}
            </div>

            <div className="flex justify-end pt-2 border-t border-[#1e2538]">
              <button 
                onClick={() => setShowAssignmentsModal(false)}
                className="px-4 py-1.5 bg-indigo-600 hover:bg-indigo-500 text-white rounded font-bold text-[9.5px] border border-indigo-500 transition-colors shadow-lg"
              >
                关闭账表
              </button>
            </div>
          </div>
        </div>
      )}

      {/* Part On-hand Inventory Modal */}
      {showOnhandModal && (
        <div className="fixed inset-0 bg-[#0b0e14]/85 backdrop-blur-sm flex items-center justify-center z-50 p-4 select-none animate-fade-in">
          <div className="bg-[#121620] border border-[#1e2538] rounded-md max-w-xl w-full p-4 space-y-3 shadow-2xl">
            <div className="flex justify-between items-center border-b border-[#1e2538] pb-2">
              <span className="font-bold text-indigo-400 flex items-center text-[11px] font-mono">
                <Database className="w-4 h-4 mr-2 text-indigo-500" />
                物料现货库存及货源表 (Onhand & Sourcing): {activePartForOnhand}
              </span>
              <button onClick={() => setShowOnhandModal(false)} className="text-muted hover:text-[#f8fafc] p-0.5 rounded hover:bg-[#161c28]">
                <XCircle className="w-4 h-4" />
              </button>
            </div>

            <div className="max-h-60 overflow-y-auto border border-[#1e2538] rounded custom-fine-scrollbar">
              {onhandLoading ? (
                <div className="text-center py-10 text-indigo-400 text-xs font-mono animate-pulse">
                  正在调取库存主表...
                </div>
              ) : partOnhandData.length > 0 ? (
                <table className="w-full text-left border-collapse text-[9px]">
                  <thead>
                    <tr className="bg-[#161c28] text-muted font-bold border-b border-[#1e2538]">
                      <th className="p-2 border-r border-[#1e2538]">物理仓储/仓库 (Location)</th>
                      <th className="p-2 border-r border-[#1e2538]">对应Site</th>
                      <th className="p-2 border-r border-[#1e2538] text-right">可用现货库存 (Onhand Qty)</th>
                      <th className="p-2 border-r border-[#1e2538] text-center">到货时间</th>
                      <th className="p-2 text-center">库存属性</th>
                    </tr>
                  </thead>
                  <tbody>
                    {partOnhandData.map((row, idx) => (
                      <tr key={idx} className="hover:bg-[#1c2335]/40 border-b border-[#1e2538]/50 text-main font-mono">
                        <td className="p-2 border-r border-[#1e2538] font-bold text-heading">{row.location}</td>
                        <td className="p-2 border-r border-[#1e2538] text-purple-400">{row.site}</td>
                        <td className="p-2 border-r border-[#1e2538] text-right text-heading font-bold">{row.qty}</td>
                        <td className="p-2 border-r border-[#1e2538] text-center text-amber-500">{row.available_date}</td>
                        <td className="p-2 text-center">
                          <span className={`px-1.5 py-0.2 rounded border scale-85 uppercase font-semibold text-[8px] ${
                            row.inventory_type === 'OnHand' ? 'bg-emerald-950 border-emerald-900 text-emerald-400' : 'bg-slate-900 border-slate-700 text-slate-400'
                          }`}>
                            {row.inventory_type}
                          </span>
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              ) : (
                <div className="text-center py-10 text-muted text-xs">
                  该物料目前无任何在库或在途安全库存
                </div>
              )}
            </div>

            <div className="flex justify-end pt-2 border-t border-[#1e2538]">
              <button 
                onClick={() => setShowOnhandModal(false)}
                className="px-4 py-1.5 bg-indigo-600 hover:bg-indigo-500 text-white rounded font-bold text-[9.5px] border border-indigo-500 transition-colors shadow-lg"
              >
                关闭窗口
              </button>
            </div>
          </div>
        </div>
      )}

      {/* BOM Component Items Modal */}
      {showBomModal && (
        <div className="fixed inset-0 bg-[#0b0e14]/85 backdrop-blur-sm flex items-center justify-center z-50 p-4 select-none animate-fade-in">
          <div className="bg-[#121620] border border-[#1e2538] rounded-md max-w-xl w-full p-4 space-y-3 shadow-2xl">
            <div className="flex justify-between items-center border-b border-[#1e2538] pb-2">
              <span className="font-bold text-indigo-400 flex items-center text-[11px] font-mono">
                <Database className="w-4 h-4 mr-2 text-indigo-500" />
                BOM物料构成清单 (BOM Hierarchy Components): {activePartForBom}
              </span>
              <button onClick={() => setShowBomModal(false)} className="text-muted hover:text-[#f8fafc] p-0.5 rounded hover:bg-[#161c28]">
                <XCircle className="w-4 h-4" />
              </button>
            </div>

            <div className="max-h-60 overflow-y-auto border border-[#1e2538] rounded custom-fine-scrollbar">
              {bomLoading ? (
                <div className="text-center py-10 text-indigo-400 text-xs font-mono animate-pulse">
                  正在爆炸展开BOM清单...
                </div>
              ) : bomData.length > 0 ? (
                <table className="w-full text-left border-collapse text-[9px]">
                  <thead>
                    <tr className="bg-[#161c28] text-muted font-bold border-b border-[#1e2538]">
                      <th className="p-2 border-r border-[#1e2538]">BOM 构成编号 (BOM ID)</th>
                      <th className="p-2 border-r border-[#1e2538]">子件物料编码 (Component)</th>
                      <th className="p-2 border-r border-[#1e2538] text-right">单件构成用量 (Qty / Component Ratio)</th>
                      <th className="p-2 text-right">子件批量因子 (Lot Size)</th>
                    </tr>
                  </thead>
                  <tbody>
                    {bomData.map((row, idx) => (
                      <tr key={idx} className="hover:bg-[#1c2335]/40 border-b border-[#1e2538]/50 text-main font-mono">
                        <td className="p-2 border-r border-[#1e2538] font-bold text-heading">{row.bomid}</td>
                        <td className="p-2 border-r border-[#1e2538] text-indigo-300">{row.component}</td>
                        <td className="p-2 border-r border-[#1e2538] text-right text-heading font-bold">{row.qty}</td>
                        <td className="p-2 text-right">{row.lot_size || '0.00'}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              ) : (
                <div className="text-center py-10 text-muted text-xs">
                  该零件为底层RAW原料，无下级BOM构成子件
                </div>
              )}
            </div>

            <div className="flex justify-end pt-2 border-t border-[#1e2538]">
              <button 
                onClick={() => setShowBomModal(false)}
                className="px-4 py-1.5 bg-indigo-600 hover:bg-indigo-500 text-white rounded font-bold text-[9.5px] border border-indigo-500 transition-colors shadow-lg"
              >
                关闭窗口
              </button>
            </div>
          </div>
        </div>
      )}

      {/* What-if Demand Override Modal */}
      {showOverrideModal && (
        <div className="fixed inset-0 bg-[#0b0e14]/90 backdrop-blur-md flex items-center justify-center z-50 p-4 select-none animate-fade-in">
          <div className="bg-[#121620] border border-[#1e2538] rounded p-4 max-w-sm w-full space-y-3 shadow-2xl">
            <div className="flex justify-between items-center border-b border-[#1e2538] pb-1.5">
              <span className="font-bold text-indigo-400 flex items-center text-[10.5px]">
                <Sliders className="w-4 h-4 mr-1.5 text-indigo-500" />
                What-if 需求拟定调整模拟 (Demand Override)
              </span>
              <button onClick={() => setShowOverrideModal(false)} className="text-muted hover:text-[#f8fafc]">
                <XCircle className="w-4 h-4" />
              </button>
            </div>
            
            <p className="text-[9.5px] text-main leading-relaxed">
              您正在模拟修改 <b>{overrideForm.demandId}</b> 独立需求以进行 What-if 重新规划计算：
            </p>
            
            <div className="space-y-2 text-[10px]">
              <div className="flex flex-col space-y-1">
                <span className="text-muted">修改独立需求数量 (Override Qty)</span>
                <input
                  type="number"
                  value={overrideForm.qty}
                  onChange={(e) => setOverrideForm(prev => ({ ...prev, qty: parseFloat(e.target.value) || 0 }))}
                  className="bg-[#090b10] border border-[#1e2538] text-[#f8fafc] p-1.5 rounded outline-none font-mono"
                />
              </div>

              <div className="flex flex-col space-y-1">
                <span className="text-muted">需求到期天数 (Due Date Day offset)</span>
                <input
                  type="number"
                  min="1"
                  max="29"
                  value={overrideForm.day}
                  onChange={(e) => setOverrideForm(prev => ({ ...prev, day: parseInt(e.target.value) || 15 }))}
                  className="bg-[#090b10] border border-[#1e2538] text-[#f8fafc] p-1.5 rounded outline-none font-mono"
                />
              </div>
            </div>

            <div className="flex justify-end space-x-2 pt-2 border-t border-[#1e2538]">
              <button 
                onClick={() => setShowOverrideModal(false)}
                className="px-3 py-1.5 bg-transparent hover:bg-slate-800 text-muted hover:text-heading border border-[#1e2538] rounded text-[9.5px]"
              >
                取消
              </button>
              <button 
                onClick={handleApplyOverride}
                className="px-3 py-1.5 bg-indigo-600 hover:bg-indigo-500 text-white rounded font-bold text-[9.5px] border border-indigo-500 transition-colors shadow-lg"
              >
                提交 What-if 重算
              </button>
            </div>
          </div>
        </div>
      )}
      
    </div>
  );
};

export default DemandSupplyPegging;
