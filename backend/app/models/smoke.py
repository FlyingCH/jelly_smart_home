from sqlalchemy import Column, Integer, Float, String, DateTime
from datetime import datetime
from app.core.database import Base


class Smoke(Base):
    __tablename__ = "smoke"

    id = Column(Integer, primary_key=True, index=True)
    smoke = Column(Float, nullable=False)
    temperature = Column(Float, nullable=True)
    humidity = Column(Float, nullable=True)
    node_id = Column(String(255), nullable=True)
    device_id = Column(String(255), nullable=True)
    recorded_at = Column(DateTime, default=datetime.utcnow)
