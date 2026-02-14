// zigbee_router.cpp
#include "tuya_zigbee_router.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tuya_zigbee_router {

static const char* TAG = "tuya_zigbee_router";


// ============================================================================
// SETUP AND LOOP
// ============================================================================

void ZigbeeRouterComponent::setup() {
  ESP_LOGI(TAG, "Initializing Tuya Zigbee Router Module");
  this->init_succeed_ = false;
}

void ZigbeeRouterComponent::loop() {
  while (this->available()) {
    uint8_t c;
    this->read_byte(&c);

    // Sync on header
    if (this->rx_buffer_.empty() && c != FRAME_HEADER_HIGH) {
      continue;
    }

    this->rx_buffer_.push_back(c);

    // Check complete header
    if (this->rx_buffer_.size() == 2) {
      if (this->rx_buffer_[0] != FRAME_HEADER_HIGH ||
          this->rx_buffer_[1] != FRAME_HEADER_LOW) {
        ESP_LOGW(TAG, "Invalid frame header");
        this->rx_buffer_.clear();
        continue;
      }
    }

    // Wait for minimum 8 bytes: header(2) + version(1) + seq(2) + cmd(1) + len(2) = 8 bytes
    if (this->rx_buffer_.size() >= 8) {
      uint16_t data_len = (this->rx_buffer_[6] << 8) | this->rx_buffer_[7];
      uint16_t frame_size = 8 + data_len + 1;  // +1 pour checksum

      if (this->rx_buffer_.size() >= frame_size) {
        if (this->validate_checksum_(this->rx_buffer_)) {
          this->parse_frame_();
        } else {
          ESP_LOGW(TAG, "Checksum validation failed");
        }
        this->rx_buffer_.clear();
      }
    }
  }

  // Period check the network status
  uint32_t now = millis();
  if (now - this->last_status_query_ >= STATUS_QUERY_INTERVAL_MS && this->init_succeed_ == true) {
    this->query_network_status_();
    this->last_status_query_ = now;
  }
}

void ZigbeeRouterComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Zigbee Router Module:");
  ESP_LOGCONFIG(TAG, "  Status query interval: %d ms", STATUS_QUERY_INTERVAL_MS);
}

// ============================================================================
// HANDLERS
// ============================================================================

void ZigbeeRouterComponent::parse_frame_() {
  uint8_t version = this->rx_buffer_[2];
  uint16_t seq = (this->rx_buffer_[3] << 8) | this->rx_buffer_[4];
  uint8_t cmd = this->rx_buffer_[5];
  uint16_t data_len = (this->rx_buffer_[6] << 8) | this->rx_buffer_[7];

  ESP_LOGV(TAG, "Received frame - CMD: 0x%02X, SEQ: 0x%04X, Data len: %d",
           cmd, seq, data_len);

  std::vector<uint8_t> data;
  if (data_len > 0) {
    data.insert(data.end(),
                this->rx_buffer_.begin() + 8,
                this->rx_buffer_.begin() + 8 + data_len);
  }

  if (!data.empty()) {
    ESP_LOGV(TAG, "Data (hex): %s", format_hex(data).c_str());
  } else {
    ESP_LOGV(TAG, "Data: <empty>");
  }

  // Handle command received from module
  switch (cmd) {
    case CMD_QUERY_PRODUCT_INFO:
      ESP_LOGD(TAG, "Module requesting product info (0x01)");
      this->handle_query_product_info_(seq);
      break;

    case CMD_SYNC_NETWORK_STATUS:
      ESP_LOGD(TAG, "Module updating network status (0x02)");
      this->handle_network_status_response_(seq, data);
      break;

    case CMD_QUERY_NETWORK_STATUS:
      ESP_LOGD(TAG, "Module responding to network status query (0x20)");
      this->handle_network_status_response_(seq, data);
      break;

    default:
      ESP_LOGD(TAG, "Unhandled command: 0x%02X", cmd);
  }
}

void ZigbeeRouterComponent::handle_query_product_info_(uint16_t seq) {
  std::string product_info = "{\"p\":\"" + PRODUCT_ID + "\",\"v\":\"" +
                             MCU_VERSION + "\",\"g\":1}";

  std::vector<uint8_t> data(product_info.begin(), product_info.end());

  this->send_frame_(CMD_QUERY_PRODUCT_INFO, data, seq);

  this->init_succeed_ = true;

  ESP_LOGI(TAG, "Sent product info response: %s", product_info.c_str());
}

void ZigbeeRouterComponent::handle_network_status_response_(
    uint16_t seq,
    const std::vector<uint8_t>& data) {

  if (data.empty()) {
    ESP_LOGW(TAG, "Network status response is empty");
    return;
  }

  uint8_t status = data[0];

  const char* status_str;
  switch (status) {
    case NOT_CONNECTED: status_str = "NOT_CONNECTED"; break;
    case CONNECTED: status_str = "CONNECTED"; break;
    case NETWORK_ERROR: status_str = "NETWORK_ERROR"; break;
    case PAIRING: status_str = "PAIRING"; break;
    default: status_str = "UNKNOWN"; break;
  }

  ESP_LOGI(TAG, "Zigbee module network status: %s (0x%02X)", status_str, status);
  if (this->connection_status_sensor_)
        this->connection_status_sensor_->publish_state(status_str);
}

// ============================================================================
// COMMANDS
// ============================================================================

void ZigbeeRouterComponent::query_network_status_() {
  std::vector<uint8_t> empty_data;
  this->send_frame_(CMD_QUERY_NETWORK_STATUS, empty_data);
  ESP_LOGD(TAG, "Querying network status (0x20)");
}

void ZigbeeRouterComponent::reset_module(uint8_t mode) {
  std::vector<uint8_t> data = {mode};
  this->send_frame_(CMD_RESET_MODULE, data);
  ESP_LOGI(TAG, "Reset/Pairing mode requested (0x03)");
}

void ZigbeeRouterComponent::reset_and_restart() {
  this->reset_module(0x00);
}

void ZigbeeRouterComponent::leave_and_rejoin() {
  this->reset_module(0x01);
}

// ============================================================================
// HELPERS
// ============================================================================

std::vector<uint8_t> ZigbeeRouterComponent::build_frame_(
    uint8_t cmd,
    const std::vector<uint8_t>& data,
    uint16_t seq) {

  uint16_t data_len = data.size();

  // Use provided seq or default counter
  uint16_t frame_seq = (seq != 0xFFFF) ? seq : this->seq_counter_;

  std::vector<uint8_t> frame = {
    FRAME_HEADER_HIGH,
    FRAME_HEADER_LOW,
    PROTOCOL_VERSION,
    (uint8_t)(frame_seq >> 8),
    (uint8_t)(frame_seq & 0xFF),
    cmd,
    (uint8_t)(data_len >> 8),
    (uint8_t)(data_len & 0xFF)
  };

  // Add data in payload
  frame.insert(frame.end(), data.begin(), data.end());

  uint8_t checksum = this->calculate_checksum_(frame);
  frame.push_back(checksum);

  return frame;
}

void ZigbeeRouterComponent::send_frame_(uint8_t cmd,
                                        const std::vector<uint8_t>& data,
                                        uint16_t seq) {
  std::vector<uint8_t> frame = this->build_frame_(cmd, data, seq);
  this->write_array(frame);

  // Increment only if we use the counter (Not an answer)
  if (seq == 0xFFFF) {
    this->seq_counter_ = (this->seq_counter_ + 1) % 0xfff1;  // Cycle 0 → 0xfff0
  }
}

bool ZigbeeRouterComponent::validate_checksum_(const std::vector<uint8_t>& frame) {
  if (frame.empty()) return false;

  uint8_t calculated = 0;

  // Checksum = Sum of all octets except last
  for (size_t i = 0; i < frame.size() - 1; i++) {
    calculated += frame[i];
  }
  calculated = calculated % 256;

  uint8_t received = frame.back();

  return calculated == received;
}

uint8_t ZigbeeRouterComponent::calculate_checksum_(const std::vector<uint8_t>& frame) {
  uint16_t sum = 0;

  for (size_t i = 0; i < frame.size(); i++) {
    sum += frame[i];
  }

  return sum % 256;
}

}  // namespace tuya_zigbee_router
}  // namespace esphome