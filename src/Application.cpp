#include "Application.h"

#include "Config.h"
#include <Arduino.h>

Application::Application()
    : _wifi(Config::LED_STATUS),
      _ota(Config::OTA_HOSTNAME, Config::OTA_PASSWORD) {}

void Application::setup() {
  Serial.begin(Config::SERIAL_BAUD);

  bool connected = _wifi.begin();

  if (connected) {
    _ota.begin();
  }
  // If not connected we are in provisioning mode.
  // _wifi.handle() will service the captive portal in loop().
}

void Application::loop() {
  _wifi.handle();

  if (_wifi.isConnected()) {
    _ota.handle();

    // application code here
  }
}
