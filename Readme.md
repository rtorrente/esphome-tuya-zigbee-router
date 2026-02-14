# ESPHome Tuya Zigbee Router

A custom ESPHome component for integrating and controlling a Tuya Zigbee Router via UART communication.

## Description
This project provides a custom ESPHome component enabling communication with a Tuya Zigbee Router through a serial interface (UART). It facilitates the integration of Tuya Zigbee devices into your ESPHome home automation ecosystem.
The implementation follows the [Tuya MCU Standard Protocol - Zigbee UART Protocol specification](https://developer.tuya.com/en/docs/mcu-standard-protocol/mcusdk-zigbee-uart-protocol?id=Kdg17v4544p37).

## Features
- UART communication support with Tuya Zigbee Router modules
- Native ESPHome integration
- Basic protocol handling and management
- Compatible with all Tuya Zigbee modules connected via UART to an ESP

##  Hardware Compatibility
### Tested Hardware
- Tuya T3E Screen - Features an ESP32 board paired with a ZT3L Zigbee module

### Supported Modules

This component is designed to work with all Tuya Zigbee modules connected via UART to an ESP microcontroller. It has been tested and verified with the ZT3L module.

## Configuration

Example configuration in your esphome yaml:

```yaml
external_components:
  - source: github://rtorrente/esphome-tuya-zigbee-router

uart:
  id: uart_zigbee
  tx_pin: GPIO20
  rx_pin: GPIO19
  baud_rate: 115200

tuya_zigbee_router:
  uart_id: uart_zigbee
  id: zigbee_router_id

button:
  - platform: template
    name: "Zigbee Module Restart"
    icon: "mdi:restart"
    on_press:
      - tuya_zigbee_router.reset:
          id: zigbee_router_id
  - platform: template
    name: "Zigbee Module Pairing"
    icon: "mdi:restart"
    on_press:
      - tuya_zigbee_router.leave_and_rejoin:
          id: zigbee_router_id

text_sensor:
  - platform: tuya_zigbee_router
    zigbee_status:
      name: "Zigbee Router Status"
```

## Zigbee2MQTT Integration

As a simple Zigbee router, this module works seamlessly with Zigbee2MQTT and other Zigbee brokers without any configuration required. The module will appear as:

**TZE200_T-ZB-RT**

### Optional: External Converters

To avoid unmanaged device warnings and enable full feature support, you can configure external converters for Zigbee2MQTT. Custom converters will be provided to enhance device integration and eliminate unmanaged status.

Refer to the Zigbee2MQTT External Converters documentation for integration instructions.

```javascript
import * as tuya from "zigbee-herdsman-converters/lib/tuya";

export default {
    zigbeeModel: ['TS0601'],
    fingerprint: [...tuya.fingerprint("TS0601", ["_TZE200_T-ZB-RT"])],
    model: "T-ZB-RT",
    vendor: "Tuya",
    description: 'Tuya Zigbee Router from https://github.com/rtorrente/esphome-tuya-zigbee-router',
    extend: [],
};
```

## License

GNU General Public License v3.0

© Romain TORRENTE ([@rtorrente](https://github.com/rtorrente)) - 2025