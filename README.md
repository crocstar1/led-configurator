# USP-1 LED Matrix Controller

Standalone ESP32 firmware for an USP-1 LED matrix controller.

This project is not full EVSE firmware. The controller reads USP-1 optocoupler
signals, maps port state, renders a WS2811 LED matrix, serves a web setup UI,
and stores matrix configuration in NVS.

## Hardware

- Data1 = GPIO16, Port 1 charging
- Data2 = GPIO17, Port 1 error
- Data3 = GPIO18, Port 2 charging
- Data4 = GPIO19, Port 2 error
- Data5 = GPIO21, Port 3 charging, reserved
- Data6 = GPIO25, Port 3 error, reserved
- Data7 = GPIO26, Port 4 charging, reserved
- Data8 = GPIO27, Port 4 error, reserved
- Inputs are active-low: LOW means active
- LED1 = GPIO22, primary matrix output
- LED2 GPIO23, LED3 GPIO32, LED4 GPIO33 are reserved

## Zone Model

Fixed zone slots:

- 0 = Off
- 1 = Port 1
- 2 = Port 2
- 3 = Port 3 reserved
- 4 = Port 4 reserved
- 5 = Logo
- 6 = QR
- 7 = Service
- 8 = Custom

Port zones are runtime-owned and display waiting, charging, or error. Free zones
are used for logo, QR-area highlight, service indication, and custom decoration.

## Build

```powershell
pio run -e esp32dev
```

## Web UI

The firmware serves an engineering setup UI with:

- Matrix Zones editor
- Port Status editor
- Free Zones editor
- Service menu with Diagnostics, Network, Security, and Firmware sections

Preview editing is client-side where possible so runtime port error indication
is not hidden by UI custom mode.

## Current State

- Port 1 and Port 2 are active by default; Port 3 and Port 4 are reserved.
- Logo, QR, Service, and Custom zones persist mode, color, brightness, and
  custom pixel layers in NVS.
- Network STA/AP fallback, Basic Auth, diagnostics, and local web OTA are
  available through the service menu.
