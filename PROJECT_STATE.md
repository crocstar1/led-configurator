# Project State

## Project Goal

This repository contains standalone firmware for an ESP32-based USP-1 LED matrix
controller.

USP-1 is treated here as a dedicated LED matrix controller, not as full EVSE
firmware. The controller reads optocoupler input signals, maps port state,
renders LED matrix zones, exposes a web setup UI, and stores configuration in
NVS.

## Confirmed Hardware Mapping

Inputs are active-low: `LOW = active`.

| Signal | GPIO | Current role |
| --- | ---: | --- |
| Data1 | GPIO16 | Port1 charging |
| Data2 | GPIO17 | Port1 error |
| Data3 | GPIO18 | Port2 charging, reserved |
| Data4 | GPIO19 | Port2 error, reserved |
| Data5 | GPIO21 | Port3 charging, reserved |
| Data6 | GPIO25 | Port3 error, reserved |
| Data7 | GPIO26 | Port4 charging, reserved |
| Data8 | GPIO27 | Port4 error, reserved |

LED outputs:

| Output | GPIO | State |
| --- | ---: | --- |
| LED1 | GPIO22 | primary matrix output |
| LED2 | GPIO23 | reserved |
| LED3 | GPIO32 | reserved |
| LED4 | GPIO33 | reserved |

## Port Logic

- Current active port count: 1.
- Port1 is active.
- Port2, Port3, and Port4 are reserved/disabled.
- Port1 input mapping:
  - Data1 = charging.
  - Data2 = error.
- Status priority: `error > charging > waiting`.
- Port zones must follow real runtime status.
- Port zone preview/custom UI must not hide runtime error indication.

## Fixed Zone Model

Fixed zone IDs:

| ID | Zone |
| ---: | --- |
| 0 | Off |
| 1 | Port1 |
| 2 | Port2 reserved |
| 3 | Port3 reserved |
| 4 | Port4 reserved |
| 5 | Logo |
| 6 | QR |
| 7 | Service |
| 8 | Custom |

Rules:

- One pixel belongs to only one zone.
- Port zones render station port status only: waiting, charging, error.
- Rainbow is not allowed for port zones.
- Free zones are separate from port runtime status.
- QR means highlighting a matrix area near/under a physical QR, not generating a QR code.

## Runtime/Edit Model

Runtime mode:

- Runtime status is more important than preview/edit state.
- Port zones are driven by real input status.
- `error` cannot be hidden by custom/preview layers.
- Free zones render independently according to their mode.

Edit/preview mode:

- UI preview should be client-side where possible.
- Editing one selected zone must lock other zones.
- Save actions should be explicit.
- If backend is unavailable, UI may run in mock/default preview mode and warn that changes exist only in the browser.

## UI Product Rules

- Visible web UI should use neutral product wording:
  - `контроллер матрицы`
  - `LED-контроллер`
  - `матрица`
  - `входы`
  - `выход LED1`
- Do not mention `УСП-1` in visible web UI unless a future task explicitly
  requests hardware-specific wording.
- Existing hardware-specific code names can stay for now. A later rename pass
  can move names such as `usp1_board_config` and `usp1_inputs` toward
  `board_config` / `hardware_config` and `discrete_inputs` / `board_inputs`.

## Already Done

- Board config added for confirmed USP-1 GPIO mapping.
- Active-low input reading implemented.
- `status_mapper` added with `error > charging > waiting`.
- `/diagnostics` endpoint added.
- LED primary output moved to GPIO22.
- Legacy EVSE/integration code removed from this project scope.
- NVS `matrix_v1` storage added.
- Backend endpoints synchronized with the UI:
  - `/save_zones`
  - `/set_bright`
  - `/set_logo_anim`
  - `/save_status_colors`
  - `/save_free_zone`
- `/get_config` extended with `layers`, `hardware_map`, `zones`, `matrix`, and
  `free_zones`.
- Status layers are applied in renderer.
- Fixed zone model added.
- Runtime/preview split improved so UI preview does not call `/set_mode`.
- UI pass added with tabs:
  - Matrix Zones / Zones editor.
  - Port Status editor.
  - Free Zones editor.
- UI mock/default mode added for local `index.html` preview without ESP32 backend.
- FreeZoneConfig storage added for Logo, QR, Service, and Custom:
  - per-zone mode: static/custom/rainbow;
  - per-zone static color;
  - per-zone brightness;
  - sparse custom pixel layer per free zone.
- Service/gear menu added:
  - Diagnostics moved out of the primary working tabs;
  - Network now has an MVP Wi-Fi settings/status screen;
  - Firmware is a placeholder.
- Basic Auth stage added:
  - `/` and existing configuration endpoints require Basic Auth;
  - `/diagnostics` remains protected;
  - `/update` OTA upload is now protected before firmware bytes are accepted;
  - WebSocket write commands now require a runtime token exposed only through
    authenticated `/get_config`.
- Auth config MVP added:
  - credentials are loaded from NVS namespace `auth_cfg`;
  - first boot or invalid auth config writes default `admin/admin`;
  - credentials are currently stored as plaintext in NVS for the MVP;
  - service menu has a Security section for changing username/password;
  - endpoints added: `/auth_status`, `/save_auth`.
  - local build passed after this stage.
- Network config MVP added:
  - hardcoded AP-only startup was replaced with STA-first / AP fallback logic;
  - network settings are stored separately in NVS namespace `net_cfg`;
  - DHCP and static IP fields are supported;
  - AP fallback defaults are `NEK_EVSE_LAB` / `12345678`;
  - service menu Network section shows mode/status/IP/RSSI/DHCP and has a
    settings form;
  - endpoints added: `/network_status`, `/scan_networks`, `/save_network`,
    `/network_reconnect`;
  - network endpoints are protected with Basic Auth.

## Currently In Work

Current focus: verify auth config MVP and network config MVP, then continue with OTA UX cleanup.

Known active review topics:

- Verify NVS-backed auth credentials and password change UX.
- Verify standalone WiFi STA/AP fallback and service menu network UI.
- Clean up OTA UX after the protected `/update` baseline.

## Known UI Issues

- Backend zone metadata names are still English (`Port 1`, `Logo`, `Service`, `Custom`), but the frontend now localizes zone labels by fixed zone ID.
- Free Zones custom pixel drawing was separated from static color in the frontend:
  - static mode fills the selected free zone;
  - custom drawing uses the selected color as a brush;
  - clear drawing clears only the selected free zone custom layer.
- Matrix Zones now need continued UX testing for overview/edit mode on desktop
  and touch devices.
- Matrix size is exposed to UI via `matrix.cols` and `matrix.rows`, but backend/storage are still compile-time fixed through `MATRIX_X`, `VIRTUAL_Y`, and `NUM_IC_CHIPS`.
- Free zone storage validates current compile-time matrix size. Future runtime matrix resize still needs migration/reset UX.
- Status/free layers should be filtered or reset when matrix size changes in a future variable-size implementation.
- Service menu build and manual desktop/mobile UI check passed.

## Backlog

- FreeZoneConfig storage follow-up: manual UI/hardware verification and any bug fixes found during reload/reboot testing.
- Network MVP follow-up:
  - validate STA connect/reconnect behavior on hardware;
  - decide whether AP fallback default password is acceptable for production;
  - add safer reset/clear-network flow;
  - consider async/non-blocking startup connection after hardware testing.
- Replace Firmware placeholder in the service menu with OTA / remote firmware update UX.
- Brightness schedule by time of day.
- Full WiFi manager polish beyond current MVP.
- OTA UX redesign.
- Multi-matrix outputs on LED1..LED4.
- Runtime configurable matrix dimensions.
- Topology selector UX and validation.
- Optional per-port status color override.
- Auth follow-up:
  - replace plaintext NVS password storage with stronger hashing or another
    safer credential format;
  - define a safe settings reset flow;
  - add factory reset / auth reset procedure;
  - review whether AP fallback may keep a default password in production.

## Build

Build command:

```powershell
pio run -e esp32dev
```

The assistant should not run PlatformIO automatically when the environment is
unstable or when the user explicitly asks not to. Ask the user to run the build.

## Do Not Touch Without Separate Task

- WiFi manager.
- OTA implementation/UX.
- Network settings.
- Rules engine or broader EVSE logic.
- Dynamic arbitrary zone creation/deletion.
- Per-port override logic.
- Brightness schedule.
- Multi-matrix FastLED outputs.
- Hardware pin mapping, unless new confirmed hardware input is provided.
