from sqlalchemy import Column, Integer, Float, String, DateTime
from datetime import datetime
from app.core.database import Base

class TemperatureHumidity(Base):
    __tablename__ = "temperature_humidity"

    id = Column(Integer, primary_key=True, index=True)
    temperature = Column(Float, nullable=False)
    humidity = Column(Float, nullable=False)
    node_id = Column(String(255), nullable=True)
    device_id = Column(String(255), nullable=True)
    recorded_at = Column(DateTime, default=datetime.utcnow)
