from sqlalchemy import Column, Integer, String, DateTime
from app.core.database import Base


class GatewayType(Base):
    __tablename__ = "gateway_type"

    number = Column(Integer, primary_key=True, autoincrement=True)
    gateway_type = Column(String(255), nullable=False)
    gateway_id = Column(String(255), nullable=False)
    mqtt_topic = Column(String(64), nullable=False)
    sub_devices = Column(Integer, nullable=False, default=0)
    add_time = Column(DateTime, nullable=False)
    remarks = Column(String(500), nullable=True)
