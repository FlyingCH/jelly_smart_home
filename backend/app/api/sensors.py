import logging
from datetime import datetime
from typing import List

from fastapi import APIRouter, Depends, HTTPException, Request
from sqlalchemy.orm import Session

from app.core.database import get_db
from app.models.device_type import DeviceType
from app.models.ele_node_device import EleNodeDevice
from app.models.gateway_type import GatewayType
from app.models.sm_node_device import SmNodeDevice
from app.models.submanagement import SubManagement
from app.models.thui_node_device import ThuiNodeDevice
from app.schemas import SensorCreate, SensorResponse, SensorUpdate, NodeDeviceResponse

router = APIRouter()
LOGGER = logging.getLogger(__name__)

DEVICE_TYPE_MAP = {
    "gateway": "gataway_device",
    "gateway_sub": "gateway_node_device",
    "direct": "directly_connected_device",
}

GATEWAY_SUB_TYPE_MAP = {
    "breaker_sub": (EleNodeDevice, "el_sensorid"),
    "temp_humi_sub": (ThuiNodeDevice, "th_sensorid"),
    "smoke_sub": (SmNodeDevice, "sm_sensorid"),
}


def _incr_device_type_quantity(db: Session, device_type: str, delta: int):
    dt_key = DEVICE_TYPE_MAP.get(device_type)
    if not dt_key:
        return
    record = db.query(DeviceType).filter_by(device_type=dt_key).first()
    if record:
        record.quantity = max(0, record.quantity + delta)


def _add_gateway_sub_device(db: Session, sensor_type: str, sensor_id: str, sensor_location: str | None):
    sub_info = GATEWAY_SUB_TYPE_MAP.get(sensor_type)
    if not sub_info:
        return
    model_cls, sensorid_attr = sub_info
    sub_record = model_cls(
        gateway_id="undef",
        add_time=datetime.now(),
        remarks=sensor_location,
        **{sensorid_attr: sensor_id},
    )
    db.add(sub_record)
    LOGGER.info("网关子设备已记录到%s: sensor_id=%s", model_cls.__tablename__, sensor_id)


def _delete_gateway_sub_device(db: Session, sensor_type: str, sensor_id: str):
    sub_info = GATEWAY_SUB_TYPE_MAP.get(sensor_type)
    if not sub_info:
        return
    model_cls, sensorid_attr = sub_info
    rows = db.query(model_cls).filter(getattr(model_cls, sensorid_attr) == sensor_id).delete()
    if rows:
        LOGGER.info("网关子设备记录已从%s删除: sensor_id=%s", model_cls.__tablename__, sensor_id)


@router.post("/sensors", response_model=SensorResponse, status_code=201)
async def add_sensor(data: SensorCreate, request: Request, db: Session = Depends(get_db)):
    existing = db.query(SubManagement).filter_by(sensor_id=data.sensor_id).first()
    if existing:
        raise HTTPException(status_code=409, detail=f"传感器 {data.sensor_id} 已存在")

    record = SubManagement(
        sensor_id=data.sensor_id,
        sensor_type=data.sensor_type,
        device_type=data.device_type,
        mqtt_topic=data.mqtt_topic,
        sensor_location=data.sensor_location,
    )
    db.add(record)

    _incr_device_type_quantity(db, data.device_type, 1)

    if data.device_type == "gateway":
        gw_record = GatewayType(
            gateway_type=DEVICE_TYPE_MAP[data.device_type],
            gateway_id=data.sensor_id,
            mqtt_topic=data.mqtt_topic,
            sub_devices=0,
            add_time=datetime.now(),
            remarks=data.sensor_location,
        )
        db.add(gw_record)
        LOGGER.info("设备已记录到gateway_type: sensor_id=%s, type=%s",
                    data.sensor_id, DEVICE_TYPE_MAP[data.device_type])

    if data.device_type == "gateway_sub":
        _add_gateway_sub_device(db, data.sensor_type, data.sensor_id, data.sensor_location)

    db.commit()
    db.refresh(record)

    mqtt_client = request.app.state.mqtt_client
    if data.device_type == "gateway":
        mqtt_client.subscribe_topic(data.mqtt_topic)
        LOGGER.info("Sensor %s added (gateway), subscribed to topic: %s", data.sensor_id, data.mqtt_topic)
    else:
        LOGGER.info("Sensor %s added (%s), skipped MQTT subscription", data.sensor_id, data.device_type)
    return record


@router.get("/sensors", response_model=List[SensorResponse])
async def list_sensors(db: Session = Depends(get_db)):
    return db.query(SubManagement).all()


@router.put("/sensors/{sensor_id}", response_model=SensorResponse)
async def update_sensor(sensor_id: str, data: SensorUpdate, request: Request, db: Session = Depends(get_db)):
    record = db.query(SubManagement).filter_by(sensor_id=sensor_id).first()
    if not record:
        raise HTTPException(status_code=404, detail=f"传感器 {sensor_id} 不存在")

    mqtt_client = request.app.state.mqtt_client
    old_topic = record.mqtt_topic
    old_device_type = record.device_type
    old_sensor_type = record.sensor_type

    if data.sensor_type is not None:
        record.sensor_type = data.sensor_type
    if data.device_type is not None:
        record.device_type = data.device_type
    if data.mqtt_topic is not None:
        record.mqtt_topic = data.mqtt_topic
    if data.sensor_location is not None:
        record.sensor_location = data.sensor_location

    new_device_type = record.device_type
    new_topic = record.mqtt_topic

    if old_device_type != new_device_type:
        _incr_device_type_quantity(db, old_device_type, -1)
        _incr_device_type_quantity(db, new_device_type, 1)

        old_is_gw = old_device_type in ("gateway", "gateway_sub")
        new_is_gw = new_device_type in ("gateway", "gateway_sub")

        if old_is_gw and not new_is_gw:
            db.query(GatewayType).filter_by(gateway_id=sensor_id).delete()
            LOGGER.info("gateway_type 记录已删除: gateway_id=%s", sensor_id)
        elif not old_is_gw and new_is_gw:
            gw_record = GatewayType(
                gateway_type=DEVICE_TYPE_MAP[new_device_type],
                gateway_id=sensor_id,
                mqtt_topic=record.mqtt_topic,
                sub_devices=0,
                add_time=datetime.now(),
                remarks=record.sensor_location,
            )
            db.add(gw_record)
            LOGGER.info("gateway_type 记录已添加: gateway_id=%s", sensor_id)
        elif old_is_gw and new_is_gw:
            gw_record = db.query(GatewayType).filter_by(gateway_id=sensor_id).first()
            if gw_record:
                gw_record.gateway_type = DEVICE_TYPE_MAP[new_device_type]
                gw_record.mqtt_topic = record.mqtt_topic
                if record.sensor_location is not None:
                    gw_record.remarks = record.sensor_location
                LOGGER.info("gateway_type 记录已更新: gateway_id=%s", sensor_id)

    new_sensor_type = record.sensor_type

    if old_device_type == "gateway_sub" and new_device_type != "gateway_sub":
        _delete_gateway_sub_device(db, old_sensor_type, sensor_id)
    elif old_device_type != "gateway_sub" and new_device_type == "gateway_sub":
        _add_gateway_sub_device(db, new_sensor_type, sensor_id, record.sensor_location)
    elif old_device_type == "gateway_sub" and new_device_type == "gateway_sub" and old_sensor_type != new_sensor_type:
        _delete_gateway_sub_device(db, old_sensor_type, sensor_id)
        _add_gateway_sub_device(db, new_sensor_type, sensor_id, record.sensor_location)

    db.commit()
    db.refresh(record)

    if old_device_type == "gateway":
        mqtt_client.unsubscribe_topic(old_topic)
    if new_device_type == "gateway":
        mqtt_client.subscribe_topic(new_topic)

    LOGGER.info("Sensor %s updated (device_type: %s -> %s, topic: %s -> %s)",
                sensor_id, old_device_type, new_device_type, old_topic, new_topic)
    return record


@router.delete("/sensors/{sensor_id}")
async def delete_sensor(sensor_id: str, request: Request, db: Session = Depends(get_db)):
    record = db.query(SubManagement).filter_by(sensor_id=sensor_id).first()
    if not record:
        raise HTTPException(status_code=404, detail=f"传感器 {sensor_id} 不存在")

    old_topic = record.mqtt_topic
    old_device_type = record.device_type
    old_sensor_type = record.sensor_type

    db.delete(record)
    _incr_device_type_quantity(db, old_device_type, -1)

    if old_device_type in ("gateway", "gateway_sub"):
        db.query(GatewayType).filter_by(gateway_id=sensor_id).delete()
        LOGGER.info("gateway_type 记录已删除: gateway_id=%s", sensor_id)

    if old_device_type == "gateway_sub":
        _delete_gateway_sub_device(db, old_sensor_type, sensor_id)

    db.commit()

    mqtt_client = request.app.state.mqtt_client
    if old_device_type == "gateway":
        mqtt_client.unsubscribe_topic(old_topic)
        LOGGER.info("Sensor %s deleted (gateway), unsubscribed from topic: %s", sensor_id, old_topic)
    else:
        LOGGER.info("Sensor %s deleted (%s), skipped MQTT unsubscription", sensor_id, old_device_type)
    return {"detail": f"传感器 {sensor_id} 已删除"}


_SUB_TYPE_MAP = {
    "breaker_sub": (EleNodeDevice, "el_sensorid"),
    "temp_humi_sub": (ThuiNodeDevice, "th_sensorid"),
    "smoke_sub": (SmNodeDevice, "sm_sensorid"),
}


@router.get("/gateway-sub-devices", response_model=List[NodeDeviceResponse])
async def list_gateway_sub_devices(sub_type: str, gateway_id: str | None = None, db: Session = Depends(get_db)):
    info = _SUB_TYPE_MAP.get(sub_type)
    if not info:
        raise HTTPException(status_code=400, detail=f"不支持的子设备类型: {sub_type}")
    model_cls, sensorid_attr = info
    query = db.query(model_cls)
    if gateway_id:
        query = query.filter_by(gateway_id=gateway_id)
    rows = query.all()
    return [
        NodeDeviceResponse(
            number=row.number,
            gateway_id=row.gateway_id,
            sensor_id=getattr(row, sensorid_attr, None),
            add_time=row.add_time,
            remarks=row.remarks,
        )
        for row in rows
    ]
