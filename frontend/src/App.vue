<template>
  <div v-if="$route.name === 'login'">
    <router-view />
  </div>

  <div v-else-if="$route.name === 'home' || $route.name === 'console' || $route.name === 'temperature' || $route.name === 'electricity'" class="overflow-hidden">
    <router-view />
  </div>

  <div v-else class="min-h-screen bg-gradient-to-br from-blue-500 to-purple-600">
    <header class="bg-white shadow-lg">
      <nav class="container mx-auto px-4 py-4">
        <div class="flex items-center justify-between">
          <h1 class="text-2xl font-bold text-gray-800">🏠 果冻智能家</h1>
          <div class="flex items-center space-x-4">
            <span class="text-sm text-gray-500">👤 {{ authStore.username }}</span>
            <button
              @click="handleLogout"
              class="text-sm text-red-500 hover:text-red-600 transition border border-red-200 rounded-lg px-3 py-1 hover:bg-red-50"
            >
              退出
            </button>
          </div>
        </div>
      </nav>
    </header>

    <main class="container mx-auto px-4 py-8">
      <router-view />
    </main>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'

const router = useRouter()
const authStore = useAuthStore()

const handleLogout = () => {
  authStore.logout()
  router.replace('/login')
}
</script>
