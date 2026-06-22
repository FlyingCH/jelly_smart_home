import json
import logging
from datetime import datetime
from typing import Any, Dict, Optional

import paho.mqtt.client as mqtt
from sqlalchemy import text

from app.core.config import MqttSettings
from app.core.database import SessionLocal, engine, MYSQL_DATABASE
from app.models.electricity import ElectricityUsage
from app.models.temperature_humidity import TemperatureHumidity
from app.models.smoke import Smoke
from app.models.submanagement import SubManagement

LOGGER = logging.getLogger(__name__)


class SmartHomeMqttClient:
    def __init__(self, settings: MqttSettings) -> None:
        self.settings = settings
        self.client = mqtt.Client(client_id=settings.client_id, clean_session=True)
        self._configure_client()

    def _configure_client(self) -> None:
        if self.settings.username:
            self.client.username_pw_set(
                username=self.settings.username,
                password=self.settings.password or None,
            )

        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message

    def start(self) -> None:
        LOGGER.info(
            "Connecting MQTT broker %s:%s",
            self.settings.broker_host,
            self.settings.broker_port,
        )
        self.client.connect_async(
            host=self.settings.broker_host,
            port=self.settings.broker_port,
            keepalive=self.settings.keepalive,
        )
        self.client.loop_start()

    def stop(self) -> None:
        try:
            self.client.loop_stop()
            self.client.disconnect()
        except Exception as exc:
            LOGGER.warning("Failed to stop MQTT client cleanly: %s", exc)

    def _load_subscription_topics(self):
        db = SessionLocal()
        try:
            records = db.query(SubManagement).filter(
                SubManagement.device_type == "gateway",
                SubManagement.mqtt_topic.isnot(None),
                SubManagement.mqtt_topic != "",
            ).all()
            topics = [r.mqtt_topic for r in records]
            return topics
        except Exception as exc:
            LOGGER.error("Failed to load subscription topics from database: %s", exc)
            return []
        finally:
            db.close()

    def subscribe_topic(self, topic: str) -> None:
        self.client.subscribe(topic, qos=1)
        LOGGER.info("Dynamically subscribed to topic: %s", topic)

    def unsubscribe_topic(self, topic: str) -> None:
        self.client.unsubscribe(topic)
        LOGGER.info("Unsubscribed from topic: %s", topic)

    def _on_connect(
        self,
        client: mqtt.Client,
        userdata: Any,
        flags: Dict[str, Any],
        rc: int,
    ) -> None:
        del userdata, flags
        if rc != 0:
            LOGGER.error("MQTT connection failed with code %s", rc)
            return

        LOGGER.info("MQTT broker connected successfully")

        topics = self._load_subscription_topics()
        if not topics:
            LOGGER.info("无网关设备订阅主题，跳过批量订阅")
            return

        for topic in topics:
            client.subscribe(topic, qos=1)
            LOGGER.info("Subscribed to topic: %s", topic)

    def _on_disconnect(
        self,
        client: mqtt.Client,
        userdata: Any,
        rc: int,
    ) -> None:
        del client, userdata
        if rc != 0:
            LOGGER.warning("MQTT disconnected unexpectedly, rc=%s", rc)
            return

        LOGGER.info("MQTT client disconnected")

    def _on_message(     # 处理MQTT消息
        self,
        client: mqtt.Client,
        userdata: Any,
        message: mqtt.MQTTMessage,
    ) -> None:
        del client, userdata
        topic = message.topic

        try:
            raw = json.loads(message.payload.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as exc:
            LOGGER.error("Invalid MQTT payload on topic %s: %s", topic, exc)
            return

        print(f"\n[MQTT] 收到主题 [{topic}] 的消息: {json.dumps(raw, ensure_ascii=False)}")

        devcd = raw.get("devcd")        # 网关ID
        devtype = raw.get("devtype")    # 设备类型
        node_id = raw.get("nodeid")     # 节点ID
        data = raw.get("data", {})     # 实际数据
        LOGGER.info("devcd=%s, devtype=%s, nodeid=%s", devcd, devtype, node_id)

        # 判断传感器类型(目前的三种传感器：温湿度、电力、烟雾)
        if devtype == "electricity_sensor":       # 电力传感器
            self._save_electricity(data, node_id, devcd)
        elif devtype == "smoke_sensor":           # 烟雾床干起
            self._save_smoke(data, node_id, devcd)
        elif devtype == "temp_humi_sensor":       # 温湿度传感器
            self._save_temperature_humidity(data, node_id, devcd)
        else:          # 没有任何的传感器类型
            LOGGER.warning("Unrecognized devtype '%s' in MQTT message", devtype)

    def _save_temperature_humidity(self, payload: Dict[str, Any], node_id: Optional[str], devcd: Optional[str] = None) -> None:
        try:
            record = TemperatureHumidity(
                temperature=float(self._read_required(payload, "temp", "temperature")),
                humidity=float(self._read_required(payload, "humi", "humidity")),
                node_id=node_id,
                device_id=devcd or self._read_optional_str(payload, "device_id", "deviceId"),
                recorded_at=self._parse_timestamp(payload),
            )
        except (KeyError, TypeError, ValueError) as exc:
            LOGGER.error("Invalid temperature payload %s, error: %s", payload, exc)
            return

        self._persist(record)

    # 电力传感器消息处理函数
    def _save_electricity(self, payload: Dict[str, Any], node_id: Optional[str], devcd: Optional[str] = None) -> None:
        table_name = node_id  # 如 "elsensor_33"
        if not table_name:
            LOGGER.error("电力传感器消息缺少 nodeid，无法确定目标表")
            return

        # 检查数据表是否存在
        try:
            with engine.connect() as conn:
                result = conn.execute(
                    text("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = :db AND table_name = :tbl"),
                    {"db": MYSQL_DATABASE, "tbl": table_name},
                )
                exists = result.scalar() > 0
        except Exception as exc:
            LOGGER.error("查询数据表 %s 是否存在时出错: %s", table_name, exc)
            return

        if not exists:
            LOGGER.error("数据表 '%s' 不存在，请先创建该表", table_name)
            return

        # 解析字段并插入数据
        try:
            sw = int(payload.get("sw", 0))
            current = float(payload.get("current", 0))
            voltage = float(payload.get("voltage", 0))
            power = float(payload.get("power", 0))
            temp = float(payload.get("temp", 0))
            factor = float(payload.get("factor", 0))
            energy = float(payload.get("energy", 0))
        except (TypeError, ValueError) as exc:
            LOGGER.error("电力传感器数据解析失败: %s, error: %s", payload, exc)
            return

        try:
            with engine.connect() as conn:
                conn.execute(
                    text(
                        f"INSERT INTO `{table_name}` "
                        "(ele_sensorid, `switch`, voltage, current, power, dlq_temp, power_factor, ele_energy) "
                        "VALUES (:sid, :sw, :voltage, :current, :power, :temp, :factor, :energy)"
                    ),
                    {
                        "sid": node_id,
                        "sw": sw,
                        "voltage": voltage,
                        "current": current,
                        "power": power,
                        "temp": temp,
                        "factor": factor,
                        "energy": energy,
                    },
                )
                conn.execute(text("COMMIT"))
            LOGGER.info("电力数据已写入表 %s: power=%.2f, voltage=%.2f", table_name, power, voltage)
        except Exception as exc:
            LOGGER.error("写入数据表 %s 失败: %s", table_name, exc)

    def _save_smoke(self, payload: Dict[str, Any], node_id: Optional[str], devcd: Optional[str] = None) -> None:
        try:
            record = Smoke(
                smoke=float(self._read_required(payload, "smoke")),
                temperature=self._read_optional_float(payload, "temp", "temperature"),
                humidity=self._read_optional_float(payload, "humi", "humidity"),
                node_id=node_id,
                device_id=devcd or self._read_optional_str(payload, "device_id", "deviceId"),
                recorded_at=self._parse_timestamp(payload),
            )
        except (KeyError, TypeError, ValueError) as exc:
            LOGGER.error("Invalid smoke payload %s, error: %s", payload, exc)
            return

        self._persist(record)


    # 保存数据到数据库
    def _persist(self, record: Any) -> None:  
        db = SessionLocal()
        try:
            db.add(record)
            db.commit()
            LOGGER.info("Saved MQTT data into table %s", record.__tablename__)
        except Exception as exc:
            db.rollback()
            LOGGER.exception("Failed to persist MQTT data: %s", exc)
        finally:
            db.close()

    @staticmethod
    def _read_required(payload: Dict[str, Any], *keys: str) -> Any:
        for key in keys:
            if key in payload:
                return payload[key]
        raise KeyError(f"Missing required keys: {keys}")

    @staticmethod
    def _read_optional_str(payload: Dict[str, Any], *keys: str) -> Optional[str]:
        for key in keys:
            value = payload.get(key)
            if value is not None:
                return str(value)
        return None

    @staticmethod
    def _read_optional_float(payload: Dict[str, Any], *keys: str) -> Optional[float]:
        for key in keys:
            value = payload.get(key)
            if value is not None:
                return float(value)
        return None

    @staticmethod
    def _parse_timestamp(payload: Dict[str, Any]) -> datetime:
        raw_value = payload.get("recorded_at") or payload.get("recordedAt")
        raw_value = raw_value or payload.get("timestamp")
        if raw_value is None:
            return datetime.utcnow()

        if isinstance(raw_value, (int, float)):
            return datetime.fromtimestamp(raw_value)

        if isinstance(raw_value, str):
            normalized = raw_value.replace("Z", "+00:00")
            return datetime.fromisoformat(normalized)

        raise ValueError("Unsupported timestamp format")
