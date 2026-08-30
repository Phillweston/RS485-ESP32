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
  M5.Display.fillScreen(TFT_BLACK);
  update_display_();
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
    if (online_) {
      online_ = false;
      screen_dirty_ = true;
    }
    return;
  }
  status_ = (uint32_t(r[3]) << 24) | (uint32_t(r[4]) << 16) |
            (uint32_t(r[5]) << 8) | r[6];
  position_ = (uint32_t(r[7]) << 24) | (uint32_t(r[8]) << 16) |
              (uint32_t(r[9]) << 8) | r[10];
  current_speed_ = (uint32_t(r[11]) << 24) | (uint32_t(r[12]) << 16) |
                   (uint32_t(r[13]) << 8) | r[14];
  online_ = true;
  screen_dirty_ = true;
}

void Fsc2aController::update_display_() {
  static const char *const PAGE_NAMES[] = {"HOME", "FSC-2A", "MQTT", "WIFI"};
  auto *wifi = esphome::wifi::global_wifi_component;
  const bool wifi_connected = wifi && wifi->is_connected();
  const bool ap_active = wifi && wifi->is_ap_active();
  const bool api_connected = esphome::api::global_api_server &&
                             esphome::api::global_api_server->is_connected();

  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(4, 4);
  M5.Display.setTextColor(TFT_CYAN);
  M5.Display.printf("FSC DOOR  %s", PAGE_NAMES[page_]);
  M5.Display.drawFastHLine(0, 16, M5.Display.width(), 0x39E7);

  if (page_ == 0) {
    M5.Display.setTextSize(2);
    M5.Display.setCursor(4, 25);
    M5.Display.setTextColor(online_ ? TFT_GREEN : TFT_RED);
    M5.Display.print(online_ ? "READY" : "OFFLINE");
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setCursor(4, 55);
    M5.Display.printf("Position: %lu mm",
                      static_cast<unsigned long>(position_));
    M5.Display.setCursor(4, 70);
    M5.Display.printf("Speed:    %lu mm/s",
                      static_cast<unsigned long>(current_speed_));
    M5.Display.setCursor(4, 85);
    M5.Display.printf("HA: %s", api_connected ? "CONNECTED" : "WAITING");
  } else if (page_ == 1) {
    M5.Display.setTextColor(online_ ? TFT_GREEN : TFT_RED);
    M5.Display.setCursor(4, 25);
    M5.Display.printf("Modbus: %s", online_ ? "ONLINE" : "OFFLINE");
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setCursor(4, 43);
    M5.Display.printf("Slave: %u", slave_);
    M5.Display.setCursor(4, 58);
    M5.Display.printf("Status: 0x%08lX", static_cast<unsigned long>(status_));
    M5.Display.setCursor(4, 73);
    M5.Display.printf("Position: %lu mm",
                      static_cast<unsigned long>(position_));
    M5.Display.setCursor(4, 88);
    M5.Display.printf("Speed: %lu mm/s",
                      static_cast<unsigned long>(current_speed_));
  } else if (page_ == 2) {
    M5.Display.setTextColor(TFT_YELLOW);
    M5.Display.setCursor(4, 28);
    M5.Display.print("MQTT: DISABLED");
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setCursor(4, 48);
    M5.Display.print("Home Assistant uses");
    M5.Display.setCursor(4, 63);
    M5.Display.print("ESPHome native API.");
    M5.Display.setCursor(4, 83);
    M5.Display.printf("API: %s", api_connected ? "CONNECTED" : "WAITING");
  } else {
    M5.Display.setCursor(4, 25);
    M5.Display.setTextColor(wifi_connected ? TFT_GREEN : TFT_YELLOW);
    M5.Display.printf("WiFi: %s", wifi_connected
                                      ? "CONNECTED"
                                      : (ap_active ? "SETUP AP" : "WAITING"));
    M5.Display.setTextColor(TFT_WHITE);
    if (wifi_connected) {
      char ssid[wifi::SSID_BUFFER_SIZE]{};
      char ip[network::IP_ADDRESS_BUFFER_SIZE]{};
      wifi->wifi_ssid_to(ssid);
      wifi->wifi_sta_ip_addresses()[0].str_to(ip);
      M5.Display.setCursor(4, 45);
      M5.Display.printf("SSID: %.18s", ssid);
      M5.Display.setCursor(4, 62);
      M5.Display.printf("IP: %s", ip);
      M5.Display.setCursor(4, 79);
      M5.Display.print("Web: port 80");
    } else if (ap_active) {
      M5.Display.setCursor(4, 45);
      M5.Display.print("AP: FSC-2A Door Setup");
      M5.Display.setCursor(4, 62);
      M5.Display.printf("Password: %s", setup_ap_password_.c_str());
      M5.Display.setCursor(4, 79);
      M5.Display.print("IP: 192.168.4.1");
    } else {
      M5.Display.setCursor(4, 45);
      M5.Display.print("Setup AP starting...");
    }
  }

  M5.Display.drawFastHLine(0, 108, M5.Display.width(), 0x39E7);
  M5.Display.setTextColor(0xBDF7);
  M5.Display.setCursor(4, 114);
  M5.Display.printf("KEY2 <  %u/%u  > KEY1", page_ + 1, PAGE_COUNT);
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
void Fsc2aController::handle_buttons_() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    page_ = (page_ + 1) % PAGE_COUNT;
    screen_dirty_ = true;
  }
  if (M5.BtnB.wasPressed()) {
    page_ = (page_ + PAGE_COUNT - 1) % PAGE_COUNT;
    screen_dirty_ = true;
  }
}

void Fsc2aController::loop() {
  handle_buttons_();
  poll();

  const uint32_t now = millis();
  if (now - last_screen_check_ >= 500) {
    last_screen_check_ = now;
    auto *wifi = esphome::wifi::global_wifi_component;
    const bool wifi_connected = wifi && wifi->is_connected();
    const bool ap_active = wifi && wifi->is_ap_active();
    const bool api_connected = esphome::api::global_api_server &&
                               esphome::api::global_api_server->is_connected();
    const network::IPAddress ip = wifi_connected
                                      ? wifi->wifi_sta_ip_addresses()[0]
                                      : network::IPAddress();
    if (wifi_connected != previous_wifi_connected_ ||
        ap_active != previous_ap_active_ ||
        api_connected != previous_api_connected_ || ip != previous_ip_) {
      previous_wifi_connected_ = wifi_connected;
      previous_ap_active_ = ap_active;
      previous_api_connected_ = api_connected;
      previous_ip_ = ip;
      screen_dirty_ = true;
    }
  }

  if (screen_dirty_) {
    screen_dirty_ = false;
    update_display_();
  }
}
}  // namespace fsc2a
}  // namespace esphome
