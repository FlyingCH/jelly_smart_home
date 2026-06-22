import logging

from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy import text
from sqlalchemy.orm import Session
from typing import List, Optional
from datetime import datetime, timedelta
from app.core.database import get_db, engine
from app.models.electricity import ElectricityUsage
from app.models.ele_node_device import EleNodeDevice
from app.schemas import ElectricityResponse, ElectricityCreate

router = APIRouter()
LOGGER = logging.getLogger(__name__)

@router.get("/electricity/latest", response_model=ElectricityResponse)
async def get_latest_electricity(db: Session = Depends(get_db)):
    latest = db.query(ElectricityUsage).order_by(ElectricityUsage.recorded_at.desc()).first()

    if latest is None:
        raise HTTPException(status_code=404, detail="暂无电力数据，请等待 MQTT 消息写入")

    return latest

@router.get("/electricity/history", response_model=List[ElectricityResponse])
async def get_electricity_history(
    hours: int = 24,
    db: Session = Depends(get_db)
):
    since = datetime.utcnow() - timedelta(hours=hours)
    records = db.query(ElectricityUsage).filter(
        ElectricityUsage.recorded_at >= since
    ).order_by(ElectricityUsage.recorded_at.desc()).limit(100).all()

    return records

@router.post("/electricity", response_model=ElectricityResponse)
async def create_electricity(
    data: ElectricityCreate,
    db: Session = Depends(get_db)
):
    record = ElectricityUsage(**data.model_dump())
    db.add(record)
    db.commit()
    db.refresh(record)
    return record


@router.get("/electric/device-real-time-list")
async def get_device_real_time_list(
    db: Session = Depends(get_db),
):
    """获取所有已绑定断路器的实时数据"""
    rows = db.query(EleNodeDevice).filter(EleNodeDevice.gateway_id != "undef").all()
    if not rows:
        return {"code": 200, "message": "success", "data": []}

    result = []
    with engine.connect() as raw_conn:
        for row in rows:
            sensor_id = row.el_sensorid
            device: dict = {
                "deviceId": sensor_id,
                "deviceName": row.remarks or sensor_id,
                "switchStatus": -1,
                "current": 0.0,
                "voltage": 0.0,
                "power": 0.0,
                "temperature": 0.0,
                "powerFactor": 0.0,
                "totalEnergy": 0.0,
                "dataTime": None,
            }
            try:
                raw = raw_conn.execute(
                    text(
                        f"SELECT * FROM `{sensor_id}` "
                        "ORDER BY add_time DESC LIMIT 1"
                    )
                ).fetchone()
                if raw:
                    raw_dict = dict(raw._mapping)
                    device["switchStatus"] = raw_dict.get("switch", 0)
                    device["current"] = float(raw_dict.get("current") or 0)
                    device["voltage"] = float(raw_dict.get("voltage") or 0)
                    device["power"] = float(raw_dict.get("power") or 0)
                    device["temperature"] = float(raw_dict.get("dlq_temp") or 0)
                    device["powerFactor"] = float(raw_dict.get("power_factor") or 0)
                    device["totalEnergy"] = float(raw_dict.get("ele_energy") or 0) / 1000.0
                    at = raw_dict.get("add_time")
                    device["dataTime"] = str(at) if at else None
            except Exception:
                LOGGER.warning("查询子设备表 %s 失败，可能不存在", sensor_id)

            result.append(device)

    return {"code": 200, "message": "success", "data": result}
