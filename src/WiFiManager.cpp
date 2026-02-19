#include "WiFiManager.h"

#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

WiFiManager::WiFiManager(uint8_t ledPin)
    : _mode(Mode::IDLE), _ledPin(ledPin), _server(80), _lastBlink(0),
      _ledState(false) {
  _ssid[0] = '\0';
  _password[0] = '\0';
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool WiFiManager::begin() {
  pinMode(_ledPin, OUTPUT);
  digitalWrite(_ledPin, LOW);

  // Check reset button (GPIO0 BOOT button)
  pinMode(Config::RESET_BUTTON, INPUT_PULLUP);
  delay(Config::RESET_BUTTON_CHECK_MS);
  if (digitalRead(Config::RESET_BUTTON) == LOW) {
    Serial.println("[WiFi] Reset button pressed. Clearing credentials...");
    clearCredentials();
    Serial.println("[WiFi] Entering provisioning mode.");
    startProvisioning();
    return false;
  }

  if (loadCredentials()) {
    Serial.printf("[WiFi] Saved SSID: %s — attempting connection...\n", _ssid);
    if (connectSTA(Config::WIFI_CONNECT_TIMEOUT_MS)) {
      _mode = Mode::CONNECTED;
      setLedSolid(true);
      Serial.printf("[WiFi] Connected. IP: %s\n",
                    WiFi.localIP().toString().c_str());
      return true;
    }
    Serial.println("[WiFi] Connection timed out. Entering provisioning mode.");
  } else {
    Serial.println("[WiFi] No saved credentials. Entering provisioning mode.");
  }

  startProvisioning();
  return false;
}

void WiFiManager::handle() {
  if (_mode != Mode::PROVISIONING)
    return;
  _dns.processNextRequest();
  _server.handleClient();
  tickProvisioningBlink();
}

bool WiFiManager::isConnected() const { return _mode == Mode::CONNECTED; }

bool WiFiManager::isProvisioning() const { return _mode == Mode::PROVISIONING; }

// ---------------------------------------------------------------------------
// NVS helpers
// ---------------------------------------------------------------------------

bool WiFiManager::loadCredentials() {
  Preferences prefs;
  prefs.begin(Config::NVS_NAMESPACE, /*readOnly=*/true);
  bool found = prefs.isKey(Config::NVS_KEY_SSID);
  if (found) {
    prefs.getString(Config::NVS_KEY_SSID, _ssid, sizeof(_ssid));
    prefs.getString(Config::NVS_KEY_PASS, _password, sizeof(_password));
  }
  prefs.end();
  return found && (_ssid[0] != '\0');
}

void WiFiManager::saveCredentials(const char *ssid, const char *pass) {
  Preferences prefs;
  prefs.begin(Config::NVS_NAMESPACE, /*readOnly=*/false);
  prefs.putString(Config::NVS_KEY_SSID, ssid);
  prefs.putString(Config::NVS_KEY_PASS, pass);
  prefs.end();
  Serial.println("[WiFi] Credentials saved. Restarting...");
  delay(500);
  ESP.restart();
}

void WiFiManager::clearCredentials() {
  Preferences prefs;
  prefs.begin(Config::NVS_NAMESPACE, /*readOnly=*/false);
  prefs.remove(Config::NVS_KEY_SSID);
  prefs.remove(Config::NVS_KEY_PASS);
  prefs.end();
  _ssid[0] = '\0';
  _password[0] = '\0';
}

// ---------------------------------------------------------------------------
// STA connection
// ---------------------------------------------------------------------------

bool WiFiManager::connectSTA(uint32_t timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(_ssid, _password);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start >= timeoutMs) {
      WiFi.disconnect(true);
      Serial.println();
      return false;
    }
    digitalWrite(_ledPin, !digitalRead(_ledPin));
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  return true;
}

// ---------------------------------------------------------------------------
// Provisioning mode (AP + DNS + HTTP)
// ---------------------------------------------------------------------------

void WiFiManager::startProvisioning() {
  _mode = Mode::PROVISIONING;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(Config::PROV_AP_SSID); // open network (no password)
  Serial.printf("[WiFi] AP started: %s  IP: %s\n", Config::PROV_AP_SSID,
                WiFi.softAPIP().toString().c_str());

  // Redirect all DNS queries to the AP IP (captive portal)
  _dns.start(53, "*", WiFi.softAPIP());

  // HTTP routes
  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/save", HTTP_POST, [this]() { handleSave(); });
  _server.onNotFound([this]() { handleNotFound(); });
  _server.begin();

  Serial.println("[WiFi] Captive portal active on http://192.168.4.1");
}

// ---------------------------------------------------------------------------
// HTTP handlers
// ---------------------------------------------------------------------------

void WiFiManager::handleRoot() {
  // Scan for available networks (blocking, ~2-3s)
  int n = WiFi.scanNetworks();

  // Build <option> tags into a fixed-size buffer (avoids String heap churn)
  char opts[2048];
  int pos = 0;
  for (int i = 0; i < n && pos < (int)sizeof(opts) - 128; i++) {
    pos += snprintf(opts + pos, sizeof(opts) - pos,
                    "<option value=\"%s\">%s (%d dBm)</option>",
                    WiFi.SSID(i).c_str(), WiFi.SSID(i).c_str(), WiFi.RSSI(i));
  }
  opts[pos] = '\0';
  WiFi.scanDelete();

  static const char HEAD[] =
      "<!DOCTYPE html><html><head>"
      "<meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>GrowGrid Setup</title>"
      "<style>"
      "body{font-family:sans-serif;max-width:360px;margin:40px auto;padding:0 "
      "16px}"
      "h1{font-size:1.4em;color:#2d6a4f}"
      "select,input{width:100%;padding:8px;margin:8px 0;box-sizing:border-box;"
      "border:1px solid #ccc;border-radius:4px}"
      "button{width:100%;padding:10px;background:#2d6a4f;color:#fff;border:"
      "none;"
      "border-radius:4px;font-size:1em;cursor:pointer}"
      "button:hover{background:#1b4332}"
      "</style></head><body>"
      "<h1>GrowGrid Wi-Fi Setup</h1>"
      "<form method='POST' action='/save'>"
      "<label>Network<select name='ssid'>";

  static const char TAIL[] =
      "</select></label>"
      "<label>Password"
      "<input type='password' name='pass' placeholder='Leave blank if open'>"
      "</label>"
      "<button type='submit'>Save &amp; Connect</button>"
      "</form></body></html>";

  _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  _server.send(200, "text/html", "");
  _server.sendContent(HEAD);
  _server.sendContent(opts);
  _server.sendContent(TAIL);
}

void WiFiManager::handleSave() {
  if (!_server.hasArg("ssid") || _server.arg("ssid").length() == 0) {
    _server.send(400, "text/plain", "Missing SSID");
    return;
  }

  char newSsid[64];
  char newPass[64];
  strncpy(newSsid, _server.arg("ssid").c_str(), sizeof(newSsid) - 1);
  newSsid[sizeof(newSsid) - 1] = '\0';
  strncpy(newPass, _server.arg("pass").c_str(), sizeof(newPass) - 1);
  newPass[sizeof(newPass) - 1] = '\0';

  static const char RESPONSE[] =
      "<!DOCTYPE html><html><body "
      "style='font-family:sans-serif;max-width:360px;"
      "margin:40px auto;padding:0 16px'>"
      "<h2 style='color:#2d6a4f'>Saved! Connecting...</h2>"
      "<p>The device will restart and connect to your network. "
      "You may close this page.</p>"
      "</body></html>";
  _server.send(200, "text/html", RESPONSE);

  delay(1000); // allow response to flush before restart
  saveCredentials(newSsid, newPass);
}

void WiFiManager::handleNotFound() {
  // Captive portal: redirect any unknown URL to the setup page.
  // This triggers the OS captive portal UI on iOS, Android, and Windows.
  _server.sendHeader("Location", "http://192.168.4.1/", true);
  _server.send(302, "text/plain", "");
}

// ---------------------------------------------------------------------------
// LED helpers
// ---------------------------------------------------------------------------

void WiFiManager::setLedSolid(bool on) {
  digitalWrite(_ledPin, on ? HIGH : LOW);
}

void WiFiManager::tickProvisioningBlink() {
  // Fast blink at ~4 Hz to signal provisioning mode
  uint32_t now = millis();
  if (now - _lastBlink >= 125) {
    _lastBlink = now;
    _ledState = !_ledState;
    digitalWrite(_ledPin, _ledState ? HIGH : LOW);
  }
}
