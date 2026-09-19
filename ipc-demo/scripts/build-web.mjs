// 构建前端 SPA：Vite + React + TypeScript -> ipc-demo/public（由后端静态托管）。
import { spawnSync } from 'node:child_process';
import path from 'node:path';
import fs from 'node:fs';
import { fileURLToPath } from 'node:url';

const APP_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const webRoot = path.join(APP_ROOT, 'src', 'web');
const vite = path.join(APP_ROOT, 'node_modules', '.bin', process.platform === 'win32' ? 'vite.cmd' : 'vite');

if (!fs.existsSync(vite)) {
  console.error('[build-web] 未找到 Vite，请先在 ipc-demo 下执行 npm install');
  process.exit(1);
}

const res = spawnSync(vite, ['build', '--config', path.join(webRoot, 'vite.config.ts')], {
  cwd: APP_ROOT, stdio: 'inherit', env: process.env,
});
if (res.status !== 0) process.exit(res.status ?? 1);

const out = path.join(APP_ROOT, 'public');
const files = fs.existsSync(out) ? fs.readdirSync(out) : [];
console.log(`[build-web] 构建完成 -> ${out} (${files.join(', ')})`);
