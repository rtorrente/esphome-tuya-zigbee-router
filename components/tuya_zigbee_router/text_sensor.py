import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from . import zigbee_router_ns, ZigbeeRouterComponent

CONF_CONNECTION_STATUS = "zigbee_status"
CONF_ZIGBEE_ROUTER = "tuya_zigbee_router"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ZIGBEE_ROUTER): cv.use_id(ZigbeeRouterComponent),
        cv.Required(CONF_CONNECTION_STATUS): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    server = await cg.get_variable(config[CONF_ZIGBEE_ROUTER])

    sensor = await text_sensor.new_text_sensor(config[CONF_CONNECTION_STATUS])
    cg.add(server.set_connection_status_sensor(sensor))