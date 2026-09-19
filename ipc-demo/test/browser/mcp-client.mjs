// 极简 MCP (stdio, JSON-RPC) 客户端：用来调用 AionUi 内置的 chrome-devtools-mcp。
import { spawn } from 'node:child_process';

export function createMcp({ command, args, env }) {
  const child = spawn(command, args, { env: { ...process.env, ...env }, stdio: ['pipe', 'pipe', 'pipe'] });
  let buf = '';
  let id = 0;
  const pending = new Map();
  const handlers = new Map();
  child.stdout.on('data', (d) => {
    buf += d.toString();
    let idx;
    while ((idx = buf.indexOf('\n')) >= 0) {
      const line = buf.slice(0, idx).trim();
      buf = buf.slice(idx + 1);
      if (!line) continue;
      let msg; try { msg = JSON.parse(line); } catch { continue; }
      if (msg.id && pending.has(msg.id)) {
        const { res, rej } = pending.get(msg.id); pending.delete(msg.id);
        msg.error ? rej(new Error(JSON.stringify(msg.error))) : res(msg.result);
      }
    }
  });
  child.stderr.on('data', () => {});
  const request = (method, params = {}) => new Promise((res, rej) => {
    const mid = ++id; pending.set(mid, { res, rej });
    child.stdin.write(JSON.stringify({ jsonrpc: '2.0', id: mid, method, params }) + '\n');
    setTimeout(() => { if (pending.has(mid)) { pending.delete(mid); rej(new Error(`timeout: ${method}`)); } }, 60000);
  });
  const notify = (method, params = {}) => child.stdin.write(JSON.stringify({ jsonrpc: '2.0', method, params }) + '\n');
  return { child, request, notify, close: () => child.kill() };
}

export async function initMcp(mcp) {
  const init = await mcp.request('initialize', {
    protocolVersion: '2024-11-05',
    capabilities: {},
    clientInfo: { name: 'ipc-demo-browser-test', version: '1.0.0' },
  });
  mcp.notify('notifications/initialized', {});
  return init;
}
