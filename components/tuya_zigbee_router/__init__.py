import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.automation import register_action, Action
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
CONF_ZIGBEE_ROUTER = "tuya_zigbee_router"

# ========================================================================
# TUYA ZIGBEE ROUTER COMPONENT
# ========================================================================

zigbee_router_ns = cg.esphome_ns.namespace(CONF_ZIGBEE_ROUTER)
ZigbeeRouterComponent = zigbee_router_ns.class_(
    "ZigbeeRouterComponent", cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ZigbeeRouterComponent),
        })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    uart_component = await cg.get_variable(config[uart.CONF_UART_ID])
    var = cg.new_Pvariable(config[cv.GenerateID()], uart_component)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)


# ============================================================================
# ACTIONS
# ============================================================================

ResetAction = zigbee_router_ns.class_("ResetAction", Action)
LeaveAndRejoinAction = zigbee_router_ns.class_("LeaveAndRejoinAction", Action)

RESET_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(ZigbeeRouterComponent),
    }
)
LEAVE_AND_REJOIN_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(ZigbeeRouterComponent),
    }
)


# Enregistrer l'action reset
@register_action("tuya_zigbee_router.reset", ResetAction, RESET_ACTION_SCHEMA)
async def zigbee_router_reset_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    component = await cg.get_variable(config[CONF_ID])
    cg.add(var.set_parent(component))
    return var

@register_action(
    "tuya_zigbee_router.leave_and_rejoin",
    LeaveAndRejoinAction,
    LEAVE_AND_REJOIN_ACTION_SCHEMA,
)
async def zigbee_router_leave_and_rejoin_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    component = await cg.get_variable(config[CONF_ID])
    cg.add(var.set_parent(component))
    return var