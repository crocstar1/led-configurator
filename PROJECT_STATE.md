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

- Current active port count default: 1 (`USP1_DEFAULT_ACTIVE_PORT_COUNT`).
- `status_mapper` is the runtime source of truth for active/reserved ports.
- `/get_config` and `/diagnostics` expose `activePortCount`.
- Port zone metadata is generated from `activePortCount`:
  - count 1: Port1 active, Port2..4 reserved;
  - count 2: Port1/Port2 active, Port3..4 reserved;
  - count 3: Port1/Port2/Port3 active, Port4 reserved;
  - count 4: Port1..Port4 active.
- Port1 input mapping:
  - Data1 = charging.
  - Data2 = error.
- Status priority: `error > charging > waiting`.
- Port zones must follow real runtime status.
- Port zone preview/custom UI must not hide runtime error indication.
- First-boot matrix defaults currently map all pixels to Port1, so Data1/Data2
  status indication is visible before manual zone setup.

## Fixed Zone Model

Fixed zone IDs:

| ID | Zone |
| ---: | --- |
| 0 | Off |
| 1 | Port1 |
| 2 | Port2, active/reserved from `activePortCount` |
| 3 | Port3, active/reserved from `activePortCount` |
| 4 | Port4, active/reserved from `activePortCount` |
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
- Clean NVS matrix storage added in `led_settings` / `matrix_cfg`.
- Backend endpoints synchronized with the UI:
  - `/save_zones`
  - `/set_bright`
  - `/save_topology`
  - `/save_status_colors`
  - `/save_free_zone`
- `/get_config` extended with `layers`, `hardware_map`, `zones`, `matrix`, and
  `free_zones`.
- Status layers are applied in renderer.
- Fixed zone model added.
- Runtime/preview split improved so UI preview is client-side and does not call
  hardware mode switching endpoints.
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
  - Firmware has an OTA upload MVP.
- Basic Auth stage added:
  - `/` and existing configuration endpoints require Basic Auth;
  - `/diagnostics` remains protected;
  - `/update` OTA upload is now protected before firmware bytes are accepted;
  - legacy WebSocket write commands were removed during cleanup.
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
  - AP fallback defaults are `LED_MATRIX_SETUP` / `12345678`;
  - service menu Network section shows mode/status/IP/RSSI/DHCP and has a
    settings form;
  - endpoints added: `/network_status`, `/scan_networks`, `/save_network`,
    `/network_reconnect`;
  - network endpoints are protected with Basic Auth.
- OTA MVP added:
  - service menu Firmware section uploads `firmware.bin` through `/update`;
  - upload progress and success/error states are shown in the web UI;
  - `/update` is protected with Basic Auth;
  - backend rejects missing, empty, or non-`.bin` uploads;
  - `ESP.restart()` is scheduled only after successful `Update.end(true)`.
- Runtime cleanup stage added:
  - WebSocket server and zone write commands were removed;
  - legacy `/set_mode`, `/save_config`, and `/set_logo_anim` endpoints were removed;
  - `ledMode`/hardware custom preview timeout was removed;
  - diagnostics now reports neutral `runtimeMode: "runtime"` and port statuses separately.
- UI clarity polish added:
  - unmarked free zones remain visible in the dropdown but are disabled with a
    `not marked` label;
  - UI warns when Port1 has no mapped pixels;
  - clearing Port1 asks for explicit confirmation;
  - service menu is a centered modal on desktop and full-screen panel on mobile;
  - service menu closes only through the Close button;
  - network and diagnostics action labels were clarified.
  - local build and manual desktop/mobile UI check passed after this stage.
- Topology selector MVP added:
  - UI exposes the four existing topology codes `0..3`;
  - topology is saved through `/save_topology` in the existing matrix config
    storage;
  - the renderer continues to use `getLEDIndex()` and the selected topology;
  - browser-only numbering preview shows physical LED order for the selected
    topology;
  - no hardware test pattern or hardware preview endpoint was added.
  - local build passed, save/reload topology works, and all four browser
    preview variants were checked.
- Matrix storage cleanup added:
  - legacy matrix fields were removed from `MatrixConfig`;
  - old `zone_map` / `zone_cfg` fallback loaders were removed;
  - current matrix storage uses one clean blob with magic/version/size and
    matrix dimension validation;
  - old, missing, or invalid matrix config now falls back to defaults.
- Runtime indication safety added:
  - active port zones cannot be saved with zero pixels;
  - UI blocks clearing the last required Port1 pixel;
  - backend `/save_zones` validates active port zone presence before applying
    a new zone map.

## Currently In Work

Current focus: build/manual verification of clean matrix storage cleanup and
active Port1 zone safety.

Known active review topics:

- Verify first-boot defaults after incompatible old matrix config is ignored.
- Verify save/reload for zones, topology, status colors, and free zones.
- Verify active port zones cannot be fully cleared while they are within
  `activePortCount`.

## Known UI Issues

- Backend zone metadata names are still English (`Port 1`, `Logo`, `Service`, `Custom`), but the frontend now localizes zone labels by fixed zone ID.
- Free Zones custom pixel drawing was separated from static color in the frontend:
  - static mode fills the selected free zone;
  - custom drawing uses the selected color as a brush;
  - clear drawing clears only the selected free zone custom layer.
- Matrix Zones now warn if Port1 is not mapped, because Data1/Data2 can work
  while no pixels are available for status indication.
- Matrix size is exposed to UI via `matrix.cols` and `matrix.rows`, but backend/storage are still compile-time fixed through `MATRIX_X`, `VIRTUAL_Y`, and `NUM_IC_CHIPS`.
- Free zone storage validates current compile-time matrix size. Future runtime matrix resize still needs migration/reset UX.
- Status/free layers should be filtered or reset when matrix size changes in a future variable-size implementation.
- Service menu currently uses a centered modal on desktop and a full-screen panel
  on mobile.
- Old matrix NVS keys are intentionally not migrated. During development,
  incompatible matrix config is reset to defaults.

## Backlog

- FreeZoneConfig storage follow-up: manual UI/hardware verification and any bug fixes found during reload/reboot testing.
- Network MVP follow-up:
  - validate STA connect/reconnect behavior on hardware;
  - decide whether AP fallback default password is acceptable for production;
  - add safer reset/clear-network flow;
  - consider async/non-blocking startup connection after hardware testing.
- OTA follow-up:
  - add firmware version display;
  - consider signed firmware validation;
  - consider rollback/recovery behavior;
  - improve post-reboot reconnect guidance after hardware testing.
- Brightness schedule by time of day.
- Full WiFi manager polish beyond current MVP.
- Multi-matrix outputs on LED1..LED4.
- Runtime configurable matrix dimensions.
- Topology follow-up:
  - hardware-safe test pattern after real matrix hardware is available;
  - clearer future topology model with start corner, direction, serpentine,
    horizontal/vertical orientation, and color order separated from topology;
  - optional remap/reset UX if topology or matrix size changes.
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
