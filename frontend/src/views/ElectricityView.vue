<template>
  <div class="elec-dashboard">
    <div class="top-bar">
      <div class="top-left">
        <div class="logo-icon">🏠</div>
        <h1 class="main-title">果冻智慧家</h1>
        <span class="sub-title">—— 一个有智慧的家</span>
        <span class="nav-divider">|</span>
        <router-link to="/" class="nav-link">首页</router-link>
        <router-link to="/temperature" class="nav-link">温湿度</router-link>
        <router-link to="/electricity" class="nav-link active">用电</router-link>
        <router-link to="/car" class="nav-link">车辆</router-link>
        <router-link to="/console" class="nav-link">控制台</router-link>
      </div>
      <div class="top-right">
        <span class="datetime">{{ currentTime }}</span>
        <span class="user-badge">👤 {{ authStore.username }}</span>
        <button class="logout-btn" @click="handleLogout">退出</button>
      </div>
    </div>

    <div class="main-content">
      <div class="overview-row">
        <div class="data-card">
          <div class="data-card-label">总实时功率</div>
          <div class="data-card-value power">{{ overview.totalPower.toFixed(1) }}<span class="unit">W</span></div>
          <div class="data-card-sub">实时监测</div>
          <div ref="miniPowerChart" class="mini-chart"></div>
        </div>
        <div class="data-card">
          <div class="data-card-label">总实时电流</div>
          <div class="data-card-value current">{{ overview.totalCurrent.toFixed(2) }}<span class="unit">A</span></div>
          <div class="data-card-sub">实时监测</div>
          <div ref="miniCurrentChart" class="mini-chart"></div>
        </div>
        <div class="data-card">
          <div class="data-card-label">今日用电量</div>
          <div class="data-card-value energy">{{ overview.dailyUsage.toFixed(2) }}<span class="unit">kWh</span></div>
          <div class="data-card-sub">累计用电</div>
          <div ref="miniEnergyChart" class="mini-chart"></div>
        </div>
      </div>

      <div class="panel-box breaker-panel">
        <div class="panel-title">
          🔌 断路器状态
          <span class="breaker-count">{{ breakers.length }} 路</span>
        </div>
        <div class="breaker-list">
          <div
            v-for="(b, idx) in breakers"
            :key="idx"
            class="breaker-card"
            :class="{ 'breaker-on': b.switchStatus === 1, 'breaker-off': b.switchStatus === 0, 'breaker-offline': b.switchStatus === -1 }"
          >
            <div class="breaker-header">
              <div class="breaker-name">
                <span class="breaker-icon">{{ b.switchStatus === 1 ? '⚡' : b.switchStatus === -1 ? '❓' : '⭕' }}</span>
                <span class="breaker-label">{{ b.deviceName }}</span>
              </div>
              <div class="breaker-switch" :class="{ on: b.switchStatus === 1, off: b.switchStatus === 0, offline: b.switchStatus === -1 }">
                {{ b.switchStatus === 1 ? '合闸' : b.switchStatus === 0 ? '分闸' : '离线' }}
              </div>
            </div>

            <div class="breaker-metrics">
              <div class="metric">
                <span class="metric-label">电流</span>
                <span class="metric-value" style="color:#A855F7">{{ b.current.toFixed(2) }}<span class="metric-unit">A</span></span>
              </div>
              <div class="metric">
                <span class="metric-label">电压</span>
                <span class="metric-value" style="color:#00D4FF">{{ b.voltage.toFixed(1) }}<span class="metric-unit">V</span></span>
              </div>
              <div class="metric">
                <span class="metric-label">功率</span>
                <span class="metric-value" style="color:#FF5E99">{{ b.power.toFixed(1) }}<span class="metric-unit">W</span></span>
              </div>
              <div class="metric">
                <span class="metric-label">温度</span>
                <span class="metric-value" :style="{ color: b.temperature > 50 ? '#FF6B6B' : '#FF9500' }">{{ b.temperature.toFixed(1) }}<span class="metric-unit">°C</span></span>
              </div>
              <div class="metric">
                <span class="metric-label">功率因数</span>
                <span class="metric-value" style="color:#10b981">{{ b.powerFactor.toFixed(2) }}</span>
              </div>
              <div class="metric">
                <span class="metric-label">累计电能</span>
                <span class="metric-value" style="color:#E0F0FF">{{ b.totalEnergy.toFixed(2) }}<span class="metric-unit">kWh</span></span>
              </div>
              <div class="metric">
                <span class="metric-label">数据时间</span>
                <span class="metric-value" style="color:#8899AA;font-size:11px">{{ b.dataTime || '-' }}</span>
              </div>
            </div>

            <div class="breaker-addr-bar">
              <span class="breaker-addr">ID: {{ b.deviceId }}</span>
            </div>
          </div>
        </div>
      </div>

      <div class="bottom-row">
        <div class="panel-box chart-panel">
          <div class="panel-title">⚡ 功率曲线图 (W)</div>
          <div ref="powerChart" class="chart-area"></div>
        </div>
        <div class="panel-box chart-panel">
          <div class="panel-title">📊 用电量趋势 (kWh)</div>
          <div ref="energyChart" class="chart-area"></div>
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
import { getLatestElectricity, getBreakers } from '@/api/electricity'
import type { Breaker, ElectricityUsage } from '@/types'

const router = useRouter()
const authStore = useAuthStore()

const currentData = ref<ElectricityUsage>({
  power: 0,
  dailyUsage: 0,
  recordedAt: '',
  deviceId: ''
})

const currentTime = ref('')

let clockTimer: number | null = null
let refreshTimer: number | null = null

const powerChart = ref<HTMLElement | null>(null)
const energyChart = ref<HTMLElement | null>(null)
const miniPowerChart = ref<HTMLElement | null>(null)
const miniCurrentChart = ref<HTMLElement | null>(null)
const miniEnergyChart = ref<HTMLElement | null>(null)

let powerInst: echarts.ECharts | null = null
let energyInst: echarts.ECharts | null = null
let miniPowerInst: echarts.ECharts | null = null
let miniCurrentInst: echarts.ECharts | null = null
let miniEnergyInst: echarts.ECharts | null = null

const breakers = ref<Breaker[]>([])

const overview = computed(() => {
  return {
    totalPower: breakers.value.reduce((s, b) => s + b.power, 0),
    totalCurrent: breakers.value.reduce((s, b) => s + b.current, 0),
    dailyUsage: currentData.value.dailyUsage || breakers.value.reduce((s, b) => s + b.totalEnergy, 0),
  }
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
    data: [10, 12, 11, 14, 13, 15],
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

const powerChartOption = {
  grid: { top: 12, right: 16, bottom: 22, left: 44 },
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
    areaStyle: {
      color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
        { offset: 0, color: 'rgba(255,94,153,0.25)' },
        { offset: 1, color: 'rgba(255,94,153,0.02)' }
      ])
    },
  }]
}

const energyChartOption = {
  grid: { top: 12, right: 16, bottom: 22, left: 44 },
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
    splitLine: { lineStyle: { color: 'rgba(0,180,255,0.08)' } },
    axisLabel: { color: '#667788', fontSize: 8 },
  },
  series: [{
    name: '用电量',
    type: 'bar',
    data: [1.2, 0.8, 0.6, 1.5, 2.1, 3.2, 4.5, 5.8, 4.2, 3.1, 2.0, 1.5],
    barWidth: 10,
    itemStyle: {
      color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
        { offset: 0, color: '#A855F7' },
        { offset: 1, color: 'rgba(168,85,247,0.3)' }
      ]),
      borderRadius: [3, 3, 0, 0],
    },
  }]
}

const fetchCurrentData = async () => {
  try {
    const data = await getLatestElectricity()
    currentData.value = data
  } catch {
    // keep current data
  }
}

const fetchBreakers = async () => {
  try {
    const data = await getBreakers()
    breakers.value = data
  } catch {
    // keep current data
  }
}

const handleResize = () => {
  powerInst?.resize()
  energyInst?.resize()
  miniPowerInst?.resize()
  miniCurrentInst?.resize()
  miniEnergyInst?.resize()
}

const handleLogout = () => {
  authStore.logout()
  router.replace('/login')
}

onMounted(async () => {
  updateClock()
  clockTimer = window.setInterval(updateClock, 1000)

  await nextTick()

  if (powerChart.value) { powerInst = echarts.init(powerChart.value); powerInst.setOption(powerChartOption) }
  if (energyChart.value) { energyInst = echarts.init(energyChart.value); energyInst.setOption(energyChartOption) }
  if (miniPowerChart.value) { miniPowerInst = echarts.init(miniPowerChart.value); miniPowerInst.setOption(miniChartOption('#FF5E99')) }
  if (miniCurrentChart.value) { miniCurrentInst = echarts.init(miniCurrentChart.value); miniCurrentInst.setOption(miniChartOption('#A855F7')) }
  if (miniEnergyChart.value) { miniEnergyInst = echarts.init(miniEnergyChart.value); miniEnergyInst.setOption(miniChartOption('#F59E0B')) }

  fetchCurrentData()
  fetchBreakers()
  refreshTimer = window.setInterval(() => {
    fetchCurrentData()
    fetchBreakers()
  }, 30000)

  window.addEventListener('resize', handleResize)
})

onUnmounted(() => {
  if (clockTimer) clearInterval(clockTimer)
  if (refreshTimer) clearInterval(refreshTimer)
  window.removeEventListener('resize', handleResize)
  powerInst?.dispose()
  energyInst?.dispose()
  miniPowerInst?.dispose()
  miniCurrentInst?.dispose()
  miniEnergyInst?.dispose()
})
</script>

<style scoped>
.elec-dashboard {
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

.logo-icon { font-size: 28px; }

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

.nav-link:hover { color: #00D4FF; }

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

.user-badge { font-size: 12px; color: #8899AA; }

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

.main-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 10px;
  min-height: 0;
  overflow-y: auto;
}

/* Overview Cards */
.overview-row {
  display: grid;
  grid-template-columns: 1fr 1fr 1fr;
  gap: 10px;
  flex-shrink: 0;
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

.data-card-value.power  { color: #FF5E99; text-shadow: 0 0 12px rgba(255,94,153,0.5); }
.data-card-value.current { color: #A855F7; text-shadow: 0 0 12px rgba(168,85,247,0.5); }
.data-card-value.energy  { color: #F59E0B; text-shadow: 0 0 12px rgba(245,158,11,0.5); }

.data-card-value .unit { font-size: 14px; font-weight: 400; margin-left: 2px; opacity: 0.7; }

.data-card-sub {
  font-size: 10px;
  color: #556677;
}

.mini-chart {
  height: 36px;
  margin-top: 4px;
}

/* Panel */
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
  margin-bottom: 10px;
  letter-spacing: 1px;
  border-left: 3px solid #00D4FF;
  padding-left: 8px;
  display: flex;
  align-items: center;
  gap: 8px;
}

.breaker-count {
  font-size: 11px;
  font-weight: 400;
  color: #667788;
  margin-left: auto;
}

/* Breaker Panel */
.breaker-panel {
  flex: 1;
  min-height: 0;
  display: flex;
  flex-direction: column;
}

.breaker-list {
  flex: 1;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding-right: 4px;
}

.breaker-list::-webkit-scrollbar {
  width: 4px;
}

.breaker-list::-webkit-scrollbar-thumb {
  background: rgba(0, 180, 255, 0.15);
  border-radius: 2px;
}

.breaker-card {
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(0, 180, 255, 0.12);
  border-radius: 5px;
  padding: 8px 12px;
  transition: border-color 0.3s;
}

.breaker-card.breaker-on {
  border-left: 3px solid rgba(22, 163, 74, 0.5);
}

.breaker-card.breaker-off {
  border-left: 3px solid rgba(255, 80, 80, 0.5);
  opacity: 0.6;
}

.breaker-card.breaker-offline {
  border-left: 3px solid rgba(150, 150, 150, 0.5);
  opacity: 0.55;
}

.breaker-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8px;
}

.breaker-name {
  display: flex;
  align-items: center;
  gap: 6px;
}

.breaker-icon {
  font-size: 14px;
}

.breaker-label {
  font-size: 13px;
  font-weight: 600;
  color: #E0F0FF;
}

.breaker-switch {
  font-size: 10px;
  font-weight: 700;
  padding: 2px 10px;
  border-radius: 10px;
  letter-spacing: 1px;
}

.breaker-switch.on {
  background: rgba(22, 163, 74, 0.15);
  color: #4ADE80;
  border: 1px solid rgba(22, 163, 74, 0.3);
}

.breaker-switch.off {
  background: rgba(255, 80, 80, 0.1);
  color: #FF6B6B;
  border: 1px solid rgba(255, 80, 80, 0.2);
}

.breaker-switch.offline {
  background: rgba(150, 150, 150, 0.1);
  color: #8899AA;
  border: 1px solid rgba(150, 150, 150, 0.25);
}

.breaker-metrics {
  display: grid;
  grid-template-columns: repeat(7, 1fr);
  gap: 6px;
  margin-bottom: 6px;
}

.metric {
  background: rgba(255, 255, 255, 0.015);
  border-radius: 3px;
  padding: 4px 6px;
  text-align: center;
}

.metric-label {
  display: block;
  font-size: 9px;
  color: #556677;
  margin-bottom: 2px;
}

.metric-value {
  font-size: 14px;
  font-weight: 700;
  font-family: 'Consolas', 'Courier New', monospace;
}

.metric-unit {
  font-size: 9px;
  font-weight: 400;
  opacity: 0.6;
  margin-left: 1px;
}

.breaker-addr-bar {
  text-align: right;
}

.breaker-addr {
  font-size: 10px;
  color: #445566;
  font-family: 'Consolas', 'Courier New', monospace;
}

/* Bottom Charts */
.bottom-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
  flex-shrink: 0;
}

.chart-panel {
  min-height: 0;
}

.chart-area {
  width: 100%;
  height: 160px;
}
</style>
