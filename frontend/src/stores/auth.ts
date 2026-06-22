import { defineStore } from 'pinia'
import { ref } from 'vue'
import apiClient from '@/api/axios'

const STORAGE_KEY = 'smart_home_auth'

function loadAuth(): { username: string; loggedIn: boolean } {
  try {
    const raw = localStorage.getItem(STORAGE_KEY)
    if (raw) {
      return JSON.parse(raw)
    }
  } catch {
  }
  return { username: '', loggedIn: false }
}

function saveAuth(username: string, loggedIn: boolean) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify({ username, loggedIn }))
}

export const useAuthStore = defineStore('auth', () => {
  const initial = loadAuth()
  const username = ref<string>(initial.username)
  const loggedIn = ref<boolean>(initial.loggedIn)

  const login = async (name: string, password: string): Promise<boolean> => {
    try {
      const data = await apiClient.post('/login', {
        account_id: name,
        passwd: password,
      }) as any

      if (data && data.success) {
        username.value = data.username
        loggedIn.value = true
        saveAuth(data.username, true)
        return true
      }
      return false
    } catch {
      return false
    }
  }

  const logout = () => {
    username.value = ''
    loggedIn.value = false
    saveAuth('', false)
  }

  return { username, loggedIn, login, logout }
})
