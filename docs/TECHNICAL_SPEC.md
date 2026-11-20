# Technical Specification — ESP8266 Modular MQTT WSS
Version 0.1.0

## Decisions
- Boards: NodeMCU ESP8266 (ESP-12E/12F) and Wemos D1 Mini.
- PlatformIO + Arduino ESP8266 core.
- Ten logical IO channels (IO1–IO10), configurable mode, GPIO, name, and active polarity.
- Wi-Fi station with SoftAP fallback after missing credentials or connection timeout.
- MQTT 3.1.1 (protocol level 4) over WSS binary WebSocket frames.
- LittleFS JSON configuration.
- CA certificate required for TLS; no insecure fallback.

## WebUI fields
Wi-Fi SSID/password; broker host, port, WebSocket path; MQTT username/password; Client ID; topic prefix; trusted CA PEM; AP SSID/password; ten IO names, GPIO mappings, modes (disabled/relay/button), and active-low settings. Passwords are never returned by GET config; blank secret inputs preserve saved secrets.

## MQTT topics
Subscribe `{topicPrefix}/io/+/set`; publish state `{topicPrefix}/io/{channel}/state`; availability `{topicPrefix}/status`. QoS 0, clean session.

## HTTP API
- GET / — WebUI
- GET/POST /api/config — non-secret settings / validated persistence
- GET /api/status — Wi-Fi, MQTT, IP, RSSI, IO state
- POST /api/io — relay command

## Hardware constraints
GPIO6–GPIO11 are reserved for flash. GPIO0/2/15 are boot strap pins; GPIO1/3 are UART. Firmware rejects flash GPIOs and duplicate assignments. Ten logical channels do not mean ten safe on-board GPIOs. A 16-bit I2C expander is recommended for ten simultaneous IOs; exact expander support awaits hardware confirmation.

## Runtime
Non-blocking Wi-Fi timeout and AP fallback; main loop services HTTP, IO, and MQTT. MQTT remaining length uses variable-byte integer encoding. Credentials are not logged or returned by config API.
