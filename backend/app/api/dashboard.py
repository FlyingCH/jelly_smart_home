from fastapi import APIRouter, Depends
from sqlalchemy.orm import Session
from app.core.database import get_db
from app.models.temperature_humidity import TemperatureHumidity
from app.models.electricity import ElectricityUsage
from app.models.smoke import Smoke
from app.schemas import DashboardSummary

router = APIRouter()


@router.get("/dashboard/summary", response_model=DashboardSummary)
async def get_dashboard_summary(db: Session = Depends(get_db)):
    latest_th = db.query(TemperatureHumidity).order_by(TemperatureHumidity.recorded_at.desc()).first()
    latest_el = db.query(ElectricityUsage).order_by(ElectricityUsage.recorded_at.desc()).first()
    latest_smoke = db.query(Smoke).order_by(Smoke.recorded_at.desc()).first()

    return DashboardSummary(
        temperature=latest_th.temperature if latest_th else 0.0,
        humidity=latest_th.humidity if latest_th else 0.0,
        power=latest_el.power if latest_el else 0.0,
        daily_usage=latest_el.daily_usage if latest_el else 0.0,
        smoke=latest_smoke.smoke if latest_smoke else None,
    )
