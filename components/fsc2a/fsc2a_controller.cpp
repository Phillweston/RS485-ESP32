#include "fsc2a_controller.h"

#include <M5Unified.h>

#include "esphome/components/api/api_server.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/log.h"

namespace esphome {
namespace fsc2a {
static const char *const TAG = "fsc2a";

void Fsc2aController::setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.setTextSize(2);
  M5.Display.fillScreen(TFT_BLACK);
  update_display_();
  this->set_timeout(1000, [this]() { this->poll(); });
}

uint16_t Fsc2aController::crc16_(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;
  while (length--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; i++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

void Fsc2aController::send_(const uint8_t *data, size_t length) {
  uint32_t now = millis();
  if (now - this->last_tx_ < 20) delay(20 - (now - this->last_tx_));
  this->write_array(data, length);
  uint16_t crc = crc16_(data, length);
  this->write_byte(crc & 0xFF);
  this->write_byte(crc >> 8);
  this->flush();
  this->last_tx_ = millis();
}

void Fsc2aController::write_register_(uint16_t address, uint16_t value) {
  uint8_t f[] = {slave_,
                 0x06,
                 uint8_t(address >> 8),
                 uint8_t(address),
                 uint8_t(value >> 8),
                 uint8_t(value)};
  send_(f, sizeof(f));
}
void Fsc2aController::write_registers32_(uint16_t address, uint32_t value) {
  uint8_t f[] = {slave_,
                 0x10,
                 uint8_t(address >> 8),
                 uint8_t(address),
                 0,
                 2,
                 4,
                 uint8_t(value >> 24),
                 uint8_t(value >> 16),
                 uint8_t(value >> 8),
                 uint8_t(value)};
  send_(f, sizeof(f));
}

void Fsc2aController::move_relative(bool reverse) {
  write_register_(0x0006, speed_);
  write_register_(0x0008, acceleration_);
  write_register_(0x000A, deceleration_);
  write_register_(0x0010, distance_);
  uint8_t f[] = {slave_, 0x05, 0, uint8_t(reverse ? 2 : 1), 0xFF, 0};
  send_(f, sizeof(f));
}
void Fsc2aController::stop() {
  uint8_t f[] = {slave_, 0x05, 0, 4, 0xFF, 0};
  send_(f, sizeof(f));
}
void Fsc2aController::set_zero() {
  uint8_t f[] = {slave_, 0x05, 0, 0x0D, 0xFF, 0};
  send_(f, sizeof(f));
}

void Fsc2aController::poll() {
  if (millis() - last_poll_ < 100) return;
  last_poll_ = millis();
  uint8_t f[] = {slave_, 0x03, 0x00, 0x48, 0x00, 0x08};
  send_(f, sizeof(f));
  uint8_t r[21];
  if (!read_frame_(r, sizeof(r))) {
    online_ = false;
    return;
  }
  status_ = (uint32_t(r[3]) << 24) | (uint32_t(r[4]) << 16) |
            (uint32_t(r[5]) << 8) | r[6];
  position_ = (uint32_t(r[7]) << 24) | (uint32_t(r[8]) << 16) |
              (uint32_t(r[9]) << 8) | r[10];
  current_speed_ = (uint32_t(r[11]) << 24) | (uint32_t(r[12]) << 16) |
                   (uint32_t(r[13]) << 8) | r[14];
  online_ = true;
  update_display_();
}

void Fsc2aController::update_display_() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(4, 6);
  M5.Display.setTextColor(TFT_CYAN);
  M5.Display.print("FSC-2A DOOR");
  auto *wifi = esphome::wifi::global_wifi_component;
  bool connected = wifi && wifi->is_connected();
  bool ap = wifi && wifi->is_ap_active();
  M5.Display.setCursor(4, 30);
  M5.Display.setTextColor(connected ? TFT_GREEN : (ap ? TFT_YELLOW : TFT_RED));
  M5.Display.printf("WiFi: %s",
                    connected ? "CONNECTED" : (ap ? "SETUP AP" : "WAITING"));
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setCursor(4, 52);
  if (connected) {
    char ip[32]{};
    wifi->wifi_sta_ip_addresses()[0].str_to(ip);
    M5.Display.printf("IP: %s", ip);
  } else if (ap)
    M5.Display.printf("AP: FSC-2A Door Setup");
  else
    M5.Display.print("AP starting...");
  M5.Display.setCursor(4, 78);
  M5.Display.setTextColor(online_ ? TFT_GREEN : TFT_RED);
  M5.Display.printf("Modbus: %s", online_ ? "ONLINE" : "OFFLINE");
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setCursor(4, 104);
  M5.Display.printf("POS %lu mm  SPD %lu", (unsigned long)position_,
                    (unsigned long)current_speed_);
  M5.Display.setCursor(4, 130);
  M5.Display.printf("HA API: %s",
                    esphome::api::global_api_server &&
                            esphome::api::global_api_server->is_connected()
                        ? "CONNECTED"
                        : "WAITING");
}

bool Fsc2aController::read_frame_(uint8_t *frame, size_t expected) {
  uint32_t start = millis();
  size_t n = 0;
  while (millis() - start < 100 && n < expected) {
    while (available() && n < expected) frame[n++] = read();
  }
  if (n != expected || frame[0] != slave_ || frame[1] != 0x03) return false;
  uint16_t got = frame[expected - 2] | (uint16_t(frame[expected - 1]) << 8);
  return got == crc16_(frame, expected - 2);
}
void Fsc2aController::loop() {}
}  // namespace fsc2a
}  // namespace esphome
