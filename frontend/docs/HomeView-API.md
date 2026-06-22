# 果冻智能家 — 大屏首页 (HomeView) API 文档

> **页面文件**: [frontend/src/views/HomeView.vue](../src/views/HomeView.vue)
> **数据刷新策略**: 页面挂载时首次拉取，之后每 **10 秒** 轮询一次

---

## 数据流架构

```
                     ┌──────────────────────┐
    设备传感器 ──MQTT──>│ SmartHomeMqttClient   │
                     │  (mqtt_service.py)    │
                     └───┬──────┬──────┬────┘
                         │      │      │
                    ┌────┘  ┌───┘  ┌───┘
                    v       v      v
              ┌─────────┬──────────┬─────────┐
              │ temp_    │electricity│ smoke   │  (MySQL)
              │ humidity │_usage    │         │
              └────┬─────┴────┬─────┴────┬────┘
                   │          │          │
        ┌──────────┼──────────┼──────────┼──────────┐
        │          v          v          v           │
        │         FastAPI 后端 (:8000)                │
        │  /api/dashboard/*                           │
        │  /api/temperature-humidity/*                │
        │  /api/electricity/*                         │
        │  /api/smoke/*                               │
        └──────────────────────┬──────────────────────┘
                               │ Vite Proxy (/api → :8000)
                               v
                     ┌─────────────────┐
                     │  HomeView.vue   │
                     │  (大屏首页)       │
                     └─────────────────┘
```

---

## 一、已有 API（当前已调用）

### 1.1 仪表盘概览 — `GET /api/dashboard/summary`

**用途**: 获取首页顶部数据卡片的核心指标。

**调用位置**: [HomeView.vue #L441-L448](../src/views/HomeView.vue#L441-L448)

**请求**:
```
GET /api/dashboard/summary
```

**响应**:
```json
{
  "temperature": 26.5,
  "humidity": 58.0,
  "power": 1250.0,
  "dailyUsage": 8.5,
  "smoke": 12.0
}
```

| 字段 | 类型 | 单位 | 来源表 | 说明 |
|------|------|------|--------|------|
| `temperature` | `float` | °C | `temperature_humidity` 最新一条 | 全局平均温度 |
| `humidity` | `float` | % | `temperature_humidity` 最新一条 | 全局平均湿度 |
| `power` | `float` | W | `electricity_usage` 最新一条 | 当前总功率 |
| `dailyUsage` | `float` | kWh | `electricity_usage` 最新一条 | 当日累计用电量 |
| `smoke` | `float` | ppm | `smoke` 最新一条 | 当前烟雾浓度 |

**对应前端面板**:

| 面板 | 绑定字段 |
|------|----------|
| 平均温度 | `dash.temperature` |
| 平均湿度 | `dash.humidity` |
| 家庭实时功率 | `dash.power` |

---

## 二、已有 API（可复用但尚未接入）

这些后端接口已经实现，可用于替换前端硬编码数据。

### 2.1 温湿度历史 — `GET /api/temperature-humidity/history`

**用途**: 获取 24h 温湿度趋势数据。

```
GET /api/temperature-humidity/history?hours=24
```

**响应**:
```json
[
  {
    "temperature": 24.0,
    "humidity": 68.0,
    "recordedAt": "2026-05-24T02:00:00",
    "deviceId": "TH_SENSOR_01",
    "nodeId": "living_room"
  }
]
```

**可替换前端面板**: 🌡️ 24h 数据趋势（温度/湿度曲线）

---

### 2.2 电力历史 — `GET /api/electricity/history`

**用途**: 获取 24h 电力数据趋势。

```
GET /api/electricity/history?hours=24
```

**响应**:
```json
[
  {
    "power": 850.0,
    "dailyUsage": 2.5,
    "recordedAt": "2026-05-24T02:00:00",
    "deviceId": "ELEC_SENSOR_01",
    "nodeId": "main_power"
  }
]
```

**可替换前端面板**: ⚡ 电力功率曲线图 (W)

---

### 2.3 烟雾历史 — `GET /api/smoke/history`

**用途**: 获取 24h 烟雾数据趋势。

```
GET /api/smoke/history?hours=24
```

**响应**:
```json
[
  {
    "smoke": 15.0,
    "temperature": 26.0,
    "humidity": 55.0,
    "recordedAt": "2026-05-24T02:00:00"
  }
]
```

**可替换前端面板**: 📈 24h 数据趋势（烟雾曲线）

---

### 2.4 温湿度最新 — `GET /api/temperature-humidity/latest`

```
GET /api/temperature-humidity/latest
```

**响应**:
```json
{
  "temperature": 26.5,
  "humidity": 58.0,
  "recordedAt": "2026-05-24T12:00:00",
  "deviceId": "TH_SENSOR_01"
}
```

---

### 2.5 电力最新 — `GET /api/electricity/latest`

```
GET /api/electricity/latest
```

**响应**:
```json
{
  "power": 1250.0,
  "dailyUsage": 8.5,
  "recordedAt": "2026-05-24T12:00:00",
  "deviceId": "ELEC_SENSOR_01"
}
```

**可替换前端面板**: 家庭实时电流 / 家庭实时功率（注意：缺少 `current` 和 `voltage` 字段，见下方新增 API）

---

### 2.6 烟雾最新 — `GET /api/smoke/latest`

```
GET /api/smoke/latest
```

**响应**:
```json
{
  "smoke": 12.0,
  "temperature": 26.0,
  "humidity": 55.0,
  "recordedAt": "2026-05-24T12:00:00"
}
```

---

## 三、待新增 API（需后端开发）

以下是前端目前硬编码模拟、需要新增后端接口支撑的 API。

---

### 3.1 各房间温湿度 — `GET /api/rooms/humiture`

**对应面板**: 🌡️ 各房间温湿度（客厅 / 卧室 / 厨房 / 浴室）

**请求**:
```
GET /api/rooms/humiture
```

**响应**:
```json
{
  "rooms": [
    {
      "name": "客厅",
      "icon": "living",
      "temperature": 26.5,
      "humidity": 58.0,
      "updatedAt": "2026-05-24T12:00:00"
    },
    {
      "name": "卧室",
      "icon": "bedroom",
      "temperature": 24.2,
      "humidity": 52.0,
      "updatedAt": "2026-05-24T12:00:00"
    },
    {
      "name": "厨房",
      "icon": "kitchen",
      "temperature": 30.1,
      "humidity": 65.0,
      "updatedAt": "2026-05-24T12:00:00"
    },
    {
      "name": "浴室",
      "icon": "bathroom",
      "temperature": 27.8,
      "humidity": 72.0,
      "updatedAt": "2026-05-24T12:00:00"
    }
  ]
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `name` | `string` | 房间名称 |
| `icon` | `string` | 房间图标标识（`living`/`bedroom`/`kitchen`/`bathroom`） |
| `temperature` | `float` | 该房间当前温度 (°C) |
| `humidity` | `float` | 该房间当前湿度 (%) |
| `updatedAt` | `string` | 最后更新时间 (ISO 8601) |

**实现建议**: 在 `temperature_humidity` 表中按 `node_id` 字段区分房间，取每个房间最新一条记录。

---

### 3.2 室外环境指数 — `GET /api/weather/outdoor`

**对应面板**: 🌤️ 室外环境指数（今日/明日天气 + PM2.5）

**请求**:
```
GET /api/weather/outdoor
```

**响应**:
```json
{
  "today": {
    "condition": "partly_cloudy",
    "description": "晴转多云",
    "tempHigh": 32.0,
    "tempLow": 22.0
  },
  "tomorrow": {
    "condition": "rain",
    "description": "小雨",
    "tempHigh": 28.0,
    "tempLow": 20.0
  },
  "pm25": {
    "value": 38.0,
    "level": "good",
    "text": "优"
  }
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `today.condition` | `string` | 天气图标标识 (`sunny`/`partly_cloudy`/`cloudy`/`rain`/`thunderstorm`/`snow`) |
| `today.description` | `string` | 天气中文描述 |
| `today.tempHigh` | `float` | 今日最高温度 (°C) |
| `today.tempLow` | `float` | 今日最低温度 (°C) |
| `tomorrow.*` | — | 同上，次日天气 |
| `pm25.value` | `float` | PM2.5 数值 (μg/m³) |
| `pm25.level` | `string` | 等级标识 (`good`/`moderate`/`unhealthy`/`hazardous`) |
| `pm25.text` | `string` | 等级中文描述 (`优`/`良`/`轻度污染`/`中度污染`/`重度污染`) |

**实现建议**: 接入第三方天气 API（如和风天气、OpenWeather），在后端做缓存（1 小时刷新一次）。

---

### 3.3 电力实时详情 — `GET /api/electricity/realtime`

**对应面板**: 家庭实时电流（A）、家庭实时功率（W）

> 当前 `GET /api/dashboard/summary` 只有 `power`，缺少 `current` (电流) 和 `voltage` (电压)。

**请求**:
```
GET /api/electricity/realtime
```

**响应**:
```json
{
  "current": 5.45,
  "voltage": 220.5,
  "power": 1250.0,
  "dailyUsage": 8.5,
  "recordedAt": "2026-05-24T12:00:00"
}
```

| 字段 | 类型 | 单位 | 说明 |
|------|------|------|------|
| `current` | `float` | A | 当前总电流 |
| `voltage` | `float` | V | 当前电压 |
| `power` | `float` | W | 当前总功率 |
| `dailyUsage` | `float` | kWh | 当日累计用电量 |
| `recordedAt` | `string` | — | 数据时间 |

**数据库变更**: `electricity_usage` 表需新增 `current` (FLOAT) 和 `voltage` (FLOAT) 两列。

---

### 3.4 电流 24h 历史 — `GET /api/electricity/history`

（复用已有接口，扩展返回字段）

**对应面板**: 🔌 电流曲线图 (A)

**新响应**（在现有基础上增加 `current`、`voltage`）:
```json
[
  {
    "power": 850.0,
    "current": 4.2,
    "voltage": 221.0,
    "dailyUsage": 2.5,
    "recordedAt": "2026-05-24T02:00:00"
  }
]
```

前端从该接口提取：
- `power` → ⚡ 电力功率曲线图
- `current` → 🔌 电流曲线图

---

### 3.5 车辆位置 — `GET /api/vehicle/location`

**对应面板**: 🗺️ 车辆实时定位

**请求**:
```
GET /api/vehicle/location
```

**响应**:
```json
{
  "plateNumber": "川A·12345",
  "latitude": 30.5728,
  "longitude": 104.0668,
  "status": "moving",
  "speed": 45.0,
  "updatedAt": "2026-05-24T12:00:00"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `plateNumber` | `string` | 车牌号 |
| `latitude` | `float` | 纬度 |
| `longitude` | `float` | 经度 |
| `status` | `string` | 车辆状态 (`moving` 巡航中 / `parked` 驻车中 / `offline` 离线) |
| `speed` | `float` | 当前速度 (km/h) |
| `updatedAt` | `string` | 定位更新时间 |

**实现建议**: 车辆通过 MQTT 上报 GPS 位置，后端存储并在 `/api/vehicle/location` 返回最新位置。

---

## 四、API 与前端面板对照总表

| 序号 | 前端面板 | 所需 API | 状态 |
|------|----------|----------|------|
| 1 | 平均温度 | `GET /dashboard/summary` → `.temperature` | ✅ 已有 |
| 2 | 平均湿度 | `GET /dashboard/summary` → `.humidity` | ✅ 已有 |
| 3 | 家庭实时电流 | `GET /electricity/realtime` → `.current` | 🆕 待新增 |
| 4 | 家庭实时功率 | `GET /dashboard/summary` → `.power` | ✅ 已有 |
| 5 | 🌡️ 各房间温湿度 | `GET /rooms/humiture` | 🆕 待新增 |
| 6 | 🌤️ 室外环境指数 | `GET /weather/outdoor` | 🆕 待新增 |
| 7 | 🗺️ 车辆实时定位 | `GET /vehicle/location` | 🆕 待新增 |
| 8 | ⚡ 电力功率曲线图 | `GET /electricity/history?hours=24` → `.power` | 🔧 需修改（已有接口无 current/voltage） |
| 9 | 🔌 电流曲线图 | `GET /electricity/history?hours=24` → `.current` | 🔧 需扩展字段 |
| 10 | 📈 24h 数据趋势 (温度/湿度) | `GET /temperature-humidity/history?hours=24` | ✅ 已有 |
| 11 | 📈 24h 数据趋势 (烟雾) | `GET /smoke/history?hours=24` | ✅ 已有 |
| 12 | 4 个迷你趋势图 | 各 `/history` 接口取最近 6 个数据点 | 🔧 可复用 |

---

## 五、统一响应格式建议

所有新增 API 建议统一使用以下格式，与现有 `types/index.ts` 的 `ApiResponse<T>` 保持一致：

```json
{
  "code": 0,
  "message": "success",
  "data": { ... }
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `code` | `int` | `0` = 成功，非 0 = 错误码 |
| `message` | `string` | 提示信息 |
| `data` | `object` | 实际业务数据 |

---

## 六、前端类型定义补充

在 [frontend/src/types/index.ts](../src/types/index.ts) 中需新增以下 TypeScript 接口：

```typescript
export interface RoomHumiture {
  name: string
  icon: string
  temperature: number
  humidity: number
  updatedAt: string
}

export interface RoomsHumitureData {
  rooms: RoomHumiture[]
}

export interface WeatherOutdoor {
  today: {
    condition: string
    description: string
    tempHigh: number
    tempLow: number
  }
  tomorrow: {
    condition: string
    description: string
    tempHigh: number
    tempLow: number
  }
  pm25: {
    value: number
    level: string
    text: string
  }
}

export interface ElectricityRealtime {
  current: number
  voltage: number
  power: number
  dailyUsage: number
  recordedAt: string
}

export interface VehicleLocation {
  plateNumber: string
  latitude: number
  longitude: number
  status: 'moving' | 'parked' | 'offline'
  speed: number
  updatedAt: string
}
```
