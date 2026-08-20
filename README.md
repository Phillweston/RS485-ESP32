# FSC-2A ESPHome Sliding Door Controller

This project exposes an FSC-2A single-axis Modbus RTU controller to Home Assistant.

## Hardware

- M5Stack StickS3 (ESP32-S3) UART TX/RX connect to the TTL side of an RS485 transceiver (MAX3485/SP3485 class).
- Transceiver A/B connect to FSC-2A RJ45 pins 4/5 (485_A/485_B).
- StickS3 Grove uses GPIO9 (yellow) and GPIO10 (white). The YAML maps GPIO10=TX and GPIO9=RX; swap them if your converter labels require the opposite UART direction.
- The configuration includes fallback AP provisioning (`FSC-2A Door Setup`), captive portal, Improv Serial, and the local ESPHome Web Server.
- Connect StickS3 GND to converter GND, converter TTL-TX to StickS3 RX, and converter TTL-RX to StickS3 TX. Do not connect the FSC-2A A/B lines directly to StickS3 pins.
- The TTL-RS485 converter must provide automatic transmit/receive direction. If it exposes DE and /RE, they need to be driven by a GPIO or tied according to the module design; the current YAML does not control a DE pin.
- Use a common signal ground and terminate the bus only at its physical ends.

## Home Assistant entities

- `cover.sliding_door`: open sends relative forward, close sends relative reverse, stop sends the stop relay.
- `button.set_door_position_zero`: writes the zero-position relay.
- Position, speed, online status and controller status are polled from read-only registers `0x0048..0x004F`.

The controller enforces a minimum 20 ms gap between frames and polls every 100 ms. The supplied implementation uses slave address 1 and a relative travel distance of 50 mm; adjust the constants in `fsc2a.yaml` or expose them as number entities for your mechanism.

## Build

Install ESPHome, create `secrets.yaml` with `wifi_ssid` and `wifi_password`, then run:

```text
esphome config fsc2a.yaml
esphome run fsc2a.yaml
```

The ESPHome web server is enabled on port 80. Physical door travel, limit switches, emergency stop behavior, and RS485 direction must still be tested on the actual hardware before putting the door into service.
