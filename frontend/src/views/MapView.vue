<template>
  <div class="space-y-6">
    <h2 class="text-3xl font-bold text-white mb-6">🗺️ 设备地图</h2>

    <div class="bg-white rounded-lg shadow-lg p-4">
      <div ref="mapContainer" class="w-full h-[600px] rounded-lg"></div>
    </div>

    <div class="grid grid-cols-1 md:grid-cols-3 gap-4">
      <div class="bg-white rounded-lg shadow-lg p-4 flex items-center space-x-3">
        <span class="text-2xl">🌡️</span>
        <div>
          <p class="text-sm text-gray-500">温湿度传感器</p>
          <p class="text-lg font-semibold">{{ deviceCount.temperature }} 个</p>
        </div>
      </div>
      <div class="bg-white rounded-lg shadow-lg p-4 flex items-center space-x-3">
        <span class="text-2xl">⚡</span>
        <div>
          <p class="text-sm text-gray-500">电力传感器</p>
          <p class="text-lg font-semibold">{{ deviceCount.electricity }} 个</p>
        </div>
      </div>
      <div class="bg-white rounded-lg shadow-lg p-4 flex items-center space-x-3">
        <span class="text-2xl">🚗</span>
        <div>
          <p class="text-sm text-gray-500">车辆设备</p>
          <p class="text-lg font-semibold">{{ deviceCount.vehicle }} 个</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import L from 'leaflet'
import 'leaflet/dist/leaflet.css'

const mapContainer = ref<HTMLElement | null>(null)
let mapInstance: L.Map | null = null
const deviceCount = ref({ temperature: 1, electricity: 1, vehicle: 0 })

interface DeviceMarker {
  id: string
  name: string
  type: 'temperature' | 'electricity' | 'vehicle'
  lat: number
  lon: number
}

const deviceIcon = (type: string): L.DivIcon => {
  const emoji = type === 'temperature' ? '🌡️' : type === 'electricity' ? '⚡' : '🚗'
  return L.divIcon({
    className: 'device-marker',
    html: `<div style="font-size:24px;background:white;border-radius:50%;padding:4px;box-shadow:0 2px 8px rgba(0,0,0,0.3);width:40px;height:40px;display:flex;align-items:center;justify-content:center;cursor:pointer;">${emoji}</div>`,
    iconSize: [40, 40],
    iconAnchor: [20, 20],
  })
}

const devices: DeviceMarker[] = [
  { id: 'th-01', name: '客厅温湿度', type: 'temperature', lat: 30.5702, lon: 104.0603 },
  { id: 'th-02', name: '卧室温湿度', type: 'temperature', lat: 30.5720, lon: 104.0620 },
  { id: 'el-01', name: '总电表', type: 'electricity', lat: 30.5710, lon: 104.0610 },
]

onMounted(() => {
  if (mapContainer.value) {
    delete (L.Icon.Default.prototype as any)._getIconUrl
    L.Icon.Default.mergeOptions({
      iconRetinaUrl: 'https://unpkg.com/leaflet@1.9.4/dist/images/marker-icon-2x.png',
      iconUrl: 'https://unpkg.com/leaflet@1.9.4/dist/images/marker-icon.png',
      shadowUrl: 'https://unpkg.com/leaflet@1.9.4/dist/images/marker-shadow.png',
    })

    mapInstance = L.map(mapContainer.value, {
      center: [30.5702, 104.0603],
      zoom: 15,
      zoomControl: true,
    })

    L.tileLayer('https://webrd0{s}.is.autonavi.com/appmaptile?lang=zh_cn&size=1&scale=1&style=8&x={x}&y={y}&z={z}', {
      subdomains: ['1', '2', '3', '4'],
      attribution: '&copy; <a href="https://www.amap.com/">高德地图</a>',
      maxZoom: 18,
    }).addTo(mapInstance)

    devices.forEach((device) => {
      const marker = L.marker([device.lat, device.lon], { icon: deviceIcon(device.type) }).addTo(mapInstance!)

      marker.bindPopup(`
        <div style="min-width:150px">
          <p style="font-size:16px;font-weight:bold;margin:0 0 4px">${device.name}</p>
          <p style="font-size:13px;color:#666;margin:0">
            类型: ${device.type === 'temperature' ? '温湿度传感器' : device.type === 'electricity' ? '电力传感器' : '车辆'}<br>
            ID: ${device.id}<br>
            位置: ${device.lat.toFixed(4)}, ${device.lon.toFixed(4)}
          </p>
        </div>
      `)
    })
  }
})

onUnmounted(() => {
  if (mapInstance) {
    mapInstance.remove()
    mapInstance = null
  }
})
</script>

<style scoped>
.device-marker {
  background: none !important;
  border: none !important;
}
</style>
