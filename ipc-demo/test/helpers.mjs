// 测试辅助：临时目录、临时配置、启动服务。
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { loadConfig, ensureDirs } from '../src/config.js';
import { openDb, closeDb } from '../src/db/duckdb.js';

// 伪造一个隔离的场景根：把仓库 demo 场景复制进来，避免测试写入仓库 data/。
export function tempConfig({ engineBin, engineTimeoutMs, scenarioRoot, importOnStart = false } = {}) {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'ipc-demo-test-'));
  let root = scenarioRoot;
  if (!root) {
    root = path.join(tmp, 'data-root');
    fs.mkdirSync(root, { recursive: true });
    // 复制 demo 场景（三个 benchmark）与 sample，保持真实可用
    const repoData = path.resolve(import.meta.dirname, '..', '..', 'ipc-core-benchmark', 'data');
    if (fs.existsSync(path.join(repoData, 'demo'))) {
      fs.cpSync(path.join(repoData, 'demo'), path.join(root, 'demo'), { recursive: true });
    }
    if (fs.existsSync(path.join(repoData, 'sample'))) {
      fs.cpSync(path.join(repoData, 'sample'), path.join(root, 'sample'), { recursive: true });
    }
  }
  const cfg = loadConfig({
    port: 0,
    duckdbPath: path.join(tmp, 'demo.duckdb'),
    runsDir: path.join(tmp, 'runs'),
    writableScenarioRoot: path.join(tmp, 'scenarios'),
    scenarioRoot: root,
    ...(engineBin ? { engineBin } : {}),
    ...(engineTimeoutMs ? { engineTimeoutMs } : {}),
  });
  return { cfg, tmp };
}

export async function withDb(fn) {
  const { cfg, tmp } = tempConfig();
  ensureDirs(cfg);
  await openDb(cfg);
  try { return await fn(cfg, tmp); } finally { await closeDb(); }
}

export async function startTestServer(cfg, opts = {}) {
  const { startServer } = await import('../src/server/index.js');
  const server = await startServer(cfg, { importOnStart: true, ...opts });
  const base = `http://127.0.0.1:${server.address().port}`;
  return {
    server, base,
    async close() { server.close(); await closeDb(); },
    async json(method, url, body) {
      const res = await fetch(base + url, {
        method,
        headers: body ? { 'content-type': 'application/json' } : {},
        body: body ? JSON.stringify(body) : undefined,
      });
      return { status: res.status, body: await res.json() };
    },
  };
}
