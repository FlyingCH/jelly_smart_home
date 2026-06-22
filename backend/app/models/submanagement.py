from sqlalchemy import Column, String
from app.core.database import Base


class SubManagement(Base):
    __tablename__ = "submanagement"

    sensor_id = Column(String(255), primary_key=True)
    sensor_type = Column(String(100), nullable=False)
    device_type = Column(String(50), nullable=False, default="direct")
    mqtt_topic = Column(String(500), nullable=False)
    sensor_location = Column(String(255), nullable=True)
