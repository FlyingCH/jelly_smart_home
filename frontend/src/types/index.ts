export interface TemperatureHumidity {
  temperature: number
  humidity: number
  recordedAt: string
  deviceId?: string
}

export interface ElectricityUsage {
  power: number
  dailyUsage: number
  recordedAt: string
  deviceId?: string
}

export interface DashboardSummary {
  temperature: number
  humidity: number
  power: number
  dailyUsage: number
  smoke?: number
}

export interface Breaker {
  deviceId: string
  deviceName: string
  gateway_id?: string
  switchStatus: number
  current: number
  voltage: number
  power: number
  temperature: number
  powerFactor: number
  totalEnergy: number
  dataTime?: string
}

export interface ApiResponse<T> {
  code: number
  message: string
  data: T
}
