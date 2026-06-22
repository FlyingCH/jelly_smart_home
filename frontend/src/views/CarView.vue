<template>
  <div class="flex flex-col h-[calc(100vh-6rem)]">
    <h2 class="text-3xl font-bold text-white mb-4">🚗 果冻的车辆</h2>

    <div class="flex flex-1 bg-white rounded-lg shadow-lg overflow-hidden">
      <div class="w-48 bg-gradient-to-b from-orange-50 to-yellow-50 border-r border-orange-200 flex-shrink-0">
        <div class="py-4 px-3 space-y-1">
          <button
            v-for="item in menuItems"
            :key="item.key"
            @click="activeMenu = item.key"
            :class="[
              'w-full text-left px-4 py-3 rounded-lg text-sm font-medium transition-all duration-200 flex items-center space-x-2',
              activeMenu === item.key
                ? 'bg-orange-500 text-white shadow-md'
                : 'text-gray-700 hover:bg-orange-100'
            ]"
          >
            <span class="text-lg">{{ item.icon }}</span>
            <span>{{ item.label }}</span>
          </button>
        </div>
      </div>

      <div class="flex-1 bg-gray-50 p-6 overflow-auto">
        <div v-if="activeMenu === 'info'" class="animate-fadeIn">
          <h3 class="text-2xl font-bold text-gray-800 mb-6">📋 车辆信息</h3>
          <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
            <div class="space-y-4">
              <div class="bg-white rounded-xl p-5 shadow-sm border border-gray-100">
                <label class="block text-sm text-gray-500 mb-1">品牌</label>
                <input v-model="carInfo.brand" class="w-full text-lg font-semibold bg-transparent border-b border-gray-200 focus:border-orange-500 outline-none py-1" placeholder="请输入品牌" />
              </div>
              <div class="bg-white rounded-xl p-5 shadow-sm border border-gray-100">
                <label class="block text-sm text-gray-500 mb-1">型号</label>
                <input v-model="carInfo.model" class="w-full text-lg font-semibold bg-transparent border-b border-gray-200 focus:border-orange-500 outline-none py-1" placeholder="请输入型号" />
              </div>
              <div class="bg-white rounded-xl p-5 shadow-sm border border-gray-100">
                <label class="block text-sm text-gray-500 mb-1">车牌号</label>
                <input v-model="carInfo.plate" class="w-full text-lg font-semibold bg-transparent border-b border-gray-200 focus:border-orange-500 outline-none py-1" placeholder="川A·XXXXX" />
              </div>
            </div>
            <div class="space-y-4">
              <div class="bg-white rounded-xl p-5 shadow-sm border border-gray-100">
                <label class="block text-sm text-gray-500 mb-1">颜色</label>
                <input v-model="carInfo.color" class="w-full text-lg font-semibold bg-transparent border-b border-gray-200 focus:border-orange-500 outline-none py-1" placeholder="请输入颜色" />
              </div>
              <div class="bg-white rounded-xl p-5 shadow-sm border border-gray-100">
                <label class="block text-sm text-gray-500 mb-1">车架号</label>
                <input v-model="carInfo.vin" class="w-full text-lg font-semibold bg-transparent border-b border-gray-200 focus:border-orange-500 outline-none py-1" placeholder="VIN 码" />
              </div>
              <div class="bg-white rounded-xl p-5 shadow-sm border border-gray-100">
                <label class="block text-sm text-gray-500 mb-1">购买日期</label>
                <input v-model="carInfo.purchaseDate" type="date" class="w-full text-lg font-semibold bg-transparent border-b border-gray-200 focus:border-orange-500 outline-none py-1" />
              </div>
            </div>
          </div>
          <button @click="saveCarInfo" class="mt-6 bg-orange-500 text-white px-8 py-3 rounded-lg hover:bg-orange-600 transition font-medium">
            💾 保存车辆信息
          </button>
        </div>

        <div v-else-if="activeMenu === 'status'" class="animate-fadeIn">
          <h3 class="text-2xl font-bold text-gray-800 mb-6">📊 车辆状态</h3>
          <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4 mb-6">
            <div class="bg-gradient-to-br from-blue-500 to-blue-600 text-white rounded-xl p-5 shadow-lg">
              <p class="text-sm opacity-80">电量/油量</p>
              <p class="text-3xl font-bold mt-2">{{ carStatus.fuel }}%</p>
              <div class="mt-3 bg-white/30 rounded-full h-2">
                <div class="bg-white rounded-full h-2 transition-all duration-500" :style="{ width: carStatus.fuel + '%' }"></div>
              </div>
            </div>
            <div class="bg-gradient-to-br from-green-500 to-green-600 text-white rounded-xl p-5 shadow-lg">
              <p class="text-sm opacity-80">续航里程</p>
              <p class="text-3xl font-bold mt-2">{{ carStatus.range }}<span class="text-lg">km</span></p>
            </div>
            <div class="bg-gradient-to-br from-purple-500 to-purple-600 text-white rounded-xl p-5 shadow-lg">
              <p class="text-sm opacity-80">总里程</p>
              <p class="text-3xl font-bold mt-2">{{ carStatus.totalMileage }}<span class="text-lg">km</span></p>
            </div>
            <div class="bg-gradient-to-br from-teal-500 to-teal-600 text-white rounded-xl p-5 shadow-lg">
              <p class="text-sm opacity-80">胎压</p>
              <p class="text-3xl font-bold mt-2">{{ carStatus.tirePressure }}<span class="text-lg">bar</span></p>
            </div>
          </div>
          <div class="bg-white rounded-xl p-5 shadow-sm border border-gray-100">
            <h4 class="font-semibold text-gray-700 mb-4">车辆健康报告</h4>
            <div class="space-y-3">
              <div class="flex items-center justify-between">
                <span class="text-gray-600">发动机状态</span>
                <span class="px-3 py-1 bg-green-100 text-green-700 rounded-full text-sm font-medium">正常</span>
              </div>
              <div class="flex items-center justify-between">
                <span class="text-gray-600">刹车系统</span>
                <span class="px-3 py-1 bg-green-100 text-green-700 rounded-full text-sm font-medium">正常</span>
              </div>
              <div class="flex items-center justify-between">
                <span class="text-gray-600">电池状态</span>
                <span class="px-3 py-1 bg-yellow-100 text-yellow-700 rounded-full text-sm font-medium">需关注</span>
              </div>
            </div>
          </div>
        </div>

        <div v-else-if="activeMenu === 'location'" class="animate-fadeIn h-full flex flex-col">
          <h3 class="text-2xl font-bold text-gray-800 mb-3">📍 车辆位置</h3>

          <div class="bg-white rounded-lg border border-gray-200 p-3 mb-3 flex items-center gap-2 flex-shrink-0">
            <div class="flex-1 relative">
              <input
                v-model="searchQuery"
                @keyup.enter="doSearch"
                type="text"
                placeholder="🔍 搜索地点，如：天府广场、春熙路..."
                class="w-full border border-gray-300 rounded-lg pl-10 pr-4 py-2.5 text-sm focus:ring-2 focus:ring-orange-500 focus:border-orange-500 outline-none"
              />
              <span class="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400">🔍</span>
            </div>
            <button
              @click="doSearch"
              :disabled="searching"
              class="bg-orange-500 text-white px-5 py-2.5 rounded-lg hover:bg-orange-600 transition text-sm font-medium disabled:opacity-50 flex-shrink-0"
            >
              {{ searching ? '搜索中...' : '搜索' }}
            </button>
            <button
              v-if="routeDestination"
              @click="clearRoute"
              class="bg-gray-200 text-gray-600 px-4 py-2.5 rounded-lg hover:bg-gray-300 transition text-sm flex-shrink-0"
            >
              ✕ 清除路线
            </button>
            <button
              v-if="routeDestination"
              @click="showRouteDetail = !showRouteDetail"
              class="bg-blue-500 text-white px-4 py-2.5 rounded-lg hover:bg-blue-600 transition text-sm flex-shrink-0"
            >
              {{ showRouteDetail ? '隐藏' : '显示' }}距离
            </button>
          </div>

          <div v-if="searchResults.length > 0" class="bg-white rounded-lg border border-gray-200 shadow-sm mb-3 flex-shrink-0 max-h-40 overflow-auto">
            <div
              v-for="(result, idx) in searchResults"
              :key="idx"
              @click="selectSearchResult(result)"
              class="px-4 py-2.5 hover:bg-orange-50 cursor-pointer border-b border-gray-100 last:border-0 flex items-center gap-2"
            >
              <span class="text-orange-500">📍</span>
              <span class="text-sm text-gray-700">{{ result.display_name }}</span>
            </div>
          </div>

          <div
            v-if="routeDestination && showRouteDetail"
            class="bg-white rounded-lg border border-blue-200 shadow-sm mb-3 p-3 flex-shrink-0"
          >
            <div class="flex items-center justify-between">
              <span class="text-sm text-gray-600">
                📍 从 <strong>{{ carMarker.name }}</strong> 到 <strong>{{ routeName }}</strong>
              </span>
              <span class="text-sm font-bold text-blue-600">{{ routeDistanceText }}</span>
            </div>
          </div>

          <div ref="mapContainer" class="w-full rounded-xl overflow-hidden border border-gray-200 shadow-sm flex-1" style="min-height: 400px;"></div>

          <div class="mt-3 flex items-center justify-between text-sm text-gray-500 flex-shrink-0">
            <div class="flex items-center gap-4">
              <span>📍 点击标记查看详情 | 🖱️ 滚轮缩放</span>
              <span v-if="refreshCountdown > 0" class="text-blue-500">
                下次刷新: {{ refreshCountdown }}s
              </span>
            </div>
            <div class="flex items-center gap-4">
              <label class="flex items-center gap-1.5 cursor-pointer">
                <input v-model="autoRefresh" type="checkbox" class="accent-orange-500 w-4 h-4" />
                <span>实时刷新</span>
              </label>
              <span class="flex items-center space-x-2">
                <span :class="['w-2 h-2 rounded-full', autoRefresh ? 'animate-pulse bg-green-500' : 'bg-green-500']"></span>
                车辆{{ autoRefresh ? '巡航中...' : '在线' }}
              </span>
            </div>
          </div>
        </div>

        <div v-else-if="activeMenu === 'add'" class="animate-fadeIn">
          <h3 class="text-2xl font-bold text-gray-800 mb-6">➕ 添加车辆</h3>
          <div class="max-w-lg">
            <div class="space-y-4">
              <div>
                <label class="block text-sm font-medium text-gray-700 mb-1">车辆品牌</label>
                <select v-model="newCar.brand" class="w-full border border-gray-300 rounded-lg px-4 py-3 focus:ring-2 focus:ring-orange-500 focus:border-orange-500">
                  <option value="">请选择品牌</option>
                  <option value="tesla">特斯拉</option>
                  <option value="byd">比亚迪</option>
                  <option value="nio">蔚来</option>
                  <option value="xpeng">小鹏</option>
                  <option value="li">理想</option>
                  <option value="other">其他</option>
                </select>
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 mb-1">车辆型号</label>
                <input v-model="newCar.model" type="text" placeholder="如 Model 3" class="w-full border border-gray-300 rounded-lg px-4 py-3 focus:ring-2 focus:ring-orange-500 focus:border-orange-500" />
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 mb-1">车牌号</label>
                <input v-model="newCar.plate" type="text" placeholder="川A·00000" class="w-full border border-gray-300 rounded-lg px-4 py-3 focus:ring-2 focus:ring-orange-500 focus:border-orange-500" />
              </div>
              <button @click="addCar" class="w-full bg-orange-500 text-white py-3 px-6 rounded-lg hover:bg-orange-600 transition font-medium text-lg">
                🚗 确认添加
              </button>
            </div>
          </div>
        </div>

        <div v-else-if="activeMenu === 'console'" class="animate-fadeIn">
          <h3 class="text-2xl font-bold text-gray-800 mb-6">⚙️ 车辆控制台</h3>
          <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div class="bg-white rounded-xl p-5 shadow-sm border border-gray-100 hover:shadow-md transition cursor-pointer" @click="remoteAction('lock')">
              <div class="text-4xl mb-3">🔒</div>
              <h4 class="font-semibold text-gray-800">远程锁车</h4>
              <p class="text-sm text-gray-500 mt-1">锁定车辆门锁</p>
            </div>
            <div class="bg-white rounded-xl p-5 shadow-sm border border-gray-100 hover:shadow-md transition cursor-pointer" @click="remoteAction('unlock')">
              <div class="text-4xl mb-3">🔓</div>
              <h4 class="font-semibold text-gray-800">远程解锁</h4>
              <p class="text-sm text-gray-500 mt-1">解锁车辆门锁</p>
            </div>
            <div class="bg-white rounded-xl p-5 shadow-sm border border-gray-100 hover:shadow-md transition cursor-pointer" @click="remoteAction('ac')">
              <div class="text-4xl mb-3">❄️</div>
              <h4 class="font-semibold text-gray-800">远程空调</h4>
              <p class="text-sm text-gray-500 mt-1">提前开启空调</p>
            </div>
            <div class="bg-white rounded-xl p-5 shadow-sm border border-gray-100 hover:shadow-md transition cursor-pointer" @click="remoteAction('flash')">
              <div class="text-4xl mb-3">💡</div>
              <h4 class="font-semibold text-gray-800">寻车功能</h4>
              <p class="text-sm text-gray-500 mt-1">闪灯鸣笛找车</p>
            </div>
          </div>
        </div>
      </div>
    </div>

    <div class="bg-gradient-to-r from-orange-400 to-orange-500 text-white text-center py-2 text-sm rounded-b-lg mt-2">
      果冻智能家 © jelly版权所有 | 车辆管理系统 v1.0
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, nextTick, watch } from 'vue'
import L from 'leaflet'
import 'leaflet/dist/leaflet.css'

interface MenuItem {
  key: string
  label: string
  icon: string
}

interface SearchResult {
  lat: string
  lon: string
  display_name: string
}

const menuItems: MenuItem[] = [
  { key: 'info', label: '车辆信息', icon: '📋' },
  { key: 'status', label: '车辆状态', icon: '📊' },
  { key: 'location', label: '车辆位置', icon: '📍' },
  { key: 'add', label: '车辆添加', icon: '➕' },
  { key: 'console', label: '控制台', icon: '⚙️' },
]

const activeMenu = ref('location')

const carInfo = ref({
  brand: '特斯拉',
  model: 'Model 3',
  plate: '川A·12345',
  color: '深海蓝',
  vin: 'LRW3E7FS0NC000000',
  purchaseDate: '2024-06-15',
})

const carStatus = ref({
  fuel: 72,
  range: 358,
  totalMileage: 12580,
  tirePressure: 2.4,
})

const newCar = ref({
  brand: '',
  model: '',
  plate: '',
})

const mapContainer = ref<HTMLElement | null>(null)
let mapInstance: L.Map | null = null
let carLeafletMarker: L.Marker | null = null
let routePolyline: L.Polyline | null = null
let destinationMarkerObj: L.Marker | null = null
let refreshTimer: ReturnType<typeof setInterval> | null = null
let countdownTimer: ReturnType<typeof setInterval> | null = null

const carMarker = ref<{ lat: number; lon: number; name: string }>({
  lat: 30.5728,
  lon: 104.0668,
  name: '川A·12345',
})

const searchQuery = ref('')
const searching = ref(false)
const searchResults = ref<SearchResult[]>([])
const routeDestination = ref<{ lat: number; lon: number } | null>(null)
const routeName = ref('')
const routeDistanceText = ref('')
const showRouteDetail = ref(false)
const autoRefresh = ref(true)
const refreshCountdown = ref(15)

const vehicleIcon = (heading: number = 0): L.DivIcon => {
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

const destIcon = L.divIcon({
  className: 'dest-marker',
  html: `<div style="
    font-size:24px;
    background:#ef4444;
    border-radius:50% 50% 50% 0;
    padding:6px;
    box-shadow:0 2px 8px rgba(239,68,68,0.4);
    width:40px;
    height:40px;
    display:flex;
    align-items:center;
    justify-content:center;
    transform:rotate(-45deg);
  "><span style="transform:rotate(45deg);font-size:18px;">📌</span></div>`,
  iconSize: [40, 40],
  iconAnchor: [10, 38],
})

const haversineDistance = (lat1: number, lon1: number, lat2: number, lon2: number): number => {
  const R = 6371
  const dLat = ((lat2 - lat1) * Math.PI) / 180
  const dLon = ((lon2 - lon1) * Math.PI) / 180
  const a =
    Math.sin(dLat / 2) * Math.sin(dLat / 2) +
    Math.cos((lat1 * Math.PI) / 180) * Math.cos((lat2 * Math.PI) / 180) *
    Math.sin(dLon / 2) * Math.sin(dLon / 2)
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a))
  return R * c
}

const drawRouteLine = (from: { lat: number; lon: number }, to: { lat: number; lon: number }) => {
  if (!mapInstance) return

  if (routePolyline) {
    mapInstance.removeLayer(routePolyline)
  }
  if (destinationMarkerObj) {
    mapInstance.removeLayer(destinationMarkerObj)
  }

  destinationMarkerObj = L.marker([to.lat, to.lon], { icon: destIcon }).addTo(mapInstance)
  destinationMarkerObj.bindPopup(`<div style="font-size:14px;font-weight:bold;color:#ef4444">🎯 ${routeName.value}</div>`)

  const steps = 40
  const points: [number, number][] = []
  for (let i = 0; i <= steps; i++) {
    const t = i / steps
    const curved = t + Math.sin(t * Math.PI) * 0.0008
    const lat = from.lat + (to.lat - from.lat) * curved
    const lon = from.lon + (to.lon - from.lon) * curved
    points.push([lat, lon])
  }

  routePolyline = L.polyline(points, {
    color: '#2563eb',
    weight: 4,
    opacity: 0.7,
    dashArray: '12 6',
  }).addTo(mapInstance)

  const dist = haversineDistance(from.lat, from.lon, to.lat, to.lon)
  routeDistanceText.value = dist < 1
    ? `${(dist * 1000).toFixed(0)}m`
    : `${dist.toFixed(1)}km`

  const bounds = L.latLngBounds(
    [Math.min(from.lat, to.lat), Math.min(from.lon, to.lon)],
    [Math.max(from.lat, to.lat), Math.max(from.lon, to.lon)],
  )
  mapInstance.fitBounds(bounds, { padding: [60, 60] })
}

const clearRoute = () => {
  routeDestination.value = null
  routeName.value = ''
  routeDistanceText.value = ''
  showRouteDetail.value = false
  if (mapInstance) {
    if (routePolyline) { mapInstance.removeLayer(routePolyline); routePolyline = null }
    if (destinationMarkerObj) { mapInstance.removeLayer(destinationMarkerObj); destinationMarkerObj = null }
  }
}

const selectSearchResult = (result: SearchResult) => {
  const lat = parseFloat(result.lat)
  const lon = parseFloat(result.lon)
  routeDestination.value = { lat, lon }
  routeName.value = result.display_name.split(',')[0].trim()
  showRouteDetail.value = true
  searchResults.value = []
  searchQuery.value = ''
  drawRouteLine(carMarker.value, { lat, lon })
}

const doSearch = async () => {
  const q = searchQuery.value.trim()
  if (!q || searching.value) return

  searching.value = true
  searchResults.value = []

  try {
    const resp = await fetch(
      `https://nominatim.openstreetmap.org/search?format=json&q=${encodeURIComponent(q)}&limit=5&accept-language=zh`,
      { headers: { 'User-Agent': 'JellySmartHome/1.0' } }
    )
    if (resp.ok) {
      const data: SearchResult[] = await resp.json()
      searchResults.value = data.filter((r) => r.lat && r.lon)
    }
  } catch {
    searchResults.value = []
  } finally {
    searching.value = false
  }
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

  if (routeDestination.value) {
    drawRouteLine(carMarker.value, routeDestination.value)
  }
}

const simulatePositionChange = () => {
  const dLat = (Math.random() - 0.5) * 0.003
  const dLon = (Math.random() - 0.5) * 0.003
  carMarker.value.lat = Math.min(30.583, Math.max(30.562, carMarker.value.lat + dLat))
  carMarker.value.lon = Math.min(104.077, Math.max(104.056, carMarker.value.lon + dLon))
  updateCarMarker()
}

const startRefresh = () => {
  stopRefresh()
  refreshTimer = setInterval(() => {
    if (mapInstance && autoRefresh.value) {
      simulatePositionChange()
    }
    refreshCountdown.value = 15
  }, 15000)

  countdownTimer = setInterval(() => {
    if (refreshCountdown.value > 0) {
      refreshCountdown.value--
    }
  }, 1000)
}

const stopRefresh = () => {
  if (refreshTimer) { clearInterval(refreshTimer); refreshTimer = null }
  if (countdownTimer) { clearInterval(countdownTimer); countdownTimer = null }
}

const initMap = () => {
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
    attribution: '&copy; <a href="https://www.amap.com/">高德地图</a>',
    maxZoom: 18,
  }).addTo(mapInstance)

  setTimeout(() => {
    mapInstance?.invalidateSize()
  }, 200)

  carLeafletMarker = L.marker([carMarker.value.lat, carMarker.value.lon], { icon: vehicleIcon() }).addTo(mapInstance)
  carLeafletMarker.bindPopup(`
    <div style="min-width:180px;font-family:sans-serif">
      <p style="font-size:15px;font-weight:bold;margin:0 0 6px;color:#333">🚗 ${carMarker.value.name}</p>
      <p style="font-size:13px;color:#666;margin:0">经纬度: ${carMarker.value.lat.toFixed(4)}, ${carMarker.value.lon.toFixed(4)}</p>
      <p style="font-size:13px;color:#666;margin:4px 0 0">状态: <span style="color:#16a34a;font-weight:bold">在线</span></p>
      <p style="font-size:13px;color:#666;margin:4px 0 0">🔁 实时巡航中</p>
    </div>
  `)

  startRefresh()
}

const destroyMap = () => {
  stopRefresh()
  clearRoute()
  carLeafletMarker = null
  if (mapInstance) {
    mapInstance.remove()
    mapInstance = null
  }
}

const saveCarInfo = () => {
  alert('车辆信息已保存！')
}

const addCar = () => {
  if (!newCar.value.brand || !newCar.value.model || !newCar.value.plate) {
    alert('请填写完整的车辆信息')
    return
  }
  alert(`车辆 ${newCar.value.plate} 添加成功！`)
  newCar.value = { brand: '', model: '', plate: '' }
}

const remoteAction = (action: string) => {
  const actions: Record<string, string> = {
    lock: '远程锁车',
    unlock: '远程解锁',
    ac: '远程空调',
    flash: '寻车功能',
  }
  alert(`${actions[action] || action}指令已发送！`)
}

watch(autoRefresh, (val) => {
  if (val) {
    startRefresh()
  } else {
    stopRefresh()
    startRefresh()
  }
})

watch(activeMenu, async (val) => {
  if (val === 'location') {
    await nextTick()
    initMap()
  } else {
    destroyMap()
  }
})

onMounted(async () => {
  if (activeMenu.value === 'location') {
    await nextTick()
    initMap()
  }
})

onUnmounted(() => {
  destroyMap()
})
</script>

<style scoped>
.animate-fadeIn {
  animation: fadeIn 0.3s ease-in-out;
}

@keyframes fadeIn {
  from { opacity: 0; transform: translateY(8px); }
  to { opacity: 1; transform: translateY(0); }
}

.vehicle-marker {
  background: none !important;
  border: none !important;
}

.dest-marker {
  background: none !important;
  border: none !important;
}
</style>
