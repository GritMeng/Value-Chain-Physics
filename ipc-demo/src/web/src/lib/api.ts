// 前端唯一数据出入口：所有数据经后端 API（底层 DuckDB）获取。
// 前端不直接访问 C++ 或 CSV 文件。

export interface Health { status: string; checks: { duckdb: any; scenarios: any; engine: any } }
export interface ScenarioSummary {
  scenarioId: string; displayName: string; source: string; rootPath: string; isComplete: boolean;
  engines: Record<string, { runnable: boolean; missing: string[] }>;
  rowCounts: Record<string, number>; datasetCount: number; emptyHint: string | null;
}
export interface ApiError { message: string; kind: string }

async function request<T>(method: string, url: string, body?: unknown): Promise<T> {
  const res = await fetch(url, {
    method,
    headers: body !== undefined ? { 'content-type': 'application/json' } : {},
    body: body !== undefined ? JSON.stringify(body) : undefined,
  });
  const text = await res.text();
  const data = text ? JSON.parse(text) : null;
  if (!res.ok) {
    const err = new Error(data?.error?.message || `请求失败 (${res.status})`) as Error & { kind?: string; status?: number };
    err.kind = data?.error?.kind;
    err.status = res.status;
    throw err;
  }
  return data as T;
}

export const api = {
  health: () => request<Health>('GET', '/api/health'),
  scenarios: () => request<{ scenarios: ScenarioSummary[]; hint: string | null }>('GET', '/api/scenarios'),
  scenario: (id: string) => request<any>('GET', `/api/scenarios/${encodeURIComponent(id)}`),
  createScenario: (scenarioId: string) => request<any>('POST', '/api/scenarios', { scenarioId }),
  inputs: (id: string) => request<any>('GET', `/api/scenarios/${encodeURIComponent(id)}/inputs`),
  inputRows: (id: string, key: string, limit = 500) =>
    request<any>('GET', `/api/scenarios/${encodeURIComponent(id)}/inputs/${key}?limit=${limit}`),
  raw: (id: string, filename: string) =>
    request<{ filename: string; content: string }>('GET', `/api/scenarios/${encodeURIComponent(id)}/raw/${filename}`),
  run: (engineType: string, body: Record<string, unknown>) =>
    request<any>('POST', `/api/runs/${engineType}`, body),
  runs: (scenarioId?: string, engineType?: string) => {
    const q = new URLSearchParams();
    if (scenarioId) q.set('scenarioId', scenarioId);
    if (engineType) q.set('engineType', engineType);
    return request<{ runs: any[] }>('GET', `/api/runs?${q}`);
  },
  runDetail: (runId: string) => request<any>('GET', `/api/runs/${encodeURIComponent(runId)}`),
  runOutputs: (runId: string, key: string) =>
    request<{ rows: any[] }>('GET', `/api/runs/${encodeURIComponent(runId)}/outputs/${key}`),
  pivot: (id: string, params: { runId?: string; metrics?: string[]; dimValue?: string; dimension?: string } = {}) => {
    const q = new URLSearchParams();
    if (params.runId) q.set('runId', params.runId);
    if (params.metrics?.length) q.set('metrics', params.metrics.join(','));
    if (params.dimValue) q.set('dimValue', params.dimValue);
    if (params.dimension) q.set('dimension', params.dimension);
    return request<any>('GET', `/api/scenarios/${encodeURIComponent(id)}/pivot?${q}`);
  },
  cell: (id: string, params: { metric: string; day: number; dimValue?: string | number | null; runId?: string }) => {
    const q = new URLSearchParams({ metric: params.metric, day: String(params.day) });
    if (params.dimValue !== null && params.dimValue !== undefined) q.set('dimValue', String(params.dimValue));
    if (params.runId) q.set('runId', params.runId);
    return request<any>('GET', `/api/scenarios/${encodeURIComponent(id)}/pivot/cell?${q}`);
  },
};

export default api;

export interface PivotRow {
  metric: string; label: string; family: string; unit: string;
  dimension: string; negativeIsOverload?: boolean;
  dimValue: number | string | null; error?: string | null;
  values: (number | null)[]; hasData: boolean;
}
export interface PivotResponse {
  scenarioId: string; runId: string | null; days: number[]; lo: number; hi: number;
  families: { key: string; label: string }[];
  metrics: { key: string; label: string; family: string; unit: string; dimension: string }[];
  rows: PivotRow[];
}
