import apiClient from './axios'
import type { Breaker, ApiResponse, ElectricityUsage } from '@/types'

export const getLatestElectricity = async (): Promise<ElectricityUsage> => {
  const response = await apiClient.get('/electricity/latest') as ElectricityUsage
  return response
}

export const getElectricityHistory = async (): Promise<ElectricityUsage[]> => {
  const response = await apiClient.get('/electricity/history') as ElectricityUsage[]
  return response
}

export const getBreakers = async (): Promise<Breaker[]> => {
  const response = await apiClient.get('/electric/device-real-time-list') as ApiResponse<Breaker[]>
  return response.data
}
