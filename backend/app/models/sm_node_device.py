from sqlalchemy import Column, Integer, String, DateTime, text
from app.core.database import Base


class SmNodeDevice(Base):
    __tablename__ = "sm_node_device"

    number = Column(Integer, primary_key=True, autoincrement=True, comment="主键ID")
    gateway_id = Column(String(50), nullable=False, comment="网关ID")
    sm_sensorid = Column(String(100), comment="传感器ID")
    add_time = Column(DateTime, server_default=text("CURRENT_TIMESTAMP"), comment="添加时间戳")
    remarks = Column(String(500), comment="备注（支持中文）")
