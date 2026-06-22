<template>
  <div class="console">
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
        <router-link to="/console" class="nav-link active">控制台</router-link>
      </div>
      <div class="top-right">
        <span class="datetime">{{ currentTime }}</span>
        <span class="user-badge">👤 {{ authStore.username }}</span>
        <button class="logout-btn" @click="handleLogout">退出</button>
      </div>
    </div>

    <div class="main-content">
      <div class="cards-row">
        <div class="panel-box export-card">
          <div class="panel-title">📥 历史数据导出</div>
          <div class="form-group">
            <label class="form-label"><span class="required-mark">*</span> 数据类型</label>
            <select v-model="exportForm.dataType" class="form-select" required>
              <option value="">请选择数据类型</option>
              <option value="temperature">温湿度数据</option>
              <option value="electricity">用电数据</option>
              <option value="car">车辆轨迹数据</option>
            </select>
          </div>
          <div class="form-group form-group-time">
            <div class="time-item">
              <label class="form-label"><span class="required-mark">*</span> 开始时间</label>
              <input v-model="exportForm.startTime" type="datetime-local" class="form-input" required />
            </div>
            <div class="time-item">
              <label class="form-label"><span class="required-mark">*</span> 结束时间</label>
              <input v-model="exportForm.endTime" type="datetime-local" class="form-input" required />
            </div>
          </div>
          <div class="form-group">
            <label class="form-label"><span class="required-mark">*</span> 导出格式</label>
            <select v-model="exportForm.format" class="form-select" required>
              <option value="">请选择导出格式</option>
              <option value="csv">CSV</option>
              <option value="json">JSON</option>
              <option value="xlsx">Excel (XLSX)</option>
            </select>
          </div>
          <button class="btn btn-export" @click="handleExport">📤 导出数据</button>
        </div>

        <div class="panel-box sensor-card">
          <div class="panel-title">➕ 添加MQTT传感器</div>
          <div class="form-group form-group-time">
            <div class="time-item">
              <label class="form-label"><span class="required-mark">*</span> 设备类型</label>
              <select v-model="sensorForm.deviceType" class="form-select" required @change="onDeviceTypeChange">
                <option value="">请选择设备类型</option>
                <option value="gateway">网关设备</option>
                <option value="gateway_sub">网关子设备</option>
                <option value="direct">直连设备</option>
              </select>
            </div>
            <div class="time-item">
              <label class="form-label"><span class="required-mark">*</span> 传感器类型</label>
              <select v-model="sensorForm.type" class="form-select" required :disabled="!sensorForm.deviceType">
                <option value="">请选择传感器类型</option>
                <option v-for="opt in sensorTypeOptions" :key="opt.value" :value="opt.value">{{ opt.label }}</option>
              </select>
            </div>
          </div>
          <div class="form-group form-group-time">
            <div class="time-item">
              <label class="form-label"><span class="required-mark">*</span> ID</label>
              <input v-model="sensorForm.deviceId" type="text" class="form-input" placeholder="请输入设备 ID" required :disabled="fieldDisabled.deviceId" />
            </div>
            <div class="time-item">
              <label class="form-label">传感器位置 <span class="optional-mark">(非必选)</span></label>
              <input v-model="sensorForm.location" type="text" class="form-input" placeholder="例如:客厅" :disabled="fieldDisabled.location" />
            </div>
          </div>
          <div class="form-group">
            <label class="form-label"><span class="required-mark">*</span> MQTT 主题</label>
            <input v-model="sensorForm.mqttTopic" type="text" class="form-input" :placeholder="mqttPlaceholder" required :disabled="fieldDisabled.mqttTopic" />
          </div>
          <button class="btn btn-add-sensor" @click="handleAddSensor">🔧 添加传感器</button>
        </div>
      </div>

      <div class="panel-box sensor-list-card">
        <div class="panel-title">📋 已注册传感器列表</div>
        <div v-if="sensorList.length === 0" class="empty-state">
          <div class="empty-icon">📭</div>
          <p class="empty-text">暂无已注册的传感器</p>
        </div>
        <div v-else class="table-wrapper">
          <table class="sensor-table">
            <thead>
              <tr>
                <th>设备类型</th>
                <th>传感器类型</th>
                <th>传感器 ID</th>
                <th>传感器位置</th>
                <th>MQTT 主题</th>
                <th>操作</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="sensor in sensorList" :key="sensor.sensor_id">
                <template v-if="editingId === sensor.sensor_id">
                  <td>
                    <select v-model="editForm.device_type" class="form-select table-select">
                      <option value="gateway">网关设备</option>
                      <option value="gateway_sub">网关子设备</option>
                      <option value="direct">直连设备</option>
                    </select>
                  </td>
                  <td>
                    <select v-model="editForm.sensor_type" class="form-select table-select">
                      <option v-for="opt in editTypeOptions" :key="opt.value" :value="opt.value">{{ opt.label }}</option>
                    </select>
                  </td>
                  <td class="id-cell">{{ sensor.sensor_id }}</td>
                  <td>
                    <input v-model="editForm.sensor_location" class="form-input table-input" placeholder="例如:客厅" />
                  </td>
                  <td>
                    <input v-model="editForm.mqtt_topic" class="form-input table-input" required />
                  </td>
                  <td class="action-cell">
                    <button class="btn-table btn-save" @click="handleEditSave(sensor.sensor_id)">保存</button>
                    <button class="btn-table btn-cancel" @click="handleEditCancel">取消</button>
                  </td>
                </template>
                <template v-else>
                  <td>{{ deviceTypeLabel(sensor.device_type) }}</td>
                  <td>{{ typeLabel(sensor.sensor_type) }}</td>
                  <td class="id-cell">{{ sensor.sensor_id }}</td>
                  <td>{{ sensor.sensor_location || '-' }}</td>
                  <td class="topic-cell">{{ sensor.mqtt_topic }}</td>
                  <td class="action-cell">
                    <button class="btn-table btn-edit" @click="handleEditStart(sensor)">修改</button>
                    <button class="btn-table btn-delete" @click="handleDelete(sensor.sensor_id)">删除</button>
                    <button v-if="sensor.device_type === 'gateway'" class="btn-table btn-add-sub" @click="handleAddSubDevice(sensor)">添加子设备</button>
                  </td>
                </template>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>

    <div v-if="showSubDeviceModal" class="modal-overlay" @click.self="closeSubDeviceModal">
      <div class="modal-dialog">
        <div class="modal-header">
          <span class="modal-label">请选择子设备类型：</span>
          <select v-model="selectedSubType" class="form-select modal-select" @change="onSubTypeChange">
            <option value="">选择子设备类型</option>
            <option value="temperature_humidity">温湿度网关子设备</option>
            <option value="circuit_breaker">断路器网关子设备</option>
            <option value="smoke">烟雾网关子设备</option>
          </select>
          <button
            class="btn-table btn-save"
            style="margin-right:6px"
            :disabled="confirmDisabled || submitting"
            @click="confirmSubDevice"
          >{{ submitting ? '提交中…' : '确定' }}</button>
          <button class="btn-table btn-cancel" @click="closeSubDeviceModal">取消</button>
        </div>
        <div class="modal-body">
          <div v-if="!selectedSubType" class="modal-placeholder">请在上方选择子设备类型</div>
          <div v-else-if="subDeviceLoading" class="modal-placeholder">加载中…</div>
          <div v-else-if="subDeviceList.length === 0" class="modal-placeholder">暂无未绑定子设备</div>
          <table v-else class="sub-device-table">
            <thead>
              <tr>
                <th style="width:40px;text-align:center">
                  <input type="checkbox" :checked="allSubDevicesSelected" @change="toggleSelectAllSubDevices" />
                </th>
                <th>序号</th>
                <th>网关ID</th>
                <th>传感器ID</th>
                <th>添加时间</th>
                <th>备注</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="(item, idx) in subDeviceList" :key="item.number">
                <td style="text-align:center">
                  <input type="checkbox" :checked="selectedSubDeviceIds.has(item.number)" @change="toggleSubDeviceSelect(item.number)" />
                </td>
                <td>{{ item.number }}</td>
                <td>{{ item.gateway_id }}</td>
                <td>{{ item.sensor_id || '-' }}</td>
                <td>{{ item.add_time ? formatTime(item.add_time) : '-' }}</td>
                <td>{{ item.remarks || '-' }}</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted, onUnmounted, watch } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'

interface SensorItem {
  sensor_id: string
  sensor_type: string
  device_type: string
  mqtt_topic: string
  sensor_location: string | null
}

interface NodeDeviceItem {
  number: number
  gateway_id: string
  sensor_id: string | null
  add_time: string | null
  remarks: string | null
}

const TYPE_LABELS: Record<string, string> = {
  temperature: '温湿度传感器',
  breaker: '断路器',
  smoke: '烟雾传感器',
  other: '其他类型',
  temp_humi_gateway: '温湿度网关',
  breaker_gateway: '断路器网关',
  smoke_gateway: '烟雾网关',
  temp_humi_sub: '温湿度传感器子设备',
  breaker_sub: '断路器子设备',
  smoke_sub: '烟雾传感器子设备',
  direct_1: '其他 1',
  direct_2: '其他 2',
  direct_3: '其他 3',
}

const DEVICE_TYPE_LABELS: Record<string, string> = {
  gateway: '网关设备',
  gateway_sub: '网关子设备',
  direct: '直连设备',
}

const SENSOR_OPTIONS: Record<string, { value: string; label: string }[]> = {
  gateway: [
    { value: 'temp_humi_gateway', label: '温湿度网关' },
    { value: 'breaker_gateway', label: '断路器网关' },
    { value: 'smoke_gateway', label: '烟雾网关' },
  ],
  gateway_sub: [
    { value: 'temp_humi_sub', label: '温湿度传感器子设备' },
    { value: 'breaker_sub', label: '断路器子设备' },
    { value: 'smoke_sub', label: '烟雾传感器子设备' },
  ],
  direct: [
    { value: 'direct_1', label: '其他 1' },
    { value: 'direct_2', label: '其他 2' },
    { value: 'direct_3', label: '其他 3' },
  ],
}

const ALL_TYPE_OPTIONS: { value: string; label: string }[] = [
  ...SENSOR_OPTIONS.gateway,
  ...SENSOR_OPTIONS.gateway_sub,
  ...SENSOR_OPTIONS.direct,
]

const typeLabel = (t: string) => TYPE_LABELS[t] || t
const deviceTypeLabel = (t: string) => DEVICE_TYPE_LABELS[t] || t

const router = useRouter()
const authStore = useAuthStore()

const currentTime = ref('')
let clockTimer: number | null = null

const exportForm = reactive({
  dataType: '',
  startTime: '',
  endTime: '',
  format: '',
})

const sensorForm = reactive({
  type: '',
  deviceType: '',
  deviceId: '',
  location: '',
  mqttTopic: '',
})

const sensorTypeOptions = computed(() => {
  return SENSOR_OPTIONS[sensorForm.deviceType] || []
})

const editTypeOptions = computed(() => {
  return ALL_TYPE_OPTIONS
})

const fieldDisabled = computed(() => {
  const dt = sensorForm.deviceType
  return {
    deviceId: !dt || dt === 'direct',
    location: !dt,
    mqttTopic: !dt || dt === 'gateway' || dt === 'gateway_sub',
  }
})

const mqttPlaceholder = computed(() => {
  const dt = sensorForm.deviceType
  if (dt === 'gateway') return '自动生成'
  if (dt === 'gateway_sub') return '自动生成'
  return '例如: /home/sensor/temp'
})

const onDeviceTypeChange = () => {
  sensorForm.type = ''
  sensorForm.deviceId = ''
  sensorForm.location = ''
  sensorForm.mqttTopic = ''
}

watch(
  () => [sensorForm.deviceType, sensorForm.deviceId],
  () => {
    const dt = sensorForm.deviceType
    if (dt === 'gateway' && sensorForm.deviceId) {
      sensorForm.mqttTopic = `/Multisensor/${sensorForm.deviceId}/data`
    } else if (dt === 'gateway_sub' && sensorForm.deviceId) {
      sensorForm.mqttTopic = `/${dt}/${sensorForm.deviceId}/data`
    }
  }
)

const sensorList = ref<SensorItem[]>([])
const editingId = ref<string | null>(null)
const editForm = reactive({
  sensor_type: '',
  device_type: '',
  sensor_location: '',
  mqtt_topic: '',
})

const showSubDeviceModal = ref(false)
const selectedSubType = ref('')
const subDeviceList = ref<NodeDeviceItem[]>([])
const subDeviceLoading = ref(false)
const submitting = ref(false)
const currentGatewayId = ref('')
const selectedSubDeviceIds = reactive(new Set<number>())

const allSubDevicesSelected = computed(() => {
  return subDeviceList.value.length > 0 && selectedSubDeviceIds.size === subDeviceList.value.length
})

const confirmDisabled = computed(() => {
  return selectedSubDeviceIds.size === 0
})

const fetchSensors = async () => {
  try {
    const resp = await fetch('/api/sensors')
    if (resp.ok) {
      sensorList.value = await resp.json()
    }
  } catch (e) {
    console.error('获取传感器列表失败', e)
  }
}

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

const handleExport = () => {
  if (!exportForm.dataType || !exportForm.startTime || !exportForm.endTime || !exportForm.format) {
    alert('请填写完整的导出信息')
    return
  }
  alert(`正在导出 ${exportForm.dataType} 数据，格式：${exportForm.format}`)
}

const handleAddSensor = async () => {
  if (!sensorForm.deviceType || !sensorForm.type || !sensorForm.deviceId || !sensorForm.mqttTopic) {
    alert('请填写完整的必选信息')
    return
  }
  try {
    const resp = await fetch('/api/sensors', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        sensor_id: sensorForm.deviceId,
        sensor_type: sensorForm.type,
        device_type: sensorForm.deviceType,
        mqtt_topic: sensorForm.mqttTopic,
        sensor_location: sensorForm.location || null,
      }),
    })
    if (!resp.ok) {
      const err = await resp.json()
      alert(`添加失败: ${err.detail}`)
      return
    }
    alert(`传感器 ${sensorForm.deviceId} 添加成功`)
    sensorForm.deviceType = ''
    sensorForm.type = ''
    sensorForm.deviceId = ''
    sensorForm.location = ''
    sensorForm.mqttTopic = ''
    fetchSensors()
  } catch (e) {
    alert(`网络错误: ${e}`)
  }
}

const handleDelete = async (sensorId: string) => {
  if (!confirm(`确定删除传感器 ${sensorId} 吗？`)) return
  try {
    const resp = await fetch(`/api/sensors/${encodeURIComponent(sensorId)}`, { method: 'DELETE' })
    if (!resp.ok) {
      const err = await resp.json()
      alert(`删除失败: ${err.detail}`)
      return
    }
    fetchSensors()
  } catch (e) {
    alert(`网络错误: ${e}`)
  }
}

const handleEditStart = (sensor: SensorItem) => {
  editingId.value = sensor.sensor_id
  editForm.device_type = sensor.device_type || 'direct'
  editForm.sensor_type = sensor.sensor_type
  editForm.sensor_location = sensor.sensor_location || ''
  editForm.mqtt_topic = sensor.mqtt_topic
}

const handleEditCancel = () => {
  editingId.value = null
}

const handleAddSubDevice = (sensor: SensorItem) => {
  currentGatewayId.value = sensor.sensor_id
  selectedSubType.value = ''
  subDeviceList.value = []
  selectedSubDeviceIds.clear()
  submitting.value = false
  showSubDeviceModal.value = true
}

const closeSubDeviceModal = () => {
  showSubDeviceModal.value = false
  selectedSubType.value = ''
  subDeviceList.value = []
  selectedSubDeviceIds.clear()
  currentGatewayId.value = ''
  submitting.value = false
}

const fetchSubDevices = async () => {
  if (!selectedSubType.value) return
  subDeviceLoading.value = true
  selectedSubDeviceIds.clear()
  try {
    const params = new URLSearchParams({
      device_type: selectedSubType.value,
    })
    const resp = await fetch(`/api/device/get-unbound-subdevices?${params}`)
    if (resp.ok) {
      const result = await resp.json()
      subDeviceList.value = result.data || []
    } else {
      subDeviceList.value = []
    }
  } catch (e) {
    console.error('获取子设备列表失败', e)
    subDeviceList.value = []
  } finally {
    subDeviceLoading.value = false
  }
}

const toggleSubDeviceSelect = (number: number) => {
  if (selectedSubDeviceIds.has(number)) {
    selectedSubDeviceIds.delete(number)
  } else {
    selectedSubDeviceIds.add(number)
  }
}

const toggleSelectAllSubDevices = () => {
  if (allSubDevicesSelected.value) {
    selectedSubDeviceIds.clear()
  } else {
    subDeviceList.value.forEach(item => selectedSubDeviceIds.add(item.number))
  }
}

const onSubTypeChange = () => {
  subDeviceList.value = []
  selectedSubDeviceIds.clear()
  if (selectedSubType.value) {
    fetchSubDevices()
  }
}

const confirmSubDevice = async () => {
  if (confirmDisabled.value) {
    alert('请先选择子设备类型并勾选子设备')
    return
  }
  submitting.value = true
  try {
    const sensorIds = subDeviceList.value
      .filter(item => selectedSubDeviceIds.has(item.number))
      .map(item => item.sensor_id)
    const resp = await fetch('/api/device/batch-bind-subdevices', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        parent_gateway_id: currentGatewayId.value,
        device_type: selectedSubType.value,
        selected_sensor_ids: sensorIds,
      }),
    })
    const result = await resp.json()
    if (resp.ok) {
      alert(result.message || '子设备绑定完成')
      closeSubDeviceModal()
      fetchSensors()
    } else {
      alert(`绑定失败: ${result.detail || '请重试'}`)
    }
  } catch (e) {
    alert(`网络错误: ${e}`)
  } finally {
    submitting.value = false
  }
}

const formatTime = (t: string) => {
  if (!t) return '-'
  return new Date(t).toLocaleString('zh-CN')
}

const handleEditSave = async (sensorId: string) => {
  if (!editForm.mqtt_topic) {
    alert('MQTT 主题不能为空')
    return
  }
  try {
    const resp = await fetch(`/api/sensors/${encodeURIComponent(sensorId)}`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        sensor_type: editForm.sensor_type,
        device_type: editForm.device_type,
        mqtt_topic: editForm.mqtt_topic,
        sensor_location: editForm.sensor_location || null,
      }),
    })
    if (!resp.ok) {
      const err = await resp.json()
      alert(`修改失败: ${err.detail}`)
      return
    }
    editingId.value = null
    fetchSensors()
  } catch (e) {
    alert(`网络错误: ${e}`)
  }
}

const handleLogout = () => {
  authStore.logout()
  router.replace('/login')
}

onMounted(() => {
  updateClock()
  clockTimer = window.setInterval(updateClock, 1000)
  fetchSensors()
})

onUnmounted(() => {
  if (clockTimer) clearInterval(clockTimer)
})
</script>

<style scoped>
.console {
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
  text-shadow: 0 0 8px rgba(0, 212, 255, 0.4);
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

.main-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 10px;
  min-height: 0;
  overflow-y: auto;
}

.cards-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
  flex-shrink: 0;
}

.panel-box {
  background: rgba(8, 34, 72, 0.55);
  border: 1px solid rgba(0, 180, 255, 0.18);
  border-radius: 6px;
  padding: 16px 20px;
  backdrop-filter: blur(4px);
}

.panel-title {
  font-size: 14px;
  font-weight: 600;
  color: #A0C8E8;
  margin-bottom: 16px;
  letter-spacing: 1px;
  border-left: 3px solid #00D4FF;
  padding-left: 8px;
}

.form-group {
  margin-bottom: 14px;
}

.form-group-time {
  display: flex;
  gap: 12px;
}

.form-group-time .time-item {
  flex: 1;
}

.form-label {
  display: block;
  font-size: 12px;
  color: #7A8FA0;
  margin-bottom: 6px;
  letter-spacing: 0.5px;
}

.form-select,
.form-input {
  width: 100%;
  padding: 8px 12px;
  background: rgba(10, 28, 56, 0.6);
  border: 1px solid rgba(0, 180, 255, 0.25);
  border-radius: 4px;
  color: #c8d8e8;
  font-size: 13px;
  font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif;
  outline: none;
  transition: border-color 0.2s;
  box-sizing: border-box;
}

.form-select:focus,
.form-input:focus {
  border-color: rgba(0, 212, 255, 0.5);
  box-shadow: 0 0 6px rgba(0, 212, 255, 0.1);
}

.form-select:disabled,
.form-input:disabled {
  opacity: 0.35;
  cursor: not-allowed;
}

.form-select option {
  background: #0a1628;
  color: #c8d8e8;
}

.form-input::placeholder {
  color: #556677;
}

.form-input::-webkit-calendar-picker-indicator {
  filter: invert(0.7);
  cursor: pointer;
}

.btn {
  width: 100%;
  padding: 10px 16px;
  border: none;
  border-radius: 5px;
  font-size: 14px;
  font-weight: 600;
  font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif;
  letter-spacing: 0.5px;
  cursor: pointer;
  transition: all 0.2s;
  margin-top: 4px;
}

.btn-export {
  background: linear-gradient(135deg, #1E6FFF, #00A3FF);
  color: #E8F4FF;
  box-shadow: 0 2px 12px rgba(30, 111, 255, 0.3);
}

.btn-export:hover {
  background: linear-gradient(135deg, #2A7AFF, #0AB3FF);
  box-shadow: 0 4px 18px rgba(30, 111, 255, 0.45);
  transform: translateY(-1px);
}

.btn-add-sensor {
  background: linear-gradient(135deg, #16A34A, #22C55E);
  color: #E8F4FF;
  box-shadow: 0 2px 12px rgba(22, 163, 74, 0.3);
}

.btn-add-sensor:hover {
  background: linear-gradient(135deg, #1AB350, #2CD06A);
  box-shadow: 0 4px 18px rgba(22, 163, 74, 0.45);
  transform: translateY(-1px);
}

.sensor-list-card {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-height: 0;
}

.empty-state {
  flex: 1;
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  padding: 40px 0;
}

.empty-icon {
  font-size: 48px;
  margin-bottom: 12px;
  opacity: 0.5;
}

.empty-text {
  font-size: 14px;
  color: #556677;
  letter-spacing: 1px;
}

.required-mark {
  color: #FF6B6B;
  margin-right: 2px;
  font-weight: bold;
}

.optional-mark {
  color: #556677;
  font-weight: normal;
  font-size: 11px;
}

.table-wrapper {
  flex: 1;
  overflow-y: auto;
  min-height: 0;
}

.sensor-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

.sensor-table thead {
  position: sticky;
  top: 0;
  z-index: 1;
}

.sensor-table th {
  background: rgba(0, 120, 200, 0.2);
  color: #A0C8E8;
  padding: 10px 12px;
  text-align: left;
  font-weight: 600;
  font-size: 12px;
  letter-spacing: 0.5px;
  border-bottom: 1px solid rgba(0, 180, 255, 0.25);
}

.sensor-table td {
  padding: 10px 12px;
  border-bottom: 1px solid rgba(0, 180, 255, 0.08);
  color: #c8d8e8;
}

.sensor-table tbody tr:hover {
  background: rgba(0, 120, 200, 0.06);
}

.id-cell {
  font-family: 'Consolas', 'Courier New', monospace;
  font-size: 12px;
}

.topic-cell {
  font-family: 'Consolas', 'Courier New', monospace;
  font-size: 12px;
  word-break: break-all;
}

.action-cell {
  white-space: nowrap;
}

.btn-table {
  padding: 4px 12px;
  border: none;
  border-radius: 3px;
  font-size: 12px;
  cursor: pointer;
  transition: all 0.2s;
  margin-right: 6px;
  font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif;
}

.btn-edit {
  background: rgba(30, 111, 255, 0.2);
  color: #4DA3FF;
  border: 1px solid rgba(30, 111, 255, 0.3);
}

.btn-edit:hover {
  background: rgba(30, 111, 255, 0.35);
}

.btn-delete {
  background: rgba(255, 80, 80, 0.15);
  color: #FF6B6B;
  border: 1px solid rgba(255, 80, 80, 0.3);
}

.btn-delete:hover {
  background: rgba(255, 80, 80, 0.3);
}

.btn-save {
  background: rgba(22, 163, 74, 0.2);
  color: #4ADE80;
  border: 1px solid rgba(22, 163, 74, 0.3);
}

.btn-save:hover {
  background: rgba(22, 163, 74, 0.35);
}

.btn-cancel {
  background: rgba(150, 150, 150, 0.15);
  color: #8899AA;
  border: 1px solid rgba(150, 150, 150, 0.3);
}

.btn-cancel:hover {
  background: rgba(150, 150, 150, 0.3);
}

.btn-add-sub {
  background: rgba(34, 197, 94, 0.15);
  color: #4ADE80;
  border: 1px solid rgba(34, 197, 94, 0.3);
}

.btn-add-sub:hover {
  background: rgba(34, 197, 94, 0.3);
}

.table-select,
.table-input {
  padding: 4px 8px;
  background: rgba(10, 28, 56, 0.6);
  border: 1px solid rgba(0, 180, 255, 0.3);
  border-radius: 3px;
  color: #c8d8e8;
  font-size: 12px;
  font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif;
  outline: none;
  box-sizing: border-box;
  width: 100%;
}

.table-select:focus,
.table-input:focus {
  border-color: rgba(0, 212, 255, 0.5);
  box-shadow: 0 0 4px rgba(0, 212, 255, 0.1);
}

.table-select option {
  background: #0a1628;
  color: #c8d8e8;
}

.modal-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.55);
  display: flex;
  justify-content: center;
  align-items: center;
  z-index: 1000;
}

.modal-dialog {
  background: rgba(18, 52, 102, 0.95);
  border: 1px solid rgba(150, 170, 190, 0.35);
  border-radius: 8px;
  width: 820px;
  max-height: 80vh;
  display: flex;
  flex-direction: column;
  box-shadow: 0 8px 40px rgba(0, 0, 0, 0.5);
}

.modal-header {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 16px 20px;
  border-bottom: 1px solid rgba(0, 180, 255, 0.15);
  flex-shrink: 0;
}

.modal-label {
  font-size: 13px;
  color: #A0C8E8;
  white-space: nowrap;
}

.modal-select {
  width: 200px;
  flex-shrink: 0;
}

.modal-body {
  flex: 1;
  overflow-y: auto;
  padding: 16px 20px;
  min-height: 200px;
  background: rgba(10, 40, 80, 0.4);
}

.modal-placeholder {
  display: flex;
  justify-content: center;
  align-items: center;
  height: 200px;
  color: #556677;
  font-size: 14px;
}

.sub-device-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

.sub-device-table thead {
  position: sticky;
  top: 0;
  z-index: 1;
}

.sub-device-table th {
  background: rgba(0, 100, 180, 0.25);
  color: #A0C8E8;
  padding: 10px 12px;
  text-align: left;
  font-weight: 600;
  font-size: 12px;
  letter-spacing: 0.5px;
  border-bottom: 1px solid rgba(0, 180, 255, 0.2);
}

.sub-device-table td {
  padding: 10px 12px;
  border-bottom: 1px solid rgba(0, 180, 255, 0.08);
  color: #c8d8e8;
}

.sub-device-table tbody tr:hover {
  background: rgba(0, 120, 200, 0.06);
}
</style>
