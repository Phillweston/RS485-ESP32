# FSC-2A ESPHome Sliding Door Controller

This project exposes an FSC-2A single-axis Modbus RTU controller to Home Assistant.

## Hardware

- M5Stack StickS3 (ESP32-S3) UART TX/RX connect to the TTL side of an RS485 transceiver (MAX3485/SP3485 class).
- Transceiver A/B connect to FSC-2A RJ45 pins 4/5 (485_A/485_B).
- StickS3 Grove uses GPIO9 (yellow) and GPIO10 (white). The YAML maps GPIO10=TX and GPIO9=RX; swap them if your converter labels require the opposite UART direction.
- The configuration starts the fallback setup AP (`FSC-2A Door Setup`) when no saved Wi-Fi network is available, and provides the local ESPHome Web Server on port 80.
- Connect StickS3 GND to converter GND, converter TTL-TX to StickS3 RX, and converter TTL-RX to StickS3 TX. Do not connect the FSC-2A A/B lines directly to StickS3 pins.
- The TTL-RS485 converter must provide automatic transmit/receive direction. If it exposes DE and /RE, they need to be driven by a GPIO or tied according to the module design; the current YAML does not control a DE pin.
- Use a common signal ground and terminate the bus only at its physical ends.

## Home Assistant entities

- `cover.sliding_door`: open sends relative forward, close sends relative reverse, stop sends the stop relay.
- `button.set_door_position_zero`: writes the zero-position relay.
- Position, speed, online status and controller status are polled from read-only registers `0x0048..0x004F`.

The controller enforces a minimum 20 ms gap between frames and polls every 100 ms. The supplied implementation uses slave address 1 and a relative travel distance of 50 mm; adjust the constants in `fsc2a.yaml` or expose them as number entities for your mechanism.

## StickS3 screen pages

The two physical buttons cycle through four pages. Navigation wraps around at the first and last page:

- `KEY1` / GPIO11 / `M5.BtnA`: next page.
- `KEY2` / GPIO12 / `M5.BtnB`: previous page.

The footer shows the current page number and button directions: `KEY2 < n/4 > KEY1`.

### 1. Home

- FSC-2A ready or offline state.
- Current position in millimetres.
- Current speed in millimetres per second.
- Home Assistant ESPHome Native API connection state.

### 2. FSC-2A controller

- Modbus online or offline state.
- Modbus slave address.
- Raw 32-bit device status register in hexadecimal.
- Current position and speed.

### 3. MQTT

- MQTT is shown as disabled because this project currently uses the ESPHome Native API instead of MQTT.
- Home Assistant Native API connection state is shown for the active messaging path.

### 4. Wi-Fi

- Connected, setup AP, or waiting state.
- When connected: Wi-Fi SSID, assigned IP address, and Web Server port.
- In setup mode: setup AP name and `192.168.4.1` configuration address.

## Build

Install ESPHome, create `secrets.yaml` from `secrets.yaml.example`, set `fallback_ap_password`, then run:

```text
esphome config fsc2a.yaml
esphome run fsc2a.yaml
```

The ESPHome web server is enabled on port 80. Physical door travel, limit switches, emergency stop behavior, and RS485 direction must still be tested on the actual hardware before putting the door into service.
