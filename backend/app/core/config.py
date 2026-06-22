import os
from dataclasses import dataclass


@dataclass(frozen=True)
class MqttSettings:
    broker_host: str = os.getenv("MQTT_BROKER_HOST", "192.168.31.213")
    broker_port: int = int(os.getenv("MQTT_BROKER_PORT", "1883"))
    username: str = os.getenv("MQTT_USERNAME", "")
    password: str = os.getenv("MQTT_PASSWORD", "")
    client_id: str = os.getenv("MQTT_CLIENT_ID", "jelly-smart-home-iot")
    keepalive: int = int(os.getenv("MQTT_KEEPALIVE", "60"))


mqtt_settings = MqttSettings()
