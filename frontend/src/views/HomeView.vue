<template>
  <div class="dashboard">
    <div class="top-bar">
      <div class="top-left">
        <div class="logo-icon">🏠</div>
        <h1 class="main-title">果冻智慧家</h1>
        <span class="sub-title">—— 一个有智慧的家</span>
        <span class="nav-divider">|</span>
        <router-link to="/" class="nav-link">首页</router-link>
        <router-link to="/temperature" class="nav-link">温湿度</router-link>
        <router-link to="/electricity" class="nav-link">用电</router-link>
        <router-link to="/car" class="nav-link">车辆</router-link>
        <router-link to=" " class="nav-link">家庭网盘</router-link>
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
            <div class="data-card-label">平均温度</div>
            <div class="data-card-value temp">{{ dash.temperature.toFixed(1) }}<span class="unit">°C</span></div>
            <div class="data-card-sub">实时监测</div>
            <div ref="miniTempChart" class="mini-chart"></div>
          </div>
          <div class="data-card">
            <div class="data-card-label">平均湿度</div>
            <div class="data-card-value humi">{{ dash.humidity.toFixed(1) }}<span class="unit">%</span></div>
            <div class="data-card-sub">实时监测</div>
            <div ref="miniHumiChart" class="mini-chart"></div>
          </div>
        </div>

        <div class="panel-box room-panels-box">
          <div class="panel-title">🌡️ 各房间温湿度</div>
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
        <div class="panel-box map-box">
          <div class="panel-title">🗺️ 车辆实时定位</div>
          <div ref="mapContainer" class="leaflet-map-container"></div>
          <div class="map-vehicle-bar">
            <span class="map-car-name">🚗 {{ carMarker.name }}</span>
            <span class="map-coords">{{ carMarker.lat.toFixed(4) }}, {{ carMarker.lon.toFixed(4) }}</span>
            <span :class="['map-status-dot', autoRefresh ? 'active' : '']"></span>
            <span class="map-status-text">{{ autoRefresh ? '巡航中' : '驻车中' }}</span>
          </div>
        </div>
      </div>

      <div class="right-panel">
        <div class="data-cards-row">
          <div class="data-card">
            <div class="data-card-label">家庭实时电流</div>
            <div class="data-card-value current">{{ dash.current.toFixed(2) }}<span class="unit">A</span></div>
            <div class="data-card-sub">实时监测</div>
            <div ref="miniCurrentChart" class="mini-chart"></div>
          </div>
          <div class="data-card">
            <div class="data-card-label">家庭实时功率</div>
            <div class="data-card-value power">{{ dash.power.toFixed(1) }}<span class="unit">W</span></div>
            <div class="data-card-sub">实时监测</div>
            <div ref="miniPowerChart" class="mini-chart"></div>
          </div>
        </div>

        <div class="panel-box line-chart-box">
          <div class="panel-title">⚡ 电力功率曲线图 (W)</div>
          <div ref="powerChart" class="chart-line"></div>
        </div>

        <div class="panel-box line-chart-box">
          <div class="panel-title">🔌 电流曲线图 (A)</div>
          <div ref="currentChart" class="chart-line"></div>
        </div>

        <div class="panel-box" style="flex:1">
          <div class="panel-title">📈 24h 数据趋势</div>
          <div ref="trendChart" class="chart-box"></div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, nextTick } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import * as echarts from 'echarts'
import L from 'leaflet'
import 'leaflet/dist/leaflet.css'
import { getDashboardSummary } from '@/api/dashboard'
import type { DashboardSummary } from '@/types'

const router = useRouter()
const authStore = useAuthStore()

const dash = ref<DashboardSummary & { current: number; voltage: number }>({
  temperature: 0, humidity: 0, power: 0, dailyUsage: 0, smoke: 0, current: 0, voltage: 0
})
const currentTime = ref('')

let clockTimer: number | null = null
let refreshTimer: number | null = null

const mapContainer = ref<HTMLElement | null>(null)
const trendChart = ref<HTMLElement | null>(null)
const powerChart = ref<HTMLElement | null>(null)
const currentChart = ref<HTMLElement | null>(null)
const miniTempChart = ref<HTMLElement | null>(null)
const miniHumiChart = ref<HTMLElement | null>(null)
const miniCurrentChart = ref<HTMLElement | null>(null)
const miniPowerChart = ref<HTMLElement | null>(null)

let mapInstance: L.Map | null = null
let carLeafletMarker: L.Marker | null = null
let leafletRefreshTimer: ReturnType<typeof setInterval> | null = null
let leafletCountdownTimer: ReturnType<typeof setInterval> | null = null
let trendInst: echarts.ECharts | null = null
let powerInst: echarts.ECharts | null = null
let currentInst: echarts.ECharts | null = null
let miniTempInst: echarts.ECharts | null = null
let miniHumiInst: echarts.ECharts | null = null
let miniCurrentInst: echarts.ECharts | null = null
let miniPowerInst: echarts.ECharts | null = null

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

const carMarker = ref<{ lat: number; lon: number; name: string }>({
  lat: 30.5728,
  lon: 104.0668,
  name: '川A·12345',
})

const autoRefresh = ref(true)

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
    areaStyle: { color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
      { offset: 0, color },
      { offset: 1, color: 'transparent' }
    ]) }
  }]
})

const powerOption = {
  grid: { top: 12, right: 16, bottom: 22, left: 40 },
  xAxis: {
    type: 'category',
    data: ['02:00', '04:00', '06:00', '08:00', '10:00', '12:00', '14:00', '16:00', '18:00', '20:00', '22:00', '24:00'],
    axisLine: { lineStyle: { color: 'rgba(0,180,255,0.2)' } },
    axisLabel: { color: '#667788', fontSize: 8 },
    axisTick: { show: false },
  },
  yAxis: {
    type: 'value',
    min: 0,
    max: 5000,
    splitLine: { lineStyle: { color: 'rgba(0,180,255,0.08)' } },
    axisLabel: { color: '#667788', fontSize: 8 },
  },
  series: [{
    name: '功率',
    type: 'line',
    data: [850, 720, 680, 1230, 1560, 2050, 2800, 3200, 2600, 1900, 1420, 980],
    smooth: true,
    symbol: 'circle',
    symbolSize: 4,
    lineStyle: { color: '#FF5E99', width: 2 },
    itemStyle: { color: '#FF5E99' },
    areaStyle: { color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
      { offset: 0, color: 'rgba(255,94,153,0.25)' },
      { offset: 1, color: 'rgba(255,94,153,0.02)' }
    ]) },
  }]
}

const currentOption = {
  grid: { top: 12, right: 16, bottom: 22, left: 40 },
  xAxis: {
    type: 'category',
    data: ['02:00', '04:00', '06:00', '08:00', '10:00', '12:00', '14:00', '16:00', '18:00', '20:00', '22:00', '24:00'],
    axisLine: { lineStyle: { color: 'rgba(0,180,255,0.2)' } },
    axisLabel: { color: '#667788', fontSize: 8 },
    axisTick: { show: false },
  },
  yAxis: {
    type: 'value',
    min: 0,
    max: 50,
    splitLine: { lineStyle: { color: 'rgba(0,180,255,0.08)' } },
    axisLabel: { color: '#667788', fontSize: 8 },
  },
  series: [{
    name: '电流',
    type: 'line',
    data: [4.2, 3.5, 3.2, 6.8, 8.5, 12.3, 16.8, 18.5, 14.2, 10.5, 7.8, 5.2],
    smooth: true,
    symbol: 'circle',
    symbolSize: 4,
    lineStyle: { color: '#A855F7', width: 2 },
    itemStyle: { color: '#A855F7' },
    areaStyle: { color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
      { offset: 0, color: 'rgba(168,85,247,0.25)' },
      { offset: 1, color: 'rgba(168,85,247,0.02)' }
    ]) },
  }]
}

const vehicleIcon = (): L.DivIcon => {
  return L.divIcon({
    className: 'vehicle-marker',
    html: `<div style="
      font-size:28px;
      background:white;
      border-radius:50%;
      padding:6px;
      box-shadow:0 2px 12px rgba(0,0,0,0.3);
      width:44px;
      height:44px;
      display:flex;
      align-items:center;
      justify-content:center;
      border:3px solid #f97316;
    ">🚗</div>`,
    iconSize: [44, 44],
    iconAnchor: [22, 22],
  })
}

const trendOption = {
  grid: { top: 16, right: 20, bottom: 28, left: 42 },
  legend: {
    data: ['温度', '湿度', '烟雾'],
    right: 0,
    top: 0,
    textStyle: { color: '#8899AA', fontSize: 10 },
    itemWidth: 12,
    itemHeight: 2,
  },
  xAxis: {
    type: 'category',
    data: ['02:00', '04:00', '06:00', '08:00', '10:00', '12:00', '14:00', '16:00', '18:00', '20:00', '22:00', '24:00'],
    axisLine: { lineStyle: { color: 'rgba(0,180,255,0.2)' } },
    axisLabel: { color: '#667788', fontSize: 9 },
    axisTick: { show: false },
  },
  yAxis: {
    type: 'value',
    splitLine: { lineStyle: { color: 'rgba(0,180,255,0.08)' } },
    axisLabel: { color: '#667788', fontSize: 9 },
  },
  series: [
    { name: '温度', type: 'line', data: [24, 23, 22, 25, 28, 30, 32, 31, 29, 27, 26, 25], smooth: true, symbol: 'circle', symbolSize: 4, lineStyle: { color: '#FF9500', width: 2 }, itemStyle: { color: '#FF9500' } },
    { name: '湿度', type: 'line', data: [68, 70, 72, 75, 72, 68, 65, 63, 65, 68, 70, 69], smooth: true, symbol: 'circle', symbolSize: 4, lineStyle: { color: '#00D4FF', width: 2 }, itemStyle: { color: '#00D4FF' } },
    { name: '烟雾', type: 'line', data: [15, 12, 10, 18, 22, 28, 35, 30, 25, 20, 18, 15], smooth: true, symbol: 'circle', symbolSize: 4, lineStyle: { color: '#4A90D9', width: 2 }, itemStyle: { color: '#4A90D9' } },
  ]
}

const initLeafletMap = async () => {
  if (!mapContainer.value || mapInstance) return

  delete (L.Icon.Default.prototype as any)._getIconUrl
  L.Icon.Default.mergeOptions({
    iconRetinaUrl: 'https://unpkg.com/leaflet@1.9.4/dist/images/marker-icon-2x.png',
    iconUrl: 'https://unpkg.com/leaflet@1.9.4/dist/images/marker-icon.png',
    shadowUrl: 'https://unpkg.com/leaflet@1.9.4/dist/images/marker-shadow.png',
  })

  mapInstance = L.map(mapContainer.value, {
    center: [carMarker.value.lat, carMarker.value.lon],
    zoom: 15,
    zoomControl: true,
  })

  L.tileLayer('https://webrd0{s}.is.autonavi.com/appmaptile?lang=zh_cn&size=1&scale=1&style=8&x={x}&y={y}&z={z}', {
    subdomains: ['1', '2', '3', '4'],
    attribution: '&copy; 高德地图',
    maxZoom: 18,
  }).addTo(mapInstance)

  setTimeout(() => { mapInstance?.invalidateSize() }, 200)

  carLeafletMarker = L.marker([carMarker.value.lat, carMarker.value.lon], { icon: vehicleIcon() }).addTo(mapInstance)
  carLeafletMarker.bindPopup(`
    <div style="min-width:180px;font-family:sans-serif">
      <p style="font-size:15px;font-weight:bold;margin:0 0 6px;color:#333">🚗 ${carMarker.value.name}</p>
      <p style="font-size:13px;color:#666;margin:0">经纬度: ${carMarker.value.lat.toFixed(4)}, ${carMarker.value.lon.toFixed(4)}</p>
      <p style="font-size:13px;color:#666;margin:4px 0 0">状态: <span style="color:#16a34a;font-weight:bold">在线</span></p>
      <p style="font-size:13px;color:#666;margin:4px 0 0">🔁 实时巡航中</p>
    </div>
  `)

  startLeafletRefresh()
}

const updateCarMarker = () => {
  if (!mapInstance || !carLeafletMarker) return
  carLeafletMarker.setLatLng([carMarker.value.lat, carMarker.value.lon])
  carLeafletMarker.setPopupContent(`
    <div style="min-width:180px;font-family:sans-serif">
      <p style="font-size:15px;font-weight:bold;margin:0 0 6px;color:#333">🚗 ${carMarker.value.name}</p>
      <p style="font-size:13px;color:#666;margin:0">经纬度: ${carMarker.value.lat.toFixed(4)}, ${carMarker.value.lon.toFixed(4)}</p>
      <p style="font-size:13px;color:#666;margin:4px 0 0">状态: <span style="color:#16a34a;font-weight:bold">在线</span></p>
      <p style="font-size:13px;color:#666;margin:4px 0 0">${autoRefresh.value ? '🔁 实时巡航中' : '📍 驻车中'}</p>
    </div>
  `)
}

const simulatePositionChange = () => {
  const dLat = (Math.random() - 0.5) * 0.003
  const dLon = (Math.random() - 0.5) * 0.003
  carMarker.value.lat = Math.min(30.583, Math.max(30.562, carMarker.value.lat + dLat))
  carMarker.value.lon = Math.min(104.077, Math.max(104.056, carMarker.value.lon + dLon))
  updateCarMarker()
}

const startLeafletRefresh = () => {
  stopLeafletRefresh()
  leafletRefreshTimer = setInterval(() => {
    if (mapInstance && autoRefresh.value) {
      simulatePositionChange()
    }
  }, 15000)
}

const stopLeafletRefresh = () => {
  if (leafletRefreshTimer) { clearInterval(leafletRefreshTimer); leafletRefreshTimer = null }
  if (leafletCountdownTimer) { clearInterval(leafletCountdownTimer); leafletCountdownTimer = null }
}

const destroyLeafletMap = () => {
  stopLeafletRefresh()
  carLeafletMarker = null
  if (mapInstance) {
    mapInstance.remove()
    mapInstance = null
  }
}

const initAllCharts = () => {
  if (trendChart.value) { trendInst = echarts.init(trendChart.value); trendInst.setOption(trendOption) }
  if (powerChart.value) { powerInst = echarts.init(powerChart.value); powerInst.setOption(powerOption) }
  if (currentChart.value) { currentInst = echarts.init(currentChart.value); currentInst.setOption(currentOption) }
  if (miniTempChart.value) { miniTempInst = echarts.init(miniTempChart.value); miniTempInst.setOption(miniChartOption('#00D4FF')) }
  if (miniHumiChart.value) { miniHumiInst = echarts.init(miniHumiChart.value); miniHumiInst.setOption(miniChartOption('#4A90D9')) }
  if (miniCurrentChart.value) { miniCurrentInst = echarts.init(miniCurrentChart.value); miniCurrentInst.setOption(miniChartOption('#A855F7')) }
  if (miniPowerChart.value) { miniPowerInst = echarts.init(miniPowerChart.value); miniPowerInst.setOption(miniChartOption('#FF5E99')) }
}

const handleResize = () => {
  trendInst?.resize()
  powerInst?.resize()
  currentInst?.resize()
  miniTempInst?.resize()
  miniHumiInst?.resize()
  miniCurrentInst?.resize()
  miniPowerInst?.resize()
  mapInstance?.invalidateSize()
}

const fetchData = async () => {
  try {
    const data = await getDashboardSummary()
    dash.value = data
  } catch {
    // use simulated defaults
  }
}

const handleLogout = () => {
  authStore.logout()
  router.replace('/login')
}

onMounted(async () => {
  updateClock()
  clockTimer = window.setInterval(updateClock, 1000)

  await nextTick()
  initAllCharts()
  initLeafletMap()
  fetchData()
  refreshTimer = window.setInterval(fetchData, 10000)

  window.addEventListener('resize', handleResize)
})

onUnmounted(() => {
  if (clockTimer) clearInterval(clockTimer)
  if (refreshTimer) clearInterval(refreshTimer)
  window.removeEventListener('resize', handleResize)
  destroyLeafletMap()
  trendInst?.dispose()
  powerInst?.dispose()
  currentInst?.dispose()
  miniTempInst?.dispose()
  miniHumiInst?.dispose()
  miniCurrentInst?.dispose()
  miniPowerInst?.dispose()
})
</script>

<style scoped>
.dashboard {
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
  grid-template-columns: 1.1fr 2.3fr 1.1fr;
  gap: 10px;
  min-height: 0;
}

.left-panel {
  display: flex;
  flex-direction: column;
  gap: 10px;
  min-height: 0;
}

.right-panel {
  display: flex;
  flex-direction: column;
  gap: 10px;
  min-height: 0;
}

.center-panel {
  display: flex;
  flex-direction: column;
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

.map-box {
  flex: 1;
  display: flex;
  flex-direction: column;
}

.leaflet-map-container {
  flex: 1;
  min-height: 0;
  border-radius: 4px;
  overflow: hidden;
}

.map-vehicle-bar {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 8px 10px;
  margin-top: 8px;
  background: rgba(0, 180, 255, 0.08);
  border-radius: 4px;
  border: 1px solid rgba(0, 180, 255, 0.12);
  flex-shrink: 0;
}

.map-car-name {
  font-size: 13px;
  font-weight: 600;
  color: #E0F0FF;
}

.map-coords {
  font-size: 11px;
  color: #667788;
  font-family: 'Consolas', 'Courier New', monospace;
  flex: 1;
}

.map-status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: #16a34a;
  flex-shrink: 0;
}

.map-status-dot.active {
  animation: pulse-dot 1.5s ease-in-out infinite;
}

@keyframes pulse-dot {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.3; }
}

.map-status-text {
  font-size: 11px;
  color: #8899AA;
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

.chart-box {
  flex: 1;
  min-height: 0;
  height: 160px;
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
.data-card-value.current { color: #A855F7; text-shadow: 0 0 12px rgba(168,85,247,0.5); }
.data-card-value.power { color: #FF5E99; text-shadow: 0 0 12px rgba(255,94,153,0.5); }
.data-card-value .unit { font-size: 14px; font-weight: 400; margin-left: 2px; opacity: 0.7; }

.data-card-sub {
  font-size: 10px;
  color: #556677;
}

.mini-chart {
  height: 36px;
  margin-top: 4px;
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

.line-chart-box {
  min-height: 0;
}

.chart-line {
  width: 100%;
  height: 140px;
}

:deep(.vehicle-marker) {
  background: none !important;
  border: none !important;
}
</style>
