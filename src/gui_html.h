#ifndef GUI_HTML_H
#define GUI_HTML_H

#include <Arduino.h>

const char PAGE_MAIN[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
    <title>УСП-1: настройка LED-матрицы</title>
    <style>
        :root {
            --bg: #f5f7fb;
            --panel: #ffffff;
            --text: #1d1d1f;
            --muted: #6e7380;
            --line: #e3e8f0;
            --soft: #eef2f7;
            --blue: #0055ff;
            --blue-soft: #eaf1ff;
            --red: #ff3b30;
            --green: #34c759;
            --orange: #ff9500;
            --purple: #af52de;
            --shadow: 0 10px 26px rgba(25, 39, 65, 0.08);
        }

        * { box-sizing: border-box; }
        body {
            margin: 0;
            padding: 18px 14px 28px;
            background: var(--bg);
            color: var(--text);
            font-family: system-ui, -apple-system, "Segoe UI", Roboto, Arial, sans-serif;
            -webkit-font-smoothing: antialiased;
            user-select: none;
        }

        .app {
            max-width: 1180px;
            margin: 0 auto;
        }

        .topbar {
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 14px;
            margin-bottom: 14px;
        }

        .brand {
            display: flex;
            flex-direction: column;
            gap: 3px;
        }

        .brand h1 {
            margin: 0;
            font-size: 24px;
            line-height: 1.05;
            letter-spacing: 0;
            font-weight: 750;
        }

        .brand span {
            color: var(--muted);
            font-size: 12px;
            font-weight: 650;
            text-transform: uppercase;
            letter-spacing: 0.08em;
        }

        .runtime-pill {
            display: flex;
            align-items: center;
            gap: 8px;
            padding: 9px 12px;
            border: 1px solid var(--line);
            border-radius: 10px;
            background: var(--panel);
            color: var(--muted);
            font-size: 12px;
            font-weight: 700;
            white-space: nowrap;
        }

        .dot {
            width: 9px;
            height: 9px;
            border-radius: 999px;
            background: var(--green);
        }

        .tabs {
            display: grid;
            grid-template-columns: repeat(4, minmax(0, 1fr));
            gap: 8px;
            margin-bottom: 14px;
        }

        .tab-btn {
            border: 1px solid var(--line);
            border-radius: 10px;
            background: var(--panel);
            color: var(--muted);
            padding: 12px 10px;
            font-size: 12px;
            font-weight: 750;
            cursor: pointer;
        }

        .tab-btn.active {
            border-color: var(--blue);
            background: var(--blue);
            color: #ffffff;
        }

        .layout {
            display: grid;
            grid-template-columns: minmax(0, 1.05fr) minmax(300px, 0.95fr);
            gap: 14px;
            align-items: start;
        }

        .panel {
            background: var(--panel);
            border: 1px solid var(--line);
            border-radius: 12px;
            box-shadow: var(--shadow);
            padding: 16px;
        }

        .panel-title {
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 12px;
            margin-bottom: 12px;
        }

        .panel-title h2 {
            margin: 0;
            font-size: 16px;
            letter-spacing: 0;
        }

        .hint {
            color: var(--muted);
            font-size: 12px;
            line-height: 1.35;
        }

        .matrix-shell {
            overflow: auto;
            padding: 12px;
            border: 1px solid var(--line);
            border-radius: 12px;
            background: #fbfcff;
            max-height: 68vh;
        }

        .matrix {
            display: grid;
            gap: 5px;
            width: max-content;
            margin: 0 auto;
            touch-action: none;
        }

        .pixel {
            width: 30px;
            height: 30px;
            border: 1px solid rgba(0, 0, 0, 0.14);
            border-radius: 7px;
            background: #e8e8ed;
            cursor: pointer;
            transition: opacity 0.12s, transform 0.06s, box-shadow 0.12s;
        }

        .pixel.active-zone {
            box-shadow: 0 0 0 2px rgba(0, 85, 255, 0.22);
        }

        .pixel.locked {
            opacity: 0.24;
            filter: grayscale(80%);
            cursor: not-allowed;
        }

        .pixel.off-zone {
            background-image: linear-gradient(135deg, rgba(0,0,0,0.05) 25%, transparent 25%, transparent 50%, rgba(0,0,0,0.05) 50%, rgba(0,0,0,0.05) 75%, transparent 75%, transparent);
            background-size: 10px 10px;
        }

        .side-stack {
            display: flex;
            flex-direction: column;
            gap: 14px;
        }

        .tab-panel { display: none; }
        .tab-panel.active { display: block; }

        .section {
            display: flex;
            flex-direction: column;
            gap: 12px;
        }

        .row {
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 12px;
            min-height: 38px;
        }

        label {
            display: block;
            color: var(--muted);
            font-size: 11px;
            font-weight: 750;
            text-transform: uppercase;
            letter-spacing: 0.06em;
            margin-bottom: 6px;
        }

        select, input[type="color"] {
            width: 100%;
            border: 1px solid var(--line);
            border-radius: 9px;
            background: #ffffff;
            color: var(--text);
            padding: 9px 10px;
            font: inherit;
            font-size: 13px;
            font-weight: 650;
        }

        input[type="range"] {
            width: 100%;
            accent-color: var(--blue);
        }

        input[type="color"] {
            height: 42px;
            padding: 3px;
            cursor: pointer;
        }

        .grid-2 {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
        }

        .btn-row {
            display: flex;
            gap: 8px;
            flex-wrap: wrap;
        }

        .btn {
            border: 1px solid var(--line);
            border-radius: 10px;
            background: #ffffff;
            color: var(--text);
            padding: 11px 12px;
            font-size: 12px;
            font-weight: 750;
            cursor: pointer;
            min-height: 40px;
        }

        .btn.primary {
            border-color: var(--blue);
            background: var(--blue);
            color: #ffffff;
        }

        .btn.danger {
            color: var(--red);
            border-color: #ffd2cf;
            background: #fff5f4;
        }

        .btn.active {
            border-color: var(--blue);
            background: var(--blue-soft);
            color: var(--blue);
        }

        .btn:disabled, select option:disabled {
            opacity: 0.45;
            cursor: not-allowed;
        }

        .legend {
            display: grid;
            grid-template-columns: repeat(2, minmax(0, 1fr));
            gap: 8px;
        }

        .legend-item {
            display: flex;
            align-items: center;
            gap: 8px;
            min-height: 30px;
            padding: 6px 8px;
            border: 1px solid var(--line);
            border-radius: 9px;
            background: #fbfcff;
            color: var(--muted);
            font-size: 12px;
            font-weight: 700;
        }

        .legend-item.selected {
            border-color: var(--blue);
            color: var(--blue);
            background: var(--blue-soft);
        }

        .swatch {
            width: 14px;
            height: 14px;
            border-radius: 4px;
            border: 1px solid rgba(0,0,0,0.18);
            flex: 0 0 auto;
        }

        .notice {
            padding: 10px 12px;
            border-radius: 10px;
            border: 1px solid #dbe6ff;
            background: #f4f8ff;
            color: #315b9f;
            font-size: 12px;
            line-height: 1.35;
        }

        .diag-grid {
            display: grid;
            grid-template-columns: repeat(2, minmax(0, 1fr));
            gap: 8px;
        }

        .diag-card {
            padding: 10px;
            border: 1px solid var(--line);
            border-radius: 10px;
            background: #fbfcff;
        }

        .diag-card strong {
            display: block;
            font-size: 12px;
            margin-bottom: 4px;
        }

        .diag-card span {
            color: var(--muted);
            font-size: 12px;
            line-height: 1.35;
        }

        .status-line {
            display: flex;
            gap: 8px;
            align-items: center;
            color: var(--muted);
            font-size: 12px;
            min-height: 18px;
        }

        @media (max-width: 860px) {
            body { padding: 12px 10px 24px; }
            .topbar { align-items: flex-start; }
            .brand h1 { font-size: 21px; }
            .tabs { grid-template-columns: repeat(2, minmax(0, 1fr)); }
            .layout { grid-template-columns: 1fr; }
            .panel { padding: 13px; border-radius: 11px; }
            .pixel { width: 28px; height: 28px; border-radius: 7px; }
            .legend { grid-template-columns: 1fr; }
            .diag-grid { grid-template-columns: 1fr; }
        }

        @media (max-width: 420px) {
            .pixel { width: 24px; height: 24px; border-radius: 6px; }
            .matrix { gap: 4px; }
            .grid-2 { grid-template-columns: 1fr; }
            .runtime-pill { display: none; }
        }
    </style>
</head>
<body>
<div class="app">
    <header class="topbar">
        <div class="brand">
            <h1>УСП-1 LED-матрица</h1>
            <span>инженерная настройка контроллера</span>
        </div>
        <div class="runtime-pill"><span class="dot" id="runtime_dot"></span><span id="runtime_label">режим: загрузка</span></div>
    </header>

    <nav class="tabs" aria-label="Разделы настройки">
        <button class="tab-btn active" data-tab="zones">Зоны матрицы</button>
        <button class="tab-btn" data-tab="status">Статусы портов</button>
        <button class="tab-btn" data-tab="free">Свободные зоны</button>
        <button class="tab-btn" data-tab="diagnostics">Диагностика</button>
    </nav>

    <main class="layout">
        <section class="panel">
            <div class="panel-title">
                <h2 id="matrix_title">Зоны матрицы</h2>
                <span class="hint" id="matrix_hint">12x8, локальное редактирование</span>
            </div>
            <div class="matrix-shell">
                <div class="matrix" id="matrix_grid"></div>
            </div>
            <div class="status-line" id="edit_state_line">Редактор: изменения локальные до сохранения.</div>
        </section>

        <aside class="side-stack">
            <section class="panel tab-panel active" id="tab_zones">
                <div class="panel-title"><h2>Зоны матрицы</h2></div>
                <div class="section">
                    <div>
                        <label for="zone_select">Активная зона</label>
                        <select id="zone_select"></select>
                    </div>
                    <div class="btn-row">
                        <button class="btn active" id="zone_brush_btn">Кисть</button>
                        <button class="btn" id="zone_erase_btn">Ластик</button>
                    </div>
                    <div class="legend" id="zone_legend"></div>
                    <div class="notice">Рисуйте внутри выбранной зоны. Чужие зоны заблокированы.</div>
                    <div class="btn-row">
                        <button class="btn primary" id="save_zones_btn">Сохранить зоны</button>
                        <button class="btn danger" id="clear_zone_btn">Очистить выбранную зону</button>
                    </div>
                </div>
            </section>

            <section class="panel tab-panel" id="tab_status">
                <div class="panel-title"><h2>Статусы портов</h2></div>
                <div class="section">
                    <div class="btn-row">
                        <button class="btn active" data-status="waiting">Ожидание</button>
                        <button class="btn" data-status="charging">Зарядка</button>
                        <button class="btn" data-status="error">Ошибка</button>
                    </div>
                    <div class="grid-2">
                        <div>
                            <label for="status_zone_select">Портовая зона</label>
                            <select id="status_zone_select"></select>
                        </div>
                        <div>
                            <label for="status_color">Цвет кисти</label>
                            <input type="color" id="status_color" value="#0055ff">
                        </div>
                    </div>
                    <div class="grid-2">
                        <div>
                            <label for="ports_brightness">Яркость портов</label>
                            <input type="range" id="ports_brightness" min="1" max="255" value="120">
                        </div>
                        <div class="status-line" id="ports_brightness_label">120</div>
                    </div>
                    <div class="notice">Предпросмотр не меняет runtime. Ошибка остаётся приоритетной.</div>
                    <div class="btn-row">
                        <button class="btn primary" id="save_status_btn">Сохранить слой статуса</button>
                        <button class="btn danger" id="clear_status_btn">Очистить слой статуса</button>
                    </div>
                </div>
            </section>

            <section class="panel tab-panel" id="tab_free">
                <div class="panel-title"><h2>Свободные зоны</h2></div>
                <div class="section">
                    <div class="grid-2">
                        <div>
                            <label for="free_zone_select">Свободная зона</label>
                            <select id="free_zone_select"></select>
                        </div>
                        <div>
                            <label for="free_mode_select">Режим</label>
                            <select id="free_mode_select">
                                <option value="static">Статичный цвет</option>
                                <option value="custom">Пиксельный рисунок</option>
                                <option value="rainbow">Радуга</option>
                            </select>
                        </div>
                    </div>
                    <div class="grid-2">
                        <div>
                            <label for="free_color">Цвет зоны</label>
                            <input type="color" id="free_color" value="#ffffff">
                        </div>
                        <div>
                            <label for="free_brightness">Яркость свободных зон</label>
                            <input type="range" id="free_brightness" min="1" max="255" value="100">
                        </div>
                    </div>
                    <div class="notice">Логотип сохраняется сейчас. QR, Сервис и Своя зона пока работают как предпросмотр.</div>
                    <div class="btn-row">
                        <button class="btn active" id="free_brush_btn">Кисть</button>
                        <button class="btn" id="free_erase_btn">Очистить рисунок</button>
                        <button class="btn primary" id="apply_free_btn">Применить доступные настройки</button>
                    </div>
                </div>
            </section>

            <section class="panel tab-panel" id="tab_diagnostics">
                <div class="panel-title"><h2>Диагностика</h2><button class="btn" id="refresh_diag_btn">Обновить</button></div>
                <div class="section">
                    <div class="diag-grid" id="diag_summary"></div>
                    <div>
                        <label>Входы Data1..Data8</label>
                        <div class="diag-grid" id="diag_inputs"></div>
                    </div>
                    <div>
                        <label>Порты</label>
                        <div class="diag-grid" id="diag_ports"></div>
                    </div>
                    <div>
                        <label>LED-выходы</label>
                        <div class="diag-grid" id="diag_leds"></div>
                    </div>
                </div>
            </section>
        </aside>
    </main>
</div>

<script>
const DEFAULT_ZONES = [
    { id: 0, name: 'Выключено', type: 'off', enabled: true, reserved: false, linked_port: null, mode: null },
    { id: 1, name: 'Порт 1', type: 'port', enabled: true, reserved: false, linked_port: 1, mode: null },
    { id: 2, name: 'Порт 2', type: 'port', enabled: false, reserved: true, linked_port: 2, mode: null },
    { id: 3, name: 'Порт 3', type: 'port', enabled: false, reserved: true, linked_port: 3, mode: null },
    { id: 4, name: 'Порт 4', type: 'port', enabled: false, reserved: true, linked_port: 4, mode: null },
    { id: 5, name: 'Логотип', type: 'free', enabled: true, reserved: false, linked_port: null, mode: 'static' },
    { id: 6, name: 'QR', type: 'free', enabled: true, reserved: false, linked_port: null, mode: 'static' },
    { id: 7, name: 'Сервис', type: 'free', enabled: true, reserved: false, linked_port: null, mode: 'static' },
    { id: 8, name: 'Пользовательская', type: 'free', enabled: true, reserved: false, linked_port: null, mode: 'custom' },
];

const zoneColors = {
    '0': '#e8e8ed',
    '1': '#0055ff',
    '2': '#4a8eff',
    '3': '#78b4ff',
    '4': '#b4d2ff',
    '5': '#1d1d1f',
    '6': '#34c759',
    '7': '#ff9500',
    '8': '#af52de',
};

const statusDefaults = {
    waiting: '#34c759',
    charging: '#0055ff',
    error: '#ff3b30',
};

const STATUS_LABELS = {
    waiting: 'ожидание',
    charging: 'зарядка',
    error: 'ошибка',
    disabled: 'отключён',
    unknown: 'неизвестно',
};

const TYPE_LABELS = {
    off: 'выкл.',
    port: 'порт',
    free: 'свободная',
};

const MODE_LABELS = {
    static: 'статичный цвет',
    custom: 'пиксельный рисунок',
    rainbow: 'радуга',
};

const ZONE_LABELS = {
    '0': 'Выкл.',
    '1': 'Порт 1',
    '2': 'Порт 2',
    '3': 'Порт 3',
    '4': 'Порт 4',
    '5': 'Логотип',
    '6': 'QR',
    '7': 'Сервис',
    '8': 'Своя зона',
};

const DEFAULT_FREE_STATIC_COLORS = {
    '5': '#ffffff',
    '6': '#34c759',
    '7': '#ff9500',
    '8': '#af52de',
};

let COLS = 12;
let ROWS = 8;
let activeTab = 'zones';
let activeZoneId = '1';
let activeStatus = 'waiting';
let activeStatusZone = '1';
let activeFreeZone = '5';
let zoneTool = 'brush';
let freeTool = 'brush';
let drawing = false;
let hardwareZonesMap = {};
let zoneMeta = [...DEFAULT_ZONES];
let statusColorLayers = { waiting: {}, charging: {}, error: {} };
let freeModes = { '5': 'static', '6': 'static', '7': 'static', '8': 'custom' };
let freeCustomLayers = { '5': {}, '6': {}, '7': {}, '8': {} };
let freeStaticColors = { ...DEFAULT_FREE_STATIC_COLORS };
let freeBrushColors = { ...DEFAULT_FREE_STATIC_COLORS };
let diagnostics = null;
let rainbowTick = 0;
let backendAvailable = false;
let mockMode = false;
let matrixConfig = { cols: 12, rows: 8, total: 96, topology: 0 };

const grid = document.getElementById('matrix_grid');

function statusLabel(status) {
    return STATUS_LABELS[status] || status || 'неизвестно';
}

function typeLabel(type) {
    return TYPE_LABELS[type] || type || 'неизвестно';
}

function modeLabel(mode) {
    return MODE_LABELS[mode] || mode || 'не задан';
}

function zoneDisplayName(zoneOrId) {
    const id = typeof zoneOrId === 'object' ? zoneOrId.id : zoneOrId;
    return ZONE_LABELS[String(id)] || (typeof zoneOrId === 'object' ? zoneOrId.name : String(id));
}

function zoneOptionLabel(zone) {
    return `${zoneDisplayName(zone)}${zone.reserved ? ' · резерв' : ''}`;
}

function buildMockConfig() {
    return {
        topo: 0,
        l_anim: 0,
        b_ports: 120,
        b_logo: 100,
        c_wait: '#34c759',
        c_charge: '#0055ff',
        c_error: '#ff3b30',
        c_logo: '#ffffff',
        matrix: { cols: 12, rows: 8, total: 96, topology: 0 },
        hardware_map: {},
        layers: { wait: {}, charge: {}, err: {} },
        zones: [...DEFAULT_ZONES],
    };
}

function buildMockDiagnostics() {
    const inputs = Array.from({ length: 8 }, (_, i) => ({
        name: `Data${i + 1}`,
        gpio: [16, 17, 18, 19, 21, 25, 26, 27][i],
        raw: 'HIGH',
        active: false,
    }));

    return {
        activePortCount: 1,
        runtimeMode: mockMode ? 'локальный предпросмотр' : 'runtime',
        primaryLedOutput: { name: 'LED1', gpio: 22, index: 1 },
        ledOutputs: [
            { name: 'LED1', gpio: 22, state: 'primary' },
            { name: 'LED2', gpio: 23, state: 'reserved' },
            { name: 'LED3', gpio: 32, state: 'reserved' },
            { name: 'LED4', gpio: 33, state: 'reserved' },
        ],
        inputs,
        ports: [
            { port: 1, enabled: true, zoneId: 1, chargingInput: 'Data1', errorInput: 'Data2', status: 'waiting' },
            { port: 2, enabled: false, zoneId: 2, chargingInput: 'Data3', errorInput: 'Data4', status: 'disabled' },
            { port: 3, enabled: false, zoneId: 3, chargingInput: 'Data5', errorInput: 'Data6', status: 'disabled' },
            { port: 4, enabled: false, zoneId: 4, chargingInput: 'Data7', errorInput: 'Data8', status: 'disabled' },
        ],
    };
}

function sanitizeZoneMapForSize(map) {
    const clean = {};
    Object.entries(map || {}).forEach(([key, value]) => {
        const [x, y] = key.split('-').map(Number);
        if (Number.isInteger(x) && Number.isInteger(y) && x >= 0 && y >= 0 && x < COLS && y < ROWS) {
            clean[key] = String(value);
        }
    });
    return clean;
}

function sanitizeLayerForSize(layer) {
    const clean = {};
    Object.entries(layer || {}).forEach(([key, value]) => {
        const [x, y] = key.split('-').map(Number);
        if (Number.isInteger(x) && Number.isInteger(y) && x >= 0 && y >= 0 && x < COLS && y < ROWS) {
            clean[key] = value;
        }
    });
    return clean;
}

function syncFreeColorInput() {
    const mode = freeModes[activeFreeZone] || 'static';
    const color = mode === 'custom'
        ? (freeBrushColors[activeFreeZone] || '#ffffff')
        : (freeStaticColors[activeFreeZone] || '#ffffff');
    document.getElementById('free_color').value = color;
}

function applyConfig(cfg) {
    const matrix = cfg.matrix || {};
    COLS = Number(matrix.cols) || COLS || 12;
    ROWS = Number(matrix.rows) || ROWS || 8;
    matrixConfig = {
        cols: COLS,
        rows: ROWS,
        total: Number(matrix.total) || COLS * ROWS,
        topology: Number(matrix.topology ?? cfg.topo ?? 0),
    };

    hardwareZonesMap = sanitizeZoneMapForSize(cfg.hardware_map || {});
    statusColorLayers.waiting = sanitizeLayerForSize(cfg.layers?.wait || {});
    statusColorLayers.charging = sanitizeLayerForSize(cfg.layers?.charge || {});
    statusColorLayers.error = sanitizeLayerForSize(cfg.layers?.err || {});
    Object.keys(freeCustomLayers).forEach(zoneId => {
        freeCustomLayers[zoneId] = sanitizeLayerForSize(freeCustomLayers[zoneId]);
    });
    zoneMeta = Array.isArray(cfg.zones) && cfg.zones.length ? cfg.zones : [...DEFAULT_ZONES];
    document.getElementById('ports_brightness').value = cfg.b_ports || 120;
    document.getElementById('ports_brightness_label').textContent = cfg.b_ports || 120;
    document.getElementById('free_brightness').value = cfg.b_logo || 100;
    freeStaticColors['5'] = cfg.c_logo || freeStaticColors['5'] || '#ffffff';
    zoneMeta.filter(z => z.type === 'free').forEach(z => { freeModes[String(z.id)] = z.mode || freeModes[String(z.id)] || 'static'; });
}

function zoneById(id) {
    return zoneMeta.find(z => String(z.id) === String(id)) || DEFAULT_ZONES.find(z => String(z.id) === String(id));
}

function isEditableZone(id) {
    const z = zoneById(id);
    return z && z.enabled && !z.reserved && z.type !== 'off';
}

function isPortZone(id) {
    const z = zoneById(id);
    return z && z.type === 'port';
}

function isFreeZone(id) {
    const z = zoneById(id);
    return z && z.type === 'free';
}

function keyOf(x, y) {
    return `${x}-${y}`;
}

function buildMatrix() {
    grid.innerHTML = '';
    grid.style.gridTemplateColumns = `repeat(${COLS}, 30px)`;

    for (let y = ROWS - 1; y >= 0; y--) {
        for (let x = 0; x < COLS; x++) {
            const p = document.createElement('div');
            p.className = 'pixel';
            p.dataset.x = x;
            p.dataset.y = y;
            p.dataset.zone = '0';
            grid.appendChild(p);
        }
    }

    redrawMatrix();
}

function buildSelects() {
    const zoneSelect = document.getElementById('zone_select');
    const statusZoneSelect = document.getElementById('status_zone_select');
    const freeZoneSelect = document.getElementById('free_zone_select');
    zoneSelect.innerHTML = '';
    statusZoneSelect.innerHTML = '';
    freeZoneSelect.innerHTML = '';

    zoneMeta.forEach(z => {
        const opt = document.createElement('option');
        opt.value = z.id;
        opt.textContent = zoneOptionLabel(z);
        opt.disabled = z.reserved || !z.enabled;
        zoneSelect.appendChild(opt);

        if (z.type === 'port') {
            const po = document.createElement('option');
            po.value = z.id;
            po.textContent = zoneOptionLabel(z);
            po.disabled = z.reserved || !z.enabled;
            statusZoneSelect.appendChild(po);
        }

        if (z.type === 'free') {
            const fo = document.createElement('option');
            fo.value = z.id;
            fo.textContent = zoneDisplayName(z);
            freeZoneSelect.appendChild(fo);
        }
    });

    zoneSelect.value = activeZoneId;
    statusZoneSelect.value = activeStatusZone;
    freeZoneSelect.value = activeFreeZone;
}

function buildLegend() {
    const legend = document.getElementById('zone_legend');
    legend.innerHTML = '';

    zoneMeta.forEach(z => {
        if (z.id === 0) return;
        const item = document.createElement('div');
        item.className = 'legend-item' + (String(z.id) === activeZoneId ? ' selected' : '');
        const swatch = document.createElement('span');
        swatch.className = 'swatch';
        swatch.style.background = zoneColors[String(z.id)] || '#d1d1d6';
        const text = document.createElement('span');
        text.textContent = `${zoneDisplayName(z)} · ${typeLabel(z.type)}${z.reserved ? ' · резерв' : ''}`;
        item.appendChild(swatch);
        item.appendChild(text);
        legend.appendChild(item);
    });
}

function pixelStatusColor(key, zoneId) {
    const custom = statusColorLayers[activeStatus][key];
    if (custom && isPortZone(zoneId)) return custom;
    if (isPortZone(zoneId)) return statusDefaults[activeStatus];
    if (isFreeZone(zoneId)) return zoneColors[zoneId];
    return zoneColors['0'];
}

function freeZoneColor(key, zoneId) {
    const mode = freeModes[zoneId] || 'static';
    if (mode === 'custom' && freeCustomLayers[zoneId] && freeCustomLayers[zoneId][key]) {
        return freeCustomLayers[zoneId][key];
    }
    if (mode === 'rainbow') {
        const n = Number(key.split('-')[0]) + Number(key.split('-')[1]) + rainbowTick;
        return `hsl(${(n * 28) % 360}, 92%, 54%)`;
    }
    if (mode === 'custom') {
        return zoneColors[zoneId] || '#e8e8ed';
    }
    return freeStaticColors[zoneId] || '#ffffff';
}

function shouldLockPixel(zoneId) {
    if (activeTab === 'zones') {
        if (!isEditableZone(activeZoneId)) return true;
        return !(zoneId === '0' || zoneId === activeZoneId);
    }
    if (activeTab === 'status') {
        return zoneId !== activeStatusZone;
    }
    if (activeTab === 'free') {
        return zoneId !== activeFreeZone;
    }
    return false;
}

function redrawMatrix() {
    document.getElementById('matrix_title').textContent =
        activeTab === 'zones' ? 'Зоны матрицы' :
        activeTab === 'status' ? 'Предпросмотр статусов портов' :
        activeTab === 'free' ? 'Предпросмотр свободных зон' : 'Диагностика';

    document.getElementById('matrix_hint').textContent =
        activeTab === 'zones' ? `${COLS}x${ROWS}, редактирование выбранной зоны` :
        activeTab === 'status' ? `слой "${statusLabel(activeStatus)}", предпросмотр только в UI` :
        activeTab === 'free' ? `${zoneDisplayName(activeFreeZone)}: ${modeLabel(freeModes[activeFreeZone])}` :
        'состояние runtime из /diagnostics';

    grid.querySelectorAll('.pixel').forEach(p => {
        const key = keyOf(p.dataset.x, p.dataset.y);
        const zoneId = hardwareZonesMap[key] || '0';
        p.dataset.zone = zoneId;
        p.classList.toggle('off-zone', zoneId === '0');
        p.classList.toggle('active-zone',
            (activeTab === 'zones' && zoneId === activeZoneId) ||
            (activeTab === 'status' && zoneId === activeStatusZone) ||
            (activeTab === 'free' && zoneId === activeFreeZone));
        p.classList.toggle('locked', shouldLockPixel(zoneId));

        if (activeTab === 'status') {
            p.style.background = pixelStatusColor(key, zoneId);
        } else if (activeTab === 'free') {
            p.style.background = zoneId === activeFreeZone ? freeZoneColor(key, zoneId) : (zoneColors[zoneId] || zoneColors['0']);
        } else {
            p.style.background = zoneColors[zoneId] || zoneColors['0'];
        }
    });

    buildLegend();
    updateEditState();
}

function updateEditState() {
    const text =
        activeTab === 'zones' ? `Редактор: зоны матрицы · зона ${activeZoneId} · ${zoneTool === 'erase' ? 'ластик' : 'кисть'}` :
        activeTab === 'status' ? `Редактор: статусы портов · ${statusLabel(activeStatus)} · зона ${activeStatusZone}` :
        activeTab === 'free' ? `Редактор: свободные зоны · зона ${activeFreeZone} · ${modeLabel(freeModes[activeFreeZone])}` :
        'Диагностика: только просмотр';
    document.getElementById('edit_state_line').textContent = mockMode
        ? `${text}. Контроллер недоступен: работает локальный предпросмотр 12x8.`
        : text;
}

function paintPixel(pixel) {
    if (!pixel || !pixel.classList.contains('pixel')) return;
    const key = keyOf(pixel.dataset.x, pixel.dataset.y);
    const currentZone = hardwareZonesMap[key] || '0';

    if (activeTab === 'zones') {
        if (!isEditableZone(activeZoneId)) return;
        if (zoneTool === 'erase') {
            if (currentZone === activeZoneId) delete hardwareZonesMap[key];
        } else if (currentZone === '0' || currentZone === activeZoneId) {
            hardwareZonesMap[key] = activeZoneId;
        }
    } else if (activeTab === 'status') {
        if (currentZone !== activeStatusZone) return;
        const color = document.getElementById('status_color').value;
        statusColorLayers[activeStatus][key] = color;
    } else if (activeTab === 'free') {
        if (currentZone !== activeFreeZone || freeModes[activeFreeZone] !== 'custom') return;
        freeCustomLayers[activeFreeZone][key] = freeBrushColors[activeFreeZone] || document.getElementById('free_color').value;
    }

    redrawMatrix();
}

function pixelFromEvent(e) {
    const point = e.touches ? e.touches[0] : e;
    return document.elementFromPoint(point.clientX, point.clientY);
}

grid.addEventListener('pointerdown', e => {
    if (!e.target.classList.contains('pixel')) return;
    e.preventDefault();
    drawing = true;
    e.target.setPointerCapture?.(e.pointerId);
    paintPixel(e.target);
});

grid.addEventListener('pointermove', e => {
    if (!drawing) return;
    e.preventDefault();
    paintPixel(pixelFromEvent(e));
});

['pointerup', 'pointercancel', 'pointerleave'].forEach(type => {
    grid.addEventListener(type, () => { drawing = false; });
});

document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        activeTab = btn.dataset.tab;
        document.querySelectorAll('.tab-btn').forEach(b => b.classList.toggle('active', b === btn));
        document.querySelectorAll('.tab-panel').forEach(p => p.classList.toggle('active', p.id === `tab_${activeTab}`));
        redrawMatrix();
        if (activeTab === 'diagnostics') loadDiagnostics();
    });
});

document.getElementById('zone_select').addEventListener('change', e => {
    activeZoneId = e.target.value;
    redrawMatrix();
});

document.getElementById('status_zone_select').addEventListener('change', e => {
    activeStatusZone = e.target.value;
    redrawMatrix();
});

document.getElementById('free_zone_select').addEventListener('change', e => {
    activeFreeZone = e.target.value;
    document.getElementById('free_mode_select').value = freeModes[activeFreeZone] || 'static';
    syncFreeColorInput();
    redrawMatrix();
});

document.getElementById('free_mode_select').addEventListener('change', e => {
    freeModes[activeFreeZone] = e.target.value;
    syncFreeColorInput();
    redrawMatrix();
});

document.getElementById('free_color').addEventListener('input', e => {
    if ((freeModes[activeFreeZone] || 'static') === 'custom') {
        freeBrushColors[activeFreeZone] = e.target.value;
    } else {
        freeStaticColors[activeFreeZone] = e.target.value;
    }
    redrawMatrix();
});
document.getElementById('status_color').addEventListener('input', redrawMatrix);

document.getElementById('zone_brush_btn').addEventListener('click', () => {
    zoneTool = 'brush';
    document.getElementById('zone_brush_btn').classList.add('active');
    document.getElementById('zone_erase_btn').classList.remove('active');
    updateEditState();
});

document.getElementById('zone_erase_btn').addEventListener('click', () => {
    zoneTool = 'erase';
    document.getElementById('zone_erase_btn').classList.add('active');
    document.getElementById('zone_brush_btn').classList.remove('active');
    updateEditState();
});

document.getElementById('free_brush_btn').addEventListener('click', () => {
    freeTool = 'brush';
    document.getElementById('free_brush_btn').classList.add('active');
    document.getElementById('free_erase_btn').classList.remove('active');
});

document.getElementById('free_erase_btn').addEventListener('click', () => {
    freeCustomLayers[activeFreeZone] = {};
    freeTool = 'brush';
    document.getElementById('free_erase_btn').classList.remove('active');
    document.getElementById('free_brush_btn').classList.remove('active');
    document.getElementById('free_brush_btn').classList.add('active');
    redrawMatrix();
});

document.querySelectorAll('[data-status]').forEach(btn => {
    btn.addEventListener('click', () => {
        activeStatus = btn.dataset.status;
        document.querySelectorAll('[data-status]').forEach(b => b.classList.toggle('active', b === btn));
        document.getElementById('status_color').value = statusDefaults[activeStatus];
        redrawMatrix();
    });
});

document.getElementById('save_zones_btn').addEventListener('click', async () => {
    if (!backendAvailable) {
        alert('Контроллер недоступен: зоны изменены только в браузере.');
        return;
    }

    try {
        const res = await fetch('/save_zones', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(hardwareZonesMap)
        });
        alert(await res.text() === 'OK' ? 'Зоны сохранены во Flash.' : 'Не удалось сохранить зоны.');
    } catch (err) {
        alert('Контроллер недоступен: зоны изменены только в браузере.');
    }
});

document.getElementById('clear_zone_btn').addEventListener('click', () => {
    Object.keys(hardwareZonesMap).forEach(key => {
        if (hardwareZonesMap[key] === activeZoneId) delete hardwareZonesMap[key];
    });
    redrawMatrix();
});

document.getElementById('save_status_btn').addEventListener('click', async () => {
    if (!backendAvailable) {
        alert('Контроллер недоступен: слой статуса изменён только в браузере.');
        return;
    }

    try {
        const res = await fetch('/save_status_colors', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ status: activeStatus, colors: statusColorLayers[activeStatus] })
        });
        alert(await res.text() === 'OK' ? 'Слой статуса сохранён.' : 'Не удалось сохранить слой статуса.');
    } catch (err) {
        alert('Контроллер недоступен: слой статуса изменён только в браузере.');
    }
});

document.getElementById('clear_status_btn').addEventListener('click', () => {
    statusColorLayers[activeStatus] = {};
    redrawMatrix();
});

document.getElementById('ports_brightness').addEventListener('input', async e => {
    document.getElementById('ports_brightness_label').textContent = e.target.value;
    if (!backendAvailable) return;
    await fetch(`/set_bright?type=ports&val=${e.target.value}`).catch(() => {});
});

document.getElementById('free_brightness').addEventListener('input', async e => {
    if (!backendAvailable) return;
    await fetch(`/set_bright?type=logo&val=${e.target.value}`).catch(() => {});
});

document.getElementById('apply_free_btn').addEventListener('click', async () => {
    const mode = freeModes[activeFreeZone] || 'static';
    if (!backendAvailable) {
        alert('Контроллер недоступен: режим свободной зоны изменён только в браузере.');
        return;
    }

    if (activeFreeZone === '5') {
        await fetch(`/set_logo_anim?val=${mode === 'rainbow' ? 1 : 0}`);
        await fetch(`/save_config?c_logo=${encodeURIComponent(freeStaticColors['5'] || '#ffffff')}`);
        alert('Настройки зоны “Логотип” применены. Остальные свободные зоны пока работают как предпросмотр MVP.');
    } else {
        alert('Эта свободная зона пока работает как предпросмотр до добавления FreeZoneConfig.');
    }
});

document.getElementById('refresh_diag_btn').addEventListener('click', loadDiagnostics);

async function loadConfig() {
    let cfg = null;

    try {
        const res = await fetch('/get_config');
        if (!res.ok) throw new Error(`get_config ${res.status}`);
        cfg = await res.json();

        if (!cfg.matrix) {
            const sizeRes = await fetch('/get_size').catch(() => null);
            if (sizeRes && sizeRes.ok) {
                const parts = (await sizeRes.text()).split(':');
                cfg.matrix = {
                    cols: Number(parts[0]) || 12,
                    rows: Number(parts[1]) || 8,
                    total: (Number(parts[0]) || 12) * (Number(parts[1]) || 8),
                    topology: cfg.topo || 0,
                };
            }
        }

        backendAvailable = true;
        mockMode = false;
    } catch (err) {
        cfg = buildMockConfig();
        backendAvailable = false;
        mockMode = true;
    }

    applyConfig(cfg);
    buildSelects();
    syncFreeColorInput();
    buildMatrix();
    buildLegend();
    await loadDiagnostics();
}

async function loadDiagnostics() {
    if (!backendAvailable) {
        diagnostics = buildMockDiagnostics();
        renderDiagnostics();
        return;
    }

    try {
        const res = await fetch('/diagnostics');
        if (!res.ok) throw new Error(`diagnostics ${res.status}`);
        diagnostics = await res.json();
        renderDiagnostics();
    } catch (err) {
        backendAvailable = false;
        mockMode = true;
        diagnostics = buildMockDiagnostics();
        renderDiagnostics();
        document.getElementById('runtime_label').textContent = 'локальный предпросмотр: контроллер недоступен';
        document.getElementById('runtime_dot').style.background = 'var(--orange)';
    }
}

function renderDiagnostics() {
    if (!diagnostics) return;
    const mode = diagnostics.runtimeMode || diagnostics.mode || 'runtime';
    const port1 = diagnostics.ports?.[0]?.status || 'unknown';
    document.getElementById('runtime_label').textContent = `${mockMode ? 'локально' : 'режим'}: ${mode} · Порт 1 ${statusLabel(port1)}`;
    document.getElementById('runtime_dot').style.background = port1 === 'error' ? 'var(--red)' : port1 === 'charging' ? 'var(--blue)' : 'var(--green)';

    document.getElementById('diag_summary').innerHTML = [
        diagCard('Активных портов', diagnostics.activePortCount ?? 1),
        diagCard('Режим runtime', mode),
        diagCard('Состояние редактора', activeTab === 'diagnostics' ? 'только просмотр' : `${activeTab} preview`),
        diagCard('Основной LED-выход', `${diagnostics.primaryLedOutput?.name || 'LED1'} / GPIO${diagnostics.primaryLedOutput?.gpio || 22}`),
        diagCard('Источник данных', mockMode ? 'локальная конфигурация по умолчанию' : 'ESP32 контроллер'),
    ].join('');

    document.getElementById('diag_inputs').innerHTML = (diagnostics.inputs || []).map(input =>
        diagCard(input.name, `GPIO${input.gpio} · ${input.raw} · ${input.active ? 'активен' : 'неактивен'}`)
    ).join('');

    document.getElementById('diag_ports').innerHTML = (diagnostics.ports || []).map(port =>
        diagCard(`Порт ${port.port}`, `${port.enabled ? statusLabel(port.status) : 'отключён / резерв'} · зона ${port.zoneId}`)
    ).join('');

    document.getElementById('diag_leds').innerHTML = (diagnostics.ledOutputs || []).map(led =>
        diagCard(led.name, `GPIO${led.gpio} · ${led.state === 'primary' ? 'основной' : 'резерв'}`)
    ).join('');
}

function diagCard(title, value) {
    return `<div class="diag-card"><strong>${title}</strong><span>${value}</span></div>`;
}

setInterval(() => {
    rainbowTick++;
    if (activeTab === 'free' && freeModes[activeFreeZone] === 'rainbow') redrawMatrix();
}, 160);

setInterval(loadDiagnostics, 2500);

loadConfig().catch(err => {
    document.getElementById('edit_state_line').textContent = 'Не удалось загрузить конфигурацию.';
    console.error(err);
});
</script>
</body>
</html>
)=====";

#endif
