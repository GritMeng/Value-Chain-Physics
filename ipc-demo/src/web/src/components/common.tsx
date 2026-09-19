import React from 'react';

export function Loading({ what = '数据' }: { what?: string }) {
  return <div className="loading" data-testid="loading">正在加载{what}…</div>;
}

export function Empty({ children }: { children: React.ReactNode }) {
  return <div className="empty" data-testid="empty">{children}</div>;
}

export function ErrorBox({ error }: { error: string | null }) {
  if (!error) return null;
  return <div className="error" style={{ margin: '0 0 12px' }} data-testid="error">{error}</div>;
}

export function Badge({ tone = 'muted', children }: { tone?: 'ok' | 'bad' | 'warn' | 'muted'; children: React.ReactNode }) {
  return <span className={`badge ${tone}`}>{children}</span>;
}

export function Kpi({ label, value, unit, tone }: { label: string; value: React.ReactNode; unit?: string; tone?: 'ok' | 'bad' | 'warn' }) {
  return (
    <div className="kpi">
      <div className="k">{label}</div>
      <div className="v" style={tone === 'bad' ? { color: 'var(--bad)' } : tone === 'ok' ? { color: 'var(--ok)' } : tone === 'warn' ? { color: 'var(--warn)' } : undefined}>
        {value}{unit && <span className="u">{unit}</span>}
      </div>
    </div>
  );
}

function fmtCell(v: any): string {
  if (v === null || v === undefined) return '';
  if (typeof v === 'number') {
    if (Number.isInteger(v)) return String(v);
    return String(Math.round(v * 10000) / 10000);
  }
  if (typeof v === 'boolean') return v ? 'true' : 'false';
  return String(v);
}

// 通用数据表格。数值列右对齐。
export function DataTable({ columns, rows, maxHeight }: { columns: string[]; rows: any[]; maxHeight?: number }) {
  if (!rows || rows.length === 0) return <Empty>暂无可展示的数据行</Empty>;
  return (
    <div className="pivot-wrap" style={{ maxHeight: maxHeight ?? 460 }}>
      <table className="data" data-testid="data-table">
        <thead>
          <tr>{columns.map((c) => <th key={c}>{c}</th>)}</tr>
        </thead>
        <tbody>
          {rows.map((r, i) => (
            <tr key={i}>
              {columns.map((c) => {
                const v = r[c];
                const isNum = typeof v === 'number';
                return <td key={c} className={isNum ? 'num' : undefined}>{fmtCell(v)}</td>;
              })}
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}

export { fmtCell };
