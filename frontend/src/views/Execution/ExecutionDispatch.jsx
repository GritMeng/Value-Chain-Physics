import React, { useState, useEffect } from 'react';
import { AgGridReact } from 'ag-grid-react';
import { BarChart, Bar, XAxis, YAxis, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import { Factory, Layers, Search } from 'lucide-react';
import { useScenario } from '../../contexts/ScenarioContext';

import 'ag-grid-community/styles/ag-grid.css';
import 'ag-grid-community/styles/ag-theme-balham.css';

const ExecutionDispatch = () => {
  const { currentScenario } = useScenario();
  
  const [coproductData, setCoproductData] = useState({ schedules: [], allocations: [] });
  const [jobs, setJobs] = useState([]);
  const [searchQuery, setSearchQuery] = useState('');
  const [loading, setLoading] = useState(false);

  const fetchCoproductData = async () => {
    try {
      const res = await fetch('/api/execution/coproducts');
      const data = await res.json();
      setCoproductData(data);
    } catch (err) {
      console.error('Error fetching coproduct data:', err);
    }
  };

  const fetchJobs = async (q = '') => {
    setLoading(true);
    try {
      const res = await fetch(`/api/execution/jobs?q=${encodeURIComponent(q)}`);
      const data = await res.json();
      setJobs(data);
    } catch (err) {
      console.error('Error fetching dispatch jobs:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchCoproductData();
    fetchJobs();
  }, [currentScenario]);

  const handleSearchChange = (e) => {
    setSearchQuery(e.target.value);
    fetchJobs(e.target.value);
  };

  // Convert coproduct allocations to chart format for Recharts
  const getChartData = () => {
    if (!coproductData.allocations) return [];
    return coproductData.allocations.map(row => ({
      name: row.order_code,
      '512MB': row.allocated_512 || 0,
      '256MB': row.allocated_256 || 0,
      '128MB': row.allocated_128 || 0,
      'Shortage(缺料)': row.shortage || 0,
    }));
  };

  // ag-Grid columns for jobs
  const jobColumnDefs = [
    {
      headerName: '零件代号 (Part Code)',
      field: 'part_code',
      sortable: true,
      filter: true,
      width: 140,
      cellRenderer: (params) => (
        <span className="font-mono font-bold text-heading">{params.value}</span>
      )
    },
    {
      headerName: '计划批量 (Qty)',
      field: 'order_qty',
      type: 'numericColumn',
      sortable: true,
      width: 100,
      cellRenderer: (params) => (
        <span className="cyber-font font-bold">{Math.round(params.value).toLocaleString()}</span>
      )
    },
    {
      headerName: '共享设备容量 (Capacity %)',
      field: 'allocated_capacity',
      type: 'numericColumn',
      sortable: true,
      width: 130,
      cellRenderer: (params) => {
        const val = params.value;
        const isOver = val > 100;
        return (
          <span className={`cyber-font font-bold ${isOver ? 'text-rose-500 animate-pulse' : 'text-purple-400'}`}>
            {val ? `${val.toFixed(1)}%` : '0%'}
          </span>
        );
      }
    },
    {
      headerName: '标称交期 (Due)',
      field: 'original_due_day',
      sortable: true,
      width: 75,
      cellRenderer: (params) => <span className="cyber-font">D{params.value}</span>
    },
    {
      headerName: '排产期 (Sched)',
      field: 'scheduled_day',
      sortable: true,
      width: 80,
      cellRenderer: (params) => {
        const sched = params.value;
        const due = params.data?.original_due_day;
        if (due === undefined || due === null) {
          return <span className="cyber-font">D{sched}</span>;
        }
        const isDelayed = sched > due;
        return (
          <div className="flex items-center space-x-1.5">
            <span className={`cyber-font font-bold ${isDelayed ? 'text-rose-500' : 'text-emerald-500'}`}>
              D{sched}
            </span>
            {isDelayed && (
              <span className="bg-rose-950 border border-rose-800 text-rose-400 text-[8px] font-bold px-1.5 rounded scale-90">
                延误 {sched - due}天
              </span>
            )}
          </div>
        );
      }
    },
    {
      headerName: '工艺成本 (Cost)',
      field: 'routing_cost',
      type: 'numericColumn',
      sortable: true,
      width: 90,
      cellRenderer: (params) => (
        <span className="cyber-font text-muted">¥{params.value ? params.value.toFixed(1) : '0'}</span>
      )
    },
    {
      headerName: '规格尺寸 (Dim)',
      field: 'dimension_val',
      sortable: true,
      width: 80,
      cellRenderer: (params) => <span className="cyber-font text-muted">{params.value || '-'}</span>
    }
  ];

  const defaultColDef = {
    resizable: true,
  };

  const chartData = getChartData();

  return (
    <div className="flex flex-row space-x-3 h-full overflow-hidden">
      
      {/* 左侧：联副产品投产余料与配额瀑布图 */}
      <div className="w-[450px] bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden select-none">
        <div className="border-b border-main pb-2 mb-3">
          <h4 className="font-bold text-heading text-xs flex items-center">
            <Layers className="w-4 h-4 text-indigo-500 mr-1.5" />
            联副产出维度降级与配额分析 (Yield Cascades & Allocations)
          </h4>
        </div>

        {/* 瀑布级联图 */}
        <div className="h-64 border border-main rounded bg-input p-2 flex flex-col mb-4">
          <div className="text-[10px] font-bold text-heading mb-2">订单物料配分关系 (Order Allocation Stack)</div>
          <div className="flex-1 w-full text-xs">
            {chartData.length > 0 ? (
              <ResponsiveContainer width="100%" height="100%">
                <BarChart
                  data={chartData}
                  layout="vertical"
                  margin={{ top: 5, right: 10, left: 10, bottom: 5 }}
                >
                  <XAxis type="number" stroke="var(--text-muted)" fontSize={9} />
                  <YAxis dataKey="name" type="category" stroke="var(--text-muted)" fontSize={9} width={50} />
                  <Tooltip 
                    contentStyle={{ backgroundColor: 'var(--bg-card)', borderColor: 'var(--border-color)', fontSize: '10px' }}
                    labelClassName="text-heading font-bold"
                  />
                  <Legend fontSize={9} wrapperStyle={{ fontSize: '9px' }} />
                  <Bar dataKey="512MB" stackId="a" fill="#10b981" />
                  <Bar dataKey="256MB" stackId="a" fill="#4f46e5" />
                  <Bar dataKey="128MB" stackId="a" fill="#d97706" />
                  <Bar dataKey="Shortage(缺料)" stackId="a" fill="#ef4444" />
                </BarChart>
              </ResponsiveContainer>
            ) : (
              <div className="text-center py-20 text-muted">无配分数据</div>
            )}
          </div>
        </div>

        {/* 联副工艺与余料对账看板 */}
        <div className="flex-1 flex flex-col overflow-hidden min-h-0">
          <div className="text-[10px] font-bold text-heading mb-2">投产工艺及滞留余料对账册 (Leftovers Ledger)</div>
          <div className="flex-1 overflow-auto border border-main rounded bg-input p-2">
            <table className="w-full text-left border-collapse text-[10px]">
              <thead>
                <tr className="border-b border-main text-muted uppercase tracking-wider font-semibold">
                  <th className="py-1.5">工艺路径</th>
                  <th className="py-1.5 text-center">投产批数</th>
                  <th className="py-1.5 text-right">512M余料</th>
                  <th className="py-1.5 text-right">256M余料</th>
                  <th className="py-1.5 text-right">128M余料</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-800/40 text-main font-mono">
                {coproductData.schedules.map((row, idx) => (
                  <tr key={idx} className="hover:bg-slate-800/20">
                    <td className="py-2 font-bold text-heading">{row.routing_code}</td>
                    <td className="py-2 text-center text-indigo-400 font-bold">{row.batch_count}</td>
                    <td className="py-2 text-right">{row.leftover_512}</td>
                    <td className="py-2 text-right">{row.leftover_256}</td>
                    <td className="py-2 text-right">{row.leftover_128}</td>
                  </tr>
                ))}
                {coproductData.schedules.length === 0 && (
                  <tr>
                    <td colSpan={5} className="py-4 text-center text-muted">无工艺排产数据</td>
                  </tr>
                )}
              </tbody>
            </table>
          </div>
        </div>
      </div>

      {/* 右侧：有限产能工单派程网格 */}
      <div className="flex-1 bg-card border border-main rounded-xs p-3 flex flex-col h-full overflow-hidden">
        
        {/* 工具条 */}
        <div className="flex justify-between items-center border-b border-main pb-2.5 mb-2.5">
          <div className="flex items-center space-x-2">
            <Factory className="w-4 h-4 text-indigo-500" />
            <h3 className="text-xs font-bold text-heading">
              有限产能工单顺序派程表 (Finite Dispatch Job Ledger)
            </h3>
          </div>

          <div className="relative w-64">
            <input
              type="text"
              value={searchQuery}
              onChange={handleSearchChange}
              placeholder="搜索派程零件号..."
              className="w-full bg-input border border-main text-heading text-xs pl-8 pr-3 py-1 rounded focus:outline-none focus:border-indigo-500"
            />
            <Search className="w-3.5 h-3.5 text-muted absolute left-2.5 top-2" />
          </div>
        </div>

        {/* ag-Grid */}
        <div className="flex-1 w-full overflow-hidden relative">
          {loading && (
            <div className="absolute inset-0 bg-card/85 backdrop-blur-xs flex items-center justify-center z-20 text-xs text-indigo-400">
              载入工单队列中...
            </div>
          )}

          <div className="ag-theme-balham-dark w-full h-full">
            <AgGridReact
              theme="legacy"
              rowData={jobs}
              columnDefs={jobColumnDefs}
              defaultColDef={defaultColDef}
              headerHeight={26}
              rowHeight={22}
              pagination={true}
              paginationPageSize={25}
            />
          </div>
        </div>

      </div>

    </div>
  );
};

export default ExecutionDispatch;
