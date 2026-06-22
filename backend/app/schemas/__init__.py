from pydantic import BaseModel
from datetime import datetime
from typing import Optional

class TemperatureHumidityBase(BaseModel):
    temperature: float
    humidity: float
    node_id: Optional[str] = None
    device_id: Optional[str] = None

class TemperatureHumidityCreate(TemperatureHumidityBase):
    pass

class TemperatureHumidityResponse(TemperatureHumidityBase):
    id: int
    recorded_at: datetime

    class Config:
        from_attributes = True

class ElectricityBase(BaseModel):
    power: float
    daily_usage: float
    node_id: Optional[str] = None
    device_id: Optional[str] = None

class ElectricityCreate(ElectricityBase):
    pass

class ElectricityResponse(ElectricityBase):
    id: int
    recorded_at: datetime

    class Config:
        from_attributes = True

class SmokeBase(BaseModel):
    smoke: float
    temperature: Optional[float] = None
    humidity: Optional[float] = None
    node_id: Optional[str] = None
    device_id: Optional[str] = None


class SmokeCreate(SmokeBase):
    pass


class SmokeResponse(SmokeBase):
    id: int
    recorded_at: datetime

    class Config:
        from_attributes = True


class DashboardSummary(BaseModel):
    temperature: float
    humidity: float
    power: float
    daily_usage: float
    smoke: Optional[float] = None


class SensorCreate(BaseModel):
    sensor_id: str
    sensor_type: str
    device_type: str = "direct"
    mqtt_topic: str
    sensor_location: Optional[str] = None


class SensorUpdate(BaseModel):
    sensor_type: Optional[str] = None
    device_type: Optional[str] = None
    mqtt_topic: Optional[str] = None
    sensor_location: Optional[str] = None


class SensorResponse(SensorCreate):
    class Config:
        from_attributes = True


class NodeDeviceResponse(BaseModel):
    number: int
    gateway_id: str
    sensor_id: Optional[str] = None
    add_time: Optional[datetime] = None
    remarks: Optional[str] = None

    class Config:
        from_attributes = True
