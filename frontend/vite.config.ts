import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { resolve } from 'path'

export default defineConfig({
  plugins: [vue()],
  resolve: {
    alias: {
      '@': resolve(__dirname, 'src')
    }
  },
  server: {
    host: '0.0.0.0',
    port: 8001,     // 前端网页的端口
    strictPort: true,
    watch: {
      ignored: ['**/public/map/**'],
    },
    proxy: {
      '/api': {
        target: 'http://localhost:8000',   // 转发到后端
        changeOrigin: true
      }
    }
  },
  preview: {
    host: '0.0.0.0',
    port: 8001,
    strictPort: true
  }
})
