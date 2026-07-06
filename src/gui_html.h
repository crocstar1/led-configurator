#ifndef GUI_HTML_H
#define GUI_HTML_H

#include <Arduino.h>

const char PAGE_MAIN[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
    <title>USP-1 Matrix Controller</title>
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
            <h1>USP-1 LED Matrix</h1>
            <span>engineering setup console</span>
        </div>
        <div class="runtime-pill"><span class="dot" id="runtime_dot"></span><span id="runtime_label">runtime: loading</span></div>
    </header>

    <nav class="tabs" aria-label="Sections">
        <button class="tab-btn active" data-tab="zones">Matrix Zones</button>
        <button class="tab-btn" data-tab="status">Port Status</button>
        <button class="tab-btn" data-tab="free">Free Zones</button>
        <button class="tab-btn" data-tab="diagnostics">Diagnostics</button>
    </nav>

    <main class="layout">
        <section class="panel">
            <div class="panel-title">
                <h2 id="matrix_title">Matrix Zones</h2>
                <span class="hint" id="matrix_hint">12x8, client-side edit</span>
            </div>
            <div class="matrix-shell">
                <div class="matrix" id="matrix_grid"></div>
            </div>
            <div class="status-line" id="edit_state_line">Edit state: runtime preview is local until Save.</div>
        </section>

        <aside class="side-stack">
            <section class="panel tab-panel active" id="tab_zones">
                <div class="panel-title"><h2>Matrix Zones</h2></div>
                <div class="section">
                    <div>
                        <label for="zone_select">Active zone</label>
                        <select id="zone_select"></select>
                    </div>
                    <div class="btn-row">
                        <button class="btn active" id="zone_brush_btn">Brush</button>
                        <button class="btn" id="zone_erase_btn">Erase selected</button>
                    </div>
                    <div class="legend" id="zone_legend"></div>
                    <div class="notice">Only Off pixels and pixels already inside the selected zone can be edited. Other zones are locked to prevent overlaps.</div>
                    <div class="btn-row">
                        <button class="btn primary" id="save_zones_btn">Save zones</button>
                        <button class="btn danger" id="clear_zone_btn">Clear selected zone</button>
                    </div>
                </div>
            </section>

            <section class="panel tab-panel" id="tab_status">
                <div class="panel-title"><h2>Port Status</h2></div>
                <div class="section">
                    <div class="btn-row">
                        <button class="btn active" data-status="waiting">Waiting</button>
                        <button class="btn" data-status="charging">Charging</button>
                        <button class="btn" data-status="error">Error</button>
                    </div>
                    <div class="grid-2">
                        <div>
                            <label for="status_zone_select">Port zone</label>
                            <select id="status_zone_select"></select>
                        </div>
                        <div>
                            <label for="status_color">Brush color</label>
                            <input type="color" id="status_color" value="#0055ff">
                        </div>
                    </div>
                    <div class="grid-2">
                        <div>
                            <label for="ports_brightness">Port brightness</label>
                            <input type="range" id="ports_brightness" min="1" max="255" value="120">
                        </div>
                        <div class="status-line" id="ports_brightness_label">120</div>
                    </div>
                    <div class="notice">Preview is UI-only. The hardware matrix follows real inputs; error remains runtime-owned.</div>
                    <div class="btn-row">
                        <button class="btn primary" id="save_status_btn">Save status layer</button>
                        <button class="btn danger" id="clear_status_btn">Clear status layer</button>
                    </div>
                </div>
            </section>

            <section class="panel tab-panel" id="tab_free">
                <div class="panel-title"><h2>Free Zones</h2></div>
                <div class="section">
                    <div class="grid-2">
                        <div>
                            <label for="free_zone_select">Free zone</label>
                            <select id="free_zone_select"></select>
                        </div>
                        <div>
                            <label for="free_mode_select">Mode</label>
                            <select id="free_mode_select">
                                <option value="static">Static</option>
                                <option value="custom">Custom drawing</option>
                                <option value="rainbow">Rainbow</option>
                            </select>
                        </div>
                    </div>
                    <div class="grid-2">
                        <div>
                            <label for="free_color">Static/custom color</label>
                            <input type="color" id="free_color" value="#ffffff">
                        </div>
                        <div>
                            <label for="free_brightness">Free brightness</label>
                            <input type="range" id="free_brightness" min="1" max="255" value="100">
                        </div>
                    </div>
                    <div class="notice">MVP storage is partial: Logo static color/brightness and Logo rainbow are persisted by existing endpoints. QR, Service and Custom modes are safe UI preview until FreeZoneConfig storage is added.</div>
                    <div class="btn-row">
                        <button class="btn active" id="free_brush_btn">Brush</button>
                        <button class="btn" id="free_erase_btn">Erase custom pixels</button>
                        <button class="btn primary" id="apply_free_btn">Apply available settings</button>
                    </div>
                </div>
            </section>

            <section class="panel tab-panel" id="tab_diagnostics">
                <div class="panel-title"><h2>Diagnostics</h2><button class="btn" id="refresh_diag_btn">Refresh</button></div>
                <div class="section">
                    <div class="diag-grid" id="diag_summary"></div>
                    <div>
                        <label>Inputs</label>
                        <div class="diag-grid" id="diag_inputs"></div>
                    </div>
                    <div>
                        <label>Ports</label>
                        <div class="diag-grid" id="diag_ports"></div>
                    </div>
                    <div>
                        <label>LED outputs</label>
                        <div class="diag-grid" id="diag_leds"></div>
                    </div>
                </div>
            </section>
        </aside>
    </main>
</div>

<script>
const DEFAULT_ZONES = [
    { id: 0, name: 'Off', type: 'off', enabled: true, reserved: false, linked_port: null, mode: null },
    { id: 1, name: 'Port 1', type: 'port', enabled: true, reserved: false, linked_port: 1, mode: null },
    { id: 2, name: 'Port 2', type: 'port', enabled: false, reserved: true, linked_port: 2, mode: null },
    { id: 3, name: 'Port 3', type: 'port', enabled: false, reserved: true, linked_port: 3, mode: null },
    { id: 4, name: 'Port 4', type: 'port', enabled: false, reserved: true, linked_port: 4, mode: null },
    { id: 5, name: 'Logo', type: 'free', enabled: true, reserved: false, linked_port: null, mode: 'static' },
    { id: 6, name: 'QR', type: 'free', enabled: true, reserved: false, linked_port: null, mode: 'static' },
    { id: 7, name: 'Service', type: 'free', enabled: true, reserved: false, linked_port: null, mode: 'static' },
    { id: 8, name: 'Custom', type: 'free', enabled: true, reserved: false, linked_port: null, mode: 'custom' },
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
let diagnostics = null;
let rainbowTick = 0;

const grid = document.getElementById('matrix_grid');

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
        opt.textContent = `${z.name}${z.reserved ? ' reserved' : ''}`;
        opt.disabled = z.reserved || !z.enabled;
        zoneSelect.appendChild(opt);

        if (z.type === 'port') {
            const po = document.createElement('option');
            po.value = z.id;
            po.textContent = `${z.name}${z.reserved ? ' reserved' : ''}`;
            po.disabled = z.reserved || !z.enabled;
            statusZoneSelect.appendChild(po);
        }

        if (z.type === 'free') {
            const fo = document.createElement('option');
            fo.value = z.id;
            fo.textContent = z.name;
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
        text.textContent = `${z.name} · ${z.type}${z.reserved ? ' · reserved' : ''}`;
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
    return document.getElementById('free_color').value || '#ffffff';
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
        activeTab === 'zones' ? 'Matrix Zones' :
        activeTab === 'status' ? 'Port Status Preview' :
        activeTab === 'free' ? 'Free Zones Preview' : 'Diagnostics View';

    document.getElementById('matrix_hint').textContent =
        activeTab === 'zones' ? `${COLS}x${ROWS}, selected zone edit` :
        activeTab === 'status' ? `${activeStatus} layer, UI-only preview` :
        activeTab === 'free' ? `${zoneById(activeFreeZone)?.name || 'Free'} mode preview` :
        'Runtime state from /diagnostics';

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
        activeTab === 'zones' ? `Edit: Matrix Zones · zone ${activeZoneId} · ${zoneTool}` :
        activeTab === 'status' ? `Edit: Port Status · ${activeStatus} · zone ${activeStatusZone}` :
        activeTab === 'free' ? `Edit: Free Zones · zone ${activeFreeZone} · ${freeModes[activeFreeZone]}` :
        'Edit: diagnostics read-only';
    document.getElementById('edit_state_line').textContent = text;
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
        if (freeTool === 'erase') {
            delete freeCustomLayers[activeFreeZone][key];
        } else {
            freeCustomLayers[activeFreeZone][key] = document.getElementById('free_color').value;
        }
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
    redrawMatrix();
});

document.getElementById('free_mode_select').addEventListener('change', e => {
    freeModes[activeFreeZone] = e.target.value;
    redrawMatrix();
});

document.getElementById('free_color').addEventListener('input', redrawMatrix);
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
    freeTool = 'erase';
    document.getElementById('free_erase_btn').classList.add('active');
    document.getElementById('free_brush_btn').classList.remove('active');
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
    const res = await fetch('/save_zones', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(hardwareZonesMap)
    });
    alert(await res.text() === 'OK' ? 'Zones saved' : 'Save failed');
});

document.getElementById('clear_zone_btn').addEventListener('click', () => {
    Object.keys(hardwareZonesMap).forEach(key => {
        if (hardwareZonesMap[key] === activeZoneId) delete hardwareZonesMap[key];
    });
    redrawMatrix();
});

document.getElementById('save_status_btn').addEventListener('click', async () => {
    const res = await fetch('/save_status_colors', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ status: activeStatus, colors: statusColorLayers[activeStatus] })
    });
    alert(await res.text() === 'OK' ? 'Status layer saved' : 'Save failed');
});

document.getElementById('clear_status_btn').addEventListener('click', () => {
    statusColorLayers[activeStatus] = {};
    redrawMatrix();
});

document.getElementById('ports_brightness').addEventListener('input', async e => {
    document.getElementById('ports_brightness_label').textContent = e.target.value;
    await fetch(`/set_bright?type=ports&val=${e.target.value}`);
});

document.getElementById('free_brightness').addEventListener('input', async e => {
    await fetch(`/set_bright?type=logo&val=${e.target.value}`);
});

document.getElementById('apply_free_btn').addEventListener('click', async () => {
    const mode = freeModes[activeFreeZone] || 'static';
    if (activeFreeZone === '5') {
        await fetch(`/set_logo_anim?val=${mode === 'rainbow' ? 1 : 0}`);
        await fetch(`/save_config?c_logo=${encodeURIComponent(document.getElementById('free_color').value)}`);
        alert('Logo settings applied. Other free zone modes are preview-only in this MVP.');
    } else {
        alert('This free zone mode is UI preview-only until FreeZoneConfig storage is added.');
    }
});

document.getElementById('refresh_diag_btn').addEventListener('click', loadDiagnostics);

async function loadConfig() {
    const sizeRes = await fetch('/get_size').catch(() => null);
    if (sizeRes && sizeRes.ok) {
        const size = await sizeRes.text();
        const parts = size.split(':');
        COLS = Number(parts[0]) || COLS;
        ROWS = Number(parts[1]) || ROWS;
    }

    const res = await fetch('/get_config');
    const cfg = await res.json();
    hardwareZonesMap = cfg.hardware_map || {};
    statusColorLayers.waiting = cfg.layers?.wait || {};
    statusColorLayers.charging = cfg.layers?.charge || {};
    statusColorLayers.error = cfg.layers?.err || {};
    zoneMeta = Array.isArray(cfg.zones) ? cfg.zones : [...DEFAULT_ZONES];
    document.getElementById('ports_brightness').value = cfg.b_ports || 120;
    document.getElementById('ports_brightness_label').textContent = cfg.b_ports || 120;
    document.getElementById('free_brightness').value = cfg.b_logo || 100;
    document.getElementById('free_color').value = cfg.c_logo || '#ffffff';
    zoneMeta.filter(z => z.type === 'free').forEach(z => { freeModes[String(z.id)] = z.mode || freeModes[String(z.id)] || 'static'; });
    buildSelects();
    buildMatrix();
    buildLegend();
    await loadDiagnostics();
}

async function loadDiagnostics() {
    try {
        const res = await fetch('/diagnostics');
        diagnostics = await res.json();
        renderDiagnostics();
    } catch (err) {
        document.getElementById('runtime_label').textContent = 'runtime: diagnostics unavailable';
        document.getElementById('runtime_dot').style.background = 'var(--orange)';
    }
}

function renderDiagnostics() {
    if (!diagnostics) return;
    const mode = diagnostics.runtimeMode || diagnostics.mode || 'runtime';
    const port1 = diagnostics.ports?.[0]?.status || 'unknown';
    document.getElementById('runtime_label').textContent = `runtime: ${mode} · Port1 ${port1}`;
    document.getElementById('runtime_dot').style.background = port1 === 'error' ? 'var(--red)' : port1 === 'charging' ? 'var(--blue)' : 'var(--green)';

    document.getElementById('diag_summary').innerHTML = [
        diagCard('Active ports', diagnostics.activePortCount ?? 1),
        diagCard('Runtime mode', mode),
        diagCard('Edit state', activeTab === 'diagnostics' ? 'read-only' : `${activeTab} preview`),
        diagCard('Primary LED', `${diagnostics.primaryLedOutput?.name || 'LED1'} / GPIO${diagnostics.primaryLedOutput?.gpio || 22}`),
    ].join('');

    document.getElementById('diag_inputs').innerHTML = (diagnostics.inputs || []).map(input =>
        diagCard(input.name, `GPIO${input.gpio} · ${input.raw} · ${input.active ? 'active' : 'idle'}`)
    ).join('');

    document.getElementById('diag_ports').innerHTML = (diagnostics.ports || []).map(port =>
        diagCard(`Port ${port.port}`, `${port.enabled ? port.status : 'disabled/reserved'} · zone ${port.zoneId}`)
    ).join('');

    document.getElementById('diag_leds').innerHTML = (diagnostics.ledOutputs || []).map(led =>
        diagCard(led.name, `GPIO${led.gpio} · ${led.state}`)
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
    document.getElementById('edit_state_line').textContent = 'Failed to load config';
    console.error(err);
});
</script>
</body>
</html>
)=====";

#endif
