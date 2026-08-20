import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart", "wifi", "api"]

CONF_SLAVE_ADDRESS = "slave_address"

fsc2a_ns = cg.esphome_ns.namespace("fsc2a")
Fsc2aController = fsc2a_ns.class_(
    "Fsc2aController", cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Fsc2aController),
            cv.Optional(CONF_SLAVE_ADDRESS, default=1): cv.int_range(min=1, max=247),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_slave(config[CONF_SLAVE_ADDRESS]))

    cg.add_library("M5Unified", None)
