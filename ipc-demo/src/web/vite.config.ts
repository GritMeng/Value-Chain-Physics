import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// 构建产物输出到 ipc-demo/public，由后端静态托管。
export default defineConfig({
  root: __dirname,
  plugins: [react()],
  build: {
    outDir: '../../public',
    emptyOutDir: true,
  },
  server: {
    proxy: {
      '/api': 'http://127.0.0.1:3001',
    },
  },
});
