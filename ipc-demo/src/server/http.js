// 极简 HTTP 服务器（仅使用 node:http，无额外 Web 框架依赖）。
// 提供 JSON API 与静态前端托管；默认仅绑定回环地址。
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { URL } from 'node:url';

const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.mjs': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.svg': 'image/svg+xml',
  '.png': 'image/png',
  '.ico': 'image/x-icon',
  '.map': 'application/json; charset=utf-8',
};

export function createServer({ routes, publicDir }) {
  return http.createServer(async (req, res) => {
    const url = new URL(req.url, `http://${req.headers.host || 'localhost'}`);
    const pathname = url.pathname;

    // API 路由
    if (pathname.startsWith('/api/')) {
      try {
        const match = routes.match(req.method, pathname);
        if (!match) {
          return sendJson(res, 404, { error: { message: `未找到路由: ${req.method} ${pathname}`, kind: 'not_found' } });
        }
        const body = await readBody(req);
        const result = await match.handler({
          req, res, params: match.params, query: Object.fromEntries(url.searchParams), body,
        });
        if (result !== undefined && !res.writableEnded) sendJson(res, result.status || 200, result.body ?? result);
      } catch (err) {
        const status = err.status || 500;
        if (status >= 500) console.error('[api] 未处理错误:', err);
        sendJson(res, status, { error: { message: err.message, kind: err.kind || 'internal_error', ...(err.missing ? { missing: err.missing } : {}) } });
      }
      return;
    }

    // 静态前端
    if (!publicDir) return sendText(res, 404, 'Not Found');
    serveStatic(publicDir, pathname, res);
  });
}

function readBody(req) {
  return new Promise((resolve, reject) => {
    if (req.method === 'GET' || req.method === 'HEAD') return resolve(null);
    let data = '';
    req.on('data', (c) => { data += c; if (data.length > 5_000_000) reject(new Error('请求体过大')); });
    req.on('end', () => {
      if (!data) return resolve(null);
      const ct = req.headers['content-type'] || '';
      if (ct.includes('application/json')) {
        try { resolve(JSON.parse(data)); } catch (e) { reject(Object.assign(new Error(`JSON 解析失败: ${e.message}`), { status: 400, kind: 'invalid_json' })); }
      } else if (ct.includes('text/plain') || ct.includes('text/csv')) {
        resolve(data);
      } else {
        try { resolve(JSON.parse(data)); } catch { resolve(data); }
      }
    });
    req.on('error', reject);
  });
}

function sendJson(res, status, body) {
  const payload = JSON.stringify(body, (k, v) => (typeof v === 'bigint' ? Number(v) : v));
  res.writeHead(status, { 'content-type': 'application/json; charset=utf-8' });
  res.end(payload);
}

function sendText(res, status, text) {
  res.writeHead(status, { 'content-type': 'text/plain; charset=utf-8' });
  res.end(text);
}

function serveStatic(root, pathname, res) {
  let rel = pathname === '/' ? 'index.html' : pathname.replace(/^\/+/, '');
  let full = path.join(root, rel);
  if (!full.startsWith(path.resolve(root)) && !full.startsWith(root)) {
    return sendText(res, 403, 'Forbidden');
  }
  if (!fs.existsSync(full) || fs.statSync(full).isDirectory()) {
    // SPA 回退：非静态资源一律返回 index.html
    full = path.join(root, 'index.html');
    if (!fs.existsSync(full)) return sendText(res, 404, '前端未构建。请运行 npm run build:web');
  }
  const ext = path.extname(full).toLowerCase();
  res.writeHead(200, { 'content-type': MIME[ext] || 'application/octet-stream' });
  fs.createReadStream(full).pipe(res);
}

// 简单路由表：支持 /api/scenarios/:scenarioId 这类路径参数
export function buildRouter() {
  const entries = [];
  return {
    add(method, pattern, handler) {
      const parts = pattern.split('/').filter(Boolean);
      entries.push({ method, parts, handler });
    },
    match(method, pathname) {
      const parts = pathname.split('/').filter(Boolean);
      for (const e of entries) {
        if (e.method !== method) continue;
        if (e.parts.length !== parts.length) continue;
        const params = {};
        let ok = true;
        for (let i = 0; i < e.parts.length; i++) {
          const p = e.parts[i];
          if (p.startsWith(':')) params[p.slice(1)] = decodeURIComponent(parts[i]);
          else if (p !== parts[i]) { ok = false; break; }
        }
        if (ok) return { handler: e.handler, params };
      }
      return null;
    },
  };
}
