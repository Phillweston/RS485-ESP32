#pragma once

#include "esphome/components/network/ip_address.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace fsc2a {

class Fsc2aController : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_slave(uint8_t slave) { this->slave_ = slave; }
  void set_setup_ap_password(const std::string &password) {
    this->setup_ap_password_ = password;
  }
  void set_speed(uint32_t value) { this->speed_ = value; }
  void set_acceleration(uint32_t value) { this->acceleration_ = value; }
  void set_deceleration(uint32_t value) { this->deceleration_ = value; }
  void set_distance(uint32_t value) { this->distance_ = value; }
  void move_relative(bool reverse);
  void stop();
  void set_zero();
  void poll();

  bool online() const { return this->online_; }
  uint32_t position() const { return this->position_; }
  uint32_t speed() const { return this->current_speed_; }
  uint32_t status() const { return this->status_; }

 protected:
  static constexpr uint8_t PAGE_COUNT = 4;

  uint16_t crc16_(const uint8_t *data, size_t length);
  void send_(const uint8_t *data, size_t length);
  bool read_frame_(uint8_t *frame, size_t expected);
  void write_register_(uint16_t address, uint16_t value);
  void write_registers32_(uint16_t address, uint32_t value);
  uint8_t slave_{1};
  uint32_t speed_{10}, acceleration_{200}, deceleration_{200}, distance_{50};
  uint32_t position_{0}, current_speed_{0}, status_{0};
  bool online_{false};
  uint32_t last_poll_{0};
  uint32_t last_tx_{0};
  uint32_t last_screen_check_{0};
  std::string setup_ap_password_;
  uint8_t page_{0};
  bool screen_dirty_{true};
  bool previous_wifi_connected_{false};
  bool previous_ap_active_{false};
  bool previous_api_connected_{false};
  network::IPAddress previous_ip_;

  void handle_buttons_();
  void update_display_();
};

}  // namespace fsc2a
}  // namespace esphome
