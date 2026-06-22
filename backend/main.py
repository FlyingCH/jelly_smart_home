import logging
from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from app.api import dashboard, temperature_humidity, electricity, smoke, map, sensors, auth, device
from app.core.config import mqtt_settings
from app.core.database import engine, init_database, SessionLocal
from app.models import SubManagement, DeviceType, GatewayType, UserInfo, EleNodeDevice, ThuiNodeDevice, SmNodeDevice
from app.services.mqtt_service import SmartHomeMqttClient

logging.basicConfig(level=logging.INFO)
LOGGER = logging.getLogger(__name__)

mqtt_client = SmartHomeMqttClient(mqtt_settings)


@asynccontextmanager
async def lifespan(app: FastAPI):
    init_database()
    
    # 创建初始化的数据表
    DeviceType.__table__.create(bind=engine, checkfirst=True)
    GatewayType.__table__.create(bind=engine, checkfirst=True)
    UserInfo.__table__.create(bind=engine, checkfirst=True)
    SubManagement.__table__.create(bind=engine, checkfirst=True)   # 可以不需要创建该表
    EleNodeDevice.__table__.create(bind=engine, checkfirst=True)
    ThuiNodeDevice.__table__.create(bind=engine, checkfirst=True)
    SmNodeDevice.__table__.create(bind=engine, checkfirst=True)




    with SessionLocal() as db:
        admin = db.query(UserInfo).filter_by(user_id=1).first()
        if not admin:
            db.add(
                UserInfo(
                    user_id=1,
                    account_id="admin",
                    passwd="85459939",
                    authorith="admin",
                )
            )
            db.commit()
            LOGGER.info("默认管理员用户已创建: account_id=admin")

        device_types = [
            ("gataway_device", "网关设备"),
            ("gateway_node_device", "网关子设备"),
            ("directly_connected_device", "直连设备"),
        ]
        for dt_name, dt_mark in device_types:
            existing_dt = db.query(DeviceType).filter_by(device_type=dt_name).first()
            if not existing_dt:
                db.add(DeviceType(device_type=dt_name, quantity=0, mark=dt_mark))
        db.commit()
        LOGGER.info("device_type 基础记录已就绪")

    LOGGER.info("Database tables are ready")
    app.state.mqtt_client = mqtt_client
    mqtt_client.start()
    try:
        yield
    finally:
        mqtt_client.stop()


app = FastAPI(title="我的智能家 API", version="1.0.0", lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(dashboard.router, prefix="/api", tags=["仪表盘"])
app.include_router(temperature_humidity.router, prefix="/api", tags=["温湿度"])
app.include_router(electricity.router, prefix="/api", tags=["用电数据"])
app.include_router(smoke.router, prefix="/api", tags=["烟雾监测"])
app.include_router(sensors.router, prefix="/api", tags=["传感器管理"])
app.include_router(device.router, prefix="/api", tags=["设备管理"])
app.include_router(map.router, prefix="/api", tags=["地图瓦片"])
app.include_router(auth.router, prefix="/api", tags=["用户认证"])

@app.get("/")
async def root():
    return {"message": "我的智能家 API", "version": "1.0.0"}

@app.get("/api/health")
async def health_check():
    return {"status": "healthy"}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
