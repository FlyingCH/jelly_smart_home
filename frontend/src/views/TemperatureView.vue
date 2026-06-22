<template>
  <div class="temp-dashboard">
    <div class="top-bar">
      <div class="top-left">
        <div class="logo-icon">🏠</div>
        <h1 class="main-title">果冻智慧家</h1>
        <span class="sub-title">—— 一个有智慧的家</span>
        <span class="nav-divider">|</span>
        <router-link to="/" class="nav-link">首页</router-link>
        <router-link to="/temperature" class="nav-link active">温湿度</router-link>
        <router-link to="/electricity" class="nav-link">用电</router-link>
        <router-link to="/car" class="nav-link">车辆</router-link>
        <router-link to="/console" class="nav-link">控制台</router-link>
      </div>
      <div class="top-right">
        <span class="datetime">{{ currentTime }}</span>
        <span class="user-badge">👤 {{ authStore.username }}</span>
        <button class="logout-btn" @click="handleLogout">退出</button>
      </div>
    </div>

    <div class="main-grid">
      <div class="left-panel">
        <div class="data-cards-row">
          <div class="data-card">
            <div class="data-card-label">当前温度</div>
            <div class="data-card-value temp">{{ currentData.temperature.toFixed(1) }}<span class="unit">°C</span></div>
            <div class="data-card-sub">实时监测</div>
            <div ref="miniTempChart" class="mini-chart"></div>
          </div>
          <div class="data-card">
            <div class="data-card-label">当前湿度</div>
            <div class="data-card-value humi">{{ currentData.humidity.toFixed(1) }}<span class="unit">%</span></div>
            <div class="data-card-sub">实时监测</div>
            <div ref="miniHumiChart" class="mini-chart"></div>
          </div>
        </div>

        <div class="panel-box room-panels-box">
          <div class="panel-title">🏠 各房间温湿度</div>
          <div class="room-grid">
            <div v-for="room in roomData" :key="room.name" class="room-card">
              <div class="room-name">{{ room.icon }} {{ room.name }}</div>
              <div class="room-temp" :style="{ color: room.tempColor }">{{ room.temp.toFixed(1) }}<span class="room-unit">°C</span></div>
              <div class="room-humi" :style="{ color: room.humiColor }">💧 {{ room.humi.toFixed(1) }}<span class="room-unit">%</span></div>
            </div>
          </div>
        </div>

        <div class="panel-box weather-panel">
          <div class="panel-title">🌤️ 室外环境指数</div>
          <div class="weather-grid">
            <div class="weather-day today">
              <div class="weather-label">今日</div>
              <div class="weather-icon">{{ weather.today.icon }}</div>
              <div class="weather-desc">{{ weather.today.desc }}</div>
              <div class="weather-temp">
                <span class="temp-high">↑{{ weather.today.high }}°</span>
                <span class="temp-low">↓{{ weather.today.low }}°</span>
              </div>
            </div>
            <div class="weather-day tomorrow">
              <div class="weather-label">明日</div>
              <div class="weather-icon">{{ weather.tomorrow.icon }}</div>
              <div class="weather-desc">{{ weather.tomorrow.desc }}</div>
              <div class="weather-temp">
                <span class="temp-high">↑{{ weather.tomorrow.high }}°</span>
                <span class="temp-low">↓{{ weather.tomorrow.low }}°</span>
              </div>
            </div>
          </div>
          <div class="pm25-bar" :class="weather.pm25Level">
            <span class="pm25-label">PM2.5</span>
            <span class="pm25-value">{{ weather.pm25 }}</span>
            <span class="pm25-unit">μg/m³</span>
            <span class="pm25-tag">{{ weather.pm25Text }}</span>
          </div>
        </div>
      </div>

      <div class="center-panel">
        <div class="panel-box info-panel">
          <div class="panel-title">📋 设备信息</div>
          <div class="info-grid">
            <div class="info-item">
              <span class="info-label">设备 ID</span>
              <span class="info-value">{{ currentData.deviceId || 'N/A' }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">记录时间</span>
              <span class="info-value">{{ currentData.recordedAt || '--' }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">数据刷新</span>
              <span class="info-value"><span class="status-dot active"></span> 自动刷新中</span>
            </div>
          </div>
        </div>

        <div class="panel-box chart-box-main">
          <div class="panel-title">📈 温湿度历史曲线</div>
          <div ref="historyChart" class="chart-area"></div>
        </div>
      </div>

      <div class="right-panel">
        <div class="panel-box stat-panel">
          <div class="panel-title">📊 温度统计</div>
          <div class="stat-grid">
            <div class="stat-card">
              <div class="stat-label">最高温度</div>
              <div class="stat-value" style="color:#FF6B6B">{{ tempStats.max.toFixed(1) }}<span class="unit">°C</span></div>
            </div>
            <div class="stat-card">
              <div class="stat-label">最低温度</div>
              <div class="stat-value" style="color:#4A90D9">{{ tempStats.min.toFixed(1) }}<span class="unit">°C</span></div>
            </div>
            <div class="stat-card">
              <div class="stat-label">平均温度</div>
              <div class="stat-value" style="color:#FF9500">{{ tempStats.avg.toFixed(1) }}<span class="unit">°C</span></div>
            </div>
          </div>
        </div>

        <div class="panel-box stat-panel">
          <div class="panel-title">💧 湿度统计</div>
          <div class="stat-grid">
            <div class="stat-card">
              <div class="stat-label">最高湿度</div>
              <div class="stat-value" style="color:#00D4FF">{{ humiStats.max.toFixed(1) }}<span class="unit">%</span></div>
            </div>
            <div class="stat-card">
              <div class="stat-label">最低湿度</div>
              <div class="stat-value" style="color:#A855F7">{{ humiStats.min.toFixed(1) }}<span class="unit">%</span></div>
            </div>
            <div class="stat-card">
              <div class="stat-label">平均湿度</div>
              <div class="stat-value" style="color:#10b981">{{ humiStats.avg.toFixed(1) }}<span class="unit">%</span></div>
            </div>
          </div>
        </div>

        <div class="panel-box" style="flex:1">
          <div class="panel-title">📋 历史记录</div>
          <div class="history-table-wrap">
            <table class="history-table">
              <thead>
                <tr>
                  <th>时间</th>
                  <th>温度</th>
                  <th>湿度</th>
                </tr>
              </thead>
              <tbody>
                <tr v-for="(item, index) in historyTableData" :key="index">
                  <td>{{ item.recordedAt }}</td>
                  <td :style="{ color: item.tempColor }">{{ item.temperature.toFixed(1) }}°C</td>
                  <td :style="{ color: item.humiColor }">{{ item.humidity.toFixed(1) }}%</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, nextTick } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import * as echarts from 'echarts'
import { getLatestTemperatureHumidity, getTemperatureHumidityHistory } from '@/api/temperature'
import type { TemperatureHumidity } from '@/types'

const router = useRouter()
const authStore = useAuthStore()

const currentData = ref<TemperatureHumidity>({
  temperature: 0,
  humidity: 0,
  recordedAt: '',
  deviceId: ''
})

const historyRaw = ref<TemperatureHumidity[]>([])
const currentTime = ref('')

let clockTimer: number | null = null
let refreshTimer: number | null = null

const historyChart = ref<HTMLElement | null>(null)
const miniTempChart = ref<HTMLElement | null>(null)
const miniHumiChart = ref<HTMLElement | null>(null)
let historyInst: echarts.ECharts | null = null
let miniTempInst: echarts.ECharts | null = null
let miniHumiInst: echarts.ECharts | null = null

const weather = ref({
  today: { icon: '🌤️', desc: '晴转多云', high: 32, low: 22 },
  tomorrow: { icon: '🌧️', desc: '小雨', high: 28, low: 20 },
  pm25: 38,
  pm25Level: 'good',
  pm25Text: '优',
})

const roomData = ref([
  { name: '客厅', icon: '🛋️', temp: 26.5, humi: 58, tempColor: '#FF9500', humiColor: '#00D4FF' },
  { name: '卧室', icon: '🛏️', temp: 24.2, humi: 52, tempColor: '#4A90D9', humiColor: '#00D4FF' },
  { name: '厨房', icon: '🍳', temp: 30.1, humi: 65, tempColor: '#FF6B6B', humiColor: '#4A90D9' },
  { name: '浴室', icon: '🛁', temp: 27.8, humi: 72, tempColor: '#FF9500', humiColor: '#4A90D9' },
])

const tempStats = computed(() => {
  if (historyRaw.value.length === 0) return { max: 0, min: 0, avg: 0 }
  const temps = historyRaw.value.map(h => h.temperature)
  return {
    max: Math.max(...temps),
    min: Math.min(...temps),
    avg: temps.reduce((a, b) => a + b, 0) / temps.length,
  }
})

const humiStats = computed(() => {
  if (historyRaw.value.length === 0) return { max: 0, min: 0, avg: 0 }
  const humis = historyRaw.value.map(h => h.humidity)
  return {
    max: Math.max(...humis),
    min: Math.min(...humis),
    avg: humis.reduce((a, b) => a + b, 0) / humis.length,
  }
})

const historyTableData = computed(() => {
  return historyRaw.value.slice(-20).reverse().map(item => ({
    ...item,
    tempColor: item.temperature > 30 ? '#FF6B6B' : item.temperature > 25 ? '#FF9500' : item.temperature > 20 ? '#00D4FF' : '#4A90D9',
    humiColor: item.humidity > 70 ? '#00D4FF' : item.humidity > 50 ? '#10b981' : '#FF9500',
  }))
})

const updateClock = () => {
  const now = new Date()
  const days = ['日', '一', '二', '三', '四', '五', '六']
  const y = now.getFullYear()
  const m = String(now.getMonth() + 1).padStart(2, '0')
  const d = String(now.getDate()).padStart(2, '0')
  const hh = String(now.getHours()).padStart(2, '0')
  const mm = String(now.getMinutes()).padStart(2, '0')
  const ss = String(now.getSeconds()).padStart(2, '0')
  const w = days[now.getDay()]
  currentTime.value = `${y}年${m}月${d}日 ${hh}:${mm}:${ss} 星期${w}`
}

const miniChartOption = (color: string) => ({
  grid: { top: 2, right: 2, bottom: 2, left: 2 },
  xAxis: { show: false, data: ['', '', '', '', '', ''] },
  yAxis: { show: false },
  series: [{
    type: 'line',
    data: [22, 24, 23, 25, 24, 26],
    smooth: true,
    symbol: 'none',
    lineStyle: { color, width: 1.5 },
    areaStyle: {
      color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
        { offset: 0, color },
        { offset: 1, color: 'transparent' }
      ])
    }
  }]
})

const historyChartOption = (times: string[], temps: number[], humis: number[]) => ({
  grid: { top: 16, right: 20, bottom: 28, left: 42 },
  tooltip: { trigger: 'axis' },
  legend: {
    data: ['温度', '湿度'],
    right: 0,
    top: 0,
    textStyle: { color: '#8899AA', fontSize: 10 },
    itemWidth: 12,
    itemHeight: 2,
  },
  xAxis: {
    type: 'category',
    data: times,
    axisLine: { lineStyle: { color: 'rgba(0,180,255,0.2)' } },
    axisLabel: { color: '#667788', fontSize: 9, rotate: times.length > 12 ? 30 : 0 },
    axisTick: { show: false },
  },
  yAxis: [
    {
      type: 'value',
      name: '温度 (°C)',
      nameTextStyle: { color: '#8899AA', fontSize: 9 },
      splitLine: { lineStyle: { color: 'rgba(0,180,255,0.08)' } },
      axisLabel: { color: '#667788', fontSize: 9 },
    },
    {
      type: 'value',
      name: '湿度 (%)',
      nameTextStyle: { color: '#8899AA', fontSize: 9 },
      splitLine: { show: false },
      axisLabel: { color: '#667788', fontSize: 9 },
    }
  ],
  series: [
    {
      name: '温度',
      type: 'line',
      data: temps,
      smooth: true,
      symbol: 'circle',
      symbolSize: 4,
      lineStyle: { color: '#FF9500', width: 2 },
      itemStyle: { color: '#FF9500' },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: 'rgba(255,149,0,0.2)' },
          { offset: 1, color: 'rgba(255,149,0,0.02)' }
        ])
      },
    },
    {
      name: '湿度',
      type: 'line',
      yAxisIndex: 1,
      data: humis,
      smooth: true,
      symbol: 'circle',
      symbolSize: 4,
      lineStyle: { color: '#00D4FF', width: 2 },
      itemStyle: { color: '#00D4FF' },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: 'rgba(0,212,255,0.2)' },
          { offset: 1, color: 'rgba(0,212,255,0.02)' }
        ])
      },
    }
  ]
})

const fetchCurrentData = async () => {
  try {
    const data = await getLatestTemperatureHumidity()
    currentData.value = data
  } catch {
    // keep current data
  }
}

const fetchHistoryData = async () => {
  try {
    const data = await getTemperatureHumidityHistory()
    historyRaw.value = data
    updateHistoryChart()
  } catch {
    // keep existing data
  }
}

const updateHistoryChart = () => {
  if (!historyInst || historyRaw.value.length === 0) return
  const times = historyRaw.value.map(item => item.recordedAt)
  const temps = historyRaw.value.map(item => item.temperature)
  const humis = historyRaw.value.map(item => item.humidity)
  historyInst.setOption(historyChartOption(times, temps, humis))
}

const handleResize = () => {
  historyInst?.resize()
  miniTempInst?.resize()
  miniHumiInst?.resize()
}

const handleLogout = () => {
  authStore.logout()
  router.replace('/login')
}

onMounted(async () => {
  updateClock()
  clockTimer = window.setInterval(updateClock, 1000)

  await nextTick()

  if (historyChart.value) {
    historyInst = echarts.init(historyChart.value)
    historyInst.setOption(historyChartOption([], [], []))
  }
  if (miniTempChart.value) {
    miniTempInst = echarts.init(miniTempChart.value)
    miniTempInst.setOption(miniChartOption('#00D4FF'))
  }
  if (miniHumiChart.value) {
    miniHumiInst = echarts.init(miniHumiChart.value)
    miniHumiInst.setOption(miniChartOption('#4A90D9'))
  }

  fetchCurrentData()
  fetchHistoryData()
  refreshTimer = window.setInterval(() => {
    fetchCurrentData()
    fetchHistoryData()
  }, 5000)

  window.addEventListener('resize', handleResize)
})

onUnmounted(() => {
  if (clockTimer) clearInterval(clockTimer)
  if (refreshTimer) clearInterval(refreshTimer)
  window.removeEventListener('resize', handleResize)
  historyInst?.dispose()
  miniTempInst?.dispose()
  miniHumiInst?.dispose()
})
</script>

<style scoped>
.temp-dashboard {
  width: 100vw;
  height: 100vh;
  overflow: hidden;
  background: #0a1628;
  background-image:
    radial-gradient(ellipse at 20% 50%, rgba(0, 120, 200, 0.08) 0%, transparent 60%),
    radial-gradient(ellipse at 80% 20%, rgba(0, 180, 220, 0.06) 0%, transparent 55%),
    linear-gradient(180deg, #0A1F44 0%, #0A1628 30%, #091220 100%);
  color: #c8d8e8;
  font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif;
  display: flex;
  flex-direction: column;
  padding: 10px 14px;
  box-sizing: border-box;
}

.top-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  height: 52px;
  flex-shrink: 0;
  margin-bottom: 8px;
  padding: 0 8px;
  border-bottom: 1px solid rgba(0, 180, 255, 0.15);
}

.top-left {
  display: flex;
  align-items: baseline;
  gap: 8px;
}

.logo-icon {
  font-size: 28px;
}

.main-title {
  font-size: 24px;
  font-weight: 800;
  color: #E8F4FF;
  letter-spacing: 3px;
  text-shadow: 0 0 18px rgba(0, 180, 255, 0.4);
}

.sub-title {
  font-size: 12px;
  color: #5599BB;
  letter-spacing: 2px;
}

.nav-divider {
  color: rgba(0, 180, 255, 0.25);
  font-size: 14px;
  margin: 0 4px;
}

.nav-link {
  font-size: 12px;
  color: #7A8FA0;
  text-decoration: none;
  letter-spacing: 0.5px;
  transition: color 0.2s;
}

.nav-link:hover {
  color: #00D4FF;
}

.nav-link.active {
  color: #00D4FF;
  font-weight: 600;
}

.top-right {
  display: flex;
  align-items: center;
  gap: 16px;
}

.datetime {
  font-size: 13px;
  color: #7A8FA0;
  font-family: 'Consolas', 'Courier New', monospace;
  letter-spacing: 1px;
}

.user-badge {
  font-size: 12px;
  color: #8899AA;
}

.logout-btn {
  background: rgba(255, 80, 80, 0.15);
  border: 1px solid rgba(255, 80, 80, 0.3);
  color: #FF6B6B;
  padding: 3px 12px;
  border-radius: 4px;
  font-size: 11px;
  cursor: pointer;
  transition: all 0.2s;
}

.logout-btn:hover {
  background: rgba(255, 80, 80, 0.25);
  border-color: rgba(255, 80, 80, 0.5);
}

.main-grid {
  flex: 1;
  display: grid;
  grid-template-columns: 1fr 1.4fr 1fr;
  gap: 10px;
  min-height: 0;
}

.left-panel,
.right-panel {
  display: flex;
  flex-direction: column;
  gap: 10px;
  min-height: 0;
}

.center-panel {
  display: flex;
  flex-direction: column;
  gap: 10px;
  min-height: 0;
}

.panel-box {
  background: rgba(8, 34, 72, 0.55);
  border: 1px solid rgba(0, 180, 255, 0.18);
  border-radius: 6px;
  padding: 10px 12px;
  backdrop-filter: blur(4px);
}

.panel-title {
  font-size: 13px;
  font-weight: 600;
  color: #A0C8E8;
  margin-bottom: 8px;
  letter-spacing: 1px;
  border-left: 3px solid #00D4FF;
  padding-left: 8px;
}

.data-cards-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

.data-card {
  background: rgba(8, 34, 72, 0.55);
  border: 1px solid rgba(0, 180, 255, 0.18);
  border-radius: 6px;
  padding: 10px 12px;
  text-align: center;
}

.data-card-label {
  font-size: 11px;
  color: #667788;
  text-transform: uppercase;
  letter-spacing: 1px;
}

.data-card-value {
  font-size: 32px;
  font-weight: 800;
  font-family: 'Consolas', 'Courier New', monospace;
  margin: 4px 0;
}

.data-card-value.temp { color: #00D4FF; text-shadow: 0 0 12px rgba(0,212,255,0.5); }
.data-card-value.humi { color: #4A90D9; text-shadow: 0 0 12px rgba(74,144,217,0.5); }
.data-card-value .unit { font-size: 14px; font-weight: 400; margin-left: 2px; opacity: 0.7; }

.data-card-sub {
  font-size: 10px;
  color: #556677;
}

.mini-chart {
  height: 36px;
  margin-top: 4px;
}

.room-panels-box {
  flex: 1;
  display: flex;
  flex-direction: column;
}

.room-grid {
  flex: 1;
  display: grid;
  grid-template-columns: 1fr 1fr;
  grid-template-rows: 1fr 1fr;
  gap: 8px;
  min-height: 0;
}

.room-card {
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(0, 180, 255, 0.12);
  border-radius: 5px;
  padding: 8px 10px;
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  gap: 3px;
}

.room-name {
  font-size: 11px;
  color: #8899AA;
  letter-spacing: 0.5px;
}

.room-temp {
  font-size: 24px;
  font-weight: 800;
  font-family: 'Consolas', 'Courier New', monospace;
}

.room-humi {
  font-size: 13px;
  font-weight: 600;
  font-family: 'Consolas', 'Courier New', monospace;
}

.room-unit {
  font-size: 11px;
  font-weight: 400;
  opacity: 0.6;
  margin-left: 1px;
}

.weather-panel {
  min-height: 0;
}

.weather-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px;
}

.weather-day {
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(0, 180, 255, 0.12);
  border-radius: 5px;
  padding: 8px 10px;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
}

.weather-label {
  font-size: 10px;
  color: #667788;
  letter-spacing: 1px;
}

.weather-icon {
  font-size: 26px;
  line-height: 1;
}

.weather-desc {
  font-size: 11px;
  color: #A0C8E8;
  font-weight: 500;
}

.weather-temp {
  display: flex;
  gap: 8px;
  margin-top: 2px;
}

.temp-high {
  font-size: 11px;
  color: #FF6B6B;
  font-weight: 600;
  font-family: 'Consolas', 'Courier New', monospace;
}

.temp-low {
  font-size: 11px;
  color: #4A90D9;
  font-weight: 600;
  font-family: 'Consolas', 'Courier New', monospace;
}

.pm25-bar {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 8px;
  padding: 8px 10px;
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(0, 180, 255, 0.12);
  border-radius: 5px;
}

.pm25-bar.good { border-color: rgba(0, 230, 118, 0.3); background: rgba(0, 230, 118, 0.06); }

.pm25-label {
  font-size: 11px;
  color: #667788;
}

.pm25-value {
  font-size: 20px;
  font-weight: 800;
  font-family: 'Consolas', 'Courier New', monospace;
  color: #E0F0FF;
}

.pm25-unit {
  font-size: 9px;
  color: #667788;
}

.pm25-tag {
  font-size: 10px;
  padding: 2px 8px;
  border-radius: 3px;
  background: rgba(0, 230, 118, 0.15);
  color: #00E676;
  margin-left: auto;
  font-weight: 600;
}

.info-panel {
  flex-shrink: 0;
}

.info-grid {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.info-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 6px 8px;
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(0, 180, 255, 0.1);
  border-radius: 4px;
}

.info-label {
  font-size: 11px;
  color: #667788;
}

.info-value {
  font-size: 12px;
  color: #A0C8E8;
  font-family: 'Consolas', 'Courier New', monospace;
  display: flex;
  align-items: center;
  gap: 6px;
}

.status-dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: #16a34a;
  flex-shrink: 0;
}

.status-dot.active {
  animation: pulse-dot 1.5s ease-in-out infinite;
}

@keyframes pulse-dot {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.3; }
}

.chart-box-main {
  flex: 1;
  display: flex;
  flex-direction: column;
}

.chart-area {
  flex: 1;
  min-height: 0;
}

.stat-panel {
  flex-shrink: 0;
}

.stat-grid {
  display: grid;
  grid-template-columns: 1fr 1fr 1fr;
  gap: 6px;
}

.stat-card {
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(0, 180, 255, 0.12);
  border-radius: 5px;
  padding: 8px 6px;
  text-align: center;
}

.stat-label {
  font-size: 10px;
  color: #667788;
  margin-bottom: 4px;
}

.stat-value {
  font-size: 20px;
  font-weight: 800;
  font-family: 'Consolas', 'Courier New', monospace;
}

.stat-value .unit {
  font-size: 10px;
  font-weight: 400;
  opacity: 0.6;
  margin-left: 1px;
}

.history-table-wrap {
  max-height: calc(100% - 30px);
  overflow-y: auto;
}

.history-table-wrap::-webkit-scrollbar {
  width: 4px;
}

.history-table-wrap::-webkit-scrollbar-thumb {
  background: rgba(0, 180, 255, 0.2);
  border-radius: 2px;
}

.history-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 11px;
}

.history-table th {
  text-align: left;
  padding: 5px 8px;
  color: #667788;
  font-weight: 500;
  font-size: 10px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  border-bottom: 1px solid rgba(0, 180, 255, 0.12);
  position: sticky;
  top: 0;
  background: rgba(8, 34, 72, 0.9);
}

.history-table td {
  padding: 5px 8px;
  color: #A0C8E8;
  font-family: 'Consolas', 'Courier New', monospace;
  border-bottom: 1px solid rgba(0, 180, 255, 0.06);
}

.history-table tbody tr:hover {
  background: rgba(0, 180, 255, 0.05);
}
</style>
