from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session
from typing import List
from datetime import datetime, timedelta
from app.core.database import get_db
from app.models.temperature_humidity import TemperatureHumidity
from app.schemas import TemperatureHumidityResponse, TemperatureHumidityCreate

router = APIRouter()

@router.get("/temperature-humidity/latest", response_model=TemperatureHumidityResponse)
async def get_latest_temperature_humidity(db: Session = Depends(get_db)):
    latest = db.query(TemperatureHumidity).order_by(TemperatureHumidity.recorded_at.desc()).first()

    if latest is None:
        raise HTTPException(status_code=404, detail="暂无温湿度数据，请等待 MQTT 消息写入")

    return latest

@router.get("/temperature-humidity/history", response_model=List[TemperatureHumidityResponse])
async def get_temperature_humidity_history(
    hours: int = 24,
    db: Session = Depends(get_db)
):
    since = datetime.utcnow() - timedelta(hours=hours)
    records = db.query(TemperatureHumidity).filter(
        TemperatureHumidity.recorded_at >= since
    ).order_by(TemperatureHumidity.recorded_at.desc()).limit(100).all()

    return records

@router.post("/temperature-humidity", response_model=TemperatureHumidityResponse)
async def create_temperature_humidity(
    data: TemperatureHumidityCreate,
    db: Session = Depends(get_db)
):
    record = TemperatureHumidity(**data.model_dump())
    db.add(record)
    db.commit()
    db.refresh(record)
    return record
