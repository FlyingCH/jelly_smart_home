from sqlalchemy import Column, Integer, Float, String, DateTime
from datetime import datetime
from app.core.database import Base

class ElectricityUsage(Base):
    __tablename__ = "electricity_usage"

    id = Column(Integer, primary_key=True, index=True)
    power = Column(Float, nullable=False)
    daily_usage = Column(Float, nullable=False)
    node_id = Column(String(255), nullable=True)
    device_id = Column(String(255), nullable=True)
    recorded_at = Column(DateTime, default=datetime.utcnow)
