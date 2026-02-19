# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-based microcontroller firmware for GrowGrid using PlatformIO and Arduino framework. The device supports WiFi provisioning via captive portal and Over-The-Air (OTA) firmware updates.

## Build Commands

```bash
# Build the project
pio run

# Upload firmware (uses OTA by default - see platformio.ini)
pio run --target upload

# Clean build artifacts
pio run --target clean

# Open serial monitor (115200 baud)
pio device monitor

# Run tests
pio test
```

## Architecture

The firmware uses a layered architecture centered around an `Application` orchestrator:

- **Application** (`Application.h/cpp`): Top-level orchestrator that coordinates WiFiManager and OTAManager lifecycles. Calls `setup()` once and `loop()` continuously.

- **WiFiManager** (`WiFiManager.h/cpp`): Handles dual-mode WiFi operation:
  - **STA mode**: Loads credentials from NVS, attempts connection with timeout
  - **Provisioning mode**: Falls back to AP mode with captive portal (DNS + WebServer) for credential entry
  - Credentials are persisted to NVS and device restarts after successful provisioning
  - LED on GPIO2 indicates status (solid = connected, 4Hz blink = provisioning)

- **OTAManager** (`OTAManager.h/cpp`): Enables Over-The-Air firmware updates using Arduino OTA. Must be initialized after WiFi is connected.

- **Config.h**: Centralized compile-time configuration namespace containing:
  - Pin assignments
  - OTA hostname/password
  - WiFi provisioning settings
  - NVS keys and namespaces

## WiFi Provisioning Flow

1. Device boots → WiFiManager loads credentials from NVS
2. If credentials exist → attempt STA connection (15s timeout)
3. If no credentials or connection fails → start AP mode ("GrowGrid-Setup")
4. User connects to AP → captive portal serves WiFi configuration page
5. User submits credentials → saved to NVS → device restarts
6. On restart → connects with new credentials

## OTA Updates

- Hostname: `grow-grid-esp32.local`
- Password: `ota-password`
- Protocol: `espota`
- OTA is only available when device is connected to WiFi in STA mode

Update OTA settings in `Config.h` if hostname or password needs to change. The `platformio.ini` file also contains these settings for the upload command.

## Development Notes

- All configuration constants are in the `Config` namespace (`Config.h`)
- The main loop only calls `_ota.handle()` when WiFi is connected
- Application-specific code should be added in `Application::loop()` within the `if (_wifi.isConnected())` block
- NVS is used for persistent WiFi credential storage across reboots
