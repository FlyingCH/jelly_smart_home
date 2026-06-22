import apiClient from './axios'
import type { TemperatureHumidity } from '@/types'

export const getLatestTemperatureHumidity = async (): Promise<TemperatureHumidity> => {
  const response = await apiClient.get<TemperatureHumidity>('/temperature-humidity/latest')
  return response.data
}

export const getTemperatureHumidityHistory = async (): Promise<TemperatureHumidity[]> => {
  const response = await apiClient.get<TemperatureHumidity[]>('/temperature-humidity/history')
  return response.data
}
