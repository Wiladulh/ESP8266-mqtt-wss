# ESP8266 MQTT over WSS — Modular Rebuild

Modular firmware for NodeMCU ESP8266 and Wemos D1 Mini. This rebuild replaces the 2018 sketch and manual MQTT parser.

## Features
- Wi-Fi station with AP fallback provisioning.
- WebUI configuration for Wi-Fi SSID/password; broker host, port, WebSocket path; MQTT username/password, Client ID, topic prefix; TLS CA PEM; AP credentials; and 10 logical IO channel mappings.
- MQTT 3.1.1 over WSS with mandatory CA certificate validation.
- Ten logical IO channels; each can be disabled, relay output, or button input.
- Local dashboard for relay control and input monitoring.
- LittleFS configuration persistence.

## Build
Install PlatformIO Core/IDE:
```sh
pio run -e nodemcuv2
pio run -e d1_mini
pio run -t uploadfs -e nodemcuv2
pio run -t upload -e nodemcuv2
```

## First boot
1. Flash firmware and LittleFS image.
2. Connect to AP `ESP8266-Setup`, initial password `esp8266setup`.
3. Open `http://192.168.4.1`.
4. Configure Wi-Fi and MQTT/WSS values, including the broker CA certificate PEM.
5. Save and reboot. Change the initial AP password.

Passwords are write-only in the configuration API. They are stored in LittleFS; ESP8266 is not a secure element.

## IO safety
Ten channels are logical, not a claim that ten safe native GPIOs exist simultaneously. GPIO6–GPIO11 are reserved for flash. GPIO0/2/15 affect boot; GPIO1/3 share UART. For ten simultaneous general-purpose IOs, an external expander such as MCP23017 is recommended; its exact hardware/wiring is not assumed by this baseline. Choose pins based on the actual board and attached circuit.

## MQTT topics
- Subscribe: `{topicPrefix}/io/+/set`
- Publish state: `{topicPrefix}/io/{1..10}/state`
- Availability: `{topicPrefix}/status`
- Relay commands: plain `ON`/`OFF` or JSON `{"state":"ON"}`.

CI compiles both PlatformIO board environments. Physical hardware, TLS broker trust chain, and relay/input wiring still require device testing.
