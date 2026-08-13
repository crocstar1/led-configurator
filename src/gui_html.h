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

        .layout > *,
        .side-stack,
        .panel {
            min-width: 0;
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
            max-width: 100%;
            box-sizing: border-box;
            padding: 12px;
            border: 1px solid var(--line);
            border-radius: 10px;
            background: #fbfcff;
            max-height: 68vh;
            -webkit-overflow-scrolling: touch;
        }

        .matrix {
            --matrix-cell-size: 30px;
            display: grid;
            gap: 5px;
            width: max-content;
            margin: 0 auto;
            touch-action: none;
        }

        .matrix.dense {
            --matrix-cell-size: 24px;
            gap: 4px;
        }

        .matrix.very-dense {
            --matrix-cell-size: 18px;
            gap: 3px;
        }

        .pixel {
            width: var(--matrix-cell-size);
            height: var(--matrix-cell-size);
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
            --topology-cell-width: 30px;
            --topology-cell-height: 26px;
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

        .topology-preview.dense {
            --topology-cell-width: 24px;
            --topology-cell-height: 22px;
        }

        .topology-preview.compact {
            width: 100%;
        }

        .topology-cell {
            width: var(--topology-cell-width);
            height: var(--topology-cell-height);
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

        .topology-direction {
            display: grid;
            gap: 5px;
            padding: 4px;
            color: var(--muted);
            font-size: 12px;
            line-height: 1.4;
        }

        .topology-direction strong {
            color: var(--text);
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

        #matrix_settings_block,
        #matrix_settings_block .advanced-body,
        #matrix_settings_block .advanced-body > * {
            min-width: 0;
            max-width: 100%;
        }

        #matrix_settings_block select,
        #matrix_settings_block input[type="number"] {
            min-width: 0;
            max-width: 100%;
        }

        #matrix_settings_block input[type="number"] {
            text-align: left;
            padding-left: 11px;
            padding-right: 11px;
            font-size: 13px;
        }

        .hardware-config-group {
            display: grid;
            gap: 10px;
        }

        #matrix_settings_block .notice,
        #matrix_settings_block .status-line,
        #matrix_settings_block .hint {
            min-width: 0;
            max-width: 100%;
            white-space: normal;
            overflow-wrap: anywhere;
        }

        #matrix_settings_block .status-line {
            flex-wrap: wrap;
        }

        #matrix_settings_block .topology-preview {
            width: 100%;
            min-width: 0;
            max-width: 100%;
            overflow-x: auto;
            overflow-y: hidden;
            justify-content: start;
            overscroll-behavior-inline: contain;
            -webkit-overflow-scrolling: touch;
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

        select, input[type="color"], input[type="text"], input[type="password"], input[type="number"] {
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

        input[type="number"] {
            max-width: 64px;
            text-align: center;
            padding-left: 6px;
            padding-right: 6px;
            font-size: 12px;
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

        .service-action-row {
            justify-content: flex-end;
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

        .service-trigger {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 7px;
        }

        .service-trigger svg {
            width: 17px;
            height: 17px;
            flex: 0 0 auto;
            fill: none;
            stroke: currentColor;
            stroke-width: 2;
            stroke-linecap: round;
            stroke-linejoin: round;
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

        .btn.secondary {
            border-color: #c9d8f5;
            background: var(--blue-soft);
            color: #315b9f;
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

        .safety-warning {
            margin-bottom: 12px;
            border-color: #f2cf91;
            background: #fff8ed;
            color: #7a4b00;
            font-weight: 700;
        }

        .brightness-control {
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .brightness-control input[type="range"] {
            min-width: 0;
            flex: 1 1 auto;
        }

        .section-divider {
            height: 1px;
            background: var(--line);
            margin: 2px 0;
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

        .network-scan-result {
            width: 100%;
            min-width: 0;
            min-height: 44px;
            box-sizing: border-box;
            color: var(--text);
            text-align: left;
            white-space: normal;
            overflow-wrap: anywhere;
        }

        button.network-scan-result {
            cursor: pointer;
            font: inherit;
        }

        button.network-scan-result:hover,
        button.network-scan-result:focus-visible,
        button.network-scan-result[aria-pressed="true"] {
            border-color: var(--blue);
            background: #eef4ff;
        }

        .network-scan-result.non-selectable {
            color: var(--muted);
        }

        .status-line {
            display: flex;
            gap: 8px;
            align-items: center;
            color: var(--muted);
            font-size: 12px;
            min-height: 18px;
        }

        .status-line.warning {
            color: var(--red);
            font-weight: 700;
        }

        .dirty-indicator {
            display: inline-flex;
            align-items: center;
            min-height: 24px;
            padding: 4px 8px;
            border: 1px solid #ffe0ad;
            border-radius: 999px;
            background: #fff8ed;
            color: #9a5d00;
            font-size: 11px;
            font-weight: 750;
            white-space: nowrap;
        }

        @media (max-width: 860px) {
            body { padding: 12px 10px 24px; }
            .topbar { align-items: flex-start; gap: 10px; }
            .brand { gap: 9px; }
            .brand-logo { width: 34px; height: 34px; }
            .brand h1 { font-size: 20px; }
            .layout { grid-template-columns: 1fr; }
            .panel { padding: 13px; border-radius: 11px; }
            .service-backdrop { align-items: stretch; padding: 0; }
            .service-panel { width: 100%; max-height: 100vh; border-radius: 0; }
            .matrix { --matrix-cell-size: 28px; }
            .matrix.dense { --matrix-cell-size: 22px; }
            .matrix.very-dense { --matrix-cell-size: 18px; }
            .topology-preview { --topology-cell-width: 28px; --topology-cell-height: 24px; }
            .topology-preview.dense { --topology-cell-width: 22px; --topology-cell-height: 20px; }
            .legend { grid-template-columns: 1fr; }
            .diag-grid { grid-template-columns: 1fr; }
            .service-tabs { grid-template-columns: repeat(2, minmax(0, 1fr)); }
        }

        @media (max-width: 420px) {
            .brand { flex-basis: 220px; }
            .brand span { font-size: 10px; letter-spacing: 0.04em; }
            .brand h1 { font-size: 18px; }
            .matrix { --matrix-cell-size: 24px; }
            .matrix.dense { --matrix-cell-size: 20px; }
            .matrix.very-dense { --matrix-cell-size: 17px; }
            .pixel { border-radius: 6px; }
            .topology-preview { --topology-cell-width: 24px; --topology-cell-height: 22px; }
            .topology-preview.dense { --topology-cell-width: 20px; --topology-cell-height: 19px; }
            .topology-cell { font-size: 9px; }
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
                <h1>Контроллер LED-индикации</h1>
                <span>Настройка матрицы</span>
            </div>
        </div>
        <div class="top-actions">
            <div class="runtime-pill"><span class="dot" id="runtime_dot"></span><span id="runtime_label">режим: загрузка</span></div>
            <button class="btn service-trigger" id="service_open_btn" type="button">
                <svg viewBox="0 0 24 24" aria-hidden="true">
                    <path d="M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.09a2 2 0 0 1 1 1.74v.5a2 2 0 0 1-1 1.74l-.15.09a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.38a2 2 0 0 0-.73-2.73l-.15-.09a2 2 0 0 1-1-1.74v-.5a2 2 0 0 1 1-1.74l.15-.09a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z"></path>
                    <circle cx="12" cy="12" r="3"></circle>
                </svg>
                <span>Сервис</span>
            </button>
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
                <span class="hint" id="matrix_hint">12×8</span>
            </div>
            <div class="notice safety-warning field-hidden" id="active_ports_warning"></div>
            <div class="matrix-shell">
                <div class="matrix" id="matrix_grid"></div>
            </div>
            <div class="status-line field-hidden" id="edit_state_line"></div>
        </section>

        <aside class="side-stack">
            <section class="panel tab-panel active" id="tab_zones">
                <div class="panel-title"><h2>Разметка</h2><span class="dirty-indicator field-hidden" id="zones_dirty_indicator">не сохранено</span></div>
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
                    <div class="notice" id="zones_mode_notice">Рисуйте внутри выбранной зоны. Чужие зоны заблокированы.</div>
                    <div class="section-divider"></div>
                    <label>Легенда</label>
                    <div class="legend" id="zone_legend"></div>
                    <div class="section-divider"></div>
                    <details class="advanced-block" id="matrix_settings_block">
                        <summary>Настройки матрицы</summary>
                        <div class="advanced-body">
                            <div class="hardware-config-group" aria-labelledby="hardware_config_title">
                                <strong id="hardware_config_title">Аппаратная конфигурация</strong>
                                <div class="grid-2">
                                    <div>
                                        <label for="hardware_rows">Строки матрицы</label>
                                        <input id="hardware_rows" type="number" min="1" max="500" step="1" inputmode="numeric">
                                    </div>
                                    <div>
                                        <label for="hardware_cols">Столбцы матрицы</label>
                                        <input id="hardware_cols" type="number" min="1" max="500" step="1" inputmode="numeric">
                                    </div>
                                </div>
                                <div class="grid-2">
                                    <div>
                                        <label for="hardware_active_ports">Активные порты</label>
                                        <select id="hardware_active_ports">
                                            <option value="1">1</option>
                                            <option value="2">2</option>
                                            <option value="3">3</option>
                                            <option value="4">4</option>
                                        </select>
                                    </div>
                                    <div>
                                        <label for="hardware_protocol">Тип светодиодов</label>
                                        <select id="hardware_protocol">
                                            <option value="ws2811_800k">WS2811</option>
                                        </select>
                                    </div>
                                </div>
                                <div>
                                    <label for="hardware_color_order">Порядок цветов</label>
                                    <select id="hardware_color_order">
                                        <option value="RGB">RGB</option>
                                        <option value="RBG">RBG</option>
                                        <option value="GRB">GRB</option>
                                        <option value="GBR">GBR</option>
                                        <option value="BRG">BRG</option>
                                        <option value="BGR">BGR</option>
                                    </select>
                                </div>
                                <div class="notice">Указывайте фактический размер подключённой матрицы. Это не обрезка изображения. Чтобы отключить часть матрицы, оставьте физический размер неизменным и назначьте ненужным пикселям «Без зоны».</div>
                                <div class="hint">Изменение строк, столбцов или числа активных портов сбрасывает разметку зон и семь пространственных слоёв. Тип светодиодов и порядок цветов применяются после перезагрузки без очистки этих данных.</div>
                                <div class="btn-row">
                                    <button class="btn primary" id="save_hardware_config_btn" type="button">Сохранить аппаратные настройки</button>
                                </div>
                                <div class="status-line" id="hardware_config_message" aria-live="polite"></div>
                            </div>
                            <div class="section-divider"></div>
                            <div>
                                <label for="topology_select">Схема физического соединения светодиодов</label>
                                <select id="topology_select"></select>
                            </div>
                            <div class="status-line" id="topology_summary"></div>
                            <div class="notice">Выберите вариант, соответствующий реальному соединению светодиодов. Эта настройка не поворачивает рисунок. Неверный вариант может зеркально отобразить или перемешать зоны на физической матрице.</div>
                            <div class="topology-preview" id="topology_preview" aria-label="Порядок светодиодов"></div>
                            <div class="btn-row">
                                <button class="btn primary" id="save_topology_btn">Сохранить топологию</button>
                            </div>
                            <div class="hint" id="topology_message"></div>
                        </div>
                    </details>
                    <div class="btn-row">
                        <button class="btn primary" id="save_zones_btn">Сохранить зоны</button>
                        <button class="btn danger" id="clear_zone_btn">Очистить выбранную зону</button>
                        <button class="btn danger" id="clear_all_zones_btn" type="button">Очистить всю разметку</button>
                    </div>
                    <div class="hint">Сохраняет только разметку пикселей по зонам.</div>
                </div>
            </section>

            <section class="panel tab-panel" id="tab_status">
                <div class="panel-title"><h2>Слои статусов</h2><span class="dirty-indicator field-hidden" id="status_dirty_indicator">не сохранено</span></div>
                <div class="section">
                    <div class="btn-row">
                        <button class="btn active" data-status="waiting">Ожидание</button>
                        <button class="btn" data-status="charging">Зарядка</button>
                        <button class="btn" data-status="error">Ошибка</button>
                    </div>
                    <div class="grid-2">
                        <div>
                            <label for="status_scope_select">Область</label>
                            <select id="status_scope_select"></select>
                        </div>
                        <div>
                            <label for="status_mode_select">Способ настройки</label>
                            <select id="status_mode_select">
                                <option value="solid">Сплошной цвет</option>
                                <option value="pixels" selected>Пиксельный рисунок</option>
                            </select>
                        </div>
                    </div>
                    <div class="grid-2">
                        <div>
                            <label for="status_color" id="status_color_label">Цвет кисти</label>
                            <input type="color" id="status_color" value="#00ff00">
                        </div>
                        <div class="notice" id="status_mode_notice"></div>
                    </div>
                    <div class="status-line" id="status_editor_context"></div>
                    <div class="btn-row">
                        <button class="btn primary field-hidden" id="apply_status_solid_btn" type="button">Залить выбранную область</button>
                        <button class="btn danger" id="clear_status_btn" type="button">Сбросить ожидание всех портов</button>
                    </div>
                    <div>
                        <label for="ports_brightness">Общая яркость портовых зон</label>
                        <div class="brightness-control">
                            <input type="range" id="ports_brightness" min="0" max="255" value="120">
                            <input type="number" id="ports_brightness_input" min="0" max="255" value="120" inputmode="numeric" aria-label="Яркость портов числом">
                        </div>
                        <div class="hint">Яркость применяется автоматически после изменения. Значение 0 полностью выключает индикацию портов, включая ошибку.</div>
                    </div>
                    <div class="btn-row">
                        <button class="btn primary" id="save_status_btn">Сохранить слой статуса</button>
                    </div>
                    <div class="hint">Сохраняется полный координатный слой только выбранного статуса. После сохранения пиксели без собственного цвета используют заводской базовый цвет.</div>
                </div>
            </section>

            <section class="panel tab-panel" id="tab_free">
                <div class="panel-title"><h2>Настройка зоны</h2><span class="dirty-indicator field-hidden" id="free_dirty_indicator">не сохранено</span></div>
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
                            <div class="brightness-control">
                                <input type="range" id="free_brightness" min="0" max="255" value="100">
                                <input type="number" id="free_brightness_input" min="0" max="255" value="100" inputmode="numeric" aria-label="Яркость свободной зоны числом">
                            </div>
                            <div class="hint">Яркость применяется автоматически после изменения. Значение 0 выключает зону, не удаляя её режим и рисунок.</div>
                        </div>
                    </div>
                    <div class="btn-row">
                        <button class="btn secondary" id="apply_free_brightness_all_btn" type="button">Применить яркость ко всем зонам</button>
                    </div>
                    <div class="hint">Сразу сохраняет яркость всех размеченных свободных зон.</div>
                    <div class="section-divider"></div>
                    <label>Легенда</label>
                    <div class="legend" id="free_legend"></div>
                    <div class="btn-row">
                        <button class="btn danger" id="free_erase_btn">Очистить рисунок</button>
                        <button class="btn primary" id="apply_free_btn">Сохранить настройки зоны</button>
                    </div>
                    <div class="hint">Сохраняет режим, цвет, яркость и рисунок выбранной свободной зоны.</div>
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
                <button class="tab-btn active" data-service="network" type="button">Сеть</button>
                <button class="tab-btn" data-service="security" type="button">Безопасность</button>
                <button class="tab-btn" data-service="firmware" type="button">Прошивка</button>
                <button class="tab-btn" data-service="diagnostics" type="button">Диагностика</button>
            </div>

            <section class="service-section" id="service_diagnostics" aria-label="Диагностика">
                <div class="section">
                    <div class="btn-row service-action-row">
                        <button class="btn" id="refresh_diag_btn" type="button">Обновить данные</button>
                    </div>
                    <div class="hint" id="diagnostics_source" aria-live="polite"></div>
                    <div class="hint" id="diagnostics_message" aria-live="polite"></div>
                    <div>
                        <label>Прошивка</label>
                        <div class="diag-grid" id="diag_firmware"></div>
                    </div>
                    <div class="section-divider"></div>
                    <div>
                        <label>Система</label>
                        <div class="diag-grid" id="diag_system"></div>
                    </div>
                    <div class="section-divider"></div>
                    <div>
                        <label>Фоновые процессы</label>
                        <div class="diag-grid" id="diag_tasks"></div>
                    </div>
                    <div class="section-divider"></div>
                    <div>
                        <label>Сеть</label>
                        <div class="diag-grid" id="diag_network_runtime"></div>
                    </div>
                    <div class="section-divider"></div>
                    <div>
                        <label>Общее</label>
                        <div class="diag-grid" id="diag_summary"></div>
                    </div>
                    <div class="section-divider"></div>
                    <div>
                        <label>Входы Data1…Data8</label>
                        <div class="diag-grid" id="diag_inputs"></div>
                    </div>
                    <div class="section-divider"></div>
                    <div>
                        <label>Порты</label>
                        <div class="diag-grid" id="diag_ports"></div>
                    </div>
                    <div class="section-divider"></div>
                    <div>
                        <label>Выходы матрицы</label>
                        <div class="diag-grid" id="diag_leds"></div>
                    </div>
                </div>
            </section>

            <section class="service-section active" id="service_network" aria-label="Сеть">
                <div class="section">
                    <div class="btn-row service-action-row">
                        <button class="btn" id="refresh_network_btn" type="button">Обновить статус сети</button>
                    </div>
                    <div class="diag-grid" id="network_summary"></div>
                    <div class="section-divider"></div>
                    <form id="network_form" class="section">
                        <div>
                            <label for="network_ssid">SSID</label>
                            <input id="network_ssid" name="ssid" type="text" autocomplete="off">
                        </div>
                        <div>
                            <label for="network_password">Пароль Wi-Fi</label>
                            <input id="network_password" name="password" type="password" autocomplete="new-password" placeholder="Введите пароль сети. Для уже сохранённого SSID поле можно оставить пустым">
                        </div>
                        <label class="row" style="justify-content:flex-start;">
                            <input id="network_dhcp" name="dhcp" type="checkbox" checked>
                            <span>DHCP</span>
                        </label>
                        <div class="grid-2" id="network_static_fields">
                            <div>
                                <label for="network_static_ip">Статический IP-адрес</label>
                                <input id="network_static_ip" name="static_ip" type="text" inputmode="numeric">
                            </div>
                            <div>
                                <label for="network_gateway">Шлюз</label>
                                <input id="network_gateway" name="gateway" type="text" inputmode="numeric">
                            </div>
                            <div>
                                <label for="network_subnet">Маска подсети</label>
                                <input id="network_subnet" name="subnet" type="text" inputmode="numeric">
                            </div>
                            <div>
                                <label for="network_dns">DNS</label>
                                <input id="network_dns" name="dns" type="text" inputmode="numeric">
                            </div>
                        </div>
                        <div class="grid-2">
                            <div>
                                <label for="network_ap_ssid">SSID резервной точки доступа</label>
                                <input id="network_ap_ssid" name="ap_ssid" type="text">
                            </div>
                            <div>
                                <label for="network_ap_password">Пароль резервной точки доступа</label>
                                <input id="network_ap_password" name="ap_password" type="password" autocomplete="new-password">
                                <div class="hint">Оставьте пустым, чтобы сохранить текущий пароль. Новый пароль: от 8 до 64 символов.</div>
                            </div>
                        </div>
                        <div class="notice safety-warning field-hidden" id="network_ap_password_warning">
                            Используется стандартный пароль резервной точки доступа. Измените его перед установкой контроллера.
                        </div>
                        <div class="btn-row">
                            <button class="btn" id="scan_network_btn" type="button">Сканировать сети</button>
                            <button class="btn primary" id="network_save_connect_btn" type="submit">Сохранить и подключиться</button>
                        </div>
                    </form>
                    <div class="diag-grid" id="network_scan"></div>
                    <div class="hint" id="network_message" aria-live="polite"></div>
                </div>
            </section>

            <section class="service-section" id="service_firmware" aria-label="Прошивка">
                <div class="section">
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
                    <div class="hint" id="firmware_message">Выберите файл прошивки firmware.bin.</div>
                </div>
            </section>

            <section class="service-section" id="service_security" aria-label="Безопасность">
                <div class="section">
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
    { id: 0, name: 'Без зоны', type: 'off', enabled: true, reserved: false, linked_port: null, mode: null },
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

const FACTORY_STATUS_COLORS = {
    waiting: '#00ff00',
    charging: '#0050ff',
    error: '#ff0000',
};
let statusDefaults = { ...FACTORY_STATUS_COLORS };
let statusBrushColors = { ...FACTORY_STATUS_COLORS };

const STATUS_LABELS = {
    waiting: 'ожидание',
    charging: 'зарядка',
    error: 'ошибка',
    disabled: 'отключён',
    unknown: 'неизвестно',
};

const STATUS_RESET_LABELS = {
    waiting: 'ожидание',
    charging: 'зарядку',
    error: 'ошибку',
};

const STATUS_EDIT_MODE_LABELS = {
    solid: 'сплошной цвет',
    pixels: 'пиксельный рисунок',
};

const TYPE_LABELS = {
    off: 'без зоны',
    port: 'порт',
    free: 'свободная',
};

const MODE_LABELS = {
    static: 'статичный цвет',
    custom: 'пиксельный рисунок',
    rainbow: 'радуга',
};

const ZONE_LABELS = {
    '0': 'Без зоны',
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
const FREE_ZONE_IDS = ['5', '6', '7', '8'];

const TOPOLOGY_OPTIONS = [
    { value: 0, key: 'vert_down', label: 'Вертикальная · старт справа снизу', hint: 'Светодиод 0 справа снизу, далее змейкой по столбцам.' },
    { value: 1, key: 'vert_up', label: 'Вертикальная · старт слева снизу', hint: 'Светодиод 0 слева снизу, далее змейкой по столбцам.' },
    { value: 2, key: 'horiz_right', label: 'Горизонтальная · старт слева сверху', hint: 'Светодиод 0 слева сверху, далее змейкой по строкам.' },
    { value: 3, key: 'horiz_left', label: 'Горизонтальная · старт справа сверху', hint: 'Светодиод 0 справа сверху, далее змейкой по строкам.' },
];
const RUNTIME_STALE_WARNING_MS = 500;

let COLS = 12;
let ROWS = 8;
let activeTab = 'zones';
let activeServiceSection = 'network';
let activeZoneId = '1';
let activeStatus = 'waiting';
let activeStatusScope = 'all';
let statusEditMode = 'pixels';
let activeFreeZone = '5';
let zoneTool = 'brush';
let zoneViewMode = 'edit';
let freeViewMode = 'edit';
let drawing = false;
let hardwareZonesMap = {};
let savedHardwareZonesMap = {};
let zoneMeta = [...DEFAULT_ZONES];
let statusColorLayers = { waiting: {}, charging: {}, error: {} };
let freeModes = { '5': 'static', '6': 'static', '7': 'static', '8': 'custom' };
let freeCustomLayers = { '5': {}, '6': {}, '7': {}, '8': {} };
let freeStaticColors = { ...DEFAULT_FREE_STATIC_COLORS };
let freeBrushColors = { ...DEFAULT_FREE_STATIC_COLORS };
let freeBrightness = { '5': 100, '6': 100, '7': 100, '8': 100 };
let appliedFreeZones = {};
let diagnostics = null;
let networkStatus = null;
let rainbowTick = 0;
let backendAvailable = false;
let mockMode = false;
let serviceOpen = false;
let diagnosticsRequestInFlight = false;
let configLoadInProgress = false;
let matrixConfig = { cols: 12, rows: 8, total: 96, topology: 0 };
let pendingTopology = 0;
let activePortCount = 2;
let hardwareConfigApplied = {
    rows: 8,
    cols: 12,
    activePortCount: 2,
    ledProtocol: 'ws2811_800k',
    colorOrder: 'BRG',
    maxLedCount: 500,
};
let hardwareConfigPending = { ...hardwareConfigApplied };
let hardwareConfigInitialized = false;
let hardwareConfigSaveInProgress = false;
let hardwareRestartInProgress = false;
let hardwareRestartPollToken = 0;
let topologySaveInProgress = false;
let uncertainTopologySubmission = null;
let zoneMapDirty = false;
let zoneMapSaveInProgress = false;
let uncertainZoneMapSubmission = null;
let statusSaveInProgress = false;
let freeZoneSaveInProgress = false;
let freeBrightnessBatchInProgress = false;
let statusLayerDirty = { waiting: false, charging: false, error: false };
let freeZoneDirty = { '5': false, '6': false, '7': false, '8': false };
let uncertainFreeZoneState = { '5': false, '6': false, '7': false, '8': false };
let portsBrightnessValue = 120;
let portsBrightnessDirty = false;
let portsBrightnessRevision = 0;
let portsBrightnessSaveTimer = null;
let portsBrightnessSaveInProgress = 0;
let freeBrightnessDirty = { '5': false, '6': false, '7': false, '8': false };
let freeBrightnessRevisions = { '5': 0, '6': 0, '7': 0, '8': 0 };
let freeBrightnessSaveTimers = {};
let freeBrightnessSaveInProgress = {};
let networkScanInFlight = false;
let networkSaveInFlight = false;
let networkFormDirty = false;
let networkFormRevision = 0;
let networkStatusRequestRevision = 0;
let selectedScannedNetwork = null;

const BRIGHTNESS_SAVE_DEBOUNCE_MS = 500;
const TOPOLOGY_NUMBER_PREVIEW_MAX_PIXELS = 256;
const HARDWARE_MAX_LED_COUNT_FALLBACK = 500;
const HARDWARE_SAVE_REQUEST_TIMEOUT_MS = 10000;
const MATRIX_SAVE_REQUEST_TIMEOUT_MS = 10000;
const HARDWARE_RESTART_POLL_MS = 1000;
const HARDWARE_RESTART_TIMEOUT_MS = 60000;
const HARDWARE_RESTART_REQUEST_TIMEOUT_MS = 4000;
const HARDWARE_SUPPORTED_PROTOCOL = 'ws2811_800k';
const HARDWARE_COLOR_ORDERS = ['RGB', 'RBG', 'GRB', 'GBR', 'BRG', 'BGR'];
const NETWORK_SCAN_POLL_MS = 500;
const NETWORK_SCAN_UI_TIMEOUT_MS = 14000;

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
    const activeLedCount = cols * rows;
    if (!Number.isInteger(activeLedCount) || activeLedCount <= 0) return map;
    for (let y = 0; y < rows; y++) {
        for (let x = 0; x < cols; x++) {
            const logicalIndex = y * cols + x;
            const portOffset = Math.floor((logicalIndex * activeCount) / activeLedCount);
            map[`${x}-${y}`] = String(1 + portOffset);
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
        b_ports: 120,
        c_wait: FACTORY_STATUS_COLORS.waiting,
        c_charge: FACTORY_STATUS_COLORS.charging,
        c_error: FACTORY_STATUS_COLORS.error,
        activePortCount: mockActivePortCount,
        matrix: { cols: 12, rows: 8, total: 96, topology: 0 },
        hardware: {
            rows: 8,
            cols: 12,
            activePortCount: mockActivePortCount,
            ledProtocol: HARDWARE_SUPPORTED_PROTOCOL,
            colorOrder: 'BRG',
            maxLedCount: HARDWARE_MAX_LED_COUNT_FALLBACK,
        },
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
        firmware: {
            version: 'локальный предпросмотр',
            build: 'нет данных',
            environment: 'браузер',
        },
        system: {
            uptimeSec: null,
            freeHeap: null,
            minFreeHeap: null,
            resetReason: 'unknown',
        },
        inputRuntime: {
            taskStarted: false,
            loopFallback: false,
            lastUpdateAgeMs: null,
        },
        ledRuntime: {
            mutexReady: false,
            taskStarted: false,
            loopFallback: false,
            lastRefreshAgeMs: null,
        },
        networkRuntime: {
            apStartState: 'not_attempted',
        },
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
        passwordConfigured: false,
        connectedSsid: '',
        ip: '',
        staMac: 'A4:CF:12:34:56:78',
        rssi: 0,
        dhcp: true,
        staticIp: '192.168.1.150',
        gateway: '192.168.1.1',
        subnet: '255.255.255.0',
        dns: '8.8.8.8',
        apSsid: 'LED_MATRIX_A1B2C3',
        apIp: '192.168.4.1',
        apClients: 1,
        defaultApPassword: false,
    };
}

function networkStatusLabel(status) {
    const labels = {
        connected: 'Подключено к роутеру',
        connecting: 'Подключение к роутеру',
        reconnecting: 'Повторное подключение к роутеру',
        not_configured: 'Сеть роутера не выбрана',
        connect_failed: 'Не удалось подключиться к роутеру; включён режим резервной точки доступа',
        connection_lost: 'Связь с роутером потеряна; включён режим резервной точки доступа',
    };
    return labels[status] || 'Неизвестное состояние';
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

function isActivePortZoneId(zoneId) {
    const portNumber = Number(zoneId);
    return Number.isInteger(portNumber) && portNumber >= 1 && portNumber <= activePortCount;
}

function statusScopeMatchesZone(zoneId, scope = activeStatusScope) {
    if (!isActivePortZoneId(zoneId)) return false;
    return scope === 'all' || String(zoneId) === String(scope);
}

function sanitizeStatusLayerForPortZones(layer) {
    const clean = sanitizeLayerForSize(layer);
    Object.keys(clean).forEach(key => {
        if (!isActivePortZoneId(hardwareZonesMap[key] ?? '0')) delete clean[key];
    });
    return clean;
}

function clearLocalStatusPixelsForChangedZones(oldMap, newMap) {
    for (let y = 0; y < ROWS; y++) {
        for (let x = 0; x < COLS; x++) {
            const key = `${x}-${y}`;
            if (String(oldMap?.[key] ?? '0') === String(newMap?.[key] ?? '0')) continue;
            delete statusColorLayers.waiting[key];
            delete statusColorLayers.charging[key];
            delete statusColorLayers.error[key];
        }
    }
}

function markStatusLayerDirty(status = activeStatus) {
    statusDefaults[status] = FACTORY_STATUS_COLORS[status];
    statusLayerDirty[status] = true;
}

function statusScopeLabel(scope = activeStatusScope) {
    return scope === 'all' ? 'все активные порты' : `Порт ${scope}`;
}

function zoneMapsEqual(left, right) {
    for (let y = 0; y < ROWS; y++) {
        for (let x = 0; x < COLS; x++) {
            const key = `${x}-${y}`;
            if (String(left?.[key] ?? '0') !== String(right?.[key] ?? '0')) return false;
        }
    }
    return true;
}

function statusEditModeLabel(mode = statusEditMode) {
    return STATUS_EDIT_MODE_LABELS[mode] || mode;
}

function sanitizeFreeLayerForZone(layer, zoneId) {
    const clean = sanitizeLayerForSize(layer);
    Object.keys(clean).forEach(key => {
        if (String(hardwareZonesMap[key] ?? '0') !== String(zoneId)) delete clean[key];
    });
    return clean;
}

function defaultFreeZoneState(zoneId) {
    const id = String(zoneId);
    return {
        mode: id === '8' ? 'custom' : 'static',
        staticColor: DEFAULT_FREE_STATIC_COLORS[id] || '#ffffff',
        brightness: 100,
        customLayer: {},
    };
}

function normalizeFreeZoneState(zoneId, source = {}) {
    const id = String(zoneId);
    const fallback = defaultFreeZoneState(id);
    const mode = ['static', 'custom', 'rainbow'].includes(source.mode)
        ? source.mode
        : fallback.mode;
    return {
        mode,
        staticColor: configHexColor(source.staticColor, fallback.staticColor),
        brightness: clampBrightnessValue(source.brightness, fallback.brightness),
        customLayer: sanitizeFreeLayerForZone(source.customLayer || {}, id),
    };
}

function appliedFreeZoneState(zoneId) {
    const id = String(zoneId);
    if (!appliedFreeZones[id]) {
        appliedFreeZones[id] = normalizeFreeZoneState(id);
    }
    return appliedFreeZones[id];
}

function captureFreeZoneDraft(zoneId) {
    const id = String(zoneId);
    return normalizeFreeZoneState(id, {
        mode: freeModes[id],
        staticColor: freeStaticColors[id],
        brightness: freeBrightness[id],
        customLayer: freeCustomLayers[id],
    });
}

function setAppliedFreeZoneState(zoneId, state) {
    const id = String(zoneId);
    const normalized = normalizeFreeZoneState(id, state);
    appliedFreeZones[id] = {
        ...normalized,
        customLayer: { ...normalized.customLayer },
    };
}

function loadFreeZoneDraftFromApplied(zoneId) {
    const id = String(zoneId);
    const applied = appliedFreeZoneState(id);
    freeModes[id] = applied.mode;
    freeStaticColors[id] = applied.staticColor;
    freeBrightness[id] = applied.brightness;
    freeCustomLayers[id] = { ...applied.customLayer };
    freeBrushColors[id] = Object.values(applied.customLayer)[0] || applied.staticColor;
}

function freeZoneDraftHasUnsavedChanges(zoneId = activeFreeZone) {
    const id = String(zoneId);
    return Boolean(
        freeZoneDirty[id] ||
        freeBrightnessDirty[id] ||
        uncertainFreeZoneState[id] ||
        freeBrightnessSaveTimers[id] !== undefined
    );
}

function discardFreeZoneDraft(zoneId) {
    const id = String(zoneId);
    cancelFreeZoneBrightnessSave(id);
    freeBrightnessRevisions[id] += 1;
    freeZoneDirty[id] = false;
    freeBrightnessDirty[id] = false;
    loadFreeZoneDraftFromApplied(id);
}

function freeCustomLayersLosePixelsWithCurrentZoneMap() {
    return FREE_ZONE_IDS.some(zoneId => {
        const appliedLayer = appliedFreeZoneState(zoneId).customLayer;
        const draftLayer = freeCustomLayers[zoneId] || {};
        return [appliedLayer, draftLayer].some(layer =>
            Object.keys(layer).some(key =>
                String(hardwareZonesMap[key] ?? '0') !== zoneId)
        );
    });
}

function clampBrightnessValue(value, fallback = 100) {
    if (value === null || value === undefined || value === '') return fallback;
    const parsed = Number(value);
    if (!Number.isFinite(parsed)) return fallback;
    return Math.max(0, Math.min(255, Math.round(parsed)));
}

function configHexColor(value, fallback) {
    return typeof value === 'string' && /^#[0-9a-fA-F]{6}$/.test(value) ? value : fallback;
}

function setBrightnessPair(rangeId, inputId, value) {
    const safe = clampBrightnessValue(value);
    const range = document.getElementById(rangeId);
    const input = document.getElementById(inputId);
    if (range) range.value = safe;
    if (input) input.value = safe;
    return safe;
}

function syncFreeColorInput() {
    const mode = freeModes[activeFreeZone] || 'static';
    const color = mode === 'custom'
        ? (freeBrushColors[activeFreeZone] || '#ffffff')
        : (freeStaticColors[activeFreeZone] || '#ffffff');
    document.getElementById('free_color').value = color;
    setBrightnessPair(
        'free_brightness',
        'free_brightness_input',
        freeBrightness[activeFreeZone] ?? 100
    );
}

function updateFreeModeControls() {
    const mode = freeModes[activeFreeZone] || 'static';
    const mapped = activeFreeZoneIsMapped();
    const savesBlocked = freeZoneEditorBlocked();
    const colorField = document.getElementById('free_color_field');
    const colorLabel = document.getElementById('free_color_label');
    const clearBtn = document.getElementById('free_erase_btn');
    const modeSelect = document.getElementById('free_mode_select');
    const colorInput = document.getElementById('free_color');
    const brightnessInput = document.getElementById('free_brightness');
    const brightnessNumber = document.getElementById('free_brightness_input');
    const applyBtn = document.getElementById('apply_free_btn');
    const applyAllBtn = document.getElementById('apply_free_brightness_all_btn');
    const notice = document.getElementById('free_zone_notice');

    colorField.classList.toggle('field-hidden', mode === 'rainbow');
    clearBtn.classList.toggle('field-hidden', mode !== 'custom' || !mapped);
    colorLabel.textContent = mode === 'custom' ? 'Цвет кисти' : 'Цвет зоны';
    modeSelect.disabled = !mapped || savesBlocked;
    colorInput.disabled = !mapped || mode === 'rainbow' || savesBlocked;
    brightnessInput.disabled = !mapped || savesBlocked;
    brightnessNumber.disabled = !mapped || savesBlocked;
    applyBtn.disabled = !mapped || savesBlocked;
    applyAllBtn.disabled = getMappedFreeZoneIds().length === 0 || savesBlocked;
    notice.classList.toggle('field-hidden', mapped);
    notice.textContent = mapped
        ? ''
        : `${zoneDisplayName(activeFreeZone)} не размечена. Сначала назначьте этой зоне пиксели во вкладке «Зоны матрицы».`;
}

function updateZoneViewControls() {
    const isOverview = zoneViewMode === 'overview';
    const savesBlocked = freeZoneEditorBlocked();
    document.getElementById('zone_edit_mode_btn').classList.toggle('active', !isOverview);
    document.getElementById('zone_overview_mode_btn').classList.toggle('active', isOverview);
    document.getElementById('zone_tools_row').classList.toggle('field-hidden', isOverview);
    document.getElementById('clear_zone_btn').disabled = isOverview || savesBlocked;
    const baseText = isOverview
        ? 'Обзор: все зоны показаны своими цветами, рисование отключено.'
        : 'Рисуйте внутри выбранной зоны. Чужие зоны заблокированы.';
    document.getElementById('zones_mode_notice').textContent = baseText;
}

function updateFreeViewControls() {
    const mapped = activeFreeZoneIsMapped();
    const savesBlocked = freeZoneEditorBlocked();
    if (!mapped && freeViewMode === 'edit') {
        freeViewMode = 'overview';
    }
    const isOverview = freeViewMode === 'overview';
    document.getElementById('free_edit_mode_btn').classList.toggle('active', !isOverview);
    document.getElementById('free_overview_mode_btn').classList.toggle('active', isOverview);
    document.getElementById('free_edit_mode_btn').disabled = !mapped;
    document.getElementById('free_erase_btn').disabled = isOverview || !mapped || savesBlocked;
}

function positiveHardwareInteger(value, fallback) {
    const parsed = Number(value);
    return Number.isInteger(parsed) && parsed > 0 ? parsed : fallback;
}

function hardwareConfigFromResponse(cfg) {
    const matrix = cfg?.matrix || {};
    const hardware = cfg?.hardware || {};
    const fallbackRows = positiveHardwareInteger(matrix.rows, 8);
    const fallbackCols = positiveHardwareInteger(matrix.cols, 12);
    const fallbackPorts = clampActivePortCount(cfg?.activePortCount ?? cfg?.active_port_count ?? 2);
    const protocol = hardware.ledProtocol === undefined
        ? HARDWARE_SUPPORTED_PROTOCOL
        : String(hardware.ledProtocol);
    const order = hardware.colorOrder === undefined ? 'BRG' : String(hardware.colorOrder);

    return {
        rows: positiveHardwareInteger(hardware.rows, fallbackRows),
        cols: positiveHardwareInteger(hardware.cols, fallbackCols),
        activePortCount: clampActivePortCount(hardware.activePortCount ?? fallbackPorts),
        ledProtocol: protocol,
        colorOrder: order,
        maxLedCount: positiveHardwareInteger(hardware.maxLedCount, HARDWARE_MAX_LED_COUNT_FALLBACK),
    };
}

function hardwareConfigsEqual(left, right) {
    return Number(left?.rows) === Number(right?.rows) &&
        Number(left?.cols) === Number(right?.cols) &&
        Number(left?.activePortCount) === Number(right?.activePortCount) &&
        String(left?.ledProtocol) === String(right?.ledProtocol) &&
        String(left?.colorOrder) === String(right?.colorOrder);
}

function captureHardwareConfigPending() {
    hardwareConfigPending = {
        rows: document.getElementById('hardware_rows').value,
        cols: document.getElementById('hardware_cols').value,
        activePortCount: document.getElementById('hardware_active_ports').value,
        ledProtocol: document.getElementById('hardware_protocol').value,
        colorOrder: document.getElementById('hardware_color_order').value,
        maxLedCount: hardwareConfigApplied.maxLedCount,
    };
    return hardwareConfigPending;
}

function syncHardwareConfigForm() {
    hardwareConfigPending = { ...hardwareConfigApplied };
    document.getElementById('hardware_rows').value = String(hardwareConfigApplied.rows);
    document.getElementById('hardware_cols').value = String(hardwareConfigApplied.cols);
    document.getElementById('hardware_active_ports').value = String(hardwareConfigApplied.activePortCount);
    document.getElementById('hardware_protocol').value = hardwareConfigApplied.ledProtocol;
    document.getElementById('hardware_color_order').value = hardwareConfigApplied.colorOrder;
}

function parseHardwareFormInteger(value) {
    const text = String(value ?? '').trim();
    if (!/^\d+$/.test(text)) return null;
    const parsed = Number(text);
    return Number.isSafeInteger(parsed) ? parsed : null;
}

function validateHardwareConfigForm(pending = hardwareConfigPending) {
    const rows = parseHardwareFormInteger(pending.rows);
    const cols = parseHardwareFormInteger(pending.cols);
    const ports = parseHardwareFormInteger(pending.activePortCount);
    const maxLedCount = positiveHardwareInteger(
        hardwareConfigApplied.maxLedCount,
        HARDWARE_MAX_LED_COUNT_FALLBACK
    );

    if (rows === null || cols === null || rows < 1 || cols < 1) {
        return { error: 'Строки и столбцы должны быть целыми числами больше нуля.' };
    }
    if (rows > HARDWARE_MAX_LED_COUNT_FALLBACK || cols > HARDWARE_MAX_LED_COUNT_FALLBACK) {
        return { error: `Строки и столбцы не должны превышать ${HARDWARE_MAX_LED_COUNT_FALLBACK}.` };
    }
    const total = rows * cols;
    if (total > maxLedCount) {
        return { error: `Матрица содержит ${total} пикселей; допустимый предел — ${maxLedCount}.` };
    }
    if (ports === null || ports < 1 || ports > 4) {
        return { error: 'Количество активных портов должно быть от 1 до 4.' };
    }
    if (total < ports) {
        return { error: 'Количество пикселей не может быть меньше количества активных портов.' };
    }
    if (pending.ledProtocol !== HARDWARE_SUPPORTED_PROTOCOL) {
        return { error: 'Выбран неподдерживаемый тип светодиодов.' };
    }
    if (!HARDWARE_COLOR_ORDERS.includes(String(pending.colorOrder))) {
        return { error: 'Выбран неподдерживаемый порядок цветов.' };
    }

    return {
        config: {
            rows,
            cols,
            activePortCount: ports,
            ledProtocol: HARDWARE_SUPPORTED_PROTOCOL,
            colorOrder: String(pending.colorOrder),
            maxLedCount,
        },
    };
}

function hardwareConfigFormHasChanges() {
    const result = validateHardwareConfigForm(hardwareConfigPending);
    return !result.config || !hardwareConfigsEqual(result.config, hardwareConfigApplied);
}

function setHardwareConfigMessage(text, isError = false) {
    const line = document.getElementById('hardware_config_message');
    line.textContent = text || '';
    line.classList.toggle('warning', Boolean(text) && isError);
}

function updateStatusEditorControls() {
    const blocked = freeZoneEditorBlocked();
    const solidMode = statusEditMode === 'solid';
    const scopeSelect = document.getElementById('status_scope_select');
    const modeSelect = document.getElementById('status_mode_select');
    const colorInput = document.getElementById('status_color');
    const applyButton = document.getElementById('apply_status_solid_btn');
    const clearButton = document.getElementById('clear_status_btn');
    const context = document.getElementById('status_editor_context');
    const notice = document.getElementById('status_mode_notice');

    scopeSelect.disabled = blocked;
    modeSelect.disabled = blocked;
    colorInput.disabled = blocked;
    applyButton.disabled = blocked;
    clearButton.disabled = blocked;
    applyButton.classList.toggle('field-hidden', !solidMode);
    clearButton.classList.toggle('field-hidden', solidMode);
    document.getElementById('status_color_label').textContent =
        solidMode ? 'Цвет заливки' : 'Цвет кисти';
    clearButton.textContent = `Сбросить ${STATUS_RESET_LABELS[activeStatus]} ${
        activeStatusScope === 'all' ? 'всех активных портов' : `Порта ${activeStatusScope}`
    }`;
    notice.textContent = solidMode
        ? `Кнопка заливки изменит координаты области «${statusScopeLabel()}» только в выбранном статусе.`
        : `Рисование разрешено только в области «${statusScopeLabel()}». Остальные пиксели портов заблокированы.`;
    context.textContent = `${statusLabel(activeStatus)} · ${statusScopeLabel()} · ${
        statusEditModeLabel()
    } · ${statusLayerDirty[activeStatus] ? 'есть несохранённые изменения' : 'изменения сохранены'}`;
    document.querySelectorAll('[data-status]').forEach(button => {
        button.disabled = blocked;
    });
}

function matrixSaveOperationInProgress() {
    return hardwareConfigSaveInProgress || hardwareRestartInProgress ||
        topologySaveInProgress || zoneMapSaveInProgress ||
        statusSaveInProgress || freeZoneSaveInProgress;
}

function matrixSavesBlockedByHardwareTransition() {
    return matrixSaveOperationInProgress() || uncertainTopologySubmission !== null;
}

function freeZoneEditorBlocked() {
    return matrixSavesBlockedByHardwareTransition() || freeBrightnessBatchInProgress;
}

function layerMapsEqual(left = {}, right = {}) {
    const leftKeys = Object.keys(left);
    const rightKeys = Object.keys(right);
    return leftKeys.length === rightKeys.length && leftKeys.every(key =>
        String(left[key]).toLowerCase() === String(right[key]).toLowerCase()
    );
}

function hasUnsavedFreeZoneScalarDrafts() {
    return FREE_ZONE_IDS.some(zoneId => {
        const draft = captureFreeZoneDraft(zoneId);
        const applied = appliedFreeZoneState(zoneId);
        return draft.mode !== applied.mode ||
            draft.staticColor.toLowerCase() !== applied.staticColor.toLowerCase() ||
            Number(draft.brightness) !== Number(applied.brightness);
    });
}

function hasUnsavedSpatialDrafts() {
    return zoneMapDirty ||
        Object.values(statusLayerDirty).some(Boolean) ||
        FREE_ZONE_IDS.some(zoneId =>
            !layerMapsEqual(captureFreeZoneDraft(zoneId).customLayer, appliedFreeZoneState(zoneId).customLayer)
        );
}

function brightnessSavePendingOrInProgress() {
    return portsBrightnessSaveTimer !== null ||
        portsBrightnessSaveInProgress > 0 ||
        Object.keys(freeBrightnessSaveTimers).length > 0 ||
        Object.keys(freeBrightnessSaveInProgress).length > 0 ||
        freeBrightnessBatchInProgress;
}

function hasUnsavedPreservedSettings() {
    return portsBrightnessDirty ||
        Object.values(freeBrightnessDirty).some(Boolean) ||
        Object.values(uncertainFreeZoneState).some(Boolean) ||
        hasUnsavedFreeZoneScalarDrafts();
}

function hasPendingMatrixChanges(includeTopology = true) {
    return zoneMapDirty ||
        (includeTopology && Number(pendingTopology) !== Number(matrixConfig.topology)) ||
        portsBrightnessDirty ||
        Object.values(statusLayerDirty).some(Boolean) ||
        Object.values(freeZoneDirty).some(Boolean) ||
        Object.values(freeBrightnessDirty).some(Boolean) ||
        Object.values(uncertainFreeZoneState).some(Boolean) ||
        portsBrightnessSaveTimer !== null ||
        Object.keys(freeBrightnessSaveTimers).length > 0;
}

function updateMatrixSaveControls() {
    const blocked = freeZoneEditorBlocked();
    [
        'hardware_rows',
        'hardware_cols',
        'hardware_active_ports',
        'hardware_protocol',
        'hardware_color_order',
        'save_hardware_config_btn',
        'topology_select',
        'save_topology_btn',
        'save_zones_btn',
        'save_status_btn',
        'status_scope_select',
        'status_mode_select',
        'status_color',
        'apply_status_solid_btn',
        'clear_zone_btn',
        'clear_all_zones_btn',
        'clear_status_btn',
        'free_zone_select',
        'free_erase_btn',
        'ports_brightness',
        'ports_brightness_input',
        'free_brightness',
        'free_brightness_input',
        'apply_free_btn',
        'apply_free_brightness_all_btn',
    ].forEach(id => {
        const element = document.getElementById(id);
        if (element) element.disabled = blocked;
    });
    if (uncertainTopologySubmission !== null && !matrixSaveOperationInProgress()) {
        document.getElementById('topology_select').disabled = false;
        document.getElementById('save_topology_btn').disabled = false;
    }
    updateFreeModeControls();
    updateZoneViewControls();
    updateFreeViewControls();
    updateStatusEditorControls();
}

function applyConfig(cfg) {
    const matrix = cfg.matrix || {};
    const preserveHardwarePending = hardwareConfigInitialized && hardwareConfigFormHasChanges();
    hardwareConfigApplied = hardwareConfigFromResponse(cfg);
    COLS = Number(matrix.cols) || COLS || 12;
    ROWS = Number(matrix.rows) || ROWS || 8;
    matrixConfig = {
        cols: COLS,
        rows: ROWS,
        total: Number(matrix.total) || COLS * ROWS,
        topology: Number(matrix.topology ?? 0),
    };
    pendingTopology = Math.max(0, Math.min(3, Number(matrixConfig.topology) || 0));
    uncertainTopologySubmission = null;
    activePortCount = hardwareConfigApplied.activePortCount;
    if (!preserveHardwarePending) syncHardwareConfigForm();
    hardwareConfigInitialized = true;
    updateMatrixSaveControls();

    hardwareZonesMap = sanitizeZoneMapForSize(cfg.hardware_map || {});
    savedHardwareZonesMap = { ...hardwareZonesMap };
    uncertainZoneMapSubmission = null;
    if (portsBrightnessSaveTimer !== null) clearTimeout(portsBrightnessSaveTimer);
    portsBrightnessSaveTimer = null;
    Object.values(freeBrightnessSaveTimers).forEach(timer => clearTimeout(timer));
    freeBrightnessSaveTimers = {};
    portsBrightnessRevision += 1;
    Object.keys(freeBrightnessRevisions).forEach(zoneId => { freeBrightnessRevisions[zoneId] += 1; });
    zoneMapDirty = false;
    statusLayerDirty = { waiting: false, charging: false, error: false };
    freeZoneDirty = { '5': false, '6': false, '7': false, '8': false };
    uncertainFreeZoneState = { '5': false, '6': false, '7': false, '8': false };
    portsBrightnessDirty = false;
    freeBrightnessDirty = { '5': false, '6': false, '7': false, '8': false };
    statusColorLayers.waiting = sanitizeStatusLayerForPortZones(cfg.layers?.wait || {});
    statusColorLayers.charging = sanitizeStatusLayerForPortZones(cfg.layers?.charge || {});
    statusColorLayers.error = sanitizeStatusLayerForPortZones(cfg.layers?.err || {});
    statusDefaults = {
        waiting: configHexColor(cfg.c_wait, FACTORY_STATUS_COLORS.waiting),
        charging: configHexColor(cfg.c_charge, FACTORY_STATUS_COLORS.charging),
        error: configHexColor(cfg.c_error, FACTORY_STATUS_COLORS.error),
    };
    statusBrushColors = { ...statusDefaults };
    document.getElementById('status_color').value = statusBrushColors[activeStatus];
    zoneMeta = zonesForActivePortCount(
        Array.isArray(cfg.zones) && cfg.zones.length ? cfg.zones : DEFAULT_ZONES,
        activePortCount
    );
    portsBrightnessValue = setBrightnessPair(
        'ports_brightness',
        'ports_brightness_input',
        cfg.b_ports ?? 120
    );
    appliedFreeZones = {};
    FREE_ZONE_IDS.forEach(zoneId => setAppliedFreeZoneState(zoneId, defaultFreeZoneState(zoneId)));
    if (Array.isArray(cfg.free_zones)) {
        cfg.free_zones.forEach(fz => {
            const zoneId = String(fz.zoneId ?? fz.id);
            if (!isFreeZone(zoneId)) return;
            setAppliedFreeZoneState(zoneId, fz);
        });
    }
    FREE_ZONE_IDS.forEach(zoneId => loadFreeZoneDraftFromApplied(zoneId));
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

function missingActivePortZones(map = hardwareZonesMap) {
    const count = clampActivePortCount(activePortCount);
    const missing = [];

    for (let port = 1; port <= count; port++) {
        const zoneId = String(port);
        if (zonePixelCountInMap(map, zoneId) === 0) {
            missing.push(zoneDisplayName(zoneId));
        }
    }

    return missing;
}

function activePortZoneErrorMessage(missingZones) {
    const missing = Array.isArray(missingZones) ? missingZones : [zoneDisplayName(missingZones)];
    return missing.length === 1
        ? `Разметьте активную зону: ${missing[0]}.`
        : `Разметьте активные зоны: ${missing.join(', ')}.`;
}

function validateActivePortZones(map = hardwareZonesMap) {
    const missing = missingActivePortZones(map);
    return missing.length ? activePortZoneErrorMessage(missing) : '';
}

function missingActivePortZoneNotice(map = hardwareZonesMap) {
    const missing = missingActivePortZones(map);

    return missing.length
        ? `Не размечены активные зоны: ${missing.join(', ')}. Сигналы портов будут читаться, но индикации негде отображаться.`
        : '';
}

function updateActivePortsWarning() {
    const warning = document.getElementById('active_ports_warning');
    const text = missingActivePortZoneNotice();
    warning.classList.toggle('field-hidden', !text);
    warning.textContent = text ? `Внимание: ${text}` : '';
}

function updateDirtyIndicators() {
    const zonesIndicator = document.getElementById('zones_dirty_indicator');
    const statusIndicator = document.getElementById('status_dirty_indicator');
    const freeIndicator = document.getElementById('free_dirty_indicator');

    const hasDirtyStatusLayer = portsBrightnessDirty || Object.values(statusLayerDirty).some(Boolean);
    const hasDirtyFreeZone = Object.values(freeZoneDirty).some(Boolean) ||
        Object.values(freeBrightnessDirty).some(Boolean) ||
        Object.values(uncertainFreeZoneState).some(Boolean);

    zonesIndicator?.classList.toggle('field-hidden', !zoneMapDirty);
    statusIndicator?.classList.toggle('field-hidden', !hasDirtyStatusLayer);
    freeIndicator?.classList.toggle('field-hidden', !hasDirtyFreeZone);
    if (freeIndicator) {
        freeIndicator.textContent = Object.values(uncertainFreeZoneState).some(Boolean)
            ? 'состояние не подтверждено'
            : 'черновик не сохранён';
    }
}

function zonePixelCountInMap(map, zoneId) {
    const target = String(zoneId);
    return Object.values(map || {}).filter(value => String(value) === target).length;
}

function activeFreeZoneIsMapped() {
    return isFreeZone(activeFreeZone) && zoneHasPixels(activeFreeZone);
}

function getMappedFreeZoneIds() {
    return zoneMeta
        .filter(z => z.type === 'free' && z.enabled && !z.reserved && zoneHasPixels(z.id))
        .map(z => String(z.id));
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
    const total = COLS * ROWS;
    const compact = total > TOPOLOGY_NUMBER_PREVIEW_MAX_PIXELS;
    preview.classList.toggle('compact', compact);
    preview.classList.toggle('dense', !compact && total > 96);

    if (compact) {
        preview.style.gridTemplateColumns = '1fr';
        let start = null;
        let end = null;
        for (let y = 0; y < ROWS; y++) {
            for (let x = 0; x < COLS; x++) {
                const index = ledIndexForTopology(x, y, normalized);
                if (index === 0) start = { x, y };
                if (index === total - 1) end = { x, y };
            }
        }

        const direction = document.createElement('div');
        direction.className = 'topology-direction';
        const title = document.createElement('strong');
        title.textContent = normalized < 2
            ? 'Вертикальная змейка по столбцам · чередование ↑ / ↓'
            : 'Горизонтальная змейка по строкам · чередование → / ←';
        const endpoints = document.createElement('span');
        endpoints.textContent = `Светодиод 0: столбец ${start?.x ?? '?'}, строка ${start?.y ?? '?'} · последний светодиод: столбец ${end?.x ?? '?'}, строка ${end?.y ?? '?'}`;
        const size = document.createElement('span');
        size.textContent = `${COLS}×${ROWS} · ${total} пикселей. Полная нумерация скрыта для читаемости.`;
        direction.append(title, endpoints, size);
        preview.appendChild(direction);
        return;
    }

    preview.style.gridTemplateColumns = `repeat(${COLS}, var(--topology-cell-width))`;
    for (let y = ROWS - 1; y >= 0; y--) {
        for (let x = 0; x < COLS; x++) {
            const index = ledIndexForTopology(x, y, normalized);
            const cell = document.createElement('div');
            cell.className = 'topology-cell';
            if (index === 0) cell.classList.add('first');
            if (index === total - 1) cell.classList.add('last');
            cell.textContent = String(index);
            cell.title = `Столбец ${x}, строка ${y}, светодиод ${index}`;
            preview.appendChild(cell);
        }
    }
}

function buildMatrix() {
    grid.innerHTML = '';
    const total = COLS * ROWS;
    grid.classList.toggle('dense', total > 96);
    grid.classList.toggle('very-dense', total > 256);
    grid.style.gridTemplateColumns = `repeat(${COLS}, var(--matrix-cell-size))`;

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

function buildStatusScopeOptions() {
    const scopeSelect = document.getElementById('status_scope_select');
    const activeCount = clampActivePortCount(activePortCount);
    if (activeStatusScope !== 'all' &&
        (!/^\d+$/.test(activeStatusScope) || Number(activeStatusScope) > activeCount)) {
        activeStatusScope = 'all';
    }

    scopeSelect.innerHTML = '';
    const allPorts = document.createElement('option');
    allPorts.value = 'all';
    allPorts.textContent = 'Все активные порты';
    scopeSelect.appendChild(allPorts);
    for (let port = 1; port <= activeCount; port++) {
        const option = document.createElement('option');
        option.value = String(port);
        option.textContent = `Порт ${port}`;
        scopeSelect.appendChild(option);
    }
    scopeSelect.value = activeStatusScope;
    document.getElementById('status_mode_select').value = statusEditMode;
}

function buildSelects() {
    const zoneSelect = document.getElementById('zone_select');
    const freeZoneSelect = document.getElementById('free_zone_select');
    zoneSelect.innerHTML = '';
    freeZoneSelect.innerHTML = '';
    let firstMappedFreeZone = null;
    let activeFreeZoneStillSelectable = false;

    zoneMeta.forEach(z => {
        if (z.type !== 'off') {
            const opt = document.createElement('option');
            opt.value = z.id;
            opt.textContent = zoneOptionLabel(z);
            opt.disabled = z.reserved || !z.enabled;
            zoneSelect.appendChild(opt);
        }

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
    buildStatusScopeOptions();
    if (!activeFreeZoneStillSelectable && firstMappedFreeZone !== null &&
        !freeZoneDraftHasUnsavedChanges(activeFreeZone)) {
        activeFreeZone = firstMappedFreeZone;
        loadFreeZoneDraftFromApplied(activeFreeZone);
    }
    freeZoneSelect.value = activeFreeZone;
    document.getElementById('free_mode_select').value = freeModes[activeFreeZone] || 'static';
    syncFreeColorInput();
    updateStatusEditorControls();
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
        const applied = appliedFreeZoneState(zoneId);
        const item = document.createElement('div');
        item.className = 'legend-item' + (zoneId === activeFreeZone ? ' selected' : '');
        const swatch = document.createElement('span');
        swatch.className = 'swatch';
        swatch.style.background = applied.mode === 'rainbow'
            ? 'linear-gradient(90deg, #ff3b30, #ff9500, #34c759, #0055ff, #af52de)'
            : applied.mode === 'custom'
                ? (Object.values(applied.customLayer)[0] || '#000000')
                : applied.staticColor;
        const text = document.createElement('span');
        text.textContent = `${zoneDisplayName(z)} · ${modeLabel(applied.mode)}`;
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

function freeZoneStateColor(key, state) {
    const mode = state?.mode || 'static';
    if (mode === 'custom' && state.customLayer && state.customLayer[key]) {
        return state.customLayer[key];
    }
    if (mode === 'rainbow') {
        const n = Number(key.split('-')[0]) + Number(key.split('-')[1]) + rainbowTick;
        return `hsl(${(n * 28) % 360}, 92%, 54%)`;
    }
    if (mode === 'custom') return '#000000';
    return state?.staticColor || '#ffffff';
}

function freeZoneDraftColor(key, zoneId) {
    const id = String(zoneId);
    return freeZoneStateColor(key, {
        mode: freeModes[id],
        staticColor: freeStaticColors[id],
        customLayer: freeCustomLayers[id],
    });
}

function freeZoneAppliedColor(key, zoneId) {
    return freeZoneStateColor(key, appliedFreeZoneState(zoneId));
}

function shouldLockPixel(zoneId) {
    if (activeTab === 'zones') {
        if (zoneViewMode === 'overview') return false;
        if (!isEditableZone(activeZoneId)) return true;
        return !(zoneId === '0' || zoneId === activeZoneId);
    }
    if (activeTab === 'status') {
        if (!isEditablePortZone(zoneId) || statusEditMode !== 'pixels') return true;
        return !statusScopeMatchesZone(zoneId);
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

    const matrixSize = `${COLS}×${ROWS}`;
    document.getElementById('matrix_hint').textContent =
        activeTab === 'zones'
            ? (zoneViewMode === 'overview'
                ? `${matrixSize} · Обзор зон`
                : `${matrixSize} · ${zoneDisplayName(activeZoneId)} · ${zoneTool === 'erase' ? 'Ластик' : 'Кисть'}`)
            : activeTab === 'status'
                ? `${matrixSize} · ${statusLabel(activeStatus)} · ${statusScopeLabel()} · ${statusEditModeLabel()}`
                : (freeViewMode === 'overview'
                    ? `${matrixSize} · Обзор свободных зон`
                    : `${matrixSize} · ${zoneDisplayName(activeFreeZone)} · ${modeLabel(freeModes[activeFreeZone])} · ${
                        uncertainFreeZoneState[activeFreeZone]
                            ? 'состояние не подтверждено'
                            : freeZoneDraftHasUnsavedChanges(activeFreeZone)
                                ? 'черновик не сохранён'
                                : 'применено'
                    }`);

    grid.querySelectorAll('.pixel').forEach(p => {
        const key = keyOf(p.dataset.x, p.dataset.y);
        const zoneId = hardwareZonesMap[key] || '0';
        p.dataset.zone = zoneId;
        p.classList.toggle('off-zone', zoneId === '0');
        p.classList.toggle('active-zone',
            (activeTab === 'zones' && zoneViewMode === 'edit' && zoneId === activeZoneId) ||
            (activeTab === 'status' && statusScopeMatchesZone(zoneId)) ||
            (activeTab === 'free' && freeViewMode === 'edit' && zoneId === activeFreeZone));
        p.classList.toggle('locked', shouldLockPixel(zoneId));

        if (activeTab === 'status') {
            p.style.background = pixelStatusColor(key, zoneId);
        } else if (activeTab === 'free') {
            p.style.background = isFreeZone(zoneId) && freeViewMode === 'overview'
                ? freeZoneAppliedColor(key, zoneId)
                : isFreeZone(zoneId) && zoneId === activeFreeZone
                    ? freeZoneDraftColor(key, zoneId)
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
    updateDirtyIndicators();
    updateActivePortsWarning();
    updateEditState();
}

function updateEditState() {
    const line = document.getElementById('edit_state_line');
    line.classList.remove('warning');
    line.classList.toggle('field-hidden', !mockMode);
    line.textContent = mockMode
        ? `Контроллер недоступен: изменения остаются только в браузере. Локальный предпросмотр ${COLS}×${ROWS}.`
        : '';
}

function paintPixel(pixel) {
    if (freeZoneEditorBlocked()) return;
    if (!pixel || !pixel.classList.contains('pixel')) return;
    const key = keyOf(pixel.dataset.x, pixel.dataset.y);
    const currentZone = hardwareZonesMap[key] || '0';

    if (activeTab === 'zones') {
        if (zoneViewMode !== 'edit') return;
        if (!isEditableZone(activeZoneId)) return;
        if (zoneTool === 'erase') {
            if (currentZone === activeZoneId) {
                delete hardwareZonesMap[key];
                zoneMapDirty = true;
            }
        } else if (currentZone === '0' || currentZone === activeZoneId) {
            if (currentZone !== activeZoneId) {
                hardwareZonesMap[key] = activeZoneId;
                zoneMapDirty = true;
            }
        }
    } else if (activeTab === 'status') {
        if (statusEditMode !== 'pixels' || !statusScopeMatchesZone(currentZone)) return;
        const color = document.getElementById('status_color').value;
        statusColorLayers[activeStatus][key] = color;
        markStatusLayerDirty();
    } else if (activeTab === 'free') {
        if (freeViewMode !== 'edit') return;
        if (currentZone !== activeFreeZone || freeModes[activeFreeZone] !== 'custom') return;
        freeCustomLayers[activeFreeZone][key] = freeBrushColors[activeFreeZone] || document.getElementById('free_color').value;
        freeZoneDirty[activeFreeZone] = true;
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
    serviceOpen = open;
    const backdrop = document.getElementById('service_backdrop');
    backdrop.classList.toggle('open', open);
    backdrop.setAttribute('aria-hidden', open ? 'false' : 'true');
    if (open && activeServiceSection === 'diagnostics') loadDiagnostics();
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
document.getElementById('network_form').addEventListener('input', handleNetworkFormInput);
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
    const previousZoneId = String(activeFreeZone);
    const nextZoneId = String(e.target.value);
    if (freeZoneEditorBlocked()) {
        e.target.value = previousZoneId;
        return;
    }
    if (freeBrightnessSaveInProgress[previousZoneId]) {
        alert('Дождитесь завершения сохранения яркости выбранной свободной зоны.');
        e.target.value = previousZoneId;
        return;
    }
    if (uncertainFreeZoneState[previousZoneId] || uncertainFreeZoneState[nextZoneId]) {
        alert('Состояние свободной зоны не подтверждено контроллером. Перечитайте конфигурацию перед переключением зоны.');
        e.target.value = previousZoneId;
        return;
    }
    if (freeZoneDraftHasUnsavedChanges(previousZoneId) &&
        !confirm(`В зоне «${zoneDisplayName(previousZoneId)}» есть несохранённый черновик. Отказаться от него и перейти к другой зоне?`)) {
        e.target.value = previousZoneId;
        return;
    }
    if (freeZoneDraftHasUnsavedChanges(previousZoneId)) {
        discardFreeZoneDraft(previousZoneId);
    }
    activeFreeZone = nextZoneId;
    loadFreeZoneDraftFromApplied(activeFreeZone);
    document.getElementById('free_mode_select').value = freeModes[activeFreeZone] || 'static';
    syncFreeColorInput();
    redrawMatrix();
});

document.getElementById('free_mode_select').addEventListener('change', e => {
    if (freeZoneEditorBlocked()) return;
    freeModes[activeFreeZone] = e.target.value;
    freeZoneDirty[activeFreeZone] = true;
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
    if (freeZoneEditorBlocked()) return;
    if ((freeModes[activeFreeZone] || 'static') === 'custom') {
        freeBrushColors[activeFreeZone] = e.target.value;
    } else {
        freeStaticColors[activeFreeZone] = e.target.value;
        freeZoneDirty[activeFreeZone] = true;
    }
    redrawMatrix();
});

function applyStatusSolidColor() {
    if (freeZoneEditorBlocked() || statusEditMode !== 'solid') return;
    const color = document.getElementById('status_color').value;
    let targetCount = 0;
    for (let y = 0; y < ROWS; y++) {
        for (let x = 0; x < COLS; x++) {
            const key = `${x}-${y}`;
            if (!statusScopeMatchesZone(hardwareZonesMap[key] ?? '0')) continue;
            statusColorLayers[activeStatus][key] = color;
            targetCount += 1;
        }
    }
    if (targetCount === 0) return;
    markStatusLayerDirty();
    redrawMatrix();
}

function resetSelectedStatusScope() {
    if (freeZoneEditorBlocked() || statusEditMode !== 'pixels') return;
    if (activeStatusScope === 'all' &&
        !confirm(`Сбросить ${STATUS_RESET_LABELS[activeStatus]} всех активных портов к заводскому базовому цвету?`)) {
        return;
    }

    if (activeStatusScope === 'all') {
        statusColorLayers[activeStatus] = {};
    } else {
        Object.keys(statusColorLayers[activeStatus]).forEach(key => {
            if (statusScopeMatchesZone(hardwareZonesMap[key] ?? '0')) {
                delete statusColorLayers[activeStatus][key];
            }
        });
    }
    markStatusLayerDirty();
    redrawMatrix();
    alert('Выбранная область сброшена только в браузере. Нажмите «Сохранить слой статуса», чтобы записать изменение в контроллер.');
}

document.getElementById('status_color').addEventListener('input', e => {
    statusBrushColors[activeStatus] = e.target.value;
    redrawMatrix();
});
document.getElementById('status_scope_select').addEventListener('change', e => {
    if (matrixSavesBlockedByHardwareTransition()) return;
    activeStatusScope = e.target.value;
    redrawMatrix();
});
document.getElementById('status_mode_select').addEventListener('change', e => {
    if (matrixSavesBlockedByHardwareTransition()) return;
    statusEditMode = e.target.value === 'solid' ? 'solid' : 'pixels';
    redrawMatrix();
});
document.getElementById('apply_status_solid_btn').addEventListener('click', applyStatusSolidColor);

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
    if (freeZoneEditorBlocked()) return;
    if (freeViewMode !== 'edit' || (freeModes[activeFreeZone] || 'static') !== 'custom') return;
    freeCustomLayers[activeFreeZone] = {};
    freeZoneDirty[activeFreeZone] = true;
    redrawMatrix();
});

document.querySelectorAll('[data-status]').forEach(btn => {
    btn.addEventListener('click', () => {
        if (matrixSavesBlockedByHardwareTransition()) return;
        activeStatus = btn.dataset.status;
        document.querySelectorAll('[data-status]').forEach(b => b.classList.toggle('active', b === btn));
        document.getElementById('status_color').value = statusBrushColors[activeStatus];
        redrawMatrix();
    });
});

[
    'hardware_rows',
    'hardware_cols',
    'hardware_active_ports',
    'hardware_protocol',
    'hardware_color_order',
].forEach(id => {
    const element = document.getElementById(id);
    element.addEventListener('input', () => {
        captureHardwareConfigPending();
        if (!hardwareRestartInProgress) setHardwareConfigMessage('');
    });
    element.addEventListener('change', () => {
        captureHardwareConfigPending();
        if (!hardwareRestartInProgress) setHardwareConfigMessage('');
    });
});

function waitForMilliseconds(milliseconds) {
    return new Promise(resolve => setTimeout(resolve, milliseconds));
}

async function fetchHardwareConfigForRestart(timeoutMs = HARDWARE_RESTART_REQUEST_TIMEOUT_MS) {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), Math.max(1, timeoutMs));
    try {
        const response = await fetch('/get_config', {
            cache: 'no-store',
            signal: controller.signal,
        });
        if (!response.ok) return null;
        return hardwareConfigFromResponse(await response.json());
    } finally {
        clearTimeout(timeout);
    }
}

async function pollForHardwareRestart(expectedHardware) {
    const token = ++hardwareRestartPollToken;
    const deadline = Date.now() + HARDWARE_RESTART_TIMEOUT_MS;

    while (hardwareRestartInProgress && token === hardwareRestartPollToken) {
        const waitMs = Math.min(HARDWARE_RESTART_POLL_MS, deadline - Date.now());
        if (waitMs <= 0) break;
        await waitForMilliseconds(waitMs);
        if (!hardwareRestartInProgress || token !== hardwareRestartPollToken) break;

        const requestTimeoutMs = Math.min(HARDWARE_RESTART_REQUEST_TIMEOUT_MS, deadline - Date.now());
        if (requestTimeoutMs <= 0) break;
        try {
            const applied = await fetchHardwareConfigForRestart(requestTimeoutMs);
            if (applied && hardwareConfigsEqual(applied, expectedHardware)) {
                setHardwareConfigMessage('Контроллер перезапущен. Обновляем страницу.');
                window.location.reload();
                return;
            }
        } catch (err) {
            // Перезапуск временно разрывает HTTP-соединение; продолжаем опрос.
        }
    }

    if (token === hardwareRestartPollToken && hardwareRestartInProgress) {
        setHardwareConfigMessage(
            'Не удалось подтвердить перезагрузку за 60 секунд. Перезагрузите контроллер или обновите страницу вручную.',
            true
        );
    }
}

function lockMatrixSavesForHardwareRestart() {
    hardwareConfigSaveInProgress = false;
    hardwareRestartInProgress = true;
    updateMatrixSaveControls();
}

document.getElementById('save_hardware_config_btn').addEventListener('click', async () => {
    if (matrixSavesBlockedByHardwareTransition()) {
        setHardwareConfigMessage('Аппаратные настройки уже сохраняются или ожидают перезагрузки.', true);
        return;
    }

    captureHardwareConfigPending();
    const validation = validateHardwareConfigForm();
    if (!validation.config) {
        setHardwareConfigMessage(validation.error, true);
        return;
    }
    const requested = validation.config;

    if (configLoadInProgress) {
        setHardwareConfigMessage('Дождитесь завершения загрузки текущей конфигурации.', true);
        return;
    }
    if (hardwareConfigsEqual(requested, hardwareConfigApplied)) {
        setHardwareConfigMessage('Настройки не изменены.');
        return;
    }
    if (mockMode || !backendAvailable) {
        setHardwareConfigMessage('Контроллер недоступен: настройки не сохранены.', true);
        return;
    }

    const spatialChange = requested.rows !== hardwareConfigApplied.rows ||
        requested.cols !== hardwareConfigApplied.cols ||
        requested.activePortCount !== hardwareConfigApplied.activePortCount;

    if (Number(pendingTopology) !== Number(matrixConfig.topology)) {
        const appliedTopology = topologyOption(matrixConfig.topology).label;
        setHardwareConfigMessage(
            `Сначала сохраните выбранную схему физического соединения либо верните выбор к применённому варианту «${appliedTopology}».`,
            true
        );
        return;
    }
    if (brightnessSavePendingOrInProgress()) {
        setHardwareConfigMessage(
            'Дождитесь завершения сохранения яркости портов и свободных зон.',
            true
        );
        return;
    }
    if (hasUnsavedPreservedSettings()) {
        setHardwareConfigMessage(
            'Режимы, статичные цвета и яркость сохраняются при изменении геометрии. Сначала сохраните или отмените их несохранённые изменения.',
            true
        );
        return;
    }
    if (!spatialChange && (
        hasPendingMatrixChanges(false) ||
        hasUnsavedSpatialDrafts() ||
        uncertainZoneMapSubmission !== null
    )) {
        setHardwareConfigMessage(
            'Тип светодиодов и порядок цветов применяются после перезагрузки без очистки зон и слоёв. Сначала сохраните или отмените все несохранённые изменения редактора.',
            true
        );
        return;
    }

    const browserDraftWarning = hasUnsavedSpatialDrafts() || uncertainZoneMapSubmission !== null
        ? ' Несохранённая разметка и несохранённые рисунки в браузере также будут отброшены.'
        : '';
    const confirmation = spatialChange
        ? `Изменение фактических строк, столбцов или числа активных портов очистит сохранённую разметку зон и все семь пространственных слоёв.${browserDraftWarning} Схема физического соединения, яркость, режимы и статичные цвета сохранятся. Продолжить?`
        : 'Тип светодиодов или порядок цветов изменится после перезагрузки контроллера. Разметка зон и семь пространственных слоёв не очищаются. Продолжить?';
    if (!confirm(confirmation)) return;

    hardwareConfigSaveInProgress = true;
    updateMatrixSaveControls();
    setHardwareConfigMessage('Сохраняем аппаратные настройки...');

    let response;
    let responseText = '';
    const saveController = new AbortController();
    const saveTimeout = setTimeout(() => saveController.abort(), HARDWARE_SAVE_REQUEST_TIMEOUT_MS);
    try {
        response = await fetch('/save_hardware_config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            signal: saveController.signal,
            body: JSON.stringify({
                rows: requested.rows,
                cols: requested.cols,
                activePortCount: requested.activePortCount,
                ledProtocol: requested.ledProtocol,
                colorOrder: requested.colorOrder,
            }),
        });
        responseText = (await response.text()).trim();
    } catch (err) {
        hardwareConfigPending = { ...requested };
        lockMatrixSavesForHardwareRestart();
        setHardwareConfigMessage(
            'Связь прервана во время сохранения. Состояние записи неизвестно; проверяем, применились ли настройки.',
            true
        );
        pollForHardwareRestart({ ...requested });
        return;
    } finally {
        clearTimeout(saveTimeout);
    }

    if (response.status === 200 && responseText === 'NO_CHANGE') {
        hardwareConfigSaveInProgress = false;
        updateMatrixSaveControls();
        setHardwareConfigMessage('Настройки не изменены.');
        setTimeout(() => window.location.reload(), 250);
        return;
    }
    if (response.status === 200 && responseText === 'REBOOT_REQUIRED') {
        hardwareConfigPending = { ...requested };
        lockMatrixSavesForHardwareRestart();
        setHardwareConfigMessage('Настройки сохранены. Контроллер перезагружается.');
        pollForHardwareRestart({ ...requested });
        return;
    }
    if (response.status === 400 && responseText === 'BAD_CONFIG') {
        hardwareConfigSaveInProgress = false;
        updateMatrixSaveControls();
        setHardwareConfigMessage('Контроллер отклонил значения аппаратной конфигурации.', true);
        return;
    }
    if (response.status === 409 && responseText === 'RESTART_PENDING') {
        lockMatrixSavesForHardwareRestart();
        setHardwareConfigMessage(
            'Настройки уже сохранены. Завершите перезагрузку контроллера вручную, затем обновите эту страницу.',
            true
        );
        return;
    }
    if (response.status === 500 && responseText === 'SAVE_FAILED') {
        hardwareConfigSaveInProgress = false;
        updateMatrixSaveControls();
        setHardwareConfigMessage('Настройки не сохранены; действующая конфигурация не изменилась.', true);
        return;
    }
    if (response.status === 500 && responseText === 'SAVED_RESTART_REQUIRED') {
        hardwareConfigPending = { ...requested };
        lockMatrixSavesForHardwareRestart();
        setHardwareConfigMessage(
            'Настройки сохранены, но автоматическая перезагрузка не запущена. Перезагрузите контроллер или питание вручную, затем обновите эту страницу.',
            true
        );
        return;
    }

    hardwareConfigSaveInProgress = false;
    updateMatrixSaveControls();
    setHardwareConfigMessage(
        `Неожиданный ответ контроллера: ${responseText || response.status}. Настройки не считаются сохранёнными.`,
        true
    );
});

document.getElementById('topology_select').addEventListener('change', e => {
    pendingTopology = Number(e.target.value);
    renderTopologyControls();
    setTopologyMessage(
        pendingTopology === Number(matrixConfig.topology)
            ? 'Выбран применённый вариант.'
            : 'Предварительная схема обновлена. Физический порядок не изменится до успешного сохранения.'
    );
});

document.getElementById('save_topology_btn').addEventListener('click', async () => {
    const retryingUncertainTopology = uncertainTopologySubmission !== null &&
        Number(pendingTopology) === Number(uncertainTopologySubmission);
    if (matrixSaveOperationInProgress()) {
        setTopologyMessage('Другое сохранение матрицы ещё не завершено.', true);
        return;
    }
    if (uncertainTopologySubmission !== null && !retryingUncertainTopology) {
        setTopologyMessage(
            `Результат предыдущего сохранения неизвестен. Повторите сохранение варианта «${topologyOption(uncertainTopologySubmission).label}» либо перечитайте конфигурацию контроллера.`,
            true
        );
        return;
    }
    if (brightnessSavePendingOrInProgress()) {
        setTopologyMessage('Дождитесь завершения сохранения яркости.', true);
        return;
    }
    if (hasPendingMatrixChanges(false) || uncertainZoneMapSubmission !== null) {
        setTopologyMessage(
            'Сначала разрешите несохранённые или неподтверждённые изменения матрицы. Перед сменой физического размера можно вместо этого вернуть выбор схемы к применённому варианту.',
            true
        );
        return;
    }

    if (pendingTopology === matrixConfig.topology) {
        setTopologyMessage('Топология не изменена.');
        return;
    }

    if (!confirm('Схема должна соответствовать реальному соединению светодиодов. Логический рисунок и координаты зон не перемещаются. Сохранить выбранный физический порядок?')) {
        return;
    }

    if (!backendAvailable) {
        setTopologyMessage('Контроллер недоступен: показана только предварительная схема, выбор не сохранён.', true);
        return;
    }

    const submittedTopology = Number(pendingTopology);
    const body = new URLSearchParams();
    body.set('topology', String(submittedTopology));

    topologySaveInProgress = true;
    updateMatrixSaveControls();
    const saveController = new AbortController();
    const saveTimeout = setTimeout(() => saveController.abort(), MATRIX_SAVE_REQUEST_TIMEOUT_MS);
    try {
        const res = await fetch('/save_topology', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body,
            signal: saveController.signal,
        });
        const responseText = (await res.text()).trim();
        if (res.status === 200 && responseText === 'OK') {
            matrixConfig.topology = submittedTopology;
            pendingTopology = submittedTopology;
            uncertainTopologySubmission = null;
            renderTopologyControls();
            setTopologyMessage('Схема физического соединения сохранена и применена.');
        } else if (res.status === 409 && responseText === 'RESTART_PENDING') {
            hardwareRestartInProgress = true;
            setTopologyMessage(
                'Контроллер уже ожидает перезагрузки после аппаратного сохранения. Перезагрузите его и обновите страницу.',
                true
            );
        } else if (
            (res.status === 400 && responseText === 'BAD_TOPOLOGY') ||
            (res.status === 500 && responseText === 'STATE_UNAVAILABLE')
        ) {
            setTopologyMessage('Контроллер отклонил сохранение схемы. Применённое состояние не изменено.', true);
        } else {
            uncertainTopologySubmission = submittedTopology;
            setTopologyMessage(
                responseText === 'APPLY_FAILED_REBOOT_REQUIRED'
                    ? 'Схема записана, но не применена в работающей прошивке. Перезагрузите контроллер и перечитайте конфигурацию.'
                    : 'Результат сохранения схемы неизвестен. Повторите сохранение того же варианта либо перечитайте конфигурацию контроллера.',
                true
            );
        }
    } catch (err) {
        uncertainTopologySubmission = submittedTopology;
        setTopologyMessage(
            'Не удалось подтвердить сохранение схемы. Повторите сохранение того же варианта либо перечитайте конфигурацию контроллера.',
            true
        );
    } finally {
        clearTimeout(saveTimeout);
        topologySaveInProgress = false;
        updateMatrixSaveControls();
    }
    if (portsBrightnessDirty) schedulePortsBrightnessSave(portsBrightnessValue, true);
    if (!zoneMapDirty) flushPendingFreeZoneBrightness();
});

document.getElementById('save_zones_btn').addEventListener('click', async () => {
    if (matrixSavesBlockedByHardwareTransition()) {
        alert('Сохранение матрицы заблокировано до перезагрузки контроллера.');
        return;
    }
    const validationError = validateActivePortZones();
    if (validationError) {
        alert(validationError);
        return;
    }
    if (uncertainZoneMapSubmission &&
        !zoneMapsEqual(hardwareZonesMap, uncertainZoneMapSubmission)) {
        alert('Результат предыдущего сохранения зон неизвестен. Перезагрузите страницу для сверки с контроллером перед сохранением другой разметки.');
        return;
    }

    if (!backendAvailable) {
        alert('Контроллер недоступен: зоны изменены только в браузере.');
        return;
    }
    if (freeCustomLayersLosePixelsWithCurrentZoneMap() &&
        !confirm('Новая разметка удалит части рисунков свободных зон, которые больше не входят в эти зоны. Продолжить?')) {
        return;
    }
    const submittedZoneMap = { ...hardwareZonesMap };

    zoneMapSaveInProgress = true;
    updateMatrixSaveControls();
    let zonesSaved = false;
    const saveController = new AbortController();
    const saveTimeout = setTimeout(() => saveController.abort(), MATRIX_SAVE_REQUEST_TIMEOUT_MS);
    try {
        const res = await fetch('/save_zones', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(submittedZoneMap),
            signal: saveController.signal,
        });
        const text = await res.text();
        const resultUncertain = res.status >= 500;
        alert(text === 'OK'
            ? 'Зоны сохранены в памяти контроллера.'
            : text === 'ACTIVE_PORT_ZONE_EMPTY'
                ? (validateActivePortZones() || 'Каждый активный порт должен иметь хотя бы один пиксель для отображения статуса.')
                : resultUncertain
                    ? 'Результат сохранения зон неизвестен. Не изменяйте разметку: повторите сохранение этой же карты или перезагрузите страницу для сверки.'
                    : 'Не удалось сохранить зоны.');
        if (res.ok && text === 'OK') {
            clearLocalStatusPixelsForChangedZones(savedHardwareZonesMap, submittedZoneMap);
            hardwareZonesMap = { ...submittedZoneMap };
            savedHardwareZonesMap = { ...submittedZoneMap };
            uncertainZoneMapSubmission = null;
            zoneMapDirty = false;
            FREE_ZONE_IDS.forEach(zoneId => {
                freeCustomLayers[zoneId] = sanitizeFreeLayerForZone(freeCustomLayers[zoneId], zoneId);
                const applied = appliedFreeZoneState(zoneId);
                applied.customLayer = sanitizeFreeLayerForZone(applied.customLayer, zoneId);
            });
            zonesSaved = true;
        } else if (resultUncertain) {
            uncertainZoneMapSubmission = { ...submittedZoneMap };
        }
    } catch (err) {
        uncertainZoneMapSubmission = { ...submittedZoneMap };
        alert('Не удалось подтвердить сохранение зон. Не изменяйте разметку: повторите сохранение этой же карты или перезагрузите страницу для сверки.');
    } finally {
        clearTimeout(saveTimeout);
        zoneMapSaveInProgress = false;
        updateMatrixSaveControls();
    }
    if (portsBrightnessDirty) {
        schedulePortsBrightnessSave(portsBrightnessValue, true);
    }
    if (zonesSaved) {
        redrawMatrix();
    }
    if (!zoneMapDirty) {
        flushPendingFreeZoneBrightness();
    }
});

document.getElementById('clear_zone_btn').addEventListener('click', () => {
    if (freeZoneEditorBlocked()) return;
    if (zonePixelCount(activeZoneId) === 0) return;
    if (isPortZone(activeZoneId)) {
        const message = isRequiredActivePortZone(activeZoneId)
            ? `Очистить ${zoneDisplayName(activeZoneId)}? Этот порт останется без индикации, и сохранить зоны не получится, пока вы не назначите ему хотя бы один пиксель.`
            : `Очистить ${zoneDisplayName(activeZoneId)}? Этот порт сейчас в резерве, но его разметка будет удалена.`;
        if (!confirm(message)) return;
    }
    Object.keys(hardwareZonesMap).forEach(key => {
        if (hardwareZonesMap[key] === activeZoneId) {
            delete hardwareZonesMap[key];
            zoneMapDirty = true;
        }
    });
    redrawMatrix();
});

document.getElementById('clear_all_zones_btn').addEventListener('click', () => {
    if (freeZoneEditorBlocked()) return;
    if (!confirm('Очистить всю разметку матрицы? Все пиксели останутся без зоны только в браузере. Сохранить такую разметку нельзя, пока активные порты не будут размечены.')) return;
    hardwareZonesMap = {};
    zoneMapDirty = true;
    redrawMatrix();
});

document.getElementById('save_status_btn').addEventListener('click', async () => {
    if (matrixSavesBlockedByHardwareTransition()) {
        alert('Сохранение матрицы заблокировано до перезагрузки контроллера.');
        return;
    }
    if (!backendAvailable) {
        alert('Контроллер недоступен: слой статуса изменён только в браузере.');
        return;
    }
    const zoneValidationError = validateActivePortZones();
    if (zoneValidationError) {
        alert(`${zoneValidationError} Сначала исправьте и сохраните разметку во вкладке «Зоны матрицы».`);
        return;
    }
    if (zoneMapDirty) {
        alert('Сначала сохраните разметку зон. Слой статуса применяется к сохранённой карте матрицы.');
        return;
    }

    const submittedStatus = activeStatus;
    const submittedColors = sanitizeStatusLayerForPortZones(
        statusColorLayers[submittedStatus]
    );
    const payload = JSON.stringify({
        status: submittedStatus,
        baseColor: FACTORY_STATUS_COLORS[submittedStatus],
        colors: submittedColors,
    });
    const saveController = new AbortController();
    const saveTimeout = setTimeout(() => saveController.abort(), MATRIX_SAVE_REQUEST_TIMEOUT_MS);
    let saved = false;
    let resultMessage = '';
    statusSaveInProgress = true;
    updateMatrixSaveControls();
    try {
        const res = await fetch('/save_status_colors', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: payload,
            signal: saveController.signal,
        });
        const responseText = await res.text();
        const ok = res.ok && responseText === 'OK';
        if (ok) {
            statusColorLayers[submittedStatus] = submittedColors;
            statusDefaults[submittedStatus] = FACTORY_STATUS_COLORS[submittedStatus];
            statusLayerDirty[submittedStatus] = false;
            saved = true;
        } else {
            statusLayerDirty[submittedStatus] = true;
        }
        resultMessage = ok ? 'Слой статуса сохранён.' : 'Не удалось сохранить слой статуса.';
    } catch (err) {
        statusLayerDirty[submittedStatus] = true;
        resultMessage = 'Не удалось подтвердить сохранение слоя статуса. Изменения оставлены как несохранённые.';
    } finally {
        clearTimeout(saveTimeout);
        statusSaveInProgress = false;
        updateMatrixSaveControls();
    }
    if (portsBrightnessDirty) schedulePortsBrightnessSave(portsBrightnessValue, true);
    if (!zoneMapDirty) flushPendingFreeZoneBrightness();
    updateDirtyIndicators();
    updateEditState();
    if (saved) redrawMatrix();
    alert(resultMessage);
});

document.getElementById('clear_status_btn').addEventListener('click', resetSelectedStatusScope);

function updatePortsBrightness(value) {
    const safe = setBrightnessPair('ports_brightness', 'ports_brightness_input', value);
    if (safe !== portsBrightnessValue) {
        portsBrightnessValue = safe;
        portsBrightnessRevision += 1;
        portsBrightnessDirty = true;
        updateDirtyIndicators();
        redrawMatrix();
    }
    return safe;
}

async function persistPortsBrightness(value, revision) {
    if (!backendAvailable || matrixSavesBlockedByHardwareTransition() ||
        portsBrightnessSaveInProgress > 0) return false;

    portsBrightnessSaveInProgress = 1;
    const saveController = new AbortController();
    const saveTimeout = setTimeout(() => saveController.abort(), MATRIX_SAVE_REQUEST_TIMEOUT_MS);
    try {
        const body = new URLSearchParams();
        body.set('type', 'ports');
        body.set('val', String(value));
        const res = await fetch('/set_bright', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body,
            signal: saveController.signal,
        });
        const ok = res.ok && (await res.text()) === 'OK';
        if (revision === portsBrightnessRevision) {
            portsBrightnessDirty = !ok;
            updateDirtyIndicators();
        }
        if (!ok) alert('Не удалось сохранить яркость портовых зон.');
        return ok;
    } catch (err) {
        if (revision === portsBrightnessRevision) {
            portsBrightnessDirty = true;
            updateDirtyIndicators();
        }
        alert('Контроллер недоступен: яркость портовых зон изменена только в браузере.');
        return false;
    } finally {
        const newerDraftExists = revision !== portsBrightnessRevision;
        clearTimeout(saveTimeout);
        portsBrightnessSaveInProgress = 0;
        if (newerDraftExists && portsBrightnessDirty) {
            schedulePortsBrightnessSave(portsBrightnessValue, true);
        }
    }
}

function schedulePortsBrightnessSave(value, immediate = false) {
    if (portsBrightnessSaveTimer !== null) clearTimeout(portsBrightnessSaveTimer);
    portsBrightnessSaveTimer = null;
    if (!portsBrightnessDirty || !backendAvailable || matrixSavesBlockedByHardwareTransition()) return;

    const revision = portsBrightnessRevision;
    portsBrightnessSaveTimer = setTimeout(() => {
        portsBrightnessSaveTimer = null;
        persistPortsBrightness(value, revision);
    }, immediate ? 0 : BRIGHTNESS_SAVE_DEBOUNCE_MS);
}

document.getElementById('ports_brightness').addEventListener('input', e => {
    updatePortsBrightness(e.target.value);
});

document.getElementById('ports_brightness').addEventListener('change', e => {
    const value = updatePortsBrightness(e.target.value);
    schedulePortsBrightnessSave(value, true);
});

document.getElementById('ports_brightness_input').addEventListener('input', e => {
    const value = updatePortsBrightness(e.target.value);
    schedulePortsBrightnessSave(value);
});

document.getElementById('ports_brightness_input').addEventListener('change', e => {
    const value = updatePortsBrightness(e.target.value);
    schedulePortsBrightnessSave(value, true);
});

function setActiveFreeBrightness(value) {
    if (freeZoneEditorBlocked()) {
        return clampBrightnessValue(freeBrightness[activeFreeZone], 100);
    }
    const zoneId = String(activeFreeZone);
    const previous = clampBrightnessValue(freeBrightness[activeFreeZone], 100);
    const safe = setBrightnessPair('free_brightness', 'free_brightness_input', value);
    freeBrightness[zoneId] = safe;
    if (safe !== previous) {
        freeBrightnessRevisions[zoneId] += 1;
        freeBrightnessDirty[zoneId] = true;
        updateDirtyIndicators();
        redrawMatrix();
    }
    return safe;
}

document.getElementById('free_brightness').addEventListener('input', e => {
    setActiveFreeBrightness(e.target.value);
});

document.getElementById('free_brightness').addEventListener('change', e => {
    const value = setActiveFreeBrightness(e.target.value);
    scheduleFreeZoneBrightnessSave(activeFreeZone, value, true);
});

document.getElementById('free_brightness_input').addEventListener('input', e => {
    const value = setActiveFreeBrightness(e.target.value);
    scheduleFreeZoneBrightnessSave(activeFreeZone, value);
});

document.getElementById('free_brightness_input').addEventListener('change', e => {
    const value = setActiveFreeBrightness(e.target.value);
    scheduleFreeZoneBrightnessSave(activeFreeZone, value, true);
});

function buildFreeZonePayload(zoneId, state = captureFreeZoneDraft(zoneId)) {
    const id = String(zoneId);
    return {
        zoneId: Number(id),
        enabled: true,
        mode: state.mode,
        staticColor: state.staticColor,
        brightness: state.brightness,
        customLayer: { ...state.customLayer },
    };
}

function buildFreeZoneBrightnessPayload(zoneId, brightness) {
    return {
        zoneId: Number(zoneId),
        brightness: clampBrightnessValue(brightness),
    };
}

async function saveFreeZonePayload(payload, signal) {
    if (matrixSavesBlockedByHardwareTransition()) return false;
    const res = await fetch('/save_free_zone', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
        signal,
    });
    const text = await res.text();
    return {
        ok: res.ok && text === 'OK',
        uncertain: res.status >= 500 || (res.ok && text !== 'OK'),
    };
}

function cancelFreeZoneBrightnessSave(zoneId) {
    const id = String(zoneId);
    if (freeBrightnessSaveTimers[id] !== undefined) {
        clearTimeout(freeBrightnessSaveTimers[id]);
        delete freeBrightnessSaveTimers[id];
    }
}

async function persistFreeZoneBrightness(zoneId, brightness, revision, showError = true) {
    const id = String(zoneId);
    if (!backendAvailable || matrixSavesBlockedByHardwareTransition() ||
        zoneMapDirty || validateActivePortZones() || freeBrightnessSaveInProgress[id]) return false;

    freeBrightnessSaveInProgress[id] = true;
    const saveController = new AbortController();
    const saveTimeout = setTimeout(() => saveController.abort(), MATRIX_SAVE_REQUEST_TIMEOUT_MS);
    try {
        const result = await saveFreeZonePayload(
            buildFreeZoneBrightnessPayload(id, brightness),
            saveController.signal
        );
        const ok = Boolean(result?.ok);
        if (result?.uncertain) {
            uncertainFreeZoneState[id] = true;
            updateDirtyIndicators();
        }
        if (ok) appliedFreeZoneState(id).brightness = clampBrightnessValue(brightness);
        if (revision === freeBrightnessRevisions[id]) {
            freeBrightnessDirty[id] = !ok;
            updateDirtyIndicators();
        }
        if (!ok && showError) {
            alert(result?.uncertain
                ? `Результат сохранения яркости зоны «${zoneDisplayName(id)}» неизвестен. Перечитайте конфигурацию контроллера.`
                : `Не удалось сохранить яркость зоны «${zoneDisplayName(id)}».`);
        }
        return ok;
    } catch (err) {
        uncertainFreeZoneState[id] = true;
        updateDirtyIndicators();
        if (revision === freeBrightnessRevisions[id]) {
            freeBrightnessDirty[id] = true;
            updateDirtyIndicators();
        }
        if (showError) alert('Не удалось подтвердить сохранение яркости свободной зоны. Перечитайте конфигурацию контроллера.');
        return false;
    } finally {
        const newerDraftExists = revision !== freeBrightnessRevisions[id];
        clearTimeout(saveTimeout);
        delete freeBrightnessSaveInProgress[id];
        if (newerDraftExists && freeBrightnessDirty[id]) {
            scheduleFreeZoneBrightnessSave(id, freeBrightness[id]);
        }
    }
}

function scheduleFreeZoneBrightnessSave(zoneId, brightness, immediate = false) {
    const id = String(zoneId);
    cancelFreeZoneBrightnessSave(id);
    if (!freeBrightnessDirty[id] || !backendAvailable ||
        matrixSavesBlockedByHardwareTransition() || freeBrightnessBatchInProgress) return;

    const revision = freeBrightnessRevisions[id];
    freeBrightnessSaveTimers[id] = setTimeout(() => {
        delete freeBrightnessSaveTimers[id];
        persistFreeZoneBrightness(id, brightness, revision);
    }, immediate ? 0 : BRIGHTNESS_SAVE_DEBOUNCE_MS);
}

function flushPendingFreeZoneBrightness() {
    Object.keys(freeBrightnessDirty).forEach(zoneId => {
        if (freeBrightnessDirty[zoneId] && zoneHasPixels(zoneId)) {
            scheduleFreeZoneBrightnessSave(zoneId, freeBrightness[zoneId], true);
        }
    });
}

function ensureFreeZoneSaveAllowed() {
    if (freeZoneEditorBlocked()) {
        alert('Сохранение матрицы заблокировано до перезагрузки контроллера.');
        return false;
    }
    if (mockMode) return true;
    if (!backendAvailable) {
        alert('Контроллер недоступен: настройки свободной зоны изменены только в браузере.');
        return false;
    }
    const zoneValidationError = validateActivePortZones();
    if (zoneValidationError) {
        alert(`${zoneValidationError} Сначала исправьте разметку во вкладке «Зоны матрицы».`);
        return false;
    }
    if (zoneMapDirty) {
        alert('Сначала сохраните разметку зон. Настройки свободной зоны применяются к сохранённой карте матрицы.');
        return false;
    }
    return true;
}

document.getElementById('apply_free_btn').addEventListener('click', async () => {
    if (!ensureFreeZoneSaveAllowed()) return;

    const zoneId = String(activeFreeZone);
    if (freeBrightnessSaveInProgress[zoneId]) {
        alert('Дождитесь завершения сохранения яркости выбранной свободной зоны.');
        return;
    }
    cancelFreeZoneBrightnessSave(zoneId);
    const submittedState = captureFreeZoneDraft(zoneId);
    const payload = buildFreeZonePayload(zoneId, submittedState);

    if (mockMode) {
        setAppliedFreeZoneState(zoneId, submittedState);
        loadFreeZoneDraftFromApplied(zoneId);
        freeZoneDirty[zoneId] = false;
        freeBrightnessDirty[zoneId] = false;
        uncertainFreeZoneState[zoneId] = false;
        updateDirtyIndicators();
        redrawMatrix();
        alert('Демонстрационный режим: черновик применён только к локальному предпросмотру. Запрос к ESP32 не отправлялся.');
        return;
    }

    const saveController = new AbortController();
    const saveTimeout = setTimeout(() => saveController.abort(), MATRIX_SAVE_REQUEST_TIMEOUT_MS);
    let saved = false;
    let resultMessage = '';
    freeZoneSaveInProgress = true;
    updateMatrixSaveControls();
    try {
        const res = await fetch('/save_free_zone', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload),
            signal: saveController.signal,
        });
        const responseText = await res.text();
        const ok = res.ok && responseText === 'OK';
        if (ok) {
            setAppliedFreeZoneState(zoneId, submittedState);
            loadFreeZoneDraftFromApplied(zoneId);
            freeZoneDirty[zoneId] = false;
            freeBrightnessDirty[zoneId] = false;
            uncertainFreeZoneState[zoneId] = false;
            saved = true;
        } else if (res.status >= 500 || res.ok) {
            uncertainFreeZoneState[zoneId] = true;
        }
        resultMessage = ok
            ? 'Настройки свободной зоны сохранены и применены.'
            : res.status >= 500 || res.ok
                ? 'Результат сохранения свободной зоны неизвестен. Черновик сохранён в браузере; перечитайте конфигурацию перед дальнейшей работой.'
                : 'Не удалось сохранить свободную зону. Черновик оставлен для повторной попытки.';
    } catch (err) {
        uncertainFreeZoneState[zoneId] = true;
        resultMessage = 'Не удалось подтвердить сохранение свободной зоны. Черновик сохранён в браузере; перечитайте конфигурацию контроллера.';
    } finally {
        clearTimeout(saveTimeout);
        freeZoneSaveInProgress = false;
        updateMatrixSaveControls();
    }
    if (portsBrightnessDirty) schedulePortsBrightnessSave(portsBrightnessValue, true);
    if (saved && !zoneMapDirty) flushPendingFreeZoneBrightness();
    updateDirtyIndicators();
    updateEditState();
    redrawMatrix();
    alert(resultMessage);
});

document.getElementById('apply_free_brightness_all_btn').addEventListener('click', async () => {
    if (freeZoneEditorBlocked()) {
        alert('Сохранение матрицы заблокировано до перезагрузки контроллера.');
        return;
    }
    const mappedZones = getMappedFreeZoneIds();
    if (!mappedZones.length) {
        alert('Нет размеченных свободных зон.');
        return;
    }

    const value = clampBrightnessValue(document.getElementById('free_brightness').value);
    if (mockMode) {
        mappedZones.forEach(zoneId => {
            freeBrightness[zoneId] = value;
            appliedFreeZoneState(zoneId).brightness = value;
            freeBrightnessDirty[zoneId] = false;
            cancelFreeZoneBrightnessSave(zoneId);
        });
        syncFreeColorInput();
        updateDirtyIndicators();
        redrawMatrix();
        alert(`Демонстрационный режим: яркость ${value} применена только к локальному предпросмотру. Запрос к ESP32 не отправлялся.`);
        return;
    }
    if (!backendAvailable) {
        const zoneId = String(activeFreeZone);
        freeBrightness[zoneId] = value;
        freeBrightnessRevisions[zoneId] += 1;
        freeBrightnessDirty[zoneId] = true;
        redrawMatrix();
        alert('Контроллер недоступен: яркость выбранной зоны оставлена в черновике.');
        return;
    }
    const zoneValidationError = validateActivePortZones();
    if (zoneValidationError) {
        alert(`${zoneValidationError} Сначала исправьте разметку во вкладке «Зоны матрицы».`);
        return;
    }
    if (zoneMapDirty) {
        alert('Сначала сохраните разметку зон. Настройки свободной зоны применяются к сохранённой карте матрицы.');
        return;
    }
    if (mappedZones.some(zoneId => freeBrightnessSaveInProgress[zoneId])) {
        alert('Дождитесь завершения текущего сохранения яркости свободной зоны.');
        return;
    }
    if (mappedZones.some(zoneId => uncertainFreeZoneState[zoneId])) {
        alert('Состояние одной из свободных зон не подтверждено. Перечитайте конфигурацию контроллера перед общей настройкой яркости.');
        return;
    }
    mappedZones.forEach(cancelFreeZoneBrightnessSave);

    const failedZones = [];
    const uncertainZones = [];
    freeBrightnessBatchInProgress = true;
    updateMatrixSaveControls();
    try {
        for (const zoneId of mappedZones) {
            cancelFreeZoneBrightnessSave(zoneId);
            freeBrightness[zoneId] = value;
            freeBrightnessRevisions[zoneId] += 1;
            freeBrightnessDirty[zoneId] = true;
            const revision = freeBrightnessRevisions[zoneId];
            const ok = await persistFreeZoneBrightness(zoneId, value, revision, false);
            if (!ok) {
                failedZones.push(zoneDisplayName(zoneId));
                if (uncertainFreeZoneState[zoneId]) uncertainZones.push(zoneDisplayName(zoneId));
                if (zoneId !== String(activeFreeZone)) {
                    freeBrightnessDirty[zoneId] = false;
                    loadFreeZoneDraftFromApplied(zoneId);
                }
            }
        }
    } catch (err) {
        failedZones.push('неизвестная зона');
    } finally {
        freeBrightnessBatchInProgress = false;
        updateMatrixSaveControls();
    }
    redrawMatrix();
    if (uncertainZones.length) {
        alert(`Результат сохранения яркости зон неизвестен: ${uncertainZones.join(', ')}. Перечитайте конфигурацию контроллера.`);
    } else if (failedZones.length) {
        alert(`Не удалось сохранить яркость зон: ${failedZones.join(', ')}.`);
    } else {
        alert(`Яркость ${value} сохранена для свободных зон: ${mappedZones.map(zoneDisplayName).join(', ')}.`);
    }
});

document.getElementById('refresh_diag_btn').addEventListener('click', loadDiagnostics);

async function loadConfigInternal() {
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
                    topology: 0,
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

async function loadConfig() {
    if (configLoadInProgress) return;
    configLoadInProgress = true;
    try {
        await loadConfigInternal();
    } finally {
        configLoadInProgress = false;
    }
}

async function loadDiagnostics() {
    if (hardwareRestartInProgress) return;
    if (!backendAvailable) {
        diagnostics = buildMockDiagnostics();
        renderDiagnostics();
        setDiagnosticsMessage('Локальный предпросмотр: данные контроллера недоступны.');
        return;
    }

    if (diagnosticsRequestInFlight) return;
    diagnosticsRequestInFlight = true;

    try {
        const res = await fetch('/diagnostics');
        if (!res.ok) throw new Error(`diagnostics ${res.status}`);
        const receivedDiagnostics = await res.json();
        if (hardwareRestartInProgress) return;
        diagnostics = receivedDiagnostics;
        renderDiagnostics();
        const runtimeWarning = diagnosticsRuntimeWarning();
        setDiagnosticsMessage(runtimeWarning, Boolean(runtimeWarning));
    } catch (err) {
        if (hardwareRestartInProgress) return;
        setDiagnosticsMessage('Связь с контроллером потеряна. Повторная проверка будет выполнена автоматически.', true);
        document.getElementById('runtime_label').textContent = 'связь с контроллером потеряна';
        document.getElementById('runtime_dot').style.background = 'var(--orange)';
    } finally {
        diagnosticsRequestInFlight = false;
    }
}

function formatUptime(seconds) {
    if (seconds === null || seconds === undefined) return 'нет данных';
    const value = Number(seconds);
    if (!Number.isFinite(value) || value < 0) return 'нет данных';
    const total = Math.floor(value);
    const days = Math.floor(total / 86400);
    const hours = Math.floor((total % 86400) / 3600);
    const minutes = Math.floor((total % 3600) / 60);
    const secs = total % 60;
    const parts = [];
    if (days) parts.push(`${days} д`);
    if (hours || days) parts.push(`${hours} ч`);
    if (minutes || hours || days) parts.push(`${minutes} мин`);
    parts.push(`${secs} с`);
    return parts.join(' ');
}

function formatMemoryKb(bytes) {
    if (bytes === null || bytes === undefined) return 'нет данных';
    const value = Number(bytes);
    return Number.isFinite(value) && value >= 0 ? `${(value / 1024).toFixed(1)} КБ` : 'нет данных';
}

function formatRuntimeAge(milliseconds) {
    if (milliseconds === null || milliseconds === undefined) return 'нет данных';
    const value = Number(milliseconds);
    if (!Number.isFinite(value) || value < 0) return 'нет данных';
    return value < 1000 ? `${Math.round(value)} мс` : `${(value / 1000).toFixed(1)} с`;
}

function resetReasonLabel(reason) {
    const labels = {
        power_on: 'подача питания',
        external: 'внешний сигнал сброса',
        software: 'программная перезагрузка',
        panic: 'аварийное исключение',
        interrupt_watchdog: 'сторожевой таймер прерываний',
        task_watchdog: 'сторожевой таймер задачи',
        watchdog: 'сторожевой таймер',
        deep_sleep: 'выход из глубокого сна',
        brownout: 'просадка питания',
        sdio: 'сброс через SDIO',
        unknown: 'неизвестно',
    };
    return labels[reason] || labels.unknown;
}

function apStartStateLabel(state) {
    const labels = {
        not_attempted: 'ещё не запускалась',
        ok: 'запущена успешно',
        failed: 'ошибка запуска',
    };
    return labels[state] || 'нет данных';
}

function diagnosticsRuntimeWarning() {
    if (mockMode || !diagnostics) return '';

    const warnings = [];
    const inputAge = Number(diagnostics.inputRuntime?.lastUpdateAgeMs);
    const ledAge = Number(diagnostics.ledRuntime?.lastRefreshAgeMs);
    if (Number.isFinite(inputAge) && inputAge > RUNTIME_STALE_WARNING_MS) {
        warnings.push('давно не обновлялись Data-входы');
    }
    if (Number.isFinite(ledAge) && ledAge > RUNTIME_STALE_WARNING_MS) {
        warnings.push('давно не обновлялась физическая матрица');
    }
    if (diagnostics.networkRuntime?.apStartState === 'failed') {
        warnings.push('резервная точка доступа не запустилась');
    }

    return warnings.length ? `Внимание: ${warnings.join('; ')}.` : '';
}

function setDiagnosticsMessage(text, isError = false) {
    const el = document.getElementById('diagnostics_message');
    el.textContent = text || '';
    el.style.color = isError ? 'var(--red)' : 'var(--muted)';
}

function setNetworkMessage(text, isError = false) {
    const el = document.getElementById('network_message');
    el.textContent = text || '';
    el.style.color = isError ? 'var(--red)' : 'var(--muted)';
}

function resetNetworkPasswordField() {
    const passwordInput = document.getElementById('network_password');
    passwordInput.readOnly = false;
    passwordInput.placeholder = 'Введите пароль сети. Для уже сохранённого SSID поле можно оставить пустым';
}

function updateNetworkActionControls() {
    document.getElementById('scan_network_btn').disabled = networkSaveInFlight || networkScanInFlight;
    document.getElementById('network_save_connect_btn').disabled =
        networkSaveInFlight || networkScanInFlight;
    document.querySelectorAll('#network_scan button.network-scan-result').forEach(button => {
        button.disabled = networkSaveInFlight;
    });
}

function clearSelectedScannedNetwork() {
    selectedScannedNetwork = null;
    document.querySelectorAll('#network_scan button.network-scan-result').forEach(button => {
        button.setAttribute('aria-pressed', 'false');
    });
    resetNetworkPasswordField();
}

function handleNetworkFormInput(event) {
    networkFormDirty = true;
    networkFormRevision += 1;
    if (event.target.id === 'network_ssid' && selectedScannedNetwork &&
        event.target.value !== selectedScannedNetwork.ssid) {
        clearSelectedScannedNetwork();
        setNetworkMessage('SSID изменён вручную. Выбор из результатов сканирования снят.');
    }
}

function fillNetworkForm(status) {
    clearSelectedScannedNetwork();
    document.getElementById('network_ssid').value = status.ssid || '';
    document.getElementById('network_password').value = '';
    document.getElementById('network_dhcp').checked = status.dhcp !== false;
    document.getElementById('network_static_ip').value = status.staticIp || '192.168.1.150';
    document.getElementById('network_gateway').value = status.gateway || '192.168.1.1';
    document.getElementById('network_subnet').value = status.subnet || '255.255.255.0';
    document.getElementById('network_dns').value = status.dns || '8.8.8.8';
    document.getElementById('network_ap_ssid').value = status.apSsid || 'LED_MATRIX_A1B2C3';
    document.getElementById('network_ap_password').value = '';
    networkFormDirty = false;
    networkFormRevision += 1;
    updateNetworkStaticFields();
}

function updateNetworkStaticFields() {
    const dhcp = document.getElementById('network_dhcp').checked;
    document.getElementById('network_static_fields').classList.toggle('field-hidden', dhcp);
}

async function loadNetworkStatus() {
    const requestRevision = ++networkStatusRequestRevision;
    if (!backendAvailable) {
        networkStatus = buildMockNetworkStatus();
        renderNetworkStatus();
        setNetworkMessage('Контроллер недоступен, показан локальный предпросмотр.');
        return;
    }

    try {
        const res = await fetch('/network_status');
        if (!res.ok) throw new Error(`network_status ${res.status}`);
        const receivedStatus = await res.json();
        if (requestRevision !== networkStatusRequestRevision) return;
        networkStatus = receivedStatus;
        renderNetworkStatus();
        if (!networkFormDirty) setNetworkMessage('');
    } catch (err) {
        if (requestRevision !== networkStatusRequestRevision) return;
        const hasLastKnownStatus = Boolean(networkStatus);
        if (!hasLastKnownStatus) {
            renderDiagCards('network_summary', [
                ['Состояние', 'Связь с контроллером потеряна'],
            ]);
        }
        setNetworkMessage(
            hasLastKnownStatus
                ? 'Не удалось получить статус сети. Показано последнее известное состояние.'
                : 'Не удалось получить статус сети: связь с контроллером потеряна.',
            true
        );
    }
}

function renderNetworkStatus() {
    if (!networkStatus) return;
    const defaultApPasswordWarning = document.getElementById('network_ap_password_warning');
    defaultApPasswordWarning.classList.toggle(
        'field-hidden',
        mockMode || !backendAvailable || networkStatus.defaultApPassword !== true
    );
    const connected = networkStatus.connected === true;
    const cards = [
        ['Состояние', mockMode
            ? 'Демонстрационные данные — ESP32 недоступна'
            : networkStatusLabel(networkStatus.status)],
        ['Сохранённая сеть', networkStatus.ssid || 'не выбрана'],
    ];
    if (connected) {
        cards.push(['Подключённая сеть', networkStatus.connectedSsid || networkStatus.ssid || 'нет данных']);
    }
    cards.push(
        ['IP-адрес в сети роутера', networkStatus.ip || 'нет'],
        ['Адресация', networkStatus.dhcp ? 'DHCP' : 'Статический IP'],
        ['MAC контроллера', networkStatus.staMac || 'нет данных'],
        ['SSID резервной точки доступа', networkStatus.apSsid || 'не задан'],
        ['IP-адрес резервной точки доступа', networkStatus.apIp || 'нет']
    );
    if (connected) {
        const rssi = Number(networkStatus.rssi);
        cards.push(['Уровень сигнала', Number.isFinite(rssi) ? `${rssi} dBm` : 'нет данных']);
    }
    renderDiagCards('network_summary', cards);
    if (!networkFormDirty && !networkSaveInFlight) fillNetworkForm(networkStatus);
}

function networkSaveErrorLabel(code) {
    const labels = {
        BAD_AP_PASSWORD: 'Пароль резервной точки доступа должен содержать от 8 до 64 символов.',
    };
    return labels[code] || code || 'неизвестная ошибка';
}

function selectScannedNetwork(network, rowIndex, button) {
    if (mockMode || !backendAvailable || networkSaveInFlight || !network.ssid) return;

    const ssid = String(network.ssid);
    const secure = network.secure === true;
    selectedScannedNetwork = { ssid, secure, rowIndex };
    document.querySelectorAll('#network_scan button.network-scan-result').forEach(candidate => {
        candidate.setAttribute('aria-pressed', candidate === button ? 'true' : 'false');
    });

    const ssidInput = document.getElementById('network_ssid');
    const passwordInput = document.getElementById('network_password');
    const canReusePassword = secure && ssid === String(networkStatus?.ssid ?? '') &&
        networkStatus?.passwordConfigured === true;
    ssidInput.value = ssid;
    passwordInput.value = '';
    passwordInput.readOnly = !secure;
    passwordInput.placeholder = !secure
        ? 'пароль не требуется для выбранной открытой сети'
        : (canReusePassword ? 'пусто = сохранить прежний пароль' : 'введите пароль выбранной сети');
    networkFormDirty = true;
    networkFormRevision += 1;

    if (!secure) {
        setNetworkMessage('Открытая сеть выбрана, но ещё не сохранена. Пароль не требуется.');
    } else if (canReusePassword) {
        setNetworkMessage('Сохранённая сеть выбрана. Изменения формы ещё не сохранены; пустое поле сохранит прежний пароль.');
    } else {
        setNetworkMessage('Сеть выбрана, но ещё не сохранена. Введите пароль выбранной сети.');
        passwordInput.focus();
    }
}

function renderNetworkScanResults(networks, demonstration = false) {
    const container = document.getElementById('network_scan');
    const results = Array.isArray(networks) ? networks : [];
    if (demonstration) {
        if (selectedScannedNetwork) clearSelectedScannedNetwork();
    } else if (selectedScannedNetwork) {
        const previousIndex = selectedScannedNetwork.rowIndex;
        const matchesSelection = network => typeof network?.ssid === 'string' &&
            network.ssid === selectedScannedNetwork.ssid &&
            (network.secure === true) === selectedScannedNetwork.secure;
        let matchingIndex = Number.isInteger(previousIndex) &&
            matchesSelection(results[previousIndex])
            ? previousIndex
            : results.findIndex(matchesSelection);

        if (matchingIndex < 0 && !selectedScannedNetwork.secure) {
            matchingIndex = results.findIndex(network => typeof network?.ssid === 'string' &&
                network.ssid === selectedScannedNetwork.ssid && network.secure === true);
            if (matchingIndex >= 0) {
                selectedScannedNetwork.secure = true;
                const passwordInput = document.getElementById('network_password');
                const canReusePassword = selectedScannedNetwork.ssid ===
                    String(networkStatus?.ssid ?? '') && networkStatus?.passwordConfigured === true;
                passwordInput.readOnly = false;
                passwordInput.placeholder = canReusePassword
                    ? 'пусто = сохранить прежний пароль'
                    : 'введите пароль выбранной сети';
            }
        }
        selectedScannedNetwork.rowIndex = matchingIndex >= 0 ? matchingIndex : null;
    }

    const nodes = results.map((network, rowIndex) => {
        const ssid = typeof network?.ssid === 'string' ? network.ssid : '';
        const secure = network?.secure === true;
        const rssi = Number(network?.rssi);
        const details = [];
        if (demonstration) details.push('демонстрационный результат');
        details.push(Number.isFinite(rssi) ? `${rssi} dBm` : 'уровень сигнала неизвестен');
        details.push(secure ? 'требуется пароль' : 'открытая сеть');

        const selectable = !demonstration && ssid.length > 0;
        const row = document.createElement(selectable ? 'button' : 'div');
        row.className = `diag-card network-scan-result${selectable ? '' : ' non-selectable'}`;
        if (selectable) {
            row.type = 'button';
            const isSelected = selectedScannedNetwork?.rowIndex === rowIndex &&
                selectedScannedNetwork?.ssid === ssid && selectedScannedNetwork?.secure === secure;
            row.setAttribute('aria-pressed', isSelected ? 'true' : 'false');
            row.addEventListener('click', () => selectScannedNetwork({ ssid, secure }, rowIndex, row));
        }

        const title = document.createElement('strong');
        const description = document.createElement('span');
        title.textContent = ssid || 'Скрытая сеть';
        description.textContent = details.join(' · ');
        row.append(title, description);
        return row;
    });

    if (!nodes.length) {
        const notice = document.createElement('div');
        notice.className = 'notice';
        notice.textContent = 'Сети не найдены.';
        nodes.push(notice);
    }
    container.replaceChildren(...nodes);
}

async function scanNetworks() {
    if (!backendAvailable) {
        renderNetworkScanResults([
            { ssid: 'Демонстрационная защищённая сеть', rssi: -45, secure: true },
            { ssid: 'Демонстрационная открытая сеть', rssi: -62, secure: false },
        ], true);
        setNetworkMessage('Демонстрационные результаты — ESP32 недоступна. Выбор и сохранение отключены.');
        return;
    }
    if (networkSaveInFlight) {
        setNetworkMessage('Дождитесь завершения сохранения настроек сети.', true);
        return;
    }
    if (networkScanInFlight) return;

    const startedAt = Date.now();
    const scanController = new AbortController();
    const scanTimeout = setTimeout(() => scanController.abort(), NETWORK_SCAN_UI_TIMEOUT_MS);
    networkScanInFlight = true;
    updateNetworkActionControls();
    setNetworkMessage('Сканирование сетей...');
    try {
        while (Date.now() - startedAt < NETWORK_SCAN_UI_TIMEOUT_MS) {
            const res = await fetch('/scan_networks', {
                cache: 'no-store',
                signal: scanController.signal,
            });
            if (res.status === 202) {
                const remainingMs = NETWORK_SCAN_UI_TIMEOUT_MS - (Date.now() - startedAt);
                if (remainingMs <= 0) throw new Error('scan timeout');
                await new Promise(resolve => setTimeout(resolve,
                    Math.min(NETWORK_SCAN_POLL_MS, remainingMs)));
                continue;
            }
            if (!res.ok) throw new Error(`scan ${res.status}`);

            const networks = await res.json();
            renderNetworkScanResults(networks);
            const hasSelectableNetwork = Array.isArray(networks) && networks.some(network =>
                typeof network?.ssid === 'string' && network.ssid.length > 0);
            setNetworkMessage(hasSelectableNetwork
                ? 'Выберите сеть из списка или введите SSID вручную.'
                : 'Доступны только скрытые сети либо сети не найдены. SSID можно ввести вручную.');
            return;
        }
        throw new Error('scan timeout');
    } catch (err) {
        setNetworkMessage('Не удалось выполнить сканирование.', true);
    } finally {
        clearTimeout(scanTimeout);
        networkScanInFlight = false;
        updateNetworkActionControls();
    }
}

async function saveNetworkConfig() {
    if (!backendAvailable) {
        setNetworkMessage('Контроллер недоступен, настройки сети не сохранены.', true);
        return;
    }
    if (networkSaveInFlight) return;
    if (networkScanInFlight) {
        setNetworkMessage('Дождитесь завершения сканирования сетей.', true);
        return;
    }

    const ssidInput = document.getElementById('network_ssid');
    const passwordInput = document.getElementById('network_password');
    const previousSavedSsid = String(networkStatus?.ssid ?? '');
    const selected = selectedScannedNetwork && ssidInput.value === selectedScannedNetwork.ssid
        ? selectedScannedNetwork
        : null;
    const submittedRevision = networkFormRevision;
    const enteredSsid = ssidInput.value;
    const ssid = selected
        ? selected.ssid
        : (enteredSsid === previousSavedSsid ? enteredSsid : enteredSsid.trim());
    let password = passwordInput.value;
    if (selected?.secure) {
        const canReusePassword = selected.ssid === previousSavedSsid &&
            networkStatus?.passwordConfigured === true;
        if (!password.length && !canReusePassword) {
            setNetworkMessage('Введите пароль выбранной сети.', true);
            passwordInput.focus();
            return;
        }
    } else if (selected) {
        password = '';
    }
    const previousPasswordConfigured = networkStatus?.passwordConfigured === true;

    const body = new URLSearchParams();
    body.set('ssid', ssid);
    body.set('password', password);
    body.set('dhcp', document.getElementById('network_dhcp').checked ? '1' : '0');
    body.set('static_ip', document.getElementById('network_static_ip').value.trim());
    body.set('gateway', document.getElementById('network_gateway').value.trim());
    body.set('subnet', document.getElementById('network_subnet').value.trim());
    body.set('dns', document.getElementById('network_dns').value.trim());
    body.set('ap_ssid', document.getElementById('network_ap_ssid').value.trim());
    body.set('ap_password', document.getElementById('network_ap_password').value);

    let settingsSaved = false;
    let formChangedDuringSave = false;
    networkSaveInFlight = true;
    updateNetworkActionControls();
    try {
        setNetworkMessage('Сохранение настроек сети...');
        const res = await fetch('/save_network', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body,
        });
        if (!res.ok) throw new Error(await res.text());
        settingsSaved = true;
        networkStatusRequestRevision += 1;
        networkStatus = {
            ...(networkStatus || {}),
            configured: ssid.length > 0,
            ssid,
            passwordConfigured: password.length > 0 ||
                (ssid === previousSavedSsid && previousPasswordConfigured),
        };

        const reconnect = await fetch('/network_reconnect', { method: 'POST' });
        if (!reconnect.ok) throw new Error(await reconnect.text());

        formChangedDuringSave = networkFormRevision !== submittedRevision;
        setNetworkMessage(formChangedDuringSave
            ? 'Отправленные настройки сохранены, переподключение запущено. Более новые изменения формы ещё не сохранены.'
            : 'Настройки сохранены. Переподключение запущено, интерфейс может временно пропасть.');
        setTimeout(loadNetworkStatus, 4000);
    } catch (err) {
        formChangedDuringSave = networkFormRevision !== submittedRevision;
        setNetworkMessage(settingsSaved
            ? `Настройки сохранены, но не удалось запустить переподключение: ${networkSaveErrorLabel(err.message)}.${formChangedDuringSave ? ' Более новые изменения формы ещё не сохранены.' : ''}`
            : `Не удалось сохранить сеть: ${networkSaveErrorLabel(err.message)}`, true);
    } finally {
        formChangedDuringSave = networkFormRevision !== submittedRevision;
        if (settingsSaved && !formChangedDuringSave) {
            networkFormDirty = false;
            passwordInput.value = '';
            document.getElementById('network_ap_password').value = '';
            clearSelectedScannedNetwork();
        }
        networkSaveInFlight = false;
        updateNetworkActionControls();
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
        setAuthMessage('Контроллер недоступен, пароль не будет сохранён.');
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
        setAuthMessage('Контроллер недоступен, пароль не сохранён.', true);
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
        setFirmwareMessage('Выберите файл прошивки firmware.bin.');
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
        setFirmwareMessage('Контроллер недоступен, загрузка прошивки невозможна.', true);
        return;
    }

    const input = document.getElementById('firmware_file');
    const button = document.getElementById('firmware_upload_btn');
    const file = input.files && input.files[0];

    if (!file) {
        setFirmwareMessage('Выберите файл прошивки firmware.bin.', true);
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
    const mode = diagnostics.runtimeMode || diagnostics.mode || 'runtime';
    const port1 = diagnostics.ports?.[0]?.status || 'unknown';
    const firmware = diagnostics.firmware || {};
    const system = diagnostics.system || {};
    const hasInputRuntime = diagnostics.inputRuntime && typeof diagnostics.inputRuntime === 'object';
    const inputRuntime = diagnostics.inputRuntime || {};
    const hasLedRuntime = diagnostics.ledRuntime && typeof diagnostics.ledRuntime === 'object';
    const ledRuntime = diagnostics.ledRuntime || {};
    const networkRuntime = diagnostics.networkRuntime || {};
    const inputUpdateMode = mockMode
        ? 'демонстрационные данные'
        : !hasInputRuntime
        ? 'нет данных'
        : (inputRuntime.taskStarted
            ? 'работает отдельно'
            : (inputRuntime.loopFallback ? 'работает через резервный основной цикл' : 'не запущено'));
    const ledUpdateMode = mockMode
        ? 'демонстрационные данные'
        : !hasLedRuntime
        ? 'нет данных'
        : (ledRuntime.taskStarted
            ? 'работает отдельно'
            : (ledRuntime.loopFallback ? 'работает через резервный основной цикл' : 'не запущено'));
    const ledMutexStatus = mockMode
        ? 'нет данных'
        : !hasLedRuntime
        ? 'нет данных'
        : (ledRuntime.mutexReady ? 'работает' : 'не запущена');
    document.getElementById('diagnostics_source').textContent = mockMode
        ? 'Демонстрационные данные — ESP32 недоступна'
        : 'Данные диагностики — получены от контроллера';
    document.getElementById('runtime_label').textContent = `${mockMode ? 'локально' : 'режим'}: ${runtimeModeLabel(mode)} · Порт 1 ${statusLabel(port1)}`;
    document.getElementById('runtime_dot').style.background = port1 === 'error' ? 'var(--red)' : port1 === 'charging' ? 'var(--blue)' : 'var(--green)';

    renderDiagCards('diag_firmware', [
        ['Версия', firmware.version || 'нет данных'],
        ['Сборка', firmware.build || 'нет данных'],
        ['Окружение', firmware.environment || 'нет данных'],
    ]);

    renderDiagCards('diag_system', [
        ['Время работы', formatUptime(system.uptimeSec)],
        ['Свободная память', formatMemoryKb(system.freeHeap)],
        ['Минимум свободной памяти', formatMemoryKb(system.minFreeHeap)],
        ['Причина перезапуска', resetReasonLabel(system.resetReason)],
    ]);

    renderDiagCards('diag_tasks', [
        ['Чтение входов Data1…Data8', inputUpdateMode],
        ['Последнее чтение входов', formatRuntimeAge(inputRuntime.lastUpdateAgeMs)],
        ['Обновление матрицы', ledUpdateMode],
        ['Последний кадр матрицы', formatRuntimeAge(ledRuntime.lastRefreshAgeMs)],
        ['Защита состояния матрицы', ledMutexStatus],
    ]);

    renderDiagCards('diag_network_runtime', [
        ['Последний запуск резервной точки', apStartStateLabel(networkRuntime.apStartState)],
    ]);

    renderDiagCards('diag_summary', [
        ['Активных портов', diagnostics.activePortCount ?? 1],
        ['Основной выход матрицы', `${diagnostics.primaryLedOutput?.name || 'LED1'} / GPIO${diagnostics.primaryLedOutput?.gpio || 22}`],
    ]);

    renderDiagCards('diag_inputs', (diagnostics.inputs || []).map(input => [
        input.name,
        `GPIO${input.gpio} · ${input.raw} · ${input.active ? 'активен' : 'неактивен'}`,
    ]));

    renderDiagCards('diag_ports', (diagnostics.ports || []).map(port => [
        `Порт ${port.port}`,
        `${port.enabled ? statusLabel(port.status) : 'отключён / резерв'} · зона ${port.zoneId}`,
    ]));

    renderDiagCards('diag_leds', (diagnostics.ledOutputs || []).map(led => [
        led.name,
        `GPIO${led.gpio} · ${led.state === 'primary' ? 'основной' : 'резерв'}`,
    ]));
}

function createDiagCard(title, value) {
    const card = document.createElement('div');
    card.className = 'diag-card';
    const heading = document.createElement('strong');
    const content = document.createElement('span');
    heading.textContent = String(title ?? '');
    content.textContent = String(value ?? '');
    card.append(heading, content);
    return card;
}

function renderDiagCards(containerId, cards, emptyText = '') {
    const container = document.getElementById(containerId);
    const nodes = (cards || []).map(([title, value]) => createDiagCard(title, value));
    if (!nodes.length && emptyText) {
        const notice = document.createElement('div');
        notice.className = 'notice';
        notice.textContent = emptyText;
        nodes.push(notice);
    }
    container.replaceChildren(...nodes);
}

setInterval(() => {
    rainbowTick++;
    const freeRainbowVisible = freeViewMode === 'overview'
        ? FREE_ZONE_IDS.some(zoneId => appliedFreeZoneState(zoneId).mode === 'rainbow')
        : freeModes[activeFreeZone] === 'rainbow';
    if (activeTab === 'free' && freeRainbowVisible) redrawMatrix();
}, 160);

setInterval(() => {
    if (backendAvailable && !hardwareRestartInProgress && serviceOpen && activeServiceSection === 'diagnostics') {
        loadDiagnostics();
    }
}, 2500);

setInterval(() => {
    if (backendAvailable && !hardwareRestartInProgress && (!serviceOpen || activeServiceSection !== 'diagnostics')) {
        loadDiagnostics();
    }
}, 30000);

loadConfig().catch(err => {
    const line = document.getElementById('edit_state_line');
    line.classList.remove('field-hidden');
    line.textContent = 'Не удалось загрузить конфигурацию.';
    console.error(err);
});
</script>
</body>
</html>
)=====";

#endif
