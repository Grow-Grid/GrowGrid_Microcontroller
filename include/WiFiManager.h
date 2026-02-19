#pragma once

#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <cstdint>

class WiFiManager {
public:
  explicit WiFiManager(uint8_t ledPin);

  // Call once in setup(). Loads NVS credentials → attempts STA connection →
  // falls back to AP provisioning. Returns true if connected in STA mode.
  bool begin();

  // Call every loop(). Services DNS + HTTP in provisioning mode; no-op
  // otherwise.
  void handle();

  bool isConnected() const;
  bool isProvisioning() const;

private:
  enum class Mode { IDLE, CONNECTED, PROVISIONING };

  Mode _mode;
  uint8_t _ledPin;
  char _ssid[64];
  char _password[64];

  WebServer _server;
  DNSServer _dns;

  uint32_t _lastBlink;
  bool _ledState;

  // NVS helpers
  bool loadCredentials();
  void saveCredentials(const char *ssid,
                       const char *pass); // writes then ESP.restart()
  void clearCredentials();

  // STA connection with timeout
  bool connectSTA(uint32_t timeoutMs);

  // Provisioning (AP + DNS + HTTP)
  void startProvisioning();
  void handleRoot();     // GET /  — WiFi scan dropdown + password field
  void handleSave();     // POST /save — validate → persist → restart
  void handleNotFound(); // redirect → captive portal

  // LED helpers
  void setLedSolid(bool on);
  void tickProvisioningBlink(); // 4 Hz blink; called from handle()
};
