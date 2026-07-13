# Agent Rules

These rules apply to work in this repository.

## Project Scope

- This is standalone ESP32 firmware for an LED matrix controller.
- Do not treat this as full EVSE firmware.
- Keep changes scoped to the standalone LED matrix controller unless the user
  explicitly asks otherwise.

## Required Context Maintenance

- After each significant implementation or design stage, update
  `PROJECT_STATE.md`.
- Keep `PROJECT_STATE.md` factual and current:
  - confirmed hardware mapping;
  - current zone model;
  - runtime/edit model;
  - completed work;
  - known issues;
  - backlog.

## Required Response Footer

Every final answer must include this block:

```text
Заключение для передачи другому диалогу:
- Что анализировалось.
- Какие проблемы найдены.
- Какие гипотезы по причинам.
- Что предлагаешь менять.
- Какие файлы будут затронуты.
- Что не трогать.
- Следующий шаг.
```

Adapt the contents to the current task, but keep the block present.

## Build and Verification

- Do not run PlatformIO automatically if the environment is unstable or if the
  user asked not to.
- Prefer asking the user to run:

```powershell
pio run -e esp32dev
```

- If the user explicitly asks the assistant to run a build, run it from the
  repository root and report the result.

## Areas Not To Change Without Separate Task

Do not modify these areas unless the user explicitly opens a task for them:

- WiFi manager.
- OTA implementation or OTA UX.
- Network settings.
- Rules engine or broader EVSE logic.
- Dynamic arbitrary zone creation/deletion.
- Per-port override logic.
- Brightness schedule.
- Multi-matrix FastLED outputs.
- Hardware pin mapping.

## UI Rules

- Preserve the visual direction:
  - light/white pleasant background;
  - blue primary buttons;
  - calm palette;
  - no dark theme;
  - no radical palette change.
- Visible web UI text should be Russian unless the user requests otherwise.
- Visible web UI should use neutral product wording such as
  `контроллер матрицы`, `LED-контроллер`, `матрица`, `входы`, and
  `выход LED1`; do not mention a specific board name in UI copy unless
  explicitly requested for a hardware-specific screen.
- Technical identifiers in code may remain English.
- Keep primary UI focused on engineering actions.
- Keep verbose diagnostic/technical details in the Diagnostics tab rather than
  spreading them across all panels.
- Runtime safety is more important than preview convenience:
  - UI preview must not hide runtime error indication;
  - port zones must not use rainbow;
  - free zones are separate from port runtime status.

## Git and GitHub Rules

- Do not push to GitHub without explicit user confirmation.
- Before commit or push, show:
  - `git status`;
  - list of files that will be committed/pushed;
  - short summary of changes.
- Push only after:
  - successful local build by the user; or
  - explicit user permission to push without a fresh build.
- Do not add unrelated reference projects, PDFs, `.pio`, firmware binaries,
  ELF files, or map files.
- For this repository, `Test-led` is the git root. Do not push the parent
  workspace.

## Safety Notes

- Do not print private keys, tokens, or SSH key contents.
- Do not change `.ssh` permissions unless explicitly asked.
- Do not use GitHub tokens unless explicitly provided and requested by the user.
