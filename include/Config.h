#pragma once

#include <cstdint>

namespace Config {

// ----- Serial -----
constexpr uint32_t SERIAL_BAUD = 115200;

// ----- Pin definitions -----
constexpr uint8_t LED_STATUS = 2;    // GPIO2 on most ESP32 dev boards
constexpr uint8_t RESET_BUTTON = 0;  // GPIO0 (BOOT button) - LOW when pressed

// ----- OTA -----
constexpr const char *OTA_HOSTNAME = "grow-grid-esp32";
constexpr const char *OTA_PASSWORD = "ota-password";

// ----- WiFi Provisioning -----
constexpr const char *PROV_AP_SSID = "GrowGrid-Setup";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t RESET_BUTTON_CHECK_MS = 100;  // Time to check button on boot
constexpr const char *NVS_NAMESPACE = "wifi_cfg";
constexpr const char *NVS_KEY_SSID = "ssid";
constexpr const char *NVS_KEY_PASS = "pass";

} // namespace Config
