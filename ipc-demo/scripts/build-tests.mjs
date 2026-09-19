// 将前端 TSX 组件预编译为 ESM JS，供 node:test 直接导入（Node 无法原生加载 .tsx）。
import { build } from 'esbuild';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const APP_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const webSrc = path.join(APP_ROOT, 'src', 'web', 'src');
const outDir = path.join(APP_ROOT, 'test', '.tsx-build');

await build({
  entryPoints: {
    PivotGrid: path.join(webSrc, 'components', 'PivotGrid.tsx'),
    common: path.join(webSrc, 'components', 'common.tsx'),
  },
  bundle: true,
  format: 'esm',
  platform: 'node',
  jsx: 'automatic',
  outdir: outDir,
  external: ['react', 'react-dom', 'react/jsx-runtime'],
  logLevel: 'warning',
});
console.log(`[build-tests] 预编译组件 -> ${outDir}`);
