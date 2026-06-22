import axios from 'axios'

const apiClient = axios.create({
  baseURL: '/api',       // 所有请求前面加 /api 
  timeout: 10000,        // 请求超时时间
  headers: {
    'Content-Type': 'application/json'
  }
})

apiClient.interceptors.response.use(
  (response) => response.data,
  (error) => {
    console.error('API Error:', error)
    return Promise.reject(error)
  }
)

export default apiClient
