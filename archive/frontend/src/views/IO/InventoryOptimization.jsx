import React, { useState, useEffect } from 'react';
import { AgGridReact } from 'ag-grid-react';
import { ShieldCheck, Percent, HelpCircle, Save } from 'lucide-react';
import { useScenario } from '../../contexts/ScenarioContext';

import 'ag-grid-community/styles/ag-grid.css';
import 'ag-grid-community/styles/ag-theme-balham.css';

const InventoryOptimization = () => {
  const { currentScenario } = useScenario();
  
  const [ioTargets, setIoTargets] = useState([]);
  const [loading, setLoading] = useState(false);
  
  // Form states
  const [selectedPart, setSelectedPart] = useState('PART_3');
  const [targetSL, setTargetSL] = useState(0.98);
  const [isUpdating, setIsUpdating] = useState(false);

  const fetchIoTTarget = async () => {
    setLoading(true);
    try {
      const res = await fetch('/api/table?name=ipc_service_level_target');
      const data = await res.json();
      setIoTargets(data);
    } catch (err) {
      console.error('Error fetching IO targets:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchIoTTarget();
  }, [currentScenario]);

  const handleSlUpdate = async (e) => {
    e.preventDefault();
    setIsUpdating(true);
    
    try {
      const res = await fetch('/api/io/target/update', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          part_code: selectedPart,
          site_code: 'SITE_001',
          service_level_target: targetSL
        })
      });
      const data = await res.json();
      
      if (data.status === 'success') {
        const alertDiv = document.createElement('div');
        alertDiv.className = "fixed bottom-5 right-5 bg-indigo-950 border border-indigo-600 text-indigo-300 px-4 py-3 rounded shadow-xl text-xs z-50 animate-bounce";
        alertDiv.innerHTML = `✔ <b>[库存安全目标策略更新成功]</b><br/> ${data.message}`;
        document.body.appendChild(alertDiv);
        setTimeout(() => alertDiv.remove(), 3000);
        
        await fetchIoTTarget();
      } else {
        alert(`策略调整失败: ${data.message}`);
      }
    } catch (err) {
      console.error('Error updating IO target:', err);
    } finally {
      setIsUpdating(false);
    }
  };

  // ag-Grid columns
  const ioColumnDefs = [
    {
      headerName: '物料代号 (Part Code)',
      field: 'part_code',
      sortable: true,
      filter: true,
      width: 140,
      cellRenderer: (params) => <span className="font-mono font-bold text-heading">{params.value}</span>
    },
    {
      headerName: '运营站点 (Site)',
      field: 'site_code',
      sortable: true,
      width: 110,
      cellRenderer: (params) => <span className="font-mono text-muted">{params.value || 'SITE_001'}</span>
    },
    {
      headerName: '客户分类等级 (Customer Segment)',
      field: 'customer_segment',
      sortable: true,
      width: 160,
      cellRenderer: (params) => (
        <span className="px-2 py-0.5 rounded bg-main border border-main text-[9.5px] font-bold">
          {params.value}
        </span>
      )
    },
    {
      headerName: '服务水平目标 (Service Level Target)',
      field: 'service_level_target',
      type: 'numericColumn',
      sortable: true,
      width: 180,
      cellRenderer: (params) => {
        const pct = params.value * 100;
        return (
          <span className="cyber-font text-[var(--color-success)] font-bold">
            {pct ? `${pct.toFixed(1)}%` : '0%'}
          </span>
        );
      }
    },
    {
      headerName: '供应波动偏差 (Lead Time Variance)',
      field: 'lead_time_variance',
      type: 'numericColumn',
      sortable: true,
      width: 180,
      cellRenderer: (params) => (
        <span className="cyber-font text-purple-400 font-bold">
          {params.value ? `${params.value.toFixed(2)} 天` : '0.00 天'}
        </span>
      )
    }
  ];

  const defaultColDef = {
    resizable: true,
  };

  const partOptions = [...new Set(ioTargets.map(t => t.part_code))].filter(Boolean);

  return (
    <div className="flex flex-row space-x-3 h-full overflow-hidden">
      
      {/* 左侧 IO 服务水位网格 */}
      <div className="flex-1 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden">
        <div className="border-b border-main pb-2 mb-3 select-none">
          <h3 className="text-xs font-bold text-heading flex items-center">
            <ShieldCheck className="w-4 h-4 text-indigo-500 mr-1.5" />
            IO 库存优化安全水位策略表 (Safety Stock & Service Level Target Policy)
          </h3>
        </div>

        <div className="flex-1 w-full overflow-hidden relative">
          {loading && (
            <div className="absolute inset-0 bg-card/85 backdrop-blur-xs flex items-center justify-center z-20 text-xs text-indigo-400">
              载入安全库存策略中...
            </div>
          )}

          <div className="ag-theme-balham-dark w-full h-full">
            <AgGridReact
              theme="legacy"
              rowData={ioTargets}
              columnDefs={ioColumnDefs}
              defaultColDef={defaultColDef}
              headerHeight={26}
              rowHeight={22}
            />
          </div>
        </div>
      </div>

      {/* 右侧水位配置面板 */}
      <div className="w-80 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden select-none">
        <div className="border-b border-main pb-2 mb-4">
          <h4 className="font-bold text-heading text-xs flex items-center">
            <Percent className="w-4 h-4 text-indigo-500 mr-1.5" />
            库存保障水平目标配置 (SL Configurator)
          </h4>
        </div>

        <form onSubmit={handleSlUpdate} className="space-y-4">
          <div className="space-y-1">
            <label className="text-[10px] text-muted font-bold block">目标零件代号 (Target Part)</label>
            <select
              value={selectedPart}
              onChange={(e) => setSelectedPart(e.target.value)}
              className="w-full bg-input border border-main text-heading text-xs p-2 rounded focus:outline-none focus:border-indigo-500 font-mono cursor-pointer"
            >
              {partOptions.map(opt => (
                <option key={opt} value={opt}>{opt}</option>
              ))}
              {partOptions.length === 0 && (
                <>
                  <option value="PART_1">PART_1</option>
                  <option value="PART_2">PART_2</option>
                  <option value="PART_3">PART_3</option>
                </>
              )}
            </select>
          </div>

          <div className="space-y-1">
            <label className="text-[10px] text-muted font-bold block">期望交付服务水平 (Service Level Target)</label>
            <input
              type="number"
              value={targetSL}
              onChange={(e) => setTargetSL(parseFloat(e.target.value) || 0)}
              className="w-full bg-input border border-main text-heading text-xs p-2 rounded focus:outline-none focus:border-indigo-500 font-mono"
              min="0.5"
              max="0.999"
              step="0.01"
            />
          </div>

          <div className="bg-indigo-950/10 border border-indigo-900/40 p-2.5 rounded text-[10px] text-main space-y-1">
            <div className="font-bold text-indigo-400 flex items-center">
              <HelpCircle className="w-3.5 h-3.5 mr-1" />
              Z-Score 防爆仓红线核算:
            </div>
            <p className="leading-relaxed">
              安全库存目标服务水平决定了物料需求中应对前置提前期供应波动的<b>安全垫深度（Z-Score）</b>。
            </p>
          </div>

          <button
            type="submit"
            disabled={isUpdating}
            className="w-full btn-premium-primary py-2 text-xs flex items-center justify-center space-x-1.5 disabled:opacity-50"
          >
            <Save className="w-3.5 h-3.5" />
            <span>{isUpdating ? '同步策略中...' : '提交并重置水位线'}</span>
          </button>
        </form>
      </div>

    </div>
  );
};

export default InventoryOptimization;
