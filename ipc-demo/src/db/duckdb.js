// DuckDB 访问封装：单一 repository 入口，供上层不直接依赖具体绑定。
import { DuckDBInstance } from '@duckdb/node-api';
import { allDdl } from './schema.js';
import { ensureDirs } from '../config.js';

let _instance = null;
let _conn = null;
let _dbPath = null;

// 建立（或复用）到指定路径的连接，并初始化 schema。
export async function openDb(cfg) {
  if (_conn && _dbPath === cfg.duckdbPath) return _conn;
  ensureDirs(cfg);
  _instance = await DuckDBInstance.create(cfg.duckdbPath);
  _conn = await _instance.connect();
  _dbPath = cfg.duckdbPath;
  for (const ddl of allDdl()) {
    await _conn.run(ddl);
  }
  return _conn;
}

export function getDb() {
  if (!_conn) throw new Error('DuckDB 未初始化，请先调用 openDb(cfg)');
  return _conn;
}

export async function closeDb() {
  // 先关连接，再关实例；两者都释放后 DuckDB 才会释放文件锁，
  // 否则同一路径被其他进程（如子进程启动的服务）打开会报 Conflicting lock。
  if (_conn) {
    try { _conn.closeSync?.(); } catch { /* 已关闭 */ }
    _conn = null;
  }
  if (_instance) {
    try { _instance.closeSync?.(); } catch { /* 已关闭 */ }
    _instance = null;
  }
  _dbPath = null;
}

// ---- 便捷查询 ----
// 返回普通 JS 对象数组（BigInt -> Number，便于 JSON 序列化）
export async function query(sql) {
  const res = await getDb().runAndReadAll(sql);
  return normalizeRows(res.getRowObjects());
}

export async function queryOne(sql) {
  const rows = await query(sql);
  return rows.length ? rows[0] : null;
}

// 执行的 DDL/DML（无返回）
export async function exec(sql) {
  await getDb().run(sql);
}

// 参数化执行：使用 DuckDB 的预处理语句避免注入
export async function execParams(sql, params) {
  const prepared = await getDb().prepare(sql);
  prepared.bind(params);
  await prepared.run();
}

// 参数化查询
export async function queryParams(sql, params) {
  const prepared = await getDb().prepare(sql);
  prepared.bind(params);
  const res = await prepared.runAndReadAll();
  return normalizeRows(res.getRowObjects());
}

// BigInt -> Number（演示数据规模不会超出安全整数范围）；
// Buffer/特殊类型原样保留。
function normalizeRows(rows) {
  return rows.map((row) => {
    const out = {};
    for (const [k, v] of Object.entries(row)) {
      out[k] = typeof v === 'bigint' ? Number(v) : v;
    }
    return out;
  });
}

// SQL 字面量转义（用于必须拼字符串的场景，如 read_csv_auto 路径）
export function sqlLiteral(value) {
  if (value === null || value === undefined) return 'NULL';
  if (typeof value === 'number') return String(value);
  if (typeof value === 'boolean') return value ? 'true' : 'false';
  return `'${String(value).replace(/'/g, "''")}'`;
}
