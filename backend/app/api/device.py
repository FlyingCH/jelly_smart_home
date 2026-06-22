import logging
from datetime import datetime
from typing import List, Optional

from fastapi import APIRouter, Depends, HTTPException, Query
from pydantic import BaseModel
from sqlalchemy import text
from sqlalchemy.orm import Session

from app.core.database import get_db, engine
from app.models.ele_node_device import EleNodeDevice
from app.models.gateway_type import GatewayType
from app.models.sm_node_device import SmNodeDevice
from app.models.thui_node_device import ThuiNodeDevice

router = APIRouter()
LOGGER = logging.getLogger(__name__)

DEVICE_TYPE_MAP: dict[str, tuple] = {
    "circuit_breaker": (EleNodeDevice, "el_sensorid"),
    "temperature_humidity": (ThuiNodeDevice, "th_sensorid"),
    "smoke": (SmNodeDevice, "sm_sensorid"),
}


class BindSubDeviceRequest(BaseModel):
    parent_gateway_id: str
    device_type: str
    selected_sensor_ids: List[str]


class UnboundDeviceItem(BaseModel):
    number: int
    gateway_id: str
    sensor_id: Optional[str] = None
    add_time: Optional[datetime] = None
    remarks: Optional[str] = None

    class Config:
        from_attributes = True


class BreakerResponse(BaseModel):
    sensor_id: str
    name: str
    gateway_id: str
    switch: int = 0
    current: float = 0.0
    voltage: float = 0.0
    power: float = 0.0
    temp: float = 0.0
    power_factor: int = 0
    energy: float = 0.0
    recorded_at: Optional[str] = None


@router.get("/device/breakers", response_model=List[BreakerResponse])
async def get_breakers(db: Session = Depends(get_db)):
    """获取所有已绑定的断路器及其最新实时数据"""
    rows = db.query(EleNodeDevice).filter(EleNodeDevice.gateway_id != "undef").all()
    result: List[BreakerResponse] = []
    for row in rows:
        sensor_id = row.el_sensorid
        name = row.remarks or sensor_id or f"断路器{row.number}"
        gw_id = row.gateway_id or ""

        latest = BreakerResponse(
            sensor_id=sensor_id or "",
            name=name,
            gateway_id=gw_id,
        )

        if sensor_id:
            try:
                sql = text(
                    f"SELECT `switch`, `voltage`, `current`, `power`, `dlq_temp`, `power_factor`, `ele_energy`, `add_time` "
                    f"FROM `{sensor_id}` ORDER BY `number` DESC LIMIT 1"
                )
                row_data = db.execute(sql).fetchone()
                if row_data:
                    latest.switch = row_data[0] or 0
                    latest.voltage = float(row_data[1] or 0)
                    latest.current = float(row_data[2] or 0)
                    latest.power = float(row_data[3] or 0)
                    latest.temp = float(row_data[4] or 0)
                    latest.power_factor = int(row_data[5] or 0)
                    latest.energy = float(row_data[6] or 0)
                    latest.recorded_at = str(row_data[7]) if row_data[7] else None
            except Exception:
                pass

        result.append(latest)
    return result


@router.get("/device/get-unbound-subdevices")
async def get_unbound_subdevices(
    device_type: str = Query(...),
    db: Session = Depends(get_db),
):
    info = DEVICE_TYPE_MAP.get(device_type)
    if not info:
        raise HTTPException(status_code=400, detail=f"不支持的子设备类型: {device_type}")
    model_cls, sensorid_attr = info
    rows = db.query(model_cls).filter_by(gateway_id="undef").all()
    data = []
    for row in rows:
        data.append({
            "number": row.number,
            "gateway_id": row.gateway_id,
            "sensor_id": getattr(row, sensorid_attr, None),
            "add_time": str(row.add_time) if row.add_time else None,
            "remarks": row.remarks,
        })
    return {"code": 200, "message": "success", "data": data}


CREATE_TABLE_SQLS: dict[str, str] = {
    "circuit_breaker": (
        "CREATE TABLE IF NOT EXISTS `{sensor_id}` ("
        "  `number` INT NOT NULL AUTO_INCREMENT COMMENT '序号',"
        "  `ele_sensorid` VARCHAR(50) NOT NULL COMMENT '传感器ID',"
        "  `switch` INT COMMENT '开关状态',"
        "  `voltage` FLOAT COMMENT '电压',"
        "  `current` FLOAT COMMENT '电流',"
        "  `power` FLOAT COMMENT '功率',"
        "  `dlq_temp` FLOAT COMMENT '断路器温度',"
        "  `power_factor` FLOAT COMMENT '功率因素',"
        "  `ele_energy` DOUBLE COMMENT '电能',"
        "  `add_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '数据插入时间',"
        "  `mark` VARCHAR(255) COMMENT '备注',"
        "  PRIMARY KEY (`number`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='{sensor_id} 断路器传感器数据表'"
    ),
    "temperature_humidity": (
        "CREATE TABLE IF NOT EXISTS `{sensor_id}` ("
        "  `number` INT NOT NULL AUTO_INCREMENT COMMENT '序号',"
        "  `th_sensorid` VARCHAR(50) NOT NULL COMMENT '传感器ID',"
        "  `temp_data` FLOAT COMMENT '温度数据',"
        "  `humi_data` FLOAT COMMENT '湿度数据',"
        "  `add_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '数据插入时间',"
        "  `mark` VARCHAR(255) COMMENT '备注',"
        "  PRIMARY KEY (`number`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='{sensor_id} 温湿度传感器数据表'"
    ),
    "smoke": (
        "CREATE TABLE IF NOT EXISTS `{sensor_id}` ("
        "  `number` INT NOT NULL AUTO_INCREMENT COMMENT '序号',"
        "  `sm_sensorid` VARCHAR(50) NOT NULL COMMENT '传感器ID',"
        "  `temp_data` FLOAT COMMENT '温度数据',"
        "  `humi_data` FLOAT COMMENT '湿度数据',"
        "  `smoke_data` INT COMMENT '烟雾数值',"
        "  `add_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '数据插入时间',"
        "  `mark` VARCHAR(255) COMMENT '备注',"
        "  PRIMARY KEY (`number`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='{sensor_id} 烟雾传感器数据表'"
    ),
}

UPDATE_SQLS: dict[str, str] = {
    "circuit_breaker": "UPDATE ele_node_device SET gateway_id = :gw_id WHERE el_sensorid IN :ids",
    "temperature_humidity": "UPDATE thui_node_device SET gateway_id = :gw_id WHERE th_sensorid IN :ids",
    "smoke": "UPDATE sm_node_device SET gateway_id = :gw_id WHERE sm_sensorid IN :ids",
}


@router.post("/device/batch-bind-subdevices")
async def batch_bind_subdevices(
    req: BindSubDeviceRequest,
    db: Session = Depends(get_db),
):
    device_type = req.device_type
    parent_gateway_id = req.parent_gateway_id
    selected_ids = req.selected_sensor_ids

    if not selected_ids:
        raise HTTPException(status_code=400, detail="请至少选择一个子设备")

    info = DEVICE_TYPE_MAP.get(device_type)
    if not info:
        raise HTTPException(status_code=400, detail=f"不支持的子设备类型: {device_type}")

    if device_type not in CREATE_TABLE_SQLS:
        raise HTTPException(status_code=400, detail=f"不支持的子设备类型: {device_type}")

    model_cls, sensorid_attr = info

    # 校验 parent_gateway_id
    gw = db.query(GatewayType).filter_by(gateway_id=parent_gateway_id).first()
    if not gw:
        raise HTTPException(status_code=404, detail=f"网关 {parent_gateway_id} 不存在")

    # 校验所有 selected_sensor_ids 存在且 gateway_id='undef'
    existing = db.query(model_cls).filter(
        getattr(model_cls, sensorid_attr).in_(selected_ids),
        model_cls.gateway_id == "undef",
    ).all()
    existing_ids = {getattr(r, sensorid_attr) for r in existing}
    missing = [sid for sid in selected_ids if sid not in existing_ids]
    if missing:
        raise HTTPException(
            status_code=400,
            detail=f"以下子设备不存在或已绑定: {', '.join(missing)}",
        )

    try:
        # 步骤 1：更新子设备表的 gateway_id
        update_sql = UPDATE_SQLS[device_type]
        db.execute(text(update_sql), {"gw_id": parent_gateway_id, "ids": tuple(selected_ids)})

        # 步骤 2：递增父网关的子设备计数
        bind_count = len(selected_ids)
        db.execute(
            text("UPDATE gateway_type SET sub_devices = sub_devices + :cnt WHERE gateway_id = :gw_id"),
            {"cnt": bind_count, "gw_id": parent_gateway_id},
        )

        # 步骤 3：为每个子设备创建专属数据表
        create_sql = CREATE_TABLE_SQLS[device_type]
        with engine.connect() as raw_conn:
            for sid in selected_ids:
                raw_conn.execute(text(create_sql.format(sensor_id=sid)))
            raw_conn.execute(text("COMMIT"))

        db.commit()
        LOGGER.info(
            "批量绑定完成: gateway=%s, type=%s, count=%d, ids=%s",
            parent_gateway_id, device_type, bind_count, selected_ids,
        )
        return {
            "code": 200,
            "message": f"绑定成功，共绑定{bind_count}个子设备",
            "data": {
                "success_count": bind_count,
                "failed_count": 0,
                "failed_sensor_ids": [],
            },
        }
    except Exception:
        db.rollback()
        LOGGER.exception("批量绑定子设备失败")
        raise HTTPException(status_code=500, detail="部分子设备绑定失败，请重试")
