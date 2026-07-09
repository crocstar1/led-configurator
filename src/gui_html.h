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
            flex-wrap: wrap;
        }

        .brand {
            display: flex;
            align-items: center;
            gap: 10px;
            min-width: 0;
            flex: 1 1 280px;
        }

        .brand-logo {
            width: 38px;
            height: 38px;
            flex: 0 0 auto;
            display: block;
        }

        .brand-copy {
            display: flex;
            flex-direction: column;
            gap: 3px;
            min-width: 0;
        }

        .brand h1 {
            margin: 0;
            font-size: 24px;
            line-height: 1.05;
            letter-spacing: 0;
            font-weight: 750;
            overflow-wrap: anywhere;
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
            flex: 0 0 auto;
            margin-left: auto;
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
            min-height: 44px;
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
            border-radius: 10px;
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
            border-radius: 10px;
            background: #fbfcff;
            max-height: 68vh;
            -webkit-overflow-scrolling: touch;
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

        .topology-preview {
            display: grid;
            gap: 4px;
            width: max-content;
            max-width: 100%;
            overflow: auto;
            padding: 8px;
            border: 1px solid var(--line);
            border-radius: 10px;
            background: #fbfcff;
        }

        .topology-cell {
            width: 30px;
            height: 26px;
            display: flex;
            align-items: center;
            justify-content: center;
            border: 1px solid var(--line);
            border-radius: 6px;
            background: #ffffff;
            color: var(--text);
            font-size: 10px;
            font-weight: 750;
            line-height: 1;
        }

        .topology-cell.first {
            border-color: var(--blue);
            background: var(--blue);
            color: #ffffff;
        }

        .topology-cell.last {
            border-color: var(--green);
            background: #eaf8ef;
        }

        .advanced-block {
            border: 1px solid var(--line);
            border-radius: 10px;
            background: #fbfcff;
            padding: 10px 12px;
        }

        .advanced-block summary {
            cursor: pointer;
            font-weight: 750;
            color: var(--text);
            list-style-position: inside;
        }

        .advanced-block[open] summary {
            margin-bottom: 10px;
        }

        .advanced-body {
            display: grid;
            gap: 10px;
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
            align-items: center;
            justify-content: center;
            padding: 16px;
            background: rgba(17, 24, 39, 0.24);
            z-index: 20;
        }

        .service-backdrop.open {
            display: flex;
        }

        .service-panel {
            width: min(760px, calc(100vw - 32px));
            max-height: calc(100vh - 32px);
            overflow: auto;
            background: var(--panel);
            border: 1px solid var(--line);
            border-radius: 10px;
            box-shadow: 0 18px 46px rgba(25, 39, 65, 0.18);
            padding: 16px;
            -webkit-overflow-scrolling: touch;
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
            gap: 11px;
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
            padding: 10px 11px;
            font: inherit;
            font-size: 13px;
            font-weight: 650;
            min-height: 44px;
        }

        input[type="checkbox"] {
            accent-color: var(--blue);
        }

        input[type="file"] {
            width: 100%;
            border: 1px dashed var(--line);
            border-radius: 9px;
            background: #ffffff;
            color: var(--text);
            padding: 10px;
            font: inherit;
            font-size: 13px;
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
            padding: 11px 13px;
            font-size: 12px;
            font-weight: 750;
            cursor: pointer;
            min-height: 44px;
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

        .progress {
            width: 100%;
            height: 10px;
            overflow: hidden;
            border-radius: 999px;
            background: var(--soft);
            border: 1px solid var(--line);
        }

        .progress-bar {
            width: 0%;
            height: 100%;
            background: var(--blue);
            transition: width 0.15s ease;
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
            .topbar { align-items: flex-start; gap: 10px; }
            .brand { gap: 9px; }
            .brand-logo { width: 34px; height: 34px; }
            .brand h1 { font-size: 21px; }
            .layout { grid-template-columns: 1fr; }
            .panel { padding: 13px; border-radius: 11px; }
            .service-backdrop { align-items: stretch; padding: 0; }
            .service-panel { width: 100%; max-height: 100vh; border-radius: 0; }
            .pixel { width: 28px; height: 28px; border-radius: 7px; }
            .legend { grid-template-columns: 1fr; }
            .diag-grid { grid-template-columns: 1fr; }
            .service-tabs { grid-template-columns: repeat(2, minmax(0, 1fr)); }
        }

        @media (max-width: 420px) {
            .brand { flex-basis: 220px; }
            .brand span { font-size: 10px; letter-spacing: 0.04em; }
            .pixel { width: 24px; height: 24px; border-radius: 6px; }
            .topology-cell { width: 24px; height: 22px; font-size: 9px; }
            .matrix { gap: 4px; }
            .grid-2 { grid-template-columns: 1fr; }
            .runtime-pill { display: none; }
            .topbar { gap: 10px; align-items: center; }
            .brand-logo { width: 32px; height: 32px; }
            .tabs { grid-template-columns: 1fr; }
            .service-tabs { grid-template-columns: repeat(2, minmax(0, 1fr)); }
            .btn-row .btn { flex: 1 1 auto; }
        }
    </style>
</head>
<body>
<div class="app">
    <header class="topbar">
        <div class="brand">
            <svg class="brand-logo" viewBox="0 0 180 180" role="img" aria-label="Логотип компании" preserveAspectRatio="xMidYMid meet">
                <g transform="translate(0,180) scale(0.1,-0.1)" fill="#07149a" stroke="none">
                    <path d="M743 1786 c-350 -67 -621 -318 -714 -661 -32 -115 -32 -335 0 -450 86 -317 329 -560 646 -646 65 -18 107 -22 225 -22 118 0 160 4 225 22 317 86 560 329 646 646 18 65 22 107 22 225 0 118 -4 160 -22 225 -86 317 -333 564 -646 645 -103 27 -284 35 -382 16z m716 -327 l111 -181 -112 -179 -111 -179 -214 0 c-117 0 -213 3 -213 6 0 4 47 84 106 180 l105 173 -105 176 c-59 97 -106 179 -106 181 0 2 96 4 214 4 l214 0 111 -181z m-539 90 c-5 -8 -44 -73 -88 -143 l-79 -128 88 -141 c66 -106 85 -144 77 -154 -6 -7 -14 -13 -18 -13 -4 0 -51 69 -104 154 l-96 154 81 134 c118 193 118 193 134 171 8 -11 10 -24 5 -34z m37 -73 c-7 -12 -37 -61 -66 -109 l-53 -88 65 -108 c54 -90 62 -110 52 -124 -10 -15 -23 0 -89 107 l-76 125 76 127 c70 115 78 125 90 109 12 -16 12 -22 1 -39z m43 -35 c8 -16 2 -33 -31 -90 l-41 -71 41 -70 c36 -59 40 -73 30 -91 -13 -26 -14 -25 -77 78 l-50 81 55 91 c30 50 56 91 58 91 2 0 9 -9 15 -19z m34 -122 l-25 -40 21 -28 c23 -34 25 -54 7 -69 -10 -9 -21 1 -46 43 l-33 54 34 55 c30 50 34 53 50 39 16 -14 15 -18 -8 -54z m49 -61 l-10 -23 -12 23 c-9 17 -9 27 0 44 l12 23 10 -23 c7 -14 7 -30 0 -44z m-302 -159 c57 -95 106 -176 107 -181 2 -5 -44 -90 -103 -188 l-107 -180 -220 0 c-120 0 -217 3 -215 8 2 4 47 80 100 168 53 89 99 168 102 177 4 9 -32 78 -90 173 -53 87 -101 166 -106 176 -9 17 1 18 209 18 l219 0 104 -171z m-542 112 c8 -15 -6 -44 -74 -156 -47 -75 -85 -139 -85 -142 0 -3 37 -66 82 -142 44 -75 82 -139 84 -142 2 -3 -3 -14 -10 -24 -12 -17 -23 -3 -110 144 -53 89 -95 166 -92 169 28 51 189 312 192 312 2 0 8 -8 13 -19z m51 -88 c0 -7 -27 -56 -60 -109 -33 -52 -60 -98 -60 -101 0 -3 27 -50 60 -105 63 -104 68 -119 47 -136 -10 -8 -30 18 -88 115 l-76 126 76 123 c58 94 79 120 88 112 7 -6 13 -17 13 -25z m37 -52 c10 -16 5 -30 -31 -88 l-43 -70 43 -73 c35 -61 40 -76 31 -92 -7 -10 -15 -18 -19 -18 -4 0 -31 41 -61 92 l-54 91 55 89 c30 48 57 88 61 88 3 0 12 -9 18 -19z m34 -120 c-26 -36 -26 -42 -2 -83 17 -28 18 -36 6 -51 -13 -17 -16 -15 -44 35 -17 29 -31 58 -31 63 0 6 14 32 31 59 30 47 31 48 45 27 14 -19 13 -24 -5 -50z m50 -57 l-12 -27 -15 21 c-12 17 -12 25 -2 41 11 18 14 19 27 6 12 -12 12 -19 2 -41z m935 -2 c6 -4 58 -85 117 -179 l107 -173 -109 -177 -109 -178 -216 -3 c-119 -1 -216 0 -216 2 0 3 48 84 106 180 l105 176 -105 172 c-57 95 -102 176 -100 180 6 10 405 10 420 0z m-429 -50 c9 -16 -2 -40 -76 -159 l-88 -141 87 -143 c74 -119 86 -145 77 -161 -16 -24 -19 -20 -126 154 l-91 148 98 160 c53 88 99 160 102 160 3 0 10 -8 17 -18z m39 -107 c-7 -11 -36 -59 -65 -107 l-53 -87 53 -88 c29 -48 59 -97 66 -109 11 -17 11 -23 0 -38 -12 -17 -21 -6 -91 108 l-77 126 77 125 c68 111 78 123 90 107 10 -14 10 -21 0 -37z m14 -125 l-42 -70 41 -71 c34 -58 39 -74 30 -91 -13 -24 -10 -28 -74 77 l-53 87 55 89 c53 85 56 89 70 70 13 -19 10 -27 -27 -91z m80 3 c0 -8 -9 -28 -21 -44 l-21 -30 26 -33 c25 -33 25 -35 8 -56 -17 -21 -17 -21 -50 35 l-33 55 32 55 c26 41 36 52 46 43 7 -6 13 -17 13 -25z m33 -95 l-10 -23 -12 23 c-9 17 -9 27 0 44 l12 23 10 -23 c7 -14 7 -30 0 -44z"/>
                    <path d="M1000 1585 c0 -3 7 -17 16 -30 15 -23 22 -25 101 -25 70 0 89 -4 110 -20 l26 -20 -97 0 c-53 0 -96 -2 -96 -5 0 -3 7 -17 16 -30 16 -24 21 -25 115 -25 99 0 99 0 99 -25 0 -25 -1 -25 -86 -25 l-85 0 21 -30 21 -30 125 0 124 0 0 69 0 69 -82 66 -83 66 -122 0 c-68 0 -123 -2 -123 -5z"/>
                    <path d="M320 1216 c0 -2 36 -63 81 -135 l81 -131 60 0 60 0 -26 44 c-14 25 -26 47 -26 50 0 3 20 6 44 6 43 0 46 -2 72 -50 l27 -50 59 0 60 0 -24 43 c-13 23 -49 84 -78 134 l-55 93 -57 0 c-32 0 -58 -3 -58 -6 0 -3 12 -25 26 -50 l26 -44 -46 0 c-44 0 -47 2 -78 50 l-33 50 -57 0 c-32 0 -58 -2 -58 -4z"/>
                    <path d="M1002 842 c2 -5 37 -66 78 -135 l75 -127 58 0 c31 0 57 3 57 6 0 3 -13 26 -30 50 -41 62 -13 59 67 -7 59 -48 63 -50 116 -47 l55 3 -74 63 -74 63 0 70 0 69 -60 0 -60 0 0 -51 c0 -43 -3 -50 -17 -47 -10 2 -31 24 -48 50 l-30 48 -58 0 c-32 0 -57 -4 -55 -8z"/>
                </g>
            </svg>
            <div class="brand-copy">
                <h1>LED-контроллер матрицы</h1>
                <span>инженерная настройка контроллера</span>
            </div>
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
                <div class="panel-title"><h2>Разметка</h2></div>
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
                    <details class="advanced-block" id="matrix_settings_block">
                        <summary>Настройки матрицы</summary>
                        <div class="advanced-body">
                            <div>
                                <label for="topology_select">Топология LED</label>
                                <select id="topology_select"></select>
                            </div>
                            <div class="status-line" id="topology_summary"></div>
                            <div class="notice">Смена топологии меняет физический порядок вывода zoneMap и слоев. Если матрица светится не в той области, выберите другой вариант.</div>
                            <div class="topology-preview" id="topology_preview" aria-label="Порядок LED"></div>
                            <div class="btn-row">
                                <button class="btn primary" id="save_topology_btn">Сохранить топологию</button>
                            </div>
                            <div class="hint" id="topology_message"></div>
                        </div>
                    </details>
                    <div class="btn-row">
                        <button class="btn primary" id="save_zones_btn">Сохранить зоны</button>
                        <button class="btn danger" id="clear_zone_btn">Очистить выбранную зону</button>
                    </div>
                </div>
            </section>

            <section class="panel tab-panel" id="tab_status">
                <div class="panel-title"><h2>Слои статусов</h2></div>
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
                <div class="panel-title"><h2>Настройка зоны</h2></div>
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
                    <div class="notice field-hidden" id="free_zone_notice">Свободная зона не размечена.</div>
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
                            <label for="free_brightness">Яркость выбранной свободной зоны</label>
                            <input type="range" id="free_brightness" min="1" max="255" value="100">
                        </div>
                    </div>
                    <label>Легенда</label>
                    <div class="legend" id="free_legend"></div>
                    <div class="btn-row">
                        <button class="btn danger" id="free_erase_btn">Очистить рисунок</button>
                        <button class="btn primary" id="apply_free_btn">Сохранить настройки зоны</button>
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
                        <button class="btn" id="refresh_diag_btn" type="button">Обновить данные</button>
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
                        <button class="btn" id="refresh_network_btn" type="button">Обновить статус сети</button>
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
                            <button class="btn" id="scan_network_btn" type="button">Сканировать сети</button>
                            <button class="btn primary" id="network_save_connect_btn" type="submit">Сохранить и подключиться</button>
                        </div>
                    </form>
                    <div class="diag-grid" id="network_scan"></div>
                    <div class="hint" id="network_message"></div>
                </div>
            </section>

            <section class="service-section" id="service_firmware">
                <div class="section">
                    <div class="row">
                        <strong>Прошивка</strong>
                    </div>
                    <form id="firmware_form" class="section">
                        <div>
                            <label for="firmware_file">Файл прошивки</label>
                            <input id="firmware_file" name="update" type="file" accept=".bin,application/octet-stream">
                        </div>
                        <div class="hint" id="firmware_file_label">Файл не выбран.</div>
                        <div class="progress" aria-hidden="true">
                            <div class="progress-bar" id="firmware_progress"></div>
                        </div>
                        <div class="btn-row">
                            <button class="btn primary" id="firmware_upload_btn" type="submit" disabled>Загрузить прошивку</button>
                        </div>
                    </form>
                    <div class="hint" id="firmware_message">Выберите firmware.bin.</div>
                </div>
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

const TOPOLOGY_OPTIONS = [
    { value: 0, key: 'vert_down', label: 'Вертикальная · старт справа снизу', hint: 'LED0 справа снизу, далее змейкой по столбцам.' },
    { value: 1, key: 'vert_up', label: 'Вертикальная · старт слева снизу', hint: 'LED0 слева снизу, далее змейкой по столбцам.' },
    { value: 2, key: 'horiz_right', label: 'Горизонтальная · старт слева сверху', hint: 'LED0 слева сверху, далее змейкой по строкам.' },
    { value: 3, key: 'horiz_left', label: 'Горизонтальная · старт справа сверху', hint: 'LED0 справа сверху, далее змейкой по строкам.' },
];

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
let pendingTopology = 0;
let activePortCount = 2;

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

function buildDefaultPortMap(cols = 12, rows = 8, portCount = 2) {
    const map = {};
    const activeCount = clampActivePortCount(portCount);
    for (let y = 0; y < rows; y++) {
        for (let x = 0; x < cols; x++) {
            const portIndex = Math.min(activeCount - 1, Math.floor((x * activeCount) / cols));
            map[`${x}-${y}`] = String(1 + portIndex);
        }
    }
    return map;
}

function clampActivePortCount(value) {
    const count = Number(value);
    return Number.isFinite(count) ? Math.max(1, Math.min(4, Math.trunc(count))) : 1;
}

function zonesForActivePortCount(zones, count) {
    const activeCount = clampActivePortCount(count);
    return (zones || DEFAULT_ZONES).map(zone => {
        const copy = { ...zone };
        if (copy.type !== 'port') return copy;

        const portNumber = Number(copy.linked_port ?? copy.linkedPort ?? copy.id);
        const active = Number.isFinite(portNumber) && portNumber >= 1 && portNumber <= activeCount;
        copy.enabled = active;
        copy.reserved = !active;
        return copy;
    });
}

function buildMockConfig() {
    const mockActivePortCount = 2;
    return {
        topo: 0,
        b_ports: 120,
        c_wait: '#34c759',
        c_charge: '#0055ff',
        c_error: '#ff3b30',
        activePortCount: mockActivePortCount,
        matrix: { cols: 12, rows: 8, total: 96, topology: 0 },
        hardware_map: buildDefaultPortMap(12, 8, mockActivePortCount),
        layers: { wait: {}, charge: {}, err: {} },
        zones: zonesForActivePortCount(DEFAULT_ZONES, mockActivePortCount),
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
        activePortCount: 2,
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
            { port: 2, enabled: true, zoneId: 2, chargingInput: 'Data3', errorInput: 'Data4', status: 'waiting' },
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
        apSsid: 'LED_MATRIX_A1B2C3',
        apIp: '192.168.4.1',
        apClients: 1,
    };
}

function networkModeLabel(mode) {
    const labels = {
        sta: 'STA',
        ap_fallback: 'Fallback AP',
        connecting: 'Подключение',
    };
    return labels[mode] || mode || 'неизвестно';
}

function networkStatusLabel(status) {
    const labels = {
        connected: 'Подключено',
        connecting: 'Подключается',
        not_configured: 'Не настроено',
        connect_failed: 'Fallback AP: сеть недоступна',
        connect_timeout: 'Fallback AP: таймаут подключения',
        mock: 'Локальный preview',
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
    const mapped = activeFreeZoneIsMapped();
    const colorField = document.getElementById('free_color_field');
    const colorLabel = document.getElementById('free_color_label');
    const clearBtn = document.getElementById('free_erase_btn');
    const modeSelect = document.getElementById('free_mode_select');
    const colorInput = document.getElementById('free_color');
    const brightnessInput = document.getElementById('free_brightness');
    const applyBtn = document.getElementById('apply_free_btn');
    const notice = document.getElementById('free_zone_notice');

    colorField.classList.toggle('field-hidden', mode === 'rainbow');
    clearBtn.classList.toggle('field-hidden', mode !== 'custom' || !mapped);
    colorLabel.textContent = mode === 'custom' ? 'Цвет кисти' : 'Цвет зоны';
    modeSelect.disabled = !mapped;
    colorInput.disabled = !mapped || mode === 'rainbow';
    brightnessInput.disabled = !mapped;
    applyBtn.disabled = !mapped;
    notice.classList.toggle('field-hidden', mapped);
    notice.textContent = mapped
        ? ''
        : `${zoneDisplayName(activeFreeZone)} не размечена. Сначала назначьте этой зоне пиксели во вкладке «Зоны матрицы».`;
}

function updateZoneViewControls() {
    const isOverview = zoneViewMode === 'overview';
    const missingActivePortNotice = missingActivePortZoneNotice();
    document.getElementById('zone_edit_mode_btn').classList.toggle('active', !isOverview);
    document.getElementById('zone_overview_mode_btn').classList.toggle('active', isOverview);
    document.getElementById('zone_tools_row').classList.toggle('field-hidden', isOverview);
    document.getElementById('clear_zone_btn').disabled = isOverview;
    const baseText = isOverview
        ? 'Обзор: все зоны показаны своими цветами, рисование отключено.'
        : 'Рисуйте внутри выбранной зоны. Чужие зоны заблокированы.';
    document.getElementById('zones_mode_notice').textContent = missingActivePortNotice
        ? `${baseText} ${missingActivePortNotice}`
        : baseText;
}

function updateFreeViewControls() {
    const mapped = activeFreeZoneIsMapped();
    if (!mapped && freeViewMode === 'edit') {
        freeViewMode = 'overview';
    }
    const isOverview = freeViewMode === 'overview';
    document.getElementById('free_edit_mode_btn').classList.toggle('active', !isOverview);
    document.getElementById('free_overview_mode_btn').classList.toggle('active', isOverview);
    document.getElementById('free_edit_mode_btn').disabled = !mapped;
    document.getElementById('free_erase_btn').disabled = isOverview || !mapped;
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
    pendingTopology = Math.max(0, Math.min(3, Number(matrixConfig.topology) || 0));
    activePortCount = clampActivePortCount(cfg.activePortCount ?? cfg.active_port_count ?? activePortCount);

    hardwareZonesMap = sanitizeZoneMapForSize(cfg.hardware_map || {});
    statusColorLayers.waiting = sanitizeLayerForSize(cfg.layers?.wait || {});
    statusColorLayers.charging = sanitizeLayerForSize(cfg.layers?.charge || {});
    statusColorLayers.error = sanitizeLayerForSize(cfg.layers?.err || {});
    Object.keys(freeCustomLayers).forEach(zoneId => {
        freeCustomLayers[zoneId] = sanitizeLayerForSize(freeCustomLayers[zoneId]);
    });
    zoneMeta = zonesForActivePortCount(
        Array.isArray(cfg.zones) && cfg.zones.length ? cfg.zones : DEFAULT_ZONES,
        activePortCount
    );
    document.getElementById('ports_brightness').value = cfg.b_ports || 120;
    document.getElementById('ports_brightness_label').textContent = cfg.b_ports || 120;
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

function zonePixelCount(zoneId) {
    const target = String(zoneId);
    return Object.values(hardwareZonesMap || {}).filter(value => String(value) === target).length;
}

function zoneHasPixels(zoneId) {
    return zonePixelCount(zoneId) > 0;
}

function isRequiredActivePortZone(zoneId) {
    const z = zoneById(zoneId);
    if (!z || z.type !== 'port' || z.reserved || !z.enabled) return false;
    const portNumber = Number(z.linked_port || z.linkedPort || z.id);
    const count = Number.isFinite(Number(activePortCount)) ? Number(activePortCount) : 1;
    return portNumber >= 1 && portNumber <= count;
}

function activePortZoneErrorMessage(zoneId) {
    return `${zoneDisplayName(zoneId)} должен иметь хотя бы один пиксель для отображения статуса.`;
}

function validateActivePortZones(map = hardwareZonesMap) {
    const count = Number.isFinite(Number(activePortCount)) ? Number(activePortCount) : 1;
    for (let port = 1; port <= count; port++) {
        const zoneId = String(port);
        if (zonePixelCountInMap(map, zoneId) === 0) {
            return activePortZoneErrorMessage(zoneId);
        }
    }
    return '';
}

function missingActivePortZoneNotice(map = hardwareZonesMap) {
    const count = clampActivePortCount(activePortCount);
    const missing = [];

    for (let port = 1; port <= count; port++) {
        const zoneId = String(port);
        if (zonePixelCountInMap(map, zoneId) === 0) {
            missing.push(zoneDisplayName(zoneId));
        }
    }

    return missing.length
        ? `Не размечены активные зоны: ${missing.join(', ')}. Сигналы портов будут читаться, но индикации негде отображаться.`
        : '';
}

function zonePixelCountInMap(map, zoneId) {
    const target = String(zoneId);
    return Object.values(map || {}).filter(value => String(value) === target).length;
}

function showEditSafetyMessage(message) {
    document.getElementById('edit_state_line').textContent = message;
}

function activeFreeZoneIsMapped() {
    return isFreeZone(activeFreeZone) && zoneHasPixels(activeFreeZone);
}

function topologyOption(value) {
    return TOPOLOGY_OPTIONS.find(item => item.value === Number(value)) || TOPOLOGY_OPTIONS[0];
}

function ledIndexForTopology(x, y, topology) {
    let tx = x;
    let ty = y;

    switch (Number(topology)) {
        case 0:
            tx = (COLS - 1) - x;
            return (tx % 2 === 0) ? (tx * ROWS) + ty : (tx * ROWS) + ((ROWS - 1) - ty);
        case 1:
            return (tx % 2 === 0) ? (tx * ROWS) + ty : (tx * ROWS) + ((ROWS - 1) - ty);
        case 2:
            ty = (ROWS - 1) - y;
            return (ty % 2 === 0) ? (ty * COLS) + tx : (ty * COLS) + ((COLS - 1) - tx);
        case 3:
            tx = (COLS - 1) - x;
            ty = (ROWS - 1) - y;
            return (ty % 2 === 0) ? (ty * COLS) + tx : (ty * COLS) + ((COLS - 1) - tx);
        default:
            return -1;
    }
}

function setTopologyMessage(text, isError = false) {
    const el = document.getElementById('topology_message');
    if (!el) return;
    el.textContent = text || '';
    el.style.color = isError ? 'var(--red)' : 'var(--muted)';
}

function renderTopologyControls() {
    const select = document.getElementById('topology_select');
    const preview = document.getElementById('topology_preview');
    const summary = document.getElementById('topology_summary');
    if (!select || !preview || !summary) return;

    if (!select.options.length) {
        TOPOLOGY_OPTIONS.forEach(item => {
            const option = document.createElement('option');
            option.value = String(item.value);
            option.textContent = item.label;
            select.appendChild(option);
        });
    }

    const normalized = Math.max(0, Math.min(3, Number(pendingTopology) || 0));
    pendingTopology = normalized;
    select.value = String(normalized);
    summary.textContent = topologyOption(normalized).hint;

    preview.innerHTML = '';
    preview.style.gridTemplateColumns = `repeat(${COLS}, 30px)`;
    const total = COLS * ROWS;
    for (let y = ROWS - 1; y >= 0; y--) {
        for (let x = 0; x < COLS; x++) {
            const index = ledIndexForTopology(x, y, normalized);
            const cell = document.createElement('div');
            cell.className = 'topology-cell';
            if (index === 0) cell.classList.add('first');
            if (index === total - 1) cell.classList.add('last');
            cell.textContent = String(index);
            cell.title = `x=${x}, y=${y}, LED=${index}`;
            preview.appendChild(cell);
        }
    }
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
    renderTopologyControls();
}

function buildSelects() {
    const zoneSelect = document.getElementById('zone_select');
    const freeZoneSelect = document.getElementById('free_zone_select');
    zoneSelect.innerHTML = '';
    freeZoneSelect.innerHTML = '';
    let firstMappedFreeZone = null;
    let activeFreeZoneStillSelectable = false;

    zoneMeta.forEach(z => {
        const opt = document.createElement('option');
        opt.value = z.id;
        opt.textContent = zoneOptionLabel(z);
        opt.disabled = z.reserved || !z.enabled;
        zoneSelect.appendChild(opt);

        if (z.type === 'free') {
            const mapped = zoneHasPixels(z.id);
            if (mapped && firstMappedFreeZone === null) firstMappedFreeZone = String(z.id);
            if (mapped && String(z.id) === String(activeFreeZone)) activeFreeZoneStillSelectable = true;
            const fo = document.createElement('option');
            fo.value = z.id;
            fo.textContent = mapped ? zoneDisplayName(z) : `${zoneDisplayName(z)} · не размечена`;
            fo.disabled = !mapped;
            freeZoneSelect.appendChild(fo);
        }
    });

    zoneSelect.value = activeZoneId;
    if (!activeFreeZoneStillSelectable && firstMappedFreeZone !== null) {
        activeFreeZone = firstMappedFreeZone;
    }
    freeZoneSelect.value = activeFreeZone;
    document.getElementById('free_mode_select').value = freeModes[activeFreeZone] || 'static';
    syncFreeColorInput();
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
    buildSelects();

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
            if (currentZone === activeZoneId) {
                delete hardwareZonesMap[key];
            }
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
        if (activeServiceSection === 'firmware') updateFirmwareFileState();
    });
});

document.getElementById('refresh_network_btn').addEventListener('click', loadNetworkStatus);
document.getElementById('scan_network_btn').addEventListener('click', scanNetworks);
document.getElementById('network_dhcp').addEventListener('change', updateNetworkStaticFields);
document.getElementById('network_form').addEventListener('submit', e => {
    e.preventDefault();
    saveNetworkConfig();
});
document.getElementById('auth_form').addEventListener('submit', e => {
    e.preventDefault();
    saveAuthConfig();
});
document.getElementById('firmware_file').addEventListener('change', updateFirmwareFileState);
document.getElementById('firmware_form').addEventListener('submit', e => {
    e.preventDefault();
    uploadFirmware();
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

document.getElementById('topology_select').addEventListener('change', e => {
    pendingTopology = Number(e.target.value);
    renderTopologyControls();
    setTopologyMessage('');
});

document.getElementById('save_topology_btn').addEventListener('click', async () => {
    if (!backendAvailable) {
        matrixConfig.topology = pendingTopology;
        setTopologyMessage('Контроллер недоступен: топология изменена только в браузере.');
        return;
    }

    const body = new URLSearchParams();
    body.set('topology', String(pendingTopology));

    try {
        const res = await fetch('/save_topology', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body,
        });
        if (!res.ok || (await res.text()) !== 'OK') throw new Error('SAVE_FAILED');
        matrixConfig.topology = pendingTopology;
        await loadConfig();
        setTopologyMessage('Топология сохранена.');
    } catch (err) {
        setTopologyMessage('Не удалось сохранить топологию.', true);
    }
});

document.getElementById('save_zones_btn').addEventListener('click', async () => {
    const validationError = validateActivePortZones();
    if (validationError) {
        alert(validationError);
        return;
    }

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
        const text = await res.text();
        alert(text === 'OK'
            ? 'Зоны сохранены во Flash.'
            : text === 'ACTIVE_PORT_ZONE_EMPTY'
                ? 'Каждый активный порт должен иметь хотя бы один пиксель для отображения статуса.'
                : 'Не удалось сохранить зоны.');
    } catch (err) {
        alert('Контроллер недоступен: зоны изменены только в браузере.');
    }
});

document.getElementById('clear_zone_btn').addEventListener('click', () => {
    if (zonePixelCount(activeZoneId) === 0) return;
    if (isPortZone(activeZoneId)) {
        const message = isRequiredActivePortZone(activeZoneId)
            ? `Очистить ${zoneDisplayName(activeZoneId)}? Этот порт останется без индикации, и сохранить зоны не получится, пока вы не назначите ему хотя бы один пиксель.`
            : `Очистить ${zoneDisplayName(activeZoneId)}? Этот порт сейчас в резерве, но его разметка будет удалена.`;
        if (!confirm(message)) return;
    }
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
    document.getElementById('network_ap_ssid').value = status.apSsid || 'LED_MATRIX_A1B2C3';
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
        diagCard('Состояние', networkStatus.connected ? 'Подключено' : networkStatusLabel(networkStatus.status)),
        diagCard('Wi-Fi SSID', networkStatus.connectedSsid || networkStatus.ssid || 'не задан'),
        diagCard('Wi-Fi IP', networkStatus.ip || 'нет'),
        diagCard('AP fallback', networkStatus.apSsid || 'не задан'),
        diagCard('AP IP', networkStatus.apIp || 'нет'),
        diagCard('RSSI', networkStatus.connected ? `${networkStatus.rssi} dBm` : 'нет'),
        diagCard('Адресация', networkStatus.dhcp ? 'DHCP' : 'Static'),
    ].join('');
    fillNetworkForm(networkStatus);
}

async function scanNetworks() {
    if (!backendAvailable) {
        document.getElementById('network_scan').innerHTML = [
            diagCard('LED_MATRIX_A1B2C3', 'mock · -45 dBm'),
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

    const button = document.getElementById('network_save_connect_btn');
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
        button.disabled = true;
        setNetworkMessage('Сохранение настроек сети...');
        const res = await fetch('/save_network', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body,
        });
        if (!res.ok) throw new Error(await res.text());

        const reconnect = await fetch('/network_reconnect', { method: 'POST' });
        if (!reconnect.ok) throw new Error(await reconnect.text());

        setNetworkMessage('Настройки сохранены. Переподключение запущено, интерфейс может временно пропасть.');
        setTimeout(loadNetworkStatus, 4000);
    } catch (err) {
        setNetworkMessage(`Не удалось сохранить или применить сеть: ${err.message}`, true);
    } finally {
        button.disabled = false;
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
        setAuthMessage(data.defaultCredentials
            ? 'Используется стандартный пароль admin/admin. Смените пароль перед эксплуатацией.'
            : '');
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

function setFirmwareProgress(percent) {
    const bar = document.getElementById('firmware_progress');
    bar.style.width = `${Math.max(0, Math.min(100, percent || 0))}%`;
}

function setFirmwareMessage(text, isError = false) {
    const el = document.getElementById('firmware_message');
    el.textContent = text || '';
    el.style.color = isError ? 'var(--red)' : 'var(--muted)';
}

function firmwareErrorLabel(code) {
    const labels = {
        AUTH_REQUIRED: 'требуется авторизация',
        NO_FILE: 'файл не выбран',
        BAD_FILE_TYPE: 'ожидается файл .bin',
        EMPTY_FILE: 'пустой файл',
        EMPTY_CHUNK: 'пустой блок данных',
        UPDATE_BEGIN_FAILED: 'не удалось начать обновление',
        UPDATE_WRITE_FAILED: 'ошибка записи прошивки',
        UPDATE_END_FAILED: 'не удалось завершить обновление',
        UPLOAD_ABORTED: 'загрузка прервана',
        UPDATE_FAILED: 'обновление не выполнено',
    };
    return labels[code] || code || 'неизвестная ошибка';
}

function updateFirmwareFileState() {
    const input = document.getElementById('firmware_file');
    const button = document.getElementById('firmware_upload_btn');
    const label = document.getElementById('firmware_file_label');
    const file = input.files && input.files[0];

    setFirmwareProgress(0);
    button.disabled = !file;

    if (!file) {
        label.textContent = 'Файл не выбран.';
        setFirmwareMessage('Выберите firmware.bin.');
        return;
    }

    const sizeKb = Math.max(1, Math.ceil(file.size / 1024));
    label.textContent = `${file.name} · ${sizeKb} КБ`;

    if (!file.name.toLowerCase().endsWith('.bin')) {
        button.disabled = true;
        setFirmwareMessage('Ожидается файл прошивки .bin.', true);
        return;
    }

    if (file.size <= 0) {
        button.disabled = true;
        setFirmwareMessage('Файл пустой.', true);
        return;
    }

    setFirmwareMessage('Файл выбран. Можно загрузить.');
}

function uploadFirmware() {
    if (!backendAvailable) {
        setFirmwareMessage('Backend недоступен, прошивка возможна только через контроллер.', true);
        return;
    }

    const input = document.getElementById('firmware_file');
    const button = document.getElementById('firmware_upload_btn');
    const file = input.files && input.files[0];

    if (!file) {
        setFirmwareMessage('Выберите firmware.bin.', true);
        return;
    }
    if (!file.name.toLowerCase().endsWith('.bin')) {
        setFirmwareMessage('Ожидается файл прошивки .bin.', true);
        return;
    }
    if (file.size <= 0) {
        setFirmwareMessage('Файл пустой.', true);
        return;
    }

    const form = new FormData();
    form.append('update', file, file.name);

    const xhr = new XMLHttpRequest();
    button.disabled = true;
    setFirmwareProgress(0);
    setFirmwareMessage('Загрузка прошивки...');

    xhr.open('POST', '/update');
    xhr.upload.onprogress = event => {
        if (event.lengthComputable) {
            setFirmwareProgress((event.loaded / event.total) * 100);
        }
    };
    xhr.onload = () => {
        button.disabled = false;
        const response = (xhr.responseText || '').trim();
        if (xhr.status >= 200 && xhr.status < 300 && response === 'OK') {
            setFirmwareProgress(100);
            button.disabled = true;
            setFirmwareMessage('Прошивка загружена. Устройство перезагружается.');
            window.setTimeout(() => setFirmwareMessage('Перезагрузка устройства...'), 700);
            return;
        }
        setFirmwareMessage(`Ошибка прошивки: ${firmwareErrorLabel(response || String(xhr.status))}.`, true);
    };
    xhr.onerror = () => {
        button.disabled = false;
        setFirmwareMessage('Ошибка соединения при загрузке прошивки.', true);
    };
    xhr.onabort = () => {
        button.disabled = false;
        setFirmwareMessage('Загрузка прошивки прервана.', true);
    };
    xhr.send(form);
}

function renderDiagnostics() {
    if (!diagnostics) return;
    const reportedActivePorts = Number(diagnostics.activePortCount);
    activePortCount = clampActivePortCount(reportedActivePorts);
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
