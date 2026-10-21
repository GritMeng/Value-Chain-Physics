import React, { useState, useEffect } from 'react';
import { AgGridReact } from 'ag-grid-react';
import { FolderGit2, CalendarClock, Sparkles } from 'lucide-react';
import { useScenario } from '../../contexts/ScenarioContext';

import 'ag-grid-community/styles/ag-grid.css';
import 'ag-grid-community/styles/ag-theme-balham.css';

const ProjectEto = () => {
  const { currentScenario, fetchKpis } = useScenario();
  
  const [projects, setProjects] = useState([]);
  const [selectedProj, setSelectedProj] = useState(null);
  const [wbsData, setWbsData] = useState([]);
  const [gridLoading, setGridLoading] = useState(false);
  const [isUpdating, setIsUpdating] = useState(false);

  const fetchProjects = async () => {
    try {
      const res = await fetch('/api/table?name=ipc_project');
      const data = await res.json();
      setProjects(data);
      if (data.length > 0 && !selectedProj) {
        setSelectedProj(data[0]);
      }
    } catch (err) {
      console.error('Error fetching ETO projects:', err);
    }
  };

  const loadWbsTasks = async (projectCode) => {
    setGridLoading(true);
    try {
      const res = await fetch(`/api/eto/wbs?project_code=${encodeURIComponent(projectCode)}`);
      const data = await res.json();
      setWbsData(data);
    } catch (err) {
      console.error('Error fetching WBS tasks:', err);
    } finally {
      setGridLoading(false);
    }
  };

  useEffect(() => {
    fetchProjects();
  }, [currentScenario]);

  useEffect(() => {
    if (selectedProj) {
      loadWbsTasks(selectedProj.project);
    } else {
      setWbsData([]);
    }
  }, [selectedProj]);

  const handleStatusChange = async (wbsCode, newStatus) => {
    setIsUpdating(true);
    try {
      const res = await fetch('/api/eto/wbs/update', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ wbs_code: wbsCode, status: newStatus }),
      });
      const data = await res.json();
      if (data.status === 'success') {
        // Refresh local task list
        if (selectedProj) {
          loadWbsTasks(selectedProj.project);
        }
        await fetchKpis();
      } else {
        alert(`WBS 状态更新失败: ${data.message}`);
      }
    } catch (err) {
      console.error('Error updating WBS status:', err);
    } finally {
      setIsUpdating(false);
    }
  };

  // ag-Grid columns
  const wbsColumnDefs = [
    {
      headerName: '任务编号 (WBS Code)',
      field: 'wbs_code',
      width: 140,
      cellRenderer: (params) => {
        if (!params.data) return null;
        const indent = params.data.wbs_level * 12;
        return (
          <div style={{ paddingLeft: `${indent}px` }} className="font-mono font-bold text-heading">
            <span className="text-indigo-400 mr-1.5">└─</span>
            {params.value}
          </div>
        );
      }
    },
    {
      headerName: '任务说明 (Description)',
      field: 'description',
      width: 200,
      cellRenderer: (params) => (
        <span className="truncate" title={params.value}>{params.value}</span>
      )
    },
    {
      headerName: '层级 (Level)',
      field: 'wbs_level',
      type: 'numericColumn',
      width: 80,
      cellRenderer: (params) => <span className="cyber-font">{params.value}</span>
    },
    {
      headerName: '父节点 (Parent WBS)',
      field: 'parent_wbs_code',
      width: 120,
      cellRenderer: (params) => <span className="font-mono text-muted">{params.value || 'ROOT'}</span>
    },
    {
      headerName: '最早工期 (Duration)',
      field: 'duration',
      width: 90,
      cellRenderer: (params) => (
        <span className="cyber-font text-purple-400">{params.value !== undefined ? `${params.value}天` : '-'}</span>
      )
    },
    {
      headerName: '计划开工 (Start Day)',
      field: 'late_start',
      width: 100,
      cellRenderer: (params) => (
        <span className="cyber-font text-emerald-500">{params.value !== undefined ? `D${params.value}` : '-'}</span>
      )
    },
    {
      headerName: '计划完工 (Finish Day)',
      field: 'late_finish',
      width: 100,
      cellRenderer: (params) => (
        <span className="cyber-font text-rose-500 font-bold">{params.value !== undefined ? `D${params.value}` : '-'}</span>
      )
    },
    {
      headerName: '状态变更 (WBS Status)',
      field: 'wbs_status',
      width: 140,
      cellRenderer: (params) => {
        if (!params.data) return null;
        const status = params.value;
        const wbsCode = params.data.wbs_code;
        return (
          <select
            value={status}
            onChange={(e) => handleStatusChange(wbsCode, e.target.value)}
            disabled={isUpdating}
            className="w-full bg-slate-900 border border-main text-[10px] text-heading p-0.5 rounded cursor-pointer font-bold select-none outline-none focus:border-indigo-500"
          >
            <option value="ACTIVE" className="text-indigo-400 font-bold">🔵 ACTIVE (进行中)</option>
            <option value="COMPLETED" className="text-emerald-400 font-bold">🟢 COMPLETED (已完成)</option>
            <option value="PENDING" className="text-amber-400 font-bold">🟡 PENDING (已挂起)</option>
          </select>
        );
      }
    }
  ];

  const defaultColDef = {
    resizable: true,
  };

  return (
    <div className="flex flex-row space-x-3 h-full overflow-hidden">
      
      {/* 左侧 ETO 项目主列表 */}
      <div className="w-64 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden select-none">
        <div className="border-b border-main pb-2 mb-3">
          <h4 className="font-bold text-heading text-xs flex items-center">
            <FolderGit2 className="w-4 h-4 text-indigo-500 mr-1.5" />
            活跃 ETO 工程项目列表
          </h4>
        </div>

        <div className="flex-1 overflow-y-auto space-y-2 pr-1">
          {projects.map(proj => (
            <button
              key={proj.project}
              onClick={() => setSelectedProj(proj)}
              className={`w-full text-left p-3 border rounded-xs transition-all flex flex-col space-y-1.5 ${
                selectedProj?.project === proj.project
                  ? 'bg-indigo-600 border-indigo-500 text-white font-bold'
                  : 'bg-input border-main text-muted hover:bg-slate-800/30 hover:text-heading'
              }`}
            >
              <div className="flex justify-between items-center w-full">
                <span className="font-mono text-xs truncate flex-1">{proj.project}</span>
                <span className={`text-[8.5px] font-bold px-1.5 py-0.2 rounded border uppercase ${
                  selectedProj?.project === proj.project ? 'bg-indigo-700 text-white' : 'bg-table-header text-muted'
                }`}>
                  {proj.project_type || 'ETO'}
                </span>
              </div>
              
              <div className="flex justify-between text-[9px] w-full text-muted">
                <span>WBS 深度: {proj.wbs_depth || 3} 层</span>
                <span>交期: {proj.delivery_lead_time ? `D${proj.delivery_lead_time} (${proj.finish_date || ''})` : '未定'}</span>
              </div>
            </button>
          ))}
          {projects.length === 0 && (
            <div className="text-center py-10 text-muted text-xs">无活跃项目</div>
          )}
        </div>
      </div>

      {/* 右侧 ag-Grid WBS 任务网络树 */}
      <div className="flex-1 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden">
        <div className="border-b border-main pb-2.5 mb-3 flex justify-between items-center select-none">
          <h3 className="text-xs font-bold text-heading flex items-center">
            <CalendarClock className="w-4 h-4 text-indigo-500 mr-1.5" />
            项目微观 WBS 级联排产网格 (Microscopic WBS Schedule Matrix)
          </h3>
          {selectedProj && (
            <span className="text-[10px] px-2 py-0.5 bg-slate-900 border border-main rounded text-indigo-400 font-mono">
              Project: {selectedProj.project}
            </span>
          )}
        </div>

        <div className="flex-1 w-full overflow-hidden relative">
          {gridLoading && (
            <div className="absolute inset-0 bg-card/85 backdrop-blur-xs flex items-center justify-center z-20 text-xs text-indigo-400">
              构建 WBS 任务级联关系...
            </div>
          )}

          <div className="ag-theme-balham-dark w-full h-full">
            <AgGridReact
              theme="legacy"
              rowData={wbsData}
              columnDefs={wbsColumnDefs}
              defaultColDef={defaultColDef}
              headerHeight={26}
              rowHeight={22}
            />
          </div>
        </div>

        {/* 注解说明 */}
        <div className="mt-2 text-[9px] text-muted flex items-center space-x-1.5 select-none">
          <Sparkles className="w-3.5 h-3.5 text-indigo-400" />
          <span>双向同步契约：直接改变单元格状态下拉列表，系统立即写入 DuckDB 物理主计划，并同步触发 EEK 重构。</span>
        </div>
      </div>

    </div>
  );
};

export default ProjectEto;
