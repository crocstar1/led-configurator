#ifndef GUI_HTML_H
#define GUI_HTML_H

#include <Arduino.h>

const char PAGE_MAIN[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
    <title>Настройка LED-матрицы</title>
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

        .top-actions {
            display: flex;
            align-items: center;
            gap: 8px;
        }

        .dot {
            width: 9px;
            height: 9px;
            border-radius: 999px;
            background: var(--green);
        }

        .tabs {
            display: grid;
            grid-template-columns: repeat(3, minmax(0, 1fr));
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

        .service-backdrop {
            position: fixed;
            inset: 0;
            display: none;
            align-items: flex-start;
            justify-content: flex-end;
            padding: 16px;
            background: rgba(17, 24, 39, 0.24);
            z-index: 20;
        }

        .service-backdrop.open {
            display: flex;
        }

        .service-panel {
            width: min(520px, 100%);
            max-height: calc(100vh - 32px);
            overflow: auto;
            background: var(--panel);
            border: 1px solid var(--line);
            border-radius: 12px;
            box-shadow: 0 18px 46px rgba(25, 39, 65, 0.18);
            padding: 16px;
        }

        .service-tabs {
            display: grid;
            grid-template-columns: repeat(4, minmax(0, 1fr));
            gap: 8px;
            margin-bottom: 12px;
        }

        .service-section { display: none; }
        .service-section.active { display: block; }

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

        select, input[type="color"], input[type="text"], input[type="password"] {
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

        input[type="checkbox"] {
            accent-color: var(--blue);
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

        .field-hidden {
            display: none !important;
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
            .topbar { gap: 10px; }
            .service-tabs, .tabs { grid-template-columns: 1fr; }
        }
    </style>
</head>
<body>
<div class="app">
    <header class="topbar">
        <div class="brand">
            <h1>LED-контроллер матрицы</h1>
            <span>инженерная настройка контроллера</span>
        </div>
        <div class="top-actions">
            <div class="runtime-pill"><span class="dot" id="runtime_dot"></span><span id="runtime_label">режим: загрузка</span></div>
            <button class="btn" id="service_open_btn" type="button">⚙ Сервис</button>
        </div>
    </header>

    <nav class="tabs" aria-label="Разделы настройки">
        <button class="tab-btn active" data-tab="zones">Зоны матрицы</button>
        <button class="tab-btn" data-tab="status">Статусы портов</button>
        <button class="tab-btn" data-tab="free">Свободные зоны</button>
    </nav>

    <main class="layout">
        <section class="panel">
            <div class="panel-title">
                <h2 id="matrix_title">Матрица</h2>
                <span class="hint" id="matrix_hint">12x8, локальное редактирование</span>
            </div>
            <div class="matrix-shell">
                <div class="matrix" id="matrix_grid"></div>
            </div>
            <div class="status-line" id="edit_state_line">Редактор: изменения локальные до сохранения.</div>
        </section>

        <aside class="side-stack">
            <section class="panel tab-panel active" id="tab_zones">
                <div class="panel-title"><h2>Настройки</h2></div>
                <div class="section">
                    <div class="btn-row" role="group" aria-label="Режим просмотра зон">
                        <button class="btn active" id="zone_edit_mode_btn">Редактирование</button>
                        <button class="btn" id="zone_overview_mode_btn">Обзор всех зон</button>
                    </div>
                    <div>
                        <label for="zone_select">Активная зона</label>
                        <select id="zone_select"></select>
                    </div>
                    <div class="btn-row" id="zone_tools_row">
                        <button class="btn active" id="zone_brush_btn">Кисть</button>
                        <button class="btn" id="zone_erase_btn">Ластик</button>
                    </div>
                    <label>Легенда</label>
                    <div class="legend" id="zone_legend"></div>
                    <div class="notice" id="zones_mode_notice">Рисуйте внутри выбранной зоны. Чужие зоны заблокированы.</div>
                    <div class="btn-row">
                        <button class="btn primary" id="save_zones_btn">Сохранить зоны</button>
                        <button class="btn danger" id="clear_zone_btn">Очистить выбранную зону</button>
                    </div>
                </div>
            </section>

            <section class="panel tab-panel" id="tab_status">
                <div class="panel-title"><h2>Настройки</h2></div>
                <div class="section">
                    <div class="btn-row">
                        <button class="btn active" data-status="waiting">Ожидание</button>
                        <button class="btn" data-status="charging">Зарядка</button>
                        <button class="btn" data-status="error">Ошибка</button>
                    </div>
                    <div class="grid-2">
                        <div>
                            <label for="status_color">Цвет кисти</label>
                            <input type="color" id="status_color" value="#0055ff">
                        </div>
                        <div class="status-line">Глобально для портовых зон</div>
                    </div>
                    <div class="grid-2">
                        <div>
                            <label for="ports_brightness">Яркость портов</label>
                            <input type="range" id="ports_brightness" min="1" max="255" value="120">
                        </div>
                        <div class="status-line" id="ports_brightness_label">120</div>
                    </div>
                    <div class="btn-row">
                        <button class="btn primary" id="save_status_btn">Сохранить слой статуса</button>
                        <button class="btn danger" id="clear_status_btn">Очистить слой статуса</button>
                    </div>
                </div>
            </section>

            <section class="panel tab-panel" id="tab_free">
                <div class="panel-title"><h2>Настройки</h2></div>
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
                    <div class="btn-row" role="group" aria-label="Режим просмотра свободных зон">
                        <button class="btn active" id="free_edit_mode_btn">Выбранная зона</button>
                        <button class="btn" id="free_overview_mode_btn">Обзор свободных зон</button>
                    </div>
                    <div class="grid-2">
                        <div id="free_color_field">
                            <label for="free_color" id="free_color_label">Цвет зоны</label>
                            <input type="color" id="free_color" value="#ffffff">
                        </div>
                        <div>
                            <label for="free_brightness">Яркость свободных зон</label>
                            <input type="range" id="free_brightness" min="1" max="255" value="100">
                        </div>
                    </div>
                    <label>Легенда</label>
                    <div class="legend" id="free_legend"></div>
                    <div class="btn-row">
                        <button class="btn danger" id="free_erase_btn">Очистить рисунок</button>
                        <button class="btn primary" id="apply_free_btn">Применить доступные настройки</button>
                    </div>
                </div>
            </section>

        </aside>
    </main>

    <div class="service-backdrop" id="service_backdrop" aria-hidden="true">
        <section class="service-panel" role="dialog" aria-modal="true" aria-label="Сервис">
            <div class="panel-title">
                <h2>Сервис</h2>
                <button class="btn" id="service_close_btn" type="button">Закрыть</button>
            </div>
            <div class="service-tabs" aria-label="Сервисные разделы">
                <button class="tab-btn active" data-service="diagnostics" type="button">Диагностика</button>
                <button class="tab-btn" data-service="network" type="button">Сеть</button>
                <button class="tab-btn" data-service="security" type="button">Безопасность</button>
                <button class="tab-btn" data-service="firmware" type="button">Прошивка</button>
            </div>

            <section class="service-section active" id="service_diagnostics">
                <div class="section">
                    <div class="row">
                        <strong>Диагностика</strong>
                        <button class="btn" id="refresh_diag_btn" type="button">Обновить</button>
                    </div>
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

            <section class="service-section" id="service_network">
                <div class="section">
                    <div class="row">
                        <strong>Сеть</strong>
                        <button class="btn" id="refresh_network_btn" type="button">Обновить</button>
                    </div>
                    <div class="diag-grid" id="network_summary"></div>
                    <form id="network_form" class="section">
                        <div>
                            <label for="network_ssid">SSID</label>
                            <input id="network_ssid" name="ssid" type="text" autocomplete="off">
                        </div>
                        <div>
                            <label for="network_password">Пароль Wi-Fi</label>
                            <input id="network_password" name="password" type="password" autocomplete="new-password" placeholder="пусто = не менять пароль">
                        </div>
                        <label class="row" style="justify-content:flex-start;">
                            <input id="network_dhcp" name="dhcp" type="checkbox" checked>
                            <span>DHCP</span>
                        </label>
                        <div class="grid-2" id="network_static_fields">
                            <div>
                                <label for="network_static_ip">Static IP</label>
                                <input id="network_static_ip" name="static_ip" type="text" inputmode="numeric">
                            </div>
                            <div>
                                <label for="network_gateway">Gateway</label>
                                <input id="network_gateway" name="gateway" type="text" inputmode="numeric">
                            </div>
                            <div>
                                <label for="network_subnet">Subnet</label>
                                <input id="network_subnet" name="subnet" type="text" inputmode="numeric">
                            </div>
                            <div>
                                <label for="network_dns">DNS</label>
                                <input id="network_dns" name="dns" type="text" inputmode="numeric">
                            </div>
                        </div>
                        <div class="grid-2">
                            <div>
                                <label for="network_ap_ssid">AP fallback SSID</label>
                                <input id="network_ap_ssid" name="ap_ssid" type="text">
                            </div>
                            <div>
                                <label for="network_ap_password">AP fallback пароль</label>
                                <input id="network_ap_password" name="ap_password" type="password">
                            </div>
                        </div>
                        <div class="btn-row">
                            <button class="btn" id="scan_network_btn" type="button">Сканировать</button>
                            <button class="btn primary" type="submit">Сохранить</button>
                            <button class="btn" id="network_apply_btn" type="button">Применить</button>
                        </div>
                    </form>
                    <div class="diag-grid" id="network_scan"></div>
                    <div class="hint" id="network_message"></div>
                </div>
            </section>

            <section class="service-section" id="service_firmware">
                <div class="notice">Удалённая прошивка будет оформлена отдельным безопасным экраном позже.</div>
            </section>

            <section class="service-section" id="service_security">
                <div class="section">
                    <div class="row">
                        <strong>Безопасность</strong>
                    </div>
                    <form id="auth_form" class="section">
                        <div>
                            <label for="auth_username">Логин</label>
                            <input id="auth_username" name="username" type="text" autocomplete="username" value="admin">
                        </div>
                        <div>
                            <label for="auth_password">Новый пароль</label>
                            <input id="auth_password" name="password" type="password" autocomplete="new-password">
                        </div>
                        <div>
                            <label for="auth_confirm">Повтор пароля</label>
                            <input id="auth_confirm" name="confirm" type="password" autocomplete="new-password">
                        </div>
                        <div class="btn-row">
                            <button class="btn primary" type="submit">Сохранить пароль</button>
                        </div>
                    </form>
                    <div class="hint" id="auth_message"></div>
                </div>
            </section>
        </section>
    </div>
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
let activeServiceSection = 'diagnostics';
let activeZoneId = '1';
let activeStatus = 'waiting';
let activeFreeZone = '5';
let zoneTool = 'brush';
let zoneViewMode = 'edit';
let freeViewMode = 'edit';
let drawing = false;
let hardwareZonesMap = {};
let zoneMeta = [...DEFAULT_ZONES];
let statusColorLayers = { waiting: {}, charging: {}, error: {} };
let freeModes = { '5': 'static', '6': 'static', '7': 'static', '8': 'custom' };
let freeCustomLayers = { '5': {}, '6': {}, '7': {}, '8': {} };
let freeStaticColors = { ...DEFAULT_FREE_STATIC_COLORS };
let freeBrushColors = { ...DEFAULT_FREE_STATIC_COLORS };
let freeBrightness = { '5': 100, '6': 100, '7': 100, '8': 100 };
let diagnostics = null;
let networkStatus = null;
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

function runtimeModeLabel(mode) {
    const labels = {
        runtime: 'рабочий',
        custom: 'пользовательский',
        preview: 'предпросмотр',
    };
    return labels[mode] || mode || 'неизвестно';
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
        free_zones: [
            { zoneId: 5, enabled: true, mode: 'static', staticColor: '#ffffff', brightness: 100, customLayer: {} },
            { zoneId: 6, enabled: true, mode: 'static', staticColor: '#34c759', brightness: 100, customLayer: {} },
            { zoneId: 7, enabled: true, mode: 'static', staticColor: '#ff9500', brightness: 100, customLayer: {} },
            { zoneId: 8, enabled: true, mode: 'custom', staticColor: '#af52de', brightness: 100, customLayer: {} },
        ],
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

function buildMockNetworkStatus() {
    return {
        mode: 'ap_fallback',
        status: 'mock',
        configured: false,
        connected: false,
        ssid: '',
        connectedSsid: '',
        ip: '',
        rssi: 0,
        dhcp: true,
        staticIp: '192.168.1.150',
        gateway: '192.168.1.1',
        subnet: '255.255.255.0',
        dns: '8.8.8.8',
        apSsid: 'NEK_EVSE_LAB',
        apIp: '192.168.4.1',
        apClients: 1,
    };
}

function networkModeLabel(mode) {
    const labels = {
        sta: 'STA',
        ap_fallback: 'AP fallback',
        connecting: 'подключение',
    };
    return labels[mode] || mode || 'неизвестно';
}

function networkStatusLabel(status) {
    const labels = {
        connected: 'подключено',
        connecting: 'подключение',
        not_configured: 'SSID не сохранен',
        connect_failed: 'подключение не удалось',
        connect_timeout: 'таймаут подключения',
        mock: 'локальный preview',
    };
    return labels[status] || status || 'неизвестно';
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
    document.getElementById('free_brightness').value = freeBrightness[activeFreeZone] || 100;
}

function updateFreeModeControls() {
    const mode = freeModes[activeFreeZone] || 'static';
    const colorField = document.getElementById('free_color_field');
    const colorLabel = document.getElementById('free_color_label');
    const clearBtn = document.getElementById('free_erase_btn');
    colorField.classList.toggle('field-hidden', mode === 'rainbow');
    clearBtn.classList.toggle('field-hidden', mode !== 'custom');
    colorLabel.textContent = mode === 'custom' ? 'Цвет кисти' : 'Цвет зоны';
}

function updateZoneViewControls() {
    const isOverview = zoneViewMode === 'overview';
    document.getElementById('zone_edit_mode_btn').classList.toggle('active', !isOverview);
    document.getElementById('zone_overview_mode_btn').classList.toggle('active', isOverview);
    document.getElementById('zone_tools_row').classList.toggle('field-hidden', isOverview);
    document.getElementById('clear_zone_btn').disabled = isOverview;
    document.getElementById('zones_mode_notice').textContent = isOverview
        ? 'Обзор: все зоны показаны своими цветами, рисование отключено.'
        : 'Рисуйте внутри выбранной зоны. Чужие зоны заблокированы.';
}

function updateFreeViewControls() {
    const isOverview = freeViewMode === 'overview';
    document.getElementById('free_edit_mode_btn').classList.toggle('active', !isOverview);
    document.getElementById('free_overview_mode_btn').classList.toggle('active', isOverview);
    document.getElementById('free_erase_btn').disabled = isOverview;
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

    if (Array.isArray(cfg.free_zones)) {
        cfg.free_zones.forEach(fz => {
            const zoneId = String(fz.zoneId ?? fz.id);
            if (!isFreeZone(zoneId)) return;
            freeModes[zoneId] = fz.mode || freeModes[zoneId] || 'static';
            freeStaticColors[zoneId] = fz.staticColor || freeStaticColors[zoneId] || '#ffffff';
            freeBrushColors[zoneId] = freeBrushColors[zoneId] || freeStaticColors[zoneId];
            freeBrightness[zoneId] = Number(fz.brightness) || freeBrightness[zoneId] || 100;
            freeCustomLayers[zoneId] = sanitizeLayerForSize(fz.customLayer || freeCustomLayers[zoneId] || {});
        });
    } else {
        freeBrightness['5'] = cfg.b_logo || freeBrightness['5'] || 100;
    }
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

function isEditablePortZone(id) {
    const z = zoneById(id);
    return z && z.type === 'port' && z.enabled && !z.reserved;
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
    const freeZoneSelect = document.getElementById('free_zone_select');
    zoneSelect.innerHTML = '';
    freeZoneSelect.innerHTML = '';

    zoneMeta.forEach(z => {
        const opt = document.createElement('option');
        opt.value = z.id;
        opt.textContent = zoneOptionLabel(z);
        opt.disabled = z.reserved || !z.enabled;
        zoneSelect.appendChild(opt);

        if (z.type === 'free') {
            const fo = document.createElement('option');
            fo.value = z.id;
            fo.textContent = zoneDisplayName(z);
            freeZoneSelect.appendChild(fo);
        }
    });

    zoneSelect.value = activeZoneId;
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

function buildFreeLegend() {
    const legend = document.getElementById('free_legend');
    legend.innerHTML = '';

    zoneMeta.filter(z => z.type === 'free').forEach(z => {
        const zoneId = String(z.id);
        const item = document.createElement('div');
        item.className = 'legend-item' + (zoneId === activeFreeZone ? ' selected' : '');
        const swatch = document.createElement('span');
        swatch.className = 'swatch';
        swatch.style.background = freeModes[zoneId] === 'rainbow'
            ? 'linear-gradient(90deg, #ff3b30, #ff9500, #34c759, #0055ff, #af52de)'
            : freeZoneColor('0-0', zoneId);
        const text = document.createElement('span');
        text.textContent = `${zoneDisplayName(z)} · ${modeLabel(freeModes[zoneId])}`;
        item.appendChild(swatch);
        item.appendChild(text);
        legend.appendChild(item);
    });
}

function pixelStatusColor(key, zoneId) {
    const custom = statusColorLayers[activeStatus][key];
    if (custom && isEditablePortZone(zoneId)) return custom;
    if (isEditablePortZone(zoneId)) return statusDefaults[activeStatus];
    if (isPortZone(zoneId)) return zoneColors[zoneId];
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
        if (zoneViewMode === 'overview') return false;
        if (!isEditableZone(activeZoneId)) return true;
        return !(zoneId === '0' || zoneId === activeZoneId);
    }
    if (activeTab === 'status') {
        return !isEditablePortZone(zoneId);
    }
    if (activeTab === 'free') {
        if (freeViewMode === 'overview') return false;
        return zoneId !== activeFreeZone;
    }
    return false;
}

function redrawMatrix() {
    document.getElementById('matrix_title').textContent =
        activeTab === 'zones' ? 'Матрица' :
        activeTab === 'status' ? 'Матрица' :
        'Матрица';

    document.getElementById('matrix_hint').textContent =
        activeTab === 'zones' ? `${COLS}x${ROWS}, ${zoneViewMode === 'overview' ? 'обзор всей разметки' : 'редактирование выбранной зоны'}` :
        activeTab === 'status' ? `слой "${statusLabel(activeStatus)}", предпросмотр только в UI` :
        `${freeViewMode === 'overview' ? 'обзор свободных зон' : `${zoneDisplayName(activeFreeZone)}: ${modeLabel(freeModes[activeFreeZone])}`}`;

    grid.querySelectorAll('.pixel').forEach(p => {
        const key = keyOf(p.dataset.x, p.dataset.y);
        const zoneId = hardwareZonesMap[key] || '0';
        p.dataset.zone = zoneId;
        p.classList.toggle('off-zone', zoneId === '0');
        p.classList.toggle('active-zone',
            (activeTab === 'zones' && zoneViewMode === 'edit' && zoneId === activeZoneId) ||
            (activeTab === 'status' && isEditablePortZone(zoneId)) ||
            (activeTab === 'free' && freeViewMode === 'edit' && zoneId === activeFreeZone));
        p.classList.toggle('locked', shouldLockPixel(zoneId));

        if (activeTab === 'status') {
            p.style.background = pixelStatusColor(key, zoneId);
        } else if (activeTab === 'free') {
            p.style.background = isFreeZone(zoneId) && (freeViewMode === 'overview' || zoneId === activeFreeZone)
                ? freeZoneColor(key, zoneId)
                : (zoneColors[zoneId] || zoneColors['0']);
        } else {
            p.style.background = zoneColors[zoneId] || zoneColors['0'];
        }
    });

    buildLegend();
    buildFreeLegend();
    updateZoneViewControls();
    updateFreeViewControls();
    updateFreeModeControls();
    updateEditState();
}

function updateEditState() {
    const text =
        activeTab === 'zones' ? (zoneViewMode === 'overview'
            ? 'Обзор: все зоны видны, рисование отключено'
            : `Редактор: зона ${activeZoneId} · ${zoneTool === 'erase' ? 'ластик' : 'кисть'}`) :
        activeTab === 'status' ? `Редактор: глобальный статус "${statusLabel(activeStatus)}"` :
        (freeViewMode === 'overview'
            ? 'Обзор: свободные зоны видны, рисование отключено'
            : `Редактор: ${zoneDisplayName(activeFreeZone)} · ${modeLabel(freeModes[activeFreeZone])}`);
    document.getElementById('edit_state_line').textContent = mockMode
        ? `${text}. Контроллер недоступен: работает локальный предпросмотр 12x8.`
        : text;
}

function paintPixel(pixel) {
    if (!pixel || !pixel.classList.contains('pixel')) return;
    const key = keyOf(pixel.dataset.x, pixel.dataset.y);
    const currentZone = hardwareZonesMap[key] || '0';

    if (activeTab === 'zones') {
        if (zoneViewMode !== 'edit') return;
        if (!isEditableZone(activeZoneId)) return;
        if (zoneTool === 'erase') {
            if (currentZone === activeZoneId) delete hardwareZonesMap[key];
        } else if (currentZone === '0' || currentZone === activeZoneId) {
            hardwareZonesMap[key] = activeZoneId;
        }
    } else if (activeTab === 'status') {
        if (!isEditablePortZone(currentZone)) return;
        const color = document.getElementById('status_color').value;
        statusColorLayers[activeStatus][key] = color;
    } else if (activeTab === 'free') {
        if (freeViewMode !== 'edit') return;
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
        if (!btn.dataset.tab) return;
        activeTab = btn.dataset.tab;
        document.querySelectorAll('[data-tab]').forEach(b => b.classList.toggle('active', b === btn));
        document.querySelectorAll('.tab-panel').forEach(p => p.classList.toggle('active', p.id === `tab_${activeTab}`));
        redrawMatrix();
    });
});

function setServiceOpen(open) {
    const backdrop = document.getElementById('service_backdrop');
    backdrop.classList.toggle('open', open);
    backdrop.setAttribute('aria-hidden', open ? 'false' : 'true');
    if (open) loadDiagnostics();
    if (open && activeServiceSection === 'network') loadNetworkStatus();
    if (open && activeServiceSection === 'security') loadAuthStatus();
}

document.getElementById('service_open_btn').addEventListener('click', () => setServiceOpen(true));
document.getElementById('service_close_btn').addEventListener('click', () => setServiceOpen(false));
document.getElementById('service_backdrop').addEventListener('click', e => {
    if (e.target.id === 'service_backdrop') setServiceOpen(false);
});

document.querySelectorAll('[data-service]').forEach(btn => {
    btn.addEventListener('click', () => {
        activeServiceSection = btn.dataset.service;
        document.querySelectorAll('[data-service]').forEach(b => b.classList.toggle('active', b === btn));
        document.querySelectorAll('.service-section').forEach(section => {
            section.classList.toggle('active', section.id === `service_${activeServiceSection}`);
        });
        if (activeServiceSection === 'diagnostics') loadDiagnostics();
        if (activeServiceSection === 'network') loadNetworkStatus();
        if (activeServiceSection === 'security') loadAuthStatus();
    });
});

document.getElementById('refresh_network_btn').addEventListener('click', loadNetworkStatus);
document.getElementById('scan_network_btn').addEventListener('click', scanNetworks);
document.getElementById('network_apply_btn').addEventListener('click', applyNetworkConfig);
document.getElementById('network_dhcp').addEventListener('change', updateNetworkStaticFields);
document.getElementById('network_form').addEventListener('submit', e => {
    e.preventDefault();
    saveNetworkConfig();
});
document.getElementById('auth_form').addEventListener('submit', e => {
    e.preventDefault();
    saveAuthConfig();
});

document.getElementById('zone_select').addEventListener('change', e => {
    activeZoneId = e.target.value;
    redrawMatrix();
});

document.getElementById('zone_edit_mode_btn').addEventListener('click', () => {
    zoneViewMode = 'edit';
    redrawMatrix();
});

document.getElementById('zone_overview_mode_btn').addEventListener('click', () => {
    zoneViewMode = 'overview';
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

document.getElementById('free_edit_mode_btn').addEventListener('click', () => {
    freeViewMode = 'edit';
    redrawMatrix();
});

document.getElementById('free_overview_mode_btn').addEventListener('click', () => {
    freeViewMode = 'overview';
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

document.getElementById('free_erase_btn').addEventListener('click', () => {
    if (freeViewMode !== 'edit' || (freeModes[activeFreeZone] || 'static') !== 'custom') return;
    freeCustomLayers[activeFreeZone] = {};
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
    freeBrightness[activeFreeZone] = Number(e.target.value) || 100;
});

document.getElementById('apply_free_btn').addEventListener('click', async () => {
    const mode = freeModes[activeFreeZone] || 'static';
    if (!backendAvailable) {
        alert('Контроллер недоступен: настройки свободной зоны изменены только в браузере.');
        return;
    }

    const payload = {
        zoneId: Number(activeFreeZone),
        enabled: true,
        mode,
        staticColor: freeStaticColors[activeFreeZone] || '#ffffff',
        brightness: freeBrightness[activeFreeZone] || 100,
        customLayer: freeCustomLayers[activeFreeZone] || {},
    };

    try {
        const res = await fetch('/save_free_zone', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload),
        });
        alert(await res.text() === 'OK' ? 'Настройки свободной зоны сохранены.' : 'Не удалось сохранить свободную зону.');
    } catch (err) {
        alert('Контроллер недоступен: настройки свободной зоны изменены только в браузере.');
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

function setNetworkMessage(text, isError = false) {
    const el = document.getElementById('network_message');
    el.textContent = text || '';
    el.style.color = isError ? 'var(--red)' : 'var(--muted)';
}

function fillNetworkForm(status) {
    document.getElementById('network_ssid').value = status.ssid || '';
    document.getElementById('network_password').value = '';
    document.getElementById('network_dhcp').checked = status.dhcp !== false;
    document.getElementById('network_static_ip').value = status.staticIp || '192.168.1.150';
    document.getElementById('network_gateway').value = status.gateway || '192.168.1.1';
    document.getElementById('network_subnet').value = status.subnet || '255.255.255.0';
    document.getElementById('network_dns').value = status.dns || '8.8.8.8';
    document.getElementById('network_ap_ssid').value = status.apSsid || 'NEK_EVSE_LAB';
    document.getElementById('network_ap_password').value = '';
    updateNetworkStaticFields();
}

function updateNetworkStaticFields() {
    const dhcp = document.getElementById('network_dhcp').checked;
    document.getElementById('network_static_fields').classList.toggle('field-hidden', dhcp);
}

async function loadNetworkStatus() {
    if (!backendAvailable) {
        networkStatus = buildMockNetworkStatus();
        renderNetworkStatus();
        setNetworkMessage('Backend недоступен, показан локальный preview.');
        return;
    }

    try {
        const res = await fetch('/network_status');
        if (!res.ok) throw new Error(`network_status ${res.status}`);
        networkStatus = await res.json();
        renderNetworkStatus();
        setNetworkMessage('');
    } catch (err) {
        networkStatus = buildMockNetworkStatus();
        renderNetworkStatus();
        setNetworkMessage('Не удалось получить статус сети.', true);
    }
}

function renderNetworkStatus() {
    if (!networkStatus) return;
    document.getElementById('network_summary').innerHTML = [
        diagCard('Режим', networkModeLabel(networkStatus.mode)),
        diagCard('Состояние', networkStatus.connected ? 'подключено' : networkStatusLabel(networkStatus.status)),
        diagCard('SSID', networkStatus.connectedSsid || networkStatus.ssid || 'не задан'),
        diagCard('IP', networkStatus.ip || networkStatus.apIp || 'нет'),
        diagCard('RSSI', networkStatus.connected ? `${networkStatus.rssi} dBm` : 'нет'),
        diagCard('Адресация', networkStatus.dhcp ? 'DHCP' : 'Static'),
    ].join('');
    fillNetworkForm(networkStatus);
}

async function scanNetworks() {
    if (!backendAvailable) {
        document.getElementById('network_scan').innerHTML = [
            diagCard('NEK_EVSE_LAB', 'mock · -45 dBm'),
            diagCard('Office Wi-Fi', 'mock · -62 dBm'),
        ].join('');
        setNetworkMessage('Сканирование работает только через контроллер.');
        return;
    }

    setNetworkMessage('Сканирование...');
    try {
        const res = await fetch('/scan_networks');
        if (!res.ok) throw new Error(`scan ${res.status}`);
        const networks = await res.json();
        document.getElementById('network_scan').innerHTML = (networks || []).map(net =>
            diagCard(net.ssid || '(hidden)', `${net.rssi} dBm · канал ${net.channel}${net.secure ? ' · пароль' : ' · открытая'}`)
        ).join('') || '<div class="notice">Сети не найдены.</div>';
        setNetworkMessage('');
    } catch (err) {
        setNetworkMessage('Не удалось выполнить сканирование.', true);
    }
}

async function saveNetworkConfig() {
    if (!backendAvailable) {
        setNetworkMessage('Backend недоступен, настройки сети не сохранены.', true);
        return;
    }

    const body = new URLSearchParams();
    body.set('ssid', document.getElementById('network_ssid').value.trim());
    body.set('password', document.getElementById('network_password').value);
    body.set('dhcp', document.getElementById('network_dhcp').checked ? '1' : '0');
    body.set('static_ip', document.getElementById('network_static_ip').value.trim());
    body.set('gateway', document.getElementById('network_gateway').value.trim());
    body.set('subnet', document.getElementById('network_subnet').value.trim());
    body.set('dns', document.getElementById('network_dns').value.trim());
    body.set('ap_ssid', document.getElementById('network_ap_ssid').value.trim());
    body.set('ap_password', document.getElementById('network_ap_password').value);

    try {
        const res = await fetch('/save_network', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body,
        });
        if (!res.ok) throw new Error(await res.text());
        setNetworkMessage('Настройки сети сохранены. Нажмите «Применить» для переподключения.');
        await loadNetworkStatus();
    } catch (err) {
        setNetworkMessage(`Не удалось сохранить сеть: ${err.message}`, true);
    }
}

async function applyNetworkConfig() {
    if (!backendAvailable) {
        setNetworkMessage('Backend недоступен, применить настройки нельзя.', true);
        return;
    }

    try {
        const res = await fetch('/network_reconnect', { method: 'POST' });
        if (!res.ok) throw new Error(await res.text());
        setNetworkMessage('Переподключение запущено. Интерфейс может временно пропасть.');
        setTimeout(loadNetworkStatus, 4000);
    } catch (err) {
        setNetworkMessage(`Не удалось применить настройки: ${err.message}`, true);
    }
}

function setAuthMessage(text, isError = false) {
    const el = document.getElementById('auth_message');
    el.textContent = text || '';
    el.style.color = isError ? 'var(--red)' : 'var(--muted)';
}

async function loadAuthStatus() {
    document.getElementById('auth_password').value = '';
    document.getElementById('auth_confirm').value = '';

    if (!backendAvailable) {
        document.getElementById('auth_username').value = 'admin';
        setAuthMessage('Backend недоступен, пароль не будет сохранён.');
        return;
    }

    try {
        const res = await fetch('/auth_status');
        if (!res.ok) throw new Error(`auth_status ${res.status}`);
        const data = await res.json();
        document.getElementById('auth_username').value = data.username || 'admin';
        setAuthMessage('');
    } catch (err) {
        setAuthMessage('Не удалось загрузить настройки безопасности.', true);
    }
}

async function saveAuthConfig() {
    if (!backendAvailable) {
        setAuthMessage('Backend недоступен, пароль не сохранён.', true);
        return;
    }

    const username = document.getElementById('auth_username').value.trim();
    const password = document.getElementById('auth_password').value;
    const confirm = document.getElementById('auth_confirm').value;

    if (!username) {
        setAuthMessage('Введите логин.', true);
        return;
    }
    if (!password) {
        setAuthMessage('Введите новый пароль.', true);
        return;
    }
    if (password.length < 6) {
        setAuthMessage('Пароль должен быть не короче 6 символов.', true);
        return;
    }
    if (password !== confirm) {
        setAuthMessage('Пароли не совпадают.', true);
        return;
    }

    const body = new URLSearchParams();
    body.set('username', username);
    body.set('password', password);
    body.set('confirm', confirm);

    try {
        const res = await fetch('/save_auth', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body,
        });
        if (!res.ok) throw new Error(await res.text());
        document.getElementById('auth_password').value = '';
        document.getElementById('auth_confirm').value = '';
        setAuthMessage('Пароль сохранён. Обновите страницу и войдите заново.');
    } catch (err) {
        setAuthMessage(`Не удалось сохранить пароль: ${err.message}`, true);
    }
}

function renderDiagnostics() {
    if (!diagnostics) return;
    const mode = diagnostics.runtimeMode || diagnostics.mode || 'runtime';
    const port1 = diagnostics.ports?.[0]?.status || 'unknown';
    document.getElementById('runtime_label').textContent = `${mockMode ? 'локально' : 'режим'}: ${runtimeModeLabel(mode)} · Порт 1 ${statusLabel(port1)}`;
    document.getElementById('runtime_dot').style.background = port1 === 'error' ? 'var(--red)' : port1 === 'charging' ? 'var(--blue)' : 'var(--green)';

    document.getElementById('diag_summary').innerHTML = [
        diagCard('Активных портов', diagnostics.activePortCount ?? 1),
        diagCard('Рабочий режим', runtimeModeLabel(mode)),
        diagCard('Состояние редактора', 'предпросмотр в UI'),
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
