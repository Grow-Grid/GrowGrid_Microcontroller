#include "OTAManager.h"

#include <Arduino.h>
#include <ArduinoOTA.h>

OTAManager::OTAManager(const char *hostname, const char *password)
    : _hostname(hostname), _password(password) {}

void OTAManager::begin() {
  ArduinoOTA.setHostname(_hostname);
  ArduinoOTA.setPassword(_password);
  registerCallbacks();
  ArduinoOTA.begin();
  Serial.println("OTA ready. Hostname: " + String(_hostname));
}

void OTAManager::handle() { ArduinoOTA.handle(); }

void OTAManager::registerCallbacks() {
  ArduinoOTA.onStart([]() {
    String type =
        (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("OTA start: " + type);
  });

  ArduinoOTA.onEnd([]() { Serial.println("\nOTA complete."); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", progress * 100 / total);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error [%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End failed");
  });
}
