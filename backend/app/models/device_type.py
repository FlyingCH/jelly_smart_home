from sqlalchemy import Column, Integer, String
from app.core.database import Base


class DeviceType(Base):
    __tablename__ = "device_type"

    number = Column(Integer, primary_key=True, autoincrement=True)
    device_type = Column(String(255), nullable=False)
    quantity = Column(Integer, nullable=False, default=0)
    mark = Column(String(500), nullable=True)
