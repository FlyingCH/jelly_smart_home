from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session
from typing import List
from datetime import datetime, timedelta
from app.core.database import get_db
from app.models.smoke import Smoke
from app.schemas import SmokeResponse, SmokeCreate

router = APIRouter()


@router.get("/smoke/latest", response_model=SmokeResponse)
async def get_latest_smoke(db: Session = Depends(get_db)):
    latest = db.query(Smoke).order_by(Smoke.recorded_at.desc()).first()

    if latest is None:
        raise HTTPException(status_code=404, detail="暂无烟雾数据，请等待 MQTT 消息写入")

    return latest


@router.get("/smoke/history", response_model=List[SmokeResponse])
async def get_smoke_history(
    hours: int = 24,
    db: Session = Depends(get_db)
):
    since = datetime.utcnow() - timedelta(hours=hours)
    records = db.query(Smoke).filter(
        Smoke.recorded_at >= since
    ).order_by(Smoke.recorded_at.desc()).limit(100).all()

    return records


@router.post("/smoke", response_model=SmokeResponse)
async def create_smoke(
    data: SmokeCreate,
    db: Session = Depends(get_db)
):
    record = Smoke(**data.model_dump())
    db.add(record)
    db.commit()
    db.refresh(record)
    return record
