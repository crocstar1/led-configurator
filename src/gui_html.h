#ifndef GUI_HTML_H
#define GUI_HTML_H

#include <Arduino.h>

const char PAGE_MAIN[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>NEK evse — Инженерная консоль кастомизации</title>
    <style>
        /* Премиальная светлая автономная тема */
        body { 
            font-family: system-ui, -apple-system, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; 
            text-align: center; 
            background: #f5f5f7; 
            color: #1d1d1f; 
            padding: 20px 15px; 
            margin: 0; 
            user-select: none; 
            -webkit-font-smoothing: antialiased;
            -moz-osx-font-smoothing: grayscale;
        }
        
        /* Двухколоночный модульный каркас */
        .dashboard-layout {
            display: grid;
            grid-template-columns: 1fr;
            gap: 16px;
            max-width: 1040px;
            margin: 0 auto;
        }
        
        /* Брендинг NEK evse в шапке */
        .header-container { 
            display: flex; 
            align-items: center; 
            justify-content: space-between; 
            width: 100%;
            max-width: 500px;
            margin: 0 auto 15px auto;
            padding: 0 4px;
            box-sizing: border-box;
        }
        
        .logo { height: 38px; width: auto; object-fit: contain; }
        
        .brand-block { text-align: left; display: flex; flex-direction: column; line-height: 1.1; }
        .brand-title { font-size: 26px; font-weight: 700; letter-spacing: -0.5px; color: #1d1d1f; text-transform: uppercase; }
        .brand-subtitle { font-size: 11px; font-weight: 600; letter-spacing: 4px; color: #0055ff; text-transform: uppercase; margin-top: 2px; }
        
        /* Векторная кнопка настроек */
        .settings-gear-btn {
            background: #e8e8ed;
            border: none;
            border-radius: 12px;
            width: 42px;
            height: 42px;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            color: #1d1d1f;
            transition: background 0.2s, transform 0.1s;
        }
        .settings-gear-btn:active { background: #d1d1d6; transform: scale(0.95); }

        .subtitle { color: #86868b; font-size: 11px; margin-top: -10px; margin-bottom: 25px; text-transform: uppercase; letter-spacing: 1px; text-align: left; max-width: 500px; margin-left: auto; margin-right: auto; padding: 0 4px; }
        
        /* Светлые карточки Apple-Style */
        .card { 
            background: #ffffff; 
            padding: 24px; 
            border-radius: 16px; 
            width: 100%;
            max-width: 500px; 
            margin: 0 auto; 
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.04), 0 1px 3px rgba(0, 0, 0, 0.02); 
            border: 1px solid #e5e5ea; 
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.04);
            box-sizing: border-box;
            text-align: left;
        }
        
        .btn-group { display: flex; gap: 12px; justify-content: center; margin: 12px 0; }
        
        /* Идеально выровненные кнопки с border-box */
        .btn { 
            flex: 1; 
            padding: 15px; 
            border: none; 
            border-radius: 12px; 
            font-size: 13px; 
            font-weight: 600; 
            cursor: pointer; 
            text-transform: uppercase; 
            letter-spacing: 0.3px; 
            transition: all 0.2s ease; 
            box-sizing: border-box;
        }
        .btn:active { transform: scale(0.98); opacity: 0.9; }
        
        .btn-wait { background: #1d1d1f; color: #ffffff; }
        .btn-charge { background: #0055ff; color: #ffffff; }
        .btn-error { background: #ff3b30; color: #ffffff; }
        .btn-clear { background: #f5f5f7; color: #ff3b30; border: 1px solid #e5e5ea; }
        .btn-save { background: #0055ff; color: #ffffff; width: 100%; margin-top: 10px; }

        /* Симуляционные кнопки входящих сигналов УСП-1 */
        .btn-signal { background: #e8e8ed; color: #1d1d1f; border: 1px solid #d1d1d6; }
        .btn-signal.active-wait { background: #1d1d1f; color: #ffffff; border-color: #1d1d1f; }
        .btn-signal.active-charge { background: #0055ff; color: #ffffff; border-color: #0055ff; }
        .btn-signal.active-error { background: #ff3b30; color: #ffffff; border-color: #ff3b30; }

        .slider-container { margin: 16px 0; text-align: left; }
        label { font-size: 11px; font-weight: 600; color: #86868b; display: block; margin-bottom: 12px; text-transform: uppercase; letter-spacing: 0.8px; }
        
        input[type=range] { width: 100%; accent-color: #0055ff; background: #e5e5ea; height: 6px; border-radius: 3px; cursor: pointer; margin: 0; }
        .val { float: right; color: #0055ff; font-weight: 700; font-family: monospace; font-size: 14px; }
        /* Переключатель-тумблер капсульного типа */
        .toggle-row { display: flex; justify-content: space-between; align-items: center; width: 100%; }
        .switch { position: relative; display: inline-block; width: 48px; height: 26px; }
        .switch input { opacity: 0; width: 0; height: 0; }
        .slider-box { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #e5e5ea; transition: .25s ease; border-radius: 30px; border: 1px solid #d1d1d6; }
        .slider-box:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background-color: #ffffff; transition: .25s ease; border-radius: 50%; box-shadow: 0 1px 3px rgba(0,0,0,0.2); }
        input:checked + .slider-box { background-color: #0055ff; border-color: #0055ff; }
        input:checked + .slider-box:before { transform: translateX(22px); }

        .controls-row { display: flex; justify-content: space-between; align-items: center; background: #f5f5f7; padding: 12px 14px; border-radius: 12px; margin-bottom: 16px; border: 1px solid #e5e5ea; box-sizing: border-box; }
        select, input[type=text] { background: #ffffff; color: #1d1d1f; border: 1px solid #d1d1d6; padding: 8px 12px; border-radius: 8px; font-weight: 600; font-size: 13px; cursor: pointer; outline: none; font-family: inherit; }
        input[type=text] { cursor: text; width: 100%; box-sizing: border-box; margin-bottom: 10px; }
        
        /* Холст светодиодной разметки CAD */
        #matrix_panel { display: block; transition: all 0.25s ease-in-out; }
        .matrix-scroll { overflow: auto; max-height: 55vh; padding: 10px; border: 1px solid #e5e5ea; border-radius: 12px; background: #ffffff; }
        .matrix { display: grid; gap: 5px; justify-content: center; margin: 0 auto; width: max-content; }
        .pixel { width: 26px; height: 26px; background: #e8e8ed; border-radius: 6px; border: 1px solid #d1d1d6; box-sizing: border-box; cursor: pointer; transition: all 0.15s; }
        
        /* Предохранитель: аппаратный замок на диоды вне выбранной зоны разметки */
        .pixel.locked { cursor: not-allowed; opacity: 0.15; filter: grayscale(100%); pointer-events: none; }

        /* ИСПРАВЛЕННАЯ ЦЕНТРИРОВАННАЯ ФЛЕКС-ЛЕГЕНДА ОБОРУДОВАНИЯ */
        .legend-container { display: flex; flex-direction: column; gap: 10px; margin-top: 14px; padding: 14px; background: #f5f5f7; border-radius: 12px; border: 1px solid #e5e5ea; box-sizing: border-box; }
        .legend-title { font-size: 11px; font-weight: 700; color: #1d1d1f; text-transform: uppercase; text-align: center; width: 100%; letter-spacing: 0.5px; margin-bottom: 2px; }
        .legend-flex-wrapper { display: flex; flex-wrap: wrap; gap: 12px; justify-content: center; width: 100%; }
        .legend-item { display: flex; align-items: center; font-size: 11px; font-weight: 600; color: #86868b; text-transform: uppercase; white-space: nowrap; }
        .legend-color { width: 12px; height: 12px; border-radius: 4px; display: inline-block; vertical-align: middle; margin-right: 6px; border: 1px solid rgba(0,0,0,0.1); }

        /* Настройка стилей в шестеренке */
        .settings-title { font-size: 14px; font-weight: 700; color: #1d1d1f; text-transform: uppercase; letter-spacing: 0.5px; margin-bottom: 14px; border-bottom: 1px solid #e5e5ea; padding-bottom: 6px; }
        .settings-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 16px; }
        .setting-item { display: flex; flex-direction: column; text-align: left; }
        .setting-item label { margin-bottom: 6px; }
        .setting-item select { width: 100%; box-sizing: border-box; }

        /* Сеточные палитры RGB главного экрана */
        .color-pickers-container { display: grid; grid-template-columns: 1fr; gap: 12px; margin: 16px 0 20px 0; }
        .color-picker-box { display: flex; flex-direction: column; align-items: center; font-size: 11px; font-weight: 600; color: #86868b; text-transform: uppercase; letter-spacing: 0.3px; }
        input[type=color] { border: 1px solid #d1d1d6; width: 100%; max-width: 80px; height: 40px; border-radius: 10px; cursor: pointer; background: #ffffff; padding: 2px; box-sizing: border-box; margin-bottom: 6px; transition: transform 0.1s; }
        input[type=color]:active { transform: scale(0.95); }

        /* Блок безопасного OTA обновления */
        .ota-box { background: #f5f5f7; border: 1px solid #e5e5ea; border-radius: 12px; padding: 16px; margin-bottom: 16px; box-sizing: border-box; }
        .progress-bar-container { display: none; background: #e5e5ea; height: 6px; border-radius: 3px; overflow: hidden; margin-top: 12px; width: 100%; }
        .progress-bar-fill { background: #0055ff; height: 100%; width: 0%; transition: width 0.1s ease; }
        .ota-status { font-size: 12px; font-weight: 600; color: #86868b; text-align: center; margin-top: 10px; line-height: 1.2; }

        /* Оверлей всплывающего окна шестеренки */
        .modal-overlay { display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.4); z-index: 1000; backdrop-filter: blur(5px); -webkit-backdrop-filter: blur(5px); }
        .modal-content { position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); background: #ffffff; width: 92%; max-width: 480px; max-height: 85vh; overflow-y: auto; padding: 24px; border-radius: 16px; box-shadow: 0 10px 40px rgba(0,0,0,0.15); box-sizing: border-box; }
        .modal-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; border-bottom: 1px solid #e5e5ea; padding-bottom: 10px; }
        .modal-title { font-size: 18px; font-weight: 700; color: #1d1d1f; }
        .modal-close { font-size: 24px; cursor: pointer; color: #86868b; line-height: 1; user-select: none; }

        /* Двухколоночная адаптивность под ПК */
        @media (min-width: 900px) {
            .dashboard-layout { grid-template-columns: 1fr 1fr; align-items: start; }
            .header-container, .subtitle { max-width: 1040px; padding: 0; }
            .card { max-width: 100%; }
            #matrix_panel { grid-column: 1; }
        }
    </style>
</head>
<body>
    <div class='header-container'>
        <img class='logo' src='' alt=''>
        <div class='brand-block'>
            <div class='brand-title'>NEK</div>
            <div class='brand-subtitle'>evse</div>
        </div>
        <!-- ВЕКТОРНАЯ ШЕСТЕРЕНКА ВЗАМЕН СМАЙЛИКА -->
        <button class='settings-gear-btn' onclick='openSettingsModal()'>
            <svg viewBox="0 0 24 24" width="20" height="20" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
                <circle cx="12" cy="12" r="3"></circle>
                <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"></path>
            </svg>
        </button>
    </div>
    
    <div class='subtitle'>Удаленный инженерный комплекс УСП-1</div>

    <div class='dashboard-layout'>
        
        <!-- КОЛОНКА 1: Трассировка перегородок -->
        <div style='display: flex; flex-direction: column; gap: 16px; width: 100%;'>
            <div class='card'>
                <div class='toggle-row'>
                    <span style='font-size: 13px; font-weight: 600; color: #1d1d1f; text-transform: uppercase; letter-spacing: 0.5px;'>Режим трассировки перегородок:</span>
                    <label class='switch'>
                        <input type='checkbox' id='view_matrix_toggle' onchange='toggleMatrixVisibility()'>
                        <span class='slider-box'></span>
                    </label>
                </div>
            </div>
            <div id='matrix_panel' class='card' style='display: none;'>
                <div class='controls-row'>
                    <span style='font-size: 11px; font-weight: 600; color: #86868b; text-transform: uppercase;'>Целевой узел разметки:</span>
                    <select id='zone_select' onchange='handleZoneVisibilityHighlight()'>
                        <option value='0'>[X] Отключить светодиод</option>
                        <option value='1' selected>Пистолет 1 (Канал Master)</option>
                        <option value='2' disabled>Пистолет 2 (reserved)</option>
                        <option value='3' disabled>Пистолет 3 (reserved)</option>
                        <option value='4' disabled>Пистолет 4 (reserved)</option>
                        <option value='5'>Брендирование (Логотип/QR)</option>
                        <option value='6'>QR зона</option>
                        <option value='7'>Service зона</option>
                        <option value='8'>Custom зона</option>
                    </select>
                </div>

                <div class='matrix-scroll'>
                    <div class='matrix' id='matrix_grid'></div>
                </div>

                <!-- ИСПРАВЛЕННАЯ ЦЕНТРИРОВАННАЯ ФЛЕКС-ЛЕГЕНДА ОБОРУДОВАНИЯ -->
                <div class='legend-container'>
                    <div class='legend-title'>Цветовая легенда оборудования:</div>
                    <div class='legend-flex-wrapper'>
                        <div class='legend-item'><span class='legend-color' style='background: #0055ff;'></span>Пистолет 1</div>
                        <div class='legend-item'><span class='legend-color' style='background: #4a8eff;'></span>Пистолет 2 reserved</div>
                        <div class='legend-item'><span class='legend-color' style='background: #78b4ff;'></span>Пистолет 3 reserved</div>
                        <div class='legend-item'><span class='legend-color' style='background: #b4d2ff;'></span>Пистолет 4 reserved</div>
                        <div class='legend-item'><span class='legend-color' style='background: #1d1d1f;'></span>Зона 5: Брендирование</div>
                        <div class='legend-item'><span class='legend-color' style='background: #34c759;'></span>Зона 6: QR</div>
                        <div class='legend-item'><span class='legend-color' style='background: #ff9500;'></span>Зона 7: Service</div>
                        <div class='legend-item'><span class='legend-color' style='background: #af52de;'></span>Зона 8: Custom</div>
                    </div>
                </div>
                
                <div style='display: flex; gap: 12px; margin-top: 14px;'>
                    <button class='btn btn-clear' onclick='clearMatrix()'>Очистить карту</button>
                    <button class='btn btn-save' onclick='saveMatrixZones()'>Записать во Flash</button>
                </div>
            </div>
        </div>
        
        <!-- КОЛОНКА 2: Управление симуляцией и кастомизация статусов -->
        <div class='card' style='display: flex; flex-direction: column; gap: 8px;'>
            <div class='slider-container'>
                <label>Яркость пистолетов (ШИМ): <span class='val' id='bright_ports_val'>120</span></label>
                <input type='range' id='bright_ports_slide' min='1' max='255' value='120' oninput="updateBrightness('ports')">
            </div>
            
            <div class='slider-container' style='margin-bottom: 12px;'>
                <label>Яркость логотипа (ШИМ): <span class='val' id='bright_logo_val'>120</span></label>
                <input type='range' id='bright_logo_slide' min='1' max='255' value='120' oninput="updateBrightness('logo')">
            </div>

            <!-- Селектор зоны для независимой попиксельной раскраски статуса -->
            <div class='controls-row' style='margin-bottom: 12px;'>
                <span style='font-size: 11px; font-weight: 600; color: #86868b; text-transform: uppercase;'>Зона кастомизации цвета:</span>
                <select id='status_zone_select' onchange='updateCanvasLockState()'>
                    <option value='1' selected>Пистолет 1</option>
                    <option value='2' disabled>Пистолет 2 reserved</option>
                    <option value='3' disabled>Пистолет 3 reserved</option>
                    <option value='4' disabled>Пистолет 4 reserved</option>
                    <option value='5'>Зона 5: Брендирование</option>
                </select>
            </div>

            <!-- ВЫНЕСЕННЫЙ НА ГЛАВНЫЙ ЭКРАН ВЫБОР РЕЖИМА БРЕНДИРОВАНИЯ -->
            <div id='brand_mode_container' class='toggle-row' style='margin-bottom: 16px; display: none;'>
                <span style='font-size: 11px; font-weight: 600; color: #86868b; text-transform: uppercase;'>Эффект переливания радуги:</span>
                <label class='switch'>
                    <input type='checkbox' id='cfg_logo_anim' onchange='toggleBrandRainbowMode()'>
                    <span class='slider-box'></span>
                </label>
            </div>

            <!-- Палитра выбора цвета для текущего пикселя/статуса -->
            <div id='status_color_container' class='color-pickers-container'>
                <div class='color-picker-box' style='grid-column: span 4;'>
                    <input type='color' id='current_paint_color' value='#0055ff' style='max-width: 80px; height: 42px;'>
                    <span style='margin-top: 4px;'>Выбрать цвет кисти</span>
                </div>
            </div>

            <label style='margin-bottom: 6px;'>Имитация входящих сигналов УСП-1:</label>
            <div class='btn-group'>
                <button id='btn_sig_wait' class='btn btn-signal active-wait' onclick='switchSimulatedSignal("waiting")'>Ожидание</button>
                <button id='btn_sig_charge' class='btn btn-signal' onclick='switchSimulatedSignal("charging")'>Зарядка</button>
                <button id='btn_sig_error' class='btn btn-signal' onclick='switchSimulatedSignal("error")'>Авария</button>
            </div>

            <button class='btn btn-save' style='margin-top: 8px;' onclick='saveCurrentStatusColors()'>Сохранить схему статуса</button>
        </div>
    </div> <!-- Конец dashboard-layout -->
    <!-- ВСПЛЫВАЮЩЕЕ ОКНО НАСТРОЕК (ШЕСТЕРЕНКА) -->
    <div id='settings_modal' class='modal-overlay' onclick='closeSettingsModal(event)'>
        <div class='modal-content'>
            <div class='modal-header'>
                <div class='modal-title'>Системные параметры УСП-1</div>
                <div class='modal-close' onclick='toggleSettingsModal(false)'>&times;</div>
            </div>
            
            <div class='settings-grid'>
                <div class='setting-item' style='grid-column: span 2;'>
                    <label>Топология укладки змейки</label>
                    <select id='cfg_topology'>
                        <option value='0'>Справа-Снизу (Вертикальная)</option>
                        <option value='1'>Слева-Снизу (Вертикальная)</option>
                        <option value='2'>Слева-Сверху (Горизонтальная)</option>
                        <option value='3'>Справа-Сверху (Горизонтальная)</option>
                    </select>
                </div>
            </div>

            <div class='settings-title' style='margin-top: 20px;'>Конфигурация сети станции</div>
            <div class='toggle-row' style='margin-bottom: 12px;'>
                <span style='font-size: 13px; font-weight: 600; color: #1d1d1f; text-transform: uppercase;'>Автоматический DHCP:</span>
                <label class='switch'>
                    <input type='checkbox' id='cfg_is_dhcp' onchange='toggleNetworkInputs()'>
                    <span class='slider-box'></span>
                </label>
            </div>
            
            <div id='static_ip_fields' class='settings-grid'>
                <div class='setting-item'>
                    <label>Статический IP</label>
                    <input type='text' id='cfg_ip' class='net-input' placeholder='192.168.1.150'>
                </div>
                <div class='setting-item'>
                    <label>Маска подсети</label>
                    <input type='text' id='cfg_mask' class='net-input' placeholder='255.255.255.0'>
                </div>
                <div class='setting-item' style='grid-column: span 2;'>
                    <label>Основной шлюз</label>
                    <input type='text' id='cfg_gw' class='net-input' placeholder='192.168.1.1'>
                </div>
            </div>

            <div class='settings-title' style='margin-top: 25px;'>Обновление ПО контроллера (OTA)</div>
            <div class='ota-box'>
                <input type='file' id='ota_file' style='display: none;' onchange='startOtaUpdate()'>
                <button class='btn' style='background: #1d1d1f; color: #ffffff; width: 100%;' onclick='document.getElementById("ota_file").click()'>Выбрать файл firmware bin</button>
                <div class='progress-bar-container' id='ota_progress_container'>
                    <div class='progress-bar-fill' id='ota_progress_fill'></div>
                </div>
                <div class='ota-status' id='ota_status'>Система готова к загрузке</div>
            </div>

            <button class='btn btn-save' style='margin-top: 15px;' onclick='saveFullGlobalConfig()'>Применить глобальные настройки</button>
        </div>
    </div>

    <!-- ЕДИНСТВЕННЫЙ ИЗОЛИРОВАННЫЙ БЛОК СКРИПТОВ В САМОМ НИЗУ СТРАНИЦЫ -->
    <script>
        let COLS = 12; 
        let ROWS = 8; 
        let isDrawing = false;
        let isDragActive = false;
        let ws;

        // Текущий симулируемый статус ("waiting", "charging", "error")
        let currentActiveSignal = "waiting"; 

        const grid = document.getElementById('matrix_grid');
        const m_panel = document.getElementById('matrix_panel');
        const m_modal = document.getElementById('settings_modal');

        // Палитра маркеров зон для аппаратного режима трассировки
        const zoneColors = {
            '0': '#e8e8ed', 
            '1': '#0055ff', 
            '2': '#4a8eff', 
            '3': '#78b4ff', 
            '4': '#b4d2ff', 
            '5': '#1d1d1f',
            '6': '#34c759',
            '7': '#ff9500',
            '8': '#af52de'
        };

        // Локальное ОЗУ CAD-интерфейса: хранит разметку перегородок и 3 независимых слоя цветов
        let hardwareZonesMap = {}; 
        let statusColorLayers = {
            "waiting": {},
            "charging": {},
            "error": {}
        };

        fetch('/get_size')
            .then(res => res.text())
            .then(sizeStr => {
                if (sizeStr && sizeStr.includes(':')) {
                    const parts = sizeStr.split(':');
                    COLS = parseInt(parts[0]) || 12;
                    ROWS = parseInt(parts[1]) || 8;
                }
                buildMatrixUI();
            })
            .catch(() => {
                buildMatrixUI(); // Резервный запуск для ПК
            });

        function buildMatrixUI() {
            grid.innerHTML = '';
            grid.style.gridTemplateColumns = `repeat(${COLS}, 26px)`;

            for (let y = ROWS - 1; y >= 0; y--) {
                for (let x = 0; x < COLS; x++) {
                    const pixel = document.createElement('div');
                    pixel.className = 'pixel';
                    pixel.id = `p-${x}-${y}`;
                    pixel.dataset.x = x;
                    pixel.dataset.y = y;
                    pixel.dataset.zone = '0';
                    grid.appendChild(pixel);
                }
            }
            initWebSocket();
            loadGlobalConfigFromDevice();
        }

        // ТУМБЛЕР РАСКРЫТИЯ ШТОРКИ ТРАССИРОВКИ
        function toggleMatrixVisibility() {
            const isTraceMode = document.getElementById('view_matrix_toggle').checked;
            m_panel.style.display = isTraceMode ? 'block' : 'none';
            
            // Принудительно переключаем визуализацию холста (Режим разметки железа VS Режим рисования)
            redrawCanvasByCurrentMode();
        }

        function toggleNetworkInputs() {
            const isDhcp = document.getElementById('cfg_is_dhcp').checked;
            const fields = document.getElementById('static_ip_fields');
            fields.style.opacity = isDhcp ? '0.35' : '1';
            fields.style.pointerEvents = isDhcp ? 'none' : 'auto';
            
            if (isDhcp) {
                document.querySelectorAll('.net-input').forEach(input => input.blur());
            }
        }

        // ФУНКЦИЯ-ХАМЕЛЕОН: ПЕРЕКРАШИВАЕТ ХОЛСТ В ЗАВИСИМОСТИ ОТ ТУМБЛЕРА ТРАССИРОВКИ И СИГНАЛА
        function redrawCanvasByCurrentMode() {
            const isTraceMode = document.getElementById('view_matrix_toggle').checked;
            
            document.querySelectorAll('.pixel').forEach(p => {
                const key = `${p.dataset.x}-${p.dataset.y}`;
                
                if (isTraceMode) {
                    // Шаг 1: Показываем чисто карту перегородок (железо)
                    const zoneId = hardwareZonesMap[key] || '0';
                    p.dataset.zone = zoneId;
                    p.style.background = zoneColors[zoneId];
                    p.classList.remove('locked');
                    p.style.opacity = '1';
                } else {
                    // Шаг 2: Показываем цвета диодов для текущего симулируемого сигнала
                    const currentColor = statusColorLayers[currentActiveSignal][key];
                    const zoneId = hardwareZonesMap[key] || '0';
                    p.dataset.zone = zoneId;
                    
                    if (currentColor) {
                        p.style.background = currentColor;
                    } else {
                        // Дефолтные пресеты, если память чистая
                        if (['5', '6', '7', '8'].includes(zoneId)) {
                            p.style.background = zoneColors[zoneId];
                        } else if (zoneId !== '0') {
                            if (currentActiveSignal === 'waiting') p.style.background = '#4cd964'; // Ожидание = Зеленый
                            if (currentActiveSignal === 'charging') p.style.background = '#0055ff'; // Зарядка = Синий
                            if (currentActiveSignal === 'error') p.style.background = '#ff3b30'; // Авария = Красный
                        } else {
                            p.style.background = '#e8e8ed'; // Мертвые зоны
                        }
                    }
                }
            });

            if (!isTraceMode) {
                updateCanvasLockState();
                handleZoneVisibilityHighlight();
            }
        }

        // ПРЕДОХРАНИТЕЛЬ: БЛОКИРУЕТ ДИОДЫ ДЛЯ РИСОВАНИЯ, ЕСЛИ ОНИ НЕ БЫЛИ РАЗМЕЧЕНЫ НА ШАГЕ 1
        function updateCanvasLockState() {
            const isTraceMode = document.getElementById('view_matrix_toggle').checked;
            if (isTraceMode) return;

            const selectedPaintZone = document.getElementById('status_zone_select').value;
            
            // Показываем тумблер переливания радуги бренда только тогда, когда выбрана Зона 5
            document.getElementById('brand_mode_container').style.display = (selectedPaintZone === '5') ? 'flex' : 'none';

            document.querySelectorAll('.pixel').forEach(p => {
                const key = `${p.dataset.x}-${p.dataset.y}`;
                const actualZone = hardwareZonesMap[key] || '0';

                if (actualZone === selectedPaintZone) {
                    p.classList.remove('locked'); // Разрешено красить внутри своей зоны
                } else {
                    p.classList.add('locked');    // Запрещено красить чужие/пустые зоны
                }
            });
        }

        // АЛГОРИТМ ПОД СВЕТКИ АКТИВНОГО УЗЛА (СЕТКА НЕ СТАНОВИТСЯ НЕВИДИМОЙ)
        function handleZoneVisibilityHighlight() {
            const isTraceMode = document.getElementById('view_matrix_toggle').checked;
            const targetZone = isTraceMode ? document.getElementById('zone_select').value : document.getElementById('status_zone_select').value;
            
            let hasActiveZonePixels = false;
            document.querySelectorAll('.pixel').forEach(p => {
                if ((hardwareZonesMap[`${p.dataset.x}-${p.dataset.y}`] || '0') === targetZone) {
                    hasActiveZonePixels = true;
                }
            });

            document.querySelectorAll('.pixel').forEach(p => {
                const actualZone = hardwareZonesMap[`${p.dataset.x}-${p.dataset.y}`] || '0';
                
                if (targetZone === '0' || !hasActiveZonePixels) {
                    p.style.opacity = '1';
                } else {
                    p.style.opacity = (actualZone === targetZone) ? '1' : '0.25';
                }
            });
        }
        function initWebSocket() {
            ws = new WebSocket('ws://' + window.location.hostname + ':81/');
            ws.onclose = function() { setTimeout(initWebSocket, 2000); };
            
            ws.onmessage = function(event) {
                if (event.data.startsWith("MAP:")) {
                    const zones = event.data.substring(4).split(',');
                    let idx = 0;
                    for (let y = ROWS - 1; y >= 0; y--) {
                        for (let x = 0; x < COLS; x++) {
                            const key = `${x}-${y}`;
                            if (zones[idx]) {
                                hardwareZonesMap[key] = zones[idx];
                            }
                            idx++;
                        }
                    }
                    redrawCanvasByCurrentMode();
                }
            };
        }

        // ВОССТАНОВЛЕННЫЕ СЛУШАТЕЛИ СОБЫТИЙ ДЛЯ МЫШИ (ПК)
        grid.addEventListener('mousedown', (e) => {
            if (e.target.classList.contains('pixel')) {
                isDrawing = true;
                isDragActive = false;
                handleSingleClick(e.target);
            }
        });
        window.addEventListener('mouseup', () => isDrawing = false);
        grid.addEventListener('mouseover', (e) => {
            if (isDrawing && e.target.classList.contains('pixel')) {
                isDragActive = true;
                handlePixelPaint(e.target, false);
            }
        });

        // ВОССТАНОВЛЕННЫЕ СЛУШАТЕЛИ СОБЫТИЙ ДЛЯ ТАЧА (СМАРТФОНЫ)
        grid.addEventListener('touchstart', (e) => {
            isDrawing = true;
            isDragActive = false;
            if (e.touches && e.touches.length > 0 && e.touches[0].target) {
                const target = e.touches[0].target;
                if (target.classList.contains('pixel')) {
                    handleSingleClick(target);
                }
            }
        });
        grid.addEventListener('touchend', () => isDrawing = false);
        grid.addEventListener('touchmove', (e) => {
            if (!isDrawing) return;
            e.preventDefault();
            isDragActive = true;
            if (e.touches && e.touches.length > 0) {
                const touch = e.touches[0];
                const target = document.elementFromPoint(touch.clientX, touch.clientY);
                if (target && target.classList.contains('pixel')) {
                    handlePixelPaint(target, false);
                }
            }
        });

        // УПРАВЛЕНИЕ ОВЕРЛЕЙНЫМ ОКНОМ ШЕСТЕРЕНКИ
        function openSettingsModal() { toggleSettingsModal(true); }
        function toggleSettingsModal(show) {
            m_modal.style.display = show ? 'block' : 'none';
        }
        function closeSettingsModal(e) {
            if (e.target === m_modal) toggleSettingsModal(false);
        }

        // ИНИЦИАЛИЗАЦИЯ ВСЕХ СИСТЕМНЫХ ПОЛЕЙ С БЭКЕНДА ПЛАТЫ УСП-1
        function loadGlobalConfigFromDevice() {
            fetch('/get_config')
                .then(res => res.json())
                .then(cfg => {
                    document.getElementById('cfg_topology').value = cfg.topo;
                    document.getElementById('bright_ports_slide').value = cfg.b_ports;
                    document.getElementById('bright_ports_val').innerText = cfg.b_ports;
                    document.getElementById('bright_logo_slide').value = cfg.b_logo;
                    document.getElementById('bright_logo_val').innerText = cfg.b_logo;
                    
                    document.getElementById('cfg_is_dhcp').checked = (cfg.dhcp === 1);
                    document.getElementById('cfg_ip').value = cfg.ip || '';
                    document.getElementById('cfg_mask').value = cfg.mask || '';
                    document.getElementById('cfg_gw').value = cfg.gw || '';
                    toggleNetworkInputs();

                    // Восстанавливаем сохраненные независимые слои цветов
                    statusColorLayers["waiting"] = cfg.layers.wait || {};
                    statusColorLayers["charging"] = cfg.layers.charge || {};
                    statusColorLayers["error"] = cfg.layers.err || {};
                    
                    // Восстанавливаем аппаратную карту перегородок
                    hardwareZonesMap = cfg.hardware_map || {};

                    // Синхронизируем тумблер эффекта переливания радуги для бренда
                    document.getElementById('cfg_logo_anim').checked = (cfg.l_anim === 1);

                    redrawCanvasByCurrentMode();
                })
                .catch(() => {});
        }

        // ЗАПИСЬ АППАРАТНОЙ КАРТЫ ЖЕЛЕЗА ВО FLASH (ШАГ 1)
        function saveMatrixZones() {
            fetch('/save_zones', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(hardwareZonesMap)
            })
            .then(res => res.text())
            .then(data => {
                if (data === "OK") {
                    alert("Аппаратная разметка перегородок успешно зафиксирована во Flash-памяти!");
                    document.getElementById('view_matrix_toggle').checked = false;
                    m_panel.style.display = 'none';
                    redrawCanvasByCurrentMode();
                }
            });
        }

        // ЗАПИСЬ ГЛОБАЛЬНЫХ ПАРАМЕТРОВ СЕТИ И ТОПОЛОГИИ В НАСТРОЙКАХ (ШЕСТЕРЕНКА)
        function saveFullGlobalConfig() {
            const topo = document.getElementById('cfg_topology').value;
            const b_ports = document.getElementById('bright_ports_slide').value;
            const b_logo = document.getElementById('bright_logo_slide').value;
            const dhcp = document.getElementById('cfg_is_dhcp').checked ? 1 : 0;
            
            // ЗАЩИТА: Если поля пустые, принудительно отправляем безопасные заводские IP
            const ip = document.getElementById('cfg_ip').value || '192.168.1.150';
            const mask = document.getElementById('cfg_mask').value || '255.255.255.0';
            const gw = document.getElementById('cfg_gw').value || '192.168.1.1';

            const url = `/save_config?topo=${topo}&b_ports=${b_ports}&b_logo=${b_logo}&dhcp=${dhcp}&ip=${ip}&mask=${mask}&gw=${gw}`;

            fetch(url)
                .then(res => res.text())
                .then(data => {
                    if (data === "OK") {
                        alert("Глобальные параметры кастомизации и сети успешно применены!");
                        toggleSettingsModal(false);
                    }
                });
        }

        // ИМИТАЦИЯ ВХОДЯЩИХ СИГНАЛОВ С СИЛОВОЙ ПЛАТЫ УСП-1 (ОЖИДАНИЕ, ЗАРЯДКА, АВАРИЯ)
        function switchSimulatedSignal(mode) {
            currentActiveSignal = mode;
            
            // Сбрасываем и подсвечиваем активную симуляционную кнопку
            document.getElementById('btn_sig_wait').className = 'btn btn-signal' + (mode === 'waiting' ? ' active-wait' : '');
            document.getElementById('btn_sig_charge').className = 'btn btn-signal' + (mode === 'charging' ? ' active-charge' : '');
            document.getElementById('btn_sig_error').className = 'btn btn-signal' + (mode === 'error' ? ' active-error' : '');
            
            redrawCanvasByCurrentMode();

// Сквозной пинок бэкенду, чтобы FastLED применил текущую яркость к новому таску:
        }

        function toggleBrandRainbowMode() {
            const isRainbow = document.getElementById('cfg_logo_anim').checked ? 1 : 0;
            fetch(`/set_logo_anim?val=${isRainbow}`);
        }

        // КНОПКА ФИКСАЦИИ ЦВЕТОВОГО СЛОЯ ДЛЯ ТЕКУЩЕГО СТАТУСА (ШАГ 2)
        function saveCurrentStatusColors() {
            fetch('/save_status_colors', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    status: currentActiveSignal,
                    colors: statusColorLayers[currentActiveSignal]
                })
            })
            .then(res => res.text())
            .then(data => {
                if (data === "OK") {
                    alert(`Индивидуальная цветовая схема для режима "${currentActiveSignal}" успешно сохранена во Flash!`);
                }
            });
        }

        function handleSingleClick(pixelEl) {
            handlePixelPaint(pixelEl, true);
        }

        function handlePixelPaint(pixelEl, allowToggle) {
            const isTraceMode = document.getElementById('view_matrix_toggle').checked;
            const key = `${pixelEl.dataset.x}-${pixelEl.dataset.y}`;

            if (isTraceMode) {
                // ШАГ 1: Свободно размечаем аппаратную карту «железа»
                const zoneSelect = document.getElementById('zone_select');
                let zoneId = zoneSelect.value;
                let currentZone = hardwareZonesMap[key] || '0';
                
                if (allowToggle && !isDragActive && currentZone === zoneId) {
                    delete hardwareZonesMap[key];
                    pixelEl.dataset.zone = '0';
                    pixelEl.style.background = zoneColors['0'];
                    return;
                }
                
                hardwareZonesMap[key] = zoneId;
                pixelEl.dataset.zone = zoneId;
                pixelEl.style.background = zoneColors[zoneId];
            } else {
                // ШАГ 2: Рисуем цвета для статусов ТОЛЬКО внутри размеченных зон
                const selectedPaintZone = document.getElementById('status_zone_select').value;
                const actualZone = hardwareZonesMap[key] || '0';
                
                // Если ячейка не принадлежит выбранной зоне кастомизации — игнорируем клик (locked)
                if (actualZone !== selectedPaintZone) return;

                let paintColor = document.getElementById('current_paint_color').value;
                
                if (allowToggle && !isDragActive && statusColorLayers[currentActiveSignal][key] === paintColor) {
                    delete statusColorLayers[currentActiveSignal][key]; // Сброс к дефолтной маске режима
                    redrawCanvasByCurrentMode();
                    return;
                }

                statusColorLayers[currentActiveSignal][key] = paintColor;
                pixelEl.style.background = paintColor;
            }
        }

        // СКВОЗНАЯ ИЗОЛИРОВАННАЯ ЯРКОСТЬ ЯРКОСТЬ (ПРИМЕНЯЕТСЯ СРАЗУ КО ВСЕМ РЕЖИМАМ)
        function updateBrightness(type) {
            const isPorts = (type === 'ports');
            const slideId = isPorts ? 'bright_ports_slide' : 'bright_logo_slide';
            const valId = isPorts ? 'bright_ports_val' : 'bright_logo_val';
            let b = document.getElementById(slideId).value;
            document.getElementById(valId).innerText = b;
            fetch(`/set_bright?type=${type}&val=${b}`);
        }

        function clearMatrix() {
            const isTraceMode = document.getElementById('view_matrix_toggle').checked;
            if (isTraceMode) {
                hardwareZonesMap = {};
            } else {
                statusColorLayers[currentActiveSignal] = {};
            }
            redrawCanvasByCurrentMode();
        }

        // АСИНХРОННЫЙ ЗАГРУЗЧИК ПРОШИВКИ С ОТЛАЖЕННЫМ ВЫВОДОМ СТАТУСОВ ОШИБОК
        function startOtaUpdate() {
            const fileInput = document.getElementById('ota_file');
            if (fileInput.files.length === 0) return;

            const file = fileInput.files[0];
            const formData = new FormData();
            formData.append("update", file);

            const xhr = new XMLHttpRequest();
            const container = document.getElementById('ota_progress_container');
            const fill = document.getElementById('ota_progress_fill');
            const status = document.getElementById('ota_status');

            container.style.display = 'block';
            status.innerText = "Идет передача бинарного файла...";
            status.style.color = "#0055ff";

            xhr.upload.addEventListener("progress", (e) => {
                if (e.lengthComputable) {
                    const percent = Math.round((e.loaded / e.total) * 100);
                    fill.style.width = percent + "%";
                    status.innerText = `Загружено: ${percent}%`;
                }
            });

            xhr.addEventListener("load", () => {
                if (xhr.status === 200 && xhr.responseText === "OK") {
                    status.innerText = "Обновление завершено! Перезапуск контроллера...";
                    status.style.color = "#4cd964";
                    alert("Прошивка успешно записана. Контроллер УСП-1 уходит в перезагрузку.");
                } else {
                    status.innerText = "Ошибка записи! Некорректный firmware bin";
                    status.style.color = "#ff3b30";
                }
            });

            xhr.addEventListener("error", () => {
                status.innerText = "Сбой сети при передаче данных.";
                status.style.color = "#ff3b30";
            });

            xhr.open("POST", "/update");
            xhr.send(formData);
        }
    </script>
</body>
</html>
)=====";

#endif
