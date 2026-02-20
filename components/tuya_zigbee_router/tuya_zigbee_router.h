#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <vector>

namespace esphome {
namespace tuya_zigbee_router {


class ZigbeeRouterComponent : public Component, public uart::UARTDevice {
 public:
  explicit ZigbeeRouterComponent(uart::UARTComponent *parent)
    : uart::UARTDevice(parent) {}

  void setup() override;
  void loop() override;
  void dump_config() override;
  void set_connection_status_sensor(esphome::text_sensor::TextSensor *sensor) { this->connection_status_sensor_ = sensor; }

  void reset_and_restart();
  void leave_and_rejoin();

 protected:
  /// Build a frame
  /// @param cmd Command to send
  /// @param data Payload Data
  /// @param seq Seq number (0xFFFF = Use seq_counter_)
  /// @return Frame vector ready to send
  std::vector<uint8_t> build_frame_(uint8_t cmd,
                                     const std::vector<uint8_t>& data,
                                     uint16_t seq = 0xFFFF);

  void send_frame_(uint8_t cmd,
                   const std::vector<uint8_t>& data,
                   uint16_t seq = 0xFFFF);
  void parse_frame_();
  bool validate_checksum_(const std::vector<uint8_t>& frame);
  uint8_t calculate_checksum_(const std::vector<uint8_t>& frame);

  void handle_query_product_info_(uint16_t seq);
  void handle_network_status_response_(uint16_t seq, const std::vector<uint8_t>& data);
  void query_network_status_();
  void reset_module(uint8_t mode);

  static constexpr uint8_t FRAME_HEADER_HIGH = 0x55;
  static constexpr uint8_t FRAME_HEADER_LOW = 0xAA;
  static constexpr uint8_t PROTOCOL_VERSION = 0x02;

  static constexpr uint8_t CMD_QUERY_PRODUCT_INFO = 0x01;
  static constexpr uint8_t CMD_SYNC_NETWORK_STATUS = 0x02;
  static constexpr uint8_t CMD_RESET_MODULE = 0x03;
  static constexpr uint8_t CMD_QUERY_NETWORK_STATUS = 0x20;

  static constexpr std::string PRODUCT_ID = "T-ZB-RT";
  static constexpr std::string MCU_VERSION = "1.0.0";

  enum NetworkStatus : uint8_t {
    NOT_CONNECTED = 0x00,
    CONNECTED = 0x01,
    NETWORK_ERROR = 0x02,
    PAIRING = 0x03
  };

  std::vector<uint8_t> rx_buffer_;
  esphome::text_sensor::TextSensor *connection_status_sensor_;
  bool init_succeed_{false};
  uint16_t seq_counter_{0};
  static constexpr uint32_t STATUS_QUERY_INTERVAL_MS = 300000; // 5 minutes
};

template<typename... Ts>
class ResetAction : public Action<Ts...> {
 public:
  void set_parent(ZigbeeRouterComponent *parent) { this->parent_ = parent; }

  void play(Ts... x) override {
    this->parent_->reset_and_restart();
  }

 protected:
  ZigbeeRouterComponent *parent_;
};

template<typename... Ts>
class LeaveAndRejoinAction : public Action<Ts...> {
 public:
  void set_parent(ZigbeeRouterComponent *parent) { this->parent_ = parent; }

  void play(Ts... x) override {
    this->parent_->leave_and_rejoin();
  }

 protected:
  ZigbeeRouterComponent *parent_;
};

}  // namespace tuya_zigbee_router
}  // namespace esphome