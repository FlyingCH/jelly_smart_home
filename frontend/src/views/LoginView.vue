<template>
  <div class="min-h-screen bg-gradient-to-br from-blue-600 via-purple-500 to-pink-400 flex items-center justify-center p-4">
    <div class="w-full max-w-md">
      <div class="bg-white rounded-2xl shadow-2xl overflow-hidden">
        <div class="bg-gradient-to-r from-blue-500 to-purple-600 p-8 text-center">
          <div class="text-5xl mb-3">🏠</div>
          <h1 class="text-2xl font-bold text-white">果冻智能家</h1>
          <p class="text-blue-100 text-sm mt-1">Smart Home Dashboard</p>
        </div>

        <div class="p-8">
          <h2 class="text-xl font-semibold text-gray-800 mb-6">🔐 登录系统</h2>

          <div v-if="errorMsg" class="mb-4 bg-red-50 border border-red-200 text-red-700 rounded-lg px-4 py-3 text-sm flex items-center gap-2">
            <span>⚠️</span>
            <span>{{ errorMsg }}</span>
          </div>

          <div class="space-y-5">
            <div>
              <label class="block text-sm font-medium text-gray-600 mb-1.5">用户名</label>
              <div class="relative">
                <span class="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400">👤</span>
                <input
                  v-model="form.username"
                  @keyup.enter="handleLogin"
                  type="text"
                  placeholder="请输入用户名"
                  class="w-full border border-gray-300 rounded-lg pl-10 pr-4 py-3 text-sm focus:ring-2 focus:ring-blue-500 focus:border-blue-500 outline-none transition"
                  autocomplete="username"
                />
              </div>
            </div>

            <div>
              <label class="block text-sm font-medium text-gray-600 mb-1.5">密码</label>
              <div class="relative">
                <span class="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400">🔒</span>
                <input
                  v-model="form.password"
                  @keyup.enter="handleLogin"
                  :type="showPassword ? 'text' : 'password'"
                  placeholder="请输入密码"
                  class="w-full border border-gray-300 rounded-lg pl-10 pr-12 py-3 text-sm focus:ring-2 focus:ring-blue-500 focus:border-blue-500 outline-none transition"
                  autocomplete="current-password"
                />
                <button
                  @click="showPassword = !showPassword"
                  type="button"
                  class="absolute right-3 top-1/2 -translate-y-1/2 text-gray-400 hover:text-gray-600 text-sm"
                >
                  {{ showPassword ? '🙈' : '👁️' }}
                </button>
              </div>
            </div>

            <button
              @click="handleLogin"
              :disabled="loading"
              class="w-full bg-gradient-to-r from-blue-500 to-purple-600 text-white py-3 rounded-lg font-medium hover:from-blue-600 hover:to-purple-700 transition shadow-lg hover:shadow-xl disabled:opacity-50 disabled:cursor-not-allowed"
            >
              <span v-if="loading" class="flex items-center justify-center gap-2">
                <span class="inline-block w-4 h-4 border-2 border-white border-t-transparent rounded-full animate-spin"></span>
                登录中...
              </span>
              <span v-else>登 录</span>
            </button>
          </div>

          <p class="text-center text-xs text-gray-400 mt-6">
            默认账号: <code class="bg-gray-100 px-1.5 py-0.5 rounded">admin</code> /
            <code class="bg-gray-100 px-1.5 py-0.5 rounded">85459939</code>
          </p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'

const router = useRouter()
const authStore = useAuthStore()

const form = ref({ username: '', password: '' })
const loading = ref(false)
const errorMsg = ref('')
const showPassword = ref(false)

const handleLogin = async () => {
  errorMsg.value = ''

  if (!form.value.username.trim()) {
    errorMsg.value = '请输入用户名'
    return
  }
  if (!form.value.password) {
    errorMsg.value = '请输入密码'
    return
  }

  loading.value = true

  const success = await authStore.login(form.value.username.trim(), form.value.password)
  loading.value = false

  if (success) {
    router.replace('/')
  } else {
    errorMsg.value = '用户名或密码错误'
  }
}
</script>
