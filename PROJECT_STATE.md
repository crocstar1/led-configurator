# Project State

## Project Goal

This repository contains standalone firmware for an ESP32-based LED matrix
controller.

The controller reads optocoupler input signals, maps port state, renders LED
matrix zones, exposes a web setup UI, and stores configuration in NVS.

Current product direction: this is no longer a demo-only or throwaway MVP. The
target is a real working standalone LED controller firmware for a charging
station, with runtime indication safety treated as the primary requirement.

The old `proj1` firmware is a reference source for ideas and behavior checks
only. Do not directly port its EVSE/OCPP/MQTT/POS/RAPI/legacy configuration
architecture unless a separate task explicitly requires one narrow piece.

## Confirmed Hardware Mapping

Inputs are active-low: `LOW = active`.

| Signal | GPIO | Current role |
| --- | ---: | --- |
| Data1 | GPIO16 | Port1 charging |
| Data2 | GPIO17 | Port1 error |
| Data3 | GPIO18 | Port2 charging |
| Data4 | GPIO19 | Port2 error |
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

- Current active port count default: 2 (`DEFAULT_ACTIVE_PORT_COUNT`).
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
- Port2 input mapping:
  - Data3 = charging.
  - Data4 = error.
- Status priority: `error > charging > waiting`.
- Port zones must follow real runtime status.
- Port zone preview/custom UI must not hide runtime error indication.
- First-boot matrix defaults split pixels across currently active port zones, so
  Port1/Port2 status indication is visible before manual zone setup.

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
- Do not mention a specific board name in visible web UI unless a future task
  explicitly requests hardware-specific wording.
- Hardware-facing code uses neutral `board_config` and `station_inputs` names.

## Already Done

- Board config added for the confirmed GPIO mapping.
- Hardware configuration and input modules use neutral standalone-controller
  names without changing GPIO or runtime behavior.
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
  - `/auth_status` reports whether default `admin/admin` credentials are still
    in use;
  - endpoints added: `/auth_status`, `/save_auth`.
  - local build passed after this stage.
- Network config MVP added:
  - hardcoded AP-only startup was replaced with STA-first / AP fallback logic;
  - STA startup is non-blocking, so the main loop continues reading Data inputs
    while Wi-Fi connects;
  - runtime states are `connecting`, `sta`, `reconnecting`, and `ap_fallback`;
  - a 500 ms watchdog allows 45 seconds for an established STA connection to
    recover before starting AP fallback;
  - AP fallback remains AP-only; when no AP client is connected, a saved STA
    network is retried once per minute without keeping permanent AP+STA mode;
  - automatic reconnect is explicitly enabled instead of relying on framework
    defaults;
  - network settings are stored separately in NVS namespace `net_cfg`;
  - DHCP and static IP fields are supported;
  - default AP fallback SSID is unique (`LED_MATRIX_XXXXXX`) using the ESP32
    chip suffix, while a saved custom AP SSID is preserved;
  - AP fallback password default is `12345678`;
  - service menu Network section shows mode/status/IP/RSSI/DHCP and has a
    settings form;
  - Network form uses one primary action: "Save and connect";
  - endpoints added: `/network_status`, `/scan_networks`, `/save_network`,
    `/network_reconnect`;
  - network endpoints are protected with Basic Auth;
  - temporary Network status request failures preserve the last real status
    instead of displaying mock AP data;
  - network watchdog changes are pending local build and hardware verification.
- Runtime input isolation and non-blocking network scan stage added and
  successfully built locally; hardware verification remains pending:
  - Data1..Data8 and `status_mapper` are updated by a dedicated 50 ms FreeRTOS
    task, independently of HTTP handlers and the Arduino `loop()`;
  - input and mapped-status readers use short critical-section snapshots instead
    of sharing mutable state by reference;
  - Wi-Fi scanning uses the framework asynchronous scan mode; `/scan_networks`
    returns HTTP 202 while scanning and the UI polls until results are ready;
  - AP fallback changes temporarily to AP+STA only while a scan is active, then
    returns to AP-only; abandoned scans are cleaned up after 15 seconds.
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
- Startup animation added:
  - on boot the matrix briefly flashes red, blue, then green;
  - the animation is skipped or interrupted if any active port has an error
    input active, then runtime rendering resumes immediately;
  - the animation does not change saved zones, colors, free zones, topology, or
    storage.
- UI clarity polish added:
  - unmarked free zones remain visible in the dropdown but are disabled with a
    `not marked` label;
  - UI warns when an active port has no mapped pixels;
  - clearing an active port zone asks for explicit confirmation;
  - service menu is a centered modal on desktop and full-screen panel on mobile;
  - service menu closes only through the Close button;
  - network and diagnostics action labels were clarified.
  - local build and manual desktop/mobile UI check passed after this stage.
- UI clarity polish follow-up:
  - topology controls stay in `Matrix Zones`, but are hidden inside the
    `Matrix settings` expandable block so the main zone editor stays focused;
  - free-zone brightness is labeled as brightness for the selected free zone;
  - hardware live preview remains intentionally disabled; preview is browser-side.
- Brightness controls were unified without changing backend endpoints:
  - port brightness and selected free-zone brightness controls update
    immediately and persist after slider release or a short input debounce;
  - browser matrix preview keeps a fixed readable visual brightness; brightness
    controls affect only persisted/physical matrix brightness;
  - brightness persistence is independent from status-layer/free-zone content
    save buttons;
  - free-zone brightness uses partial `/save_free_zone` payloads, so changing
    brightness does not save pending mode, color, or custom-layer edits;
  - batch brightness applies only to mapped free zones and also sends
    brightness-only payloads.
- UI terminology polish keeps zone `0` as the unassigned pixel state labeled
  `Без зоны`, but excludes it from the active-zone selector because pixels are
  returned to zone `0` with the eraser and clear actions. Mixed implementation
  terms use Russian user-facing wording, and the Service icon is a lightweight
  inline SVG.
- Matrix UI context and dimension safety were tightened:
  - missing active-port zones are shown once in a dedicated warning above the
    matrix instead of being repeated in editor notices;
  - the matrix header keeps only dimensions and the selected zone/status/mode,
    while backend-unavailable text is limited to mock mode;
  - topology numbering remains available up to 256 pixels, with a compact
    direction/start/end summary for larger matrices;
  - invalid or dimension-incompatible matrix storage clears matrix/status/free
    layer keys in `led_settings` before loading defaults, without touching
    network or auth namespaces.
- UI diagnostics reliability pass added:
  - mock mode is selected only when the initial `/get_config` request is
    unavailable;
  - a temporary `/diagnostics` failure reports lost communication without
    disabling the real backend session, and later requests can recover;
  - diagnostics polling runs every 2.5 seconds only while its service section
    is open; outside it, a 30-second refresh keeps the header runtime status
    current and allows communication recovery without frequent heavy requests;
  - network and diagnostics values are rendered through DOM `textContent`
    instead of inserting external strings through `innerHTML`.
- API/storage correctness pass implemented and successfully built locally;
  save/reload/reboot behavior remains part of hardware acceptance testing:
  - current save endpoints return success only after NVS write/read-back
    verification; storage failures return HTTP 500 instead of false `OK`;
  - `/set_bright` is POST-only and its UI caller uses POST form data;
  - status/free custom layers are strictly validated for JSON shape, color,
    duplicate coordinates, entry count, and current compile-time matrix bounds;
  - `/save_zones` rejects malformed or out-of-range map entries instead of
    silently saving a partial map;
  - `/save_free_zone` validates the full payload before writing, and a
    brightness-only payload changes only that zone's brightness;
  - frontend canonical sources are `matrix.topology` and
    `free_zones[].mode`; legacy response aliases remain for compatibility;
  - frontend status fallback colors come from `/get_config`, with hardcoded
    colors retained only for mock/invalid-response fallback;
  - multi-key status/free/network/auth writes use best-effort rollback on a
    detected write failure. NVS has no transaction across separate keys, so a
    sudden power loss between commits remains a small residual consistency
    risk.
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
  - UI allows temporary clearing while editing and warns that the invalid map
    cannot be saved until every active port has at least one pixel;
  - backend `/save_zones` validates active port zone presence before applying
    a new zone map.

## Currently In Work

The safe cleanup, diagnostics reliability pass, API/storage correctness pass,
and mechanical neutral rename have been successfully built locally.

Current focus: hardware verification of isolated Data input updates,
asynchronous Wi-Fi scanning, non-blocking STA, network watchdog transitions,
and controlled AP fallback.

Known active review topics:

- Recheck zones, statuses, free zones, diagnostics, network, auth, and OTA after
  flashing the tested build to hardware.
- Verify diagnostics polling uses the 2.5-second active rate only while the
  diagnostics service section is open, falls back to a 30-second status refresh
  outside it, and recovers after a temporary failure.
- Verify matrix zones, topology, status layers/default colors, free-zone full
  and brightness-only saves, network settings, and auth credentials survive
  reload and reboot and do not report success on failed writes.

## Known UI Issues

- Backend zone metadata names are still English (`Port 1`, `Logo`, `Service`, `Custom`), but the frontend now localizes zone labels by fixed zone ID.
- Free Zones custom pixel drawing was separated from static color in the frontend:
  - static mode fills the selected free zone;
  - custom drawing uses the selected color as a brush;
  - clear drawing clears only the selected free zone custom layer.
- Matrix Zones now warn if an active port is not mapped, because inputs can work
  while no pixels are available for status indication.
- Main UI status line also warns when any active port zone is missing pixels.
- Matrix zone, status layer, and free zone editors show dirty indicators when
  changes are local and not yet saved.
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
  - consider reset/clear-network flow only if field testing shows it is needed.
- Auth hardening follow-up:
  - password hashing, captive portal, HTTPS, signed OTA, physical factory reset,
    and auth reset are intentionally deferred.
- OTA follow-up:
  - add firmware version display;
  - consider signed firmware validation;
  - consider rollback/recovery behavior;
  - improve post-reboot reconnect guidance after hardware testing.
- Brightness schedule by time of day is deferred until real matrix brightness is
  checked on hardware or a customer requirement confirms it is needed.
  - `proj1` had related night behavior through SNTP, sunrise/sunset calculation,
    coordinates, and backlight brightness.
  - `proj1` did not have a universal per-zone LED matrix brightness schedule.
  - For this project, schedule is not critical before real matrix validation.
  - If needed later, MVP should be: enabled/disabled, night start/end,
    day/night brightness multiplier, NTP/timezone status, and fallback that keeps
    manual brightness when time is unavailable.
- Full WiFi manager polish beyond current MVP.
- Multi-matrix outputs on LED1..LED4.
- Runtime configurable matrix dimensions.
- Topology follow-up:
  - hardware-safe test pattern after real matrix hardware is available;
  - clearer future topology model with start corner, direction, serpentine,
    horizontal/vertical orientation, and color order separated from topology;
  - optional remap/reset UX if topology or matrix size changes.
- Optional per-port status color override.
- UI maintainability follow-up:
  - consider splitting `gui_html.h` into source HTML/CSS/JS with a lightweight
    build step when the single PROGMEM file becomes too hard to maintain;
  - do not introduce LittleFS/SPIFFS only for UI organization unless separate
    assets or field-update requirements justify it.
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
