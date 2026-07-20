# Как устроена прошивка standalone LED-контроллера

Этот документ описывает фактическое устройство текущей прошивки из репозитория
`Test-led`. Он предназначен для начинающего разработчика и инженера, который
настраивает контроллер на объекте.

Документ привязан к текущим файлам и функциям проекта. Если поведение зависит от
реальной платы, матрицы или электрической схемы, это отмечено отдельно.

## 1. Общая цель прошивки

Прошивка превращает ESP32 в самостоятельный контроллер светодиодной матрицы
зарядной станции. У контроллера четыре основные задачи:

1. Читать восемь дискретных сигналов `Data1..Data8` через оптопары.
2. Определять состояние каждого активного порта: ожидание, зарядка или ошибка.
3. Показывать это состояние на назначенных участках физической LED-матрицы.
4. Давать инженеру web-интерфейс для настройки матрицы, сети, пароля и прошивки.

Главный источник состояния станции - реальные уровни на `Data1..Data8`. Web UI
не задает рабочий статус порта и не может включить аппаратный режим
предпросмотра. Это сделано специально: ошибка на входе должна иметь приоритет
над редактированием интерфейса.

Сейчас по умолчанию активны два порта:

- `Data1` и `Data2` управляют статусом Port1;
- `Data3` и `Data4` управляют статусом Port2;
- `Data5..Data8` подготовлены для Port3 и Port4, но эти порты пока находятся в
  резерве.

### Отличие от `proj1`

`proj1` - исходная большая прошивка зарядной станции. В ней были EVSE-функции,
OCPP, MQTT, RAPI, POS, общий тяжелый конфиг и более широкий web-интерфейс.
Текущий проект не является урезанной копией `proj1`. Это отдельная standalone-
прошивка для одной задачи: принять дискретные статусы и безопасно отобразить их
на LED-матрице.

Из `proj1` используются только идеи и справочная информация, например подходы к
Wi-Fi, OTA, авторизации и нумерации матрицы. EVSE/OCPP/MQTT/POS/RAPI в этот проект
не переносятся, пока для LED-контроллера нет отдельного требования.

### Термины, которые встретятся дальше

- Backend - код внутри ESP32: он читает входы, хранит настройки и отвечает на
  HTTP-запросы.
- Frontend или UI - страница и JavaScript, которые выполняются в браузере.
- Runtime - обычный рабочий режим после запуска.
- NVS - область flash-памяти ESP32 для настроек, переживающих перезагрузку.
- Task - отдельный повторяющийся цикл FreeRTOS внутри ESP32.
- Renderer - механизм, который выбирает цвет каждого пикселя.
- Endpoint - отдельный URL backend, например POST `/save_zones`.
- active-low - вход считается активным при низком уровне `LOW`.

Ниже эти термины разобраны подробнее в контексте конкретных функций.

## 2. Карта файлов проекта

### Документация и сборка

| Файл | Назначение |
| --- | --- |
| `README.md` | Краткое описание назначения, аппаратных сигналов и сборки. |
| `PROJECT_STATE.md` | Текущее проектное состояние, принятые решения, ограничения и backlog. |
| `AGENTS.md` | Правила работы с репозиторием. |
| `platformio.ini` | Окружение PlatformIO: плата `esp32dev`, Arduino framework и FastLED. |

### Код прошивки

| Файл | Что находится внутри | Кто использует |
| --- | --- | --- |
| `src/main.cpp` | `setup()`, `loop()`, web-сервер, HTTP-обработчики, OTA и задача чтения входов. | Это точка сборки всех модулей. Arduino вызывает `setup()` и `loop()`. |
| `src/board_config.h` | GPIO `Data1..Data8`, GPIO `LED1..LED4`, active-low и `DEFAULT_ACTIVE_PORT_COUNT`. | `station_inputs`, `status_mapper`, `led_manager`, diagnostics. |
| `src/station_inputs.h/.cpp` | Чтение GPIO и безопасный снимок `StationInputState`. | Задача входов, startup-анимация, diagnostics. |
| `src/status_mapper.h/.cpp` | Преобразование входов в `PortStatus`, карта входов порта, active/reserved. | LED renderer, diagnostics, metadata зон. |
| `src/zone_model.h` | Фиксированные `zoneId`, типы зон и режимы свободных зон. | Storage, backend, renderer и UI JSON. |
| `src/led_manager.h/.cpp` | Размер матрицы, topology, FastLED, startup-анимация, слои и механизм отрисовки. | `main.cpp` вызывает setup и безопасные функции применения настроек. |
| `src/matrix_config_storage.h/.cpp` | Загрузка, проверка и сохранение matrix config, zoneMap, status/free layers в NVS. | `led_manager` при старте и HTTP-обработчики при сохранении. |
| `src/network_config.h/.cpp` | STA, AP fallback, DHCP/static IP, watchdog, reconnect и async scan. | `main.cpp` и сетевые endpoints. |
| `src/auth_config.h/.cpp` | Basic Auth credentials и их NVS-хранилище. | Все защищенные HTTP-обработчики. |
| `src/gui_html.h` | Полная HTML/CSS/JavaScript-страница в `PAGE_MAIN` (`PROGMEM`). | `/` отдает страницу браузеру. JavaScript вызывает endpoints. |

### Важные структуры данных

| Структура/переменная | Где | Смысл |
| --- | --- | --- |
| `StationInputState` | `station_inputs.h` | Текущие raw-уровни и признаки активности `Data1..Data8`. |
| `StatusMapperState` | `status_mapper.h` | Количество активных портов, карта входов и итоговые статусы. |
| `MatrixConfig cfg` | `led_manager.cpp` | Topology, общая яркость портовых зон и базовые цвета статусов. |
| `uint8_t zoneMap[]` | `led_manager.cpp` | Для каждого физического LED хранит `zoneId`. |
| `FreeZoneConfig freeZoneConfigs[]` | `led_manager.cpp` | Режим, цвет и яркость Logo/QR/Service/Custom. |
| `NetworkConfig` | `network_config.h` | STA и AP параметры сети. |

### Карта источников истины

Источник истины - место, которому остальные части кода доверяют как актуальному:

| Данные | Источник истины |
| --- | --- |
| Электрические Data levels | Снимок `StationInputState` из `station_inputs` |
| Статусы и active/reserved ports | `StatusMapperState` из `status_mapper` |
| Topology, port brightness, fallback colors, zoneMap | `cfg` и `zoneMap` в RAM, восстановленные из `matrix_cfg` |
| Status layers | NVS keys `layer_wait/layer_chg/layer_err` и их caches в `led_manager` |
| Free-zone config/layers | `freeZoneConfigs` и free-layer caches, восстановленные из `led_settings` |
| Network | `NetworkConfig` и runtime state в `network_config` |
| Auth | Текущие credentials в `auth_config`, восстановленные из `auth_cfg` |

## 3. Где начинается выполнение

Arduino framework вызывает две функции из `src/main.cpp`:

- `setup()` - один раз после включения или reset;
- `loop()` - затем повторно, пока устройство работает.

### Порядок `setup()`

Текущая последовательность находится в `main.cpp`, функция `setup()`:

1. `Serial.begin(115200)` включает диагностический Serial.
2. `auth_config_setup()` загружает логин и пароль.
3. `station_inputs_setup()` настраивает `Data1..Data8` как `INPUT_PULLUP` и
   впервые читает их.
4. `status_mapper_setup()` создает карту портов и устанавливает число активных
   портов из `DEFAULT_ACTIVE_PORT_COUNT`.
5. `updatePortStatusFromInputs()` сразу вычисляет первые реальные статусы.
6. `led_setup()` настраивает FastLED, загружает matrix-настройки из NVS,
   выполняет startup-анимацию и запускает LED-задачу.
7. `startInputTask()` запускает отдельную задачу чтения входов каждые 50 мс.
8. `network_setup()` загружает сеть и неблокирующе начинает STA или поднимает
   AP fallback.
9. `server.on(...)` регистрирует URL web API.
10. `server.begin()` запускает HTTP-сервер на порту 80.

Сетевое подключение не ожидается блокирующим циклом. Если сохранен Wi-Fi, web-
сервер уже запущен, пока ESP32 подключается. Он станет доступен, когда у ESP32
появится доступный STA- или AP-интерфейс.

### Что делает `loop()`

`loop()` короткий:

- `server.handleClient()` обслуживает один HTTP-запрос;
- `network_loop()` обновляет сетевое состояние и watchdog;
- если отдельная input task не создалась, каждые 50 мс выполняется резервное
  чтение входов;
- после успешной OTA запускается отложенный `ESP.restart()`.

Основное чтение входов и отрисовка LED не зависят от скорости HTTP-обработчика:
они вынесены в отдельные задачи FreeRTOS. FreeRTOS task - это отдельный регулярно
выполняемый цикл внутри ESP32.

## 4. Полная цепочка Data -> статус -> матрица

### 4.1 GPIO и active-low

GPIO заданы в `src/board_config.h`:

| Сигнал | GPIO | Роль |
| --- | ---: | --- |
| Data1 | 16 | Port1 charging |
| Data2 | 17 | Port1 error |
| Data3 | 18 | Port2 charging |
| Data4 | 19 | Port2 error |
| Data5 | 21 | Port3 charging, резерв |
| Data6 | 25 | Port3 error, резерв |
| Data7 | 26 | Port4 charging, резерв |
| Data8 | 27 | Port4 error, резерв |

`BOARD_INPUT_ACTIVE_LOW = true` означает active-low: низкий электрический уровень
`LOW` считается активным сигналом, а `HIGH` - неактивным. Входы настроены как
`INPUT_PULLUP`, поэтому без активного сигнала они подтянуты вверх.

### 4.2 Чтение входов

`inputTaskWorker()` из `main.cpp` выполняется каждые 50 мс. Она вызывает:

1. `station_inputs_update()`;
2. `station_inputs_get_snapshot()`;
3. `status_mapper_update()`.

`station_inputs_update()` читает каждый GPIO через `digitalRead()`. В локальной
структуре запоминаются:

- `rawLevel[i]` - фактический `LOW` или `HIGH`;
- `active[i]` - уже переведенное логическое значение с учетом active-low.

После чтения готовый снимок коротко копируется в общую переменную под
критической секцией. Критическая секция - очень короткая защита от ситуации,
когда одна задача пишет структуру, а другая одновременно ее читает.

### 4.3 Преобразование в статус порта

`status_mapper_setup()` строит карту автоматически:

- Port1: charging input `0` (Data1), error input `1` (Data2), zoneId `1`;
- Port2: Data3/Data4, zoneId `2`;
- Port3: Data5/Data6, zoneId `3`;
- Port4: Data7/Data8, zoneId `4`.

Порты с индексом меньше `DEFAULT_ACTIVE_PORT_COUNT` включены. При текущем
значении `2` Port1 и Port2 активны, Port3 и Port4 получают статус `disabled`.

`status_mapper_update()` всегда проверяет error раньше charging:

```text
если error активен    -> ERROR
иначе если charging   -> CHARGING
иначе                 -> WAITING
```

Поэтому при одновременном charging и error результатом будет error.

### 4.4 Пример для Port1

| Data1 charging | Data2 error | Результат |
| --- | --- | --- |
| LOW | HIGH | `charging` |
| HIGH | LOW | `error` |
| LOW | LOW | `error`, потому что ошибка важнее зарядки |
| HIGH | HIGH | `waiting` |

Например, Data1 стал `LOW`, а Data2 остался `HIGH`:

1. `digitalRead(GPIO16)` возвращает `LOW`.
2. `station_inputs_update()` записывает `active[0] = true`.
3. Для Port1 `status_mapper_update()` видит error `false`, charging `true`.
4. `statuses[0]` становится `PORT_STATUS_CHARGING`.
5. LED-задача получает снимок `StatusMapperState`.
6. Каждый пиксель зоны Port1 окрашивается слоем charging либо базовым цветом
   charging.

### 4.5 Как статус попадает в LED

LED-задача `ledTaskWorker()` работает независимо и примерно каждые 15 мс
проверяет, пора ли обновить анимацию. Фактический вызов `led_refresh_internal()`
выполняется при обновлении пульса, примерно каждые 25 мс или немного реже.

Renderer - механизм отрисовки, то есть код, который для каждого физического LED
выбирает итоговый цвет. В проекте это `led_refresh_internal()`.

Он берет безопасный снимок `StatusMapperState`, проходит по `zoneMap[]` и для
портовой зоны выбирает текущий статус соответствующего порта.

## 5. Как работает матрица

### 5.1 Физический LED и координата

Физический LED имеет номер в последовательной цепочке WS2811: `0, 1, 2, ...`.
Этот номер определяет, в каком порядке данные уходят по одному проводу.

Инженеру удобнее работать с координатой на прямоугольнике:

- `x` - столбец;
- `y` - строка.

При текущих define матрица имеет `MATRIX_X = 12`, `VIRTUAL_Y = 8`, всего
`NUM_IC_CHIPS = 96` светодиодов. Размер задается при сборке в
`src/led_manager.h`, а не через UI.

### 5.2 Что такое zoneMap и zoneId

`zoneMap` - карта принадлежности пикселей зонам. В текущем NVS blob она хранится
как массив по физическим LED index. Значение каждого элемента - `zoneId`:

| zoneId | Значение |
| ---: | --- |
| 0 | Без зоны, LED выключен |
| 1 | Port1 |
| 2 | Port2 |
| 3 | Port3 |
| 4 | Port4 |
| 5 | Logo |
| 6 | QR |
| 7 | Service |
| 8 | Custom |

Zone 0 нужна для стертых и не назначенных пикселей. В UI ее нельзя выбрать как
рабочую зону, но ластик и команды очистки переводят пиксель именно в zone 0.
Renderer всегда показывает такой LED черным.

Один пиксель может принадлежать только одной зоне. Поэтому свободная зона не
накладывается поверх портовой зоны и не может скрыть ее error: пиксель либо
портовый, либо свободный.

### 5.3 Port zones

Port1..Port4 - зоны рабочих статусов. Для каждого их пикселя renderer:

1. находит номер порта по `zoneId`;
2. берет status из `status_mapper`;
3. пытается взять индивидуальный цвет пикселя из слоя выбранного статуса;
4. если для пикселя цвет не задан, берет общий fallback-цвет этого статуса.

Поведение статусов:

- waiting - цвет плавно пульсирует треугольным изменением яркости;
- charging - цвет горит постоянно;
- error - цвет мигает с периодом переключения 500 мс;
- disabled - пиксели резервного порта выключены.

Если настроенный error-цвет слишком темный, `visibleErrorColor()` подставляет
видимый красный. Это дополнительная защита от практически невидимой ошибки.

`bright_ports` - общая яркость всех портовых зон и всех трех статусов. Диапазон
backend: `1..255`.

### 5.4 Status layers

Status layer - сохраненная разреженная карта цветов для одного статуса. Она
хранит только окрашенные координаты, например:

```json
{"0-0":"#ff0000","1-0":"#0066ff"}
```

Слои `waiting`, `charging` и `error` независимы и сохраняются отдельно. Они
хранятся координатами `x-y`, а при загрузке переводятся в физические индексы
через текущую topology.

Слой может содержать цвета разных пикселей Port1 и Port2. Renderer применяет
его только к пикселям портовых зон. Если у конкретного пикселя нет записи в
слое, используется общий цвет `color_wait`, `color_charge` или `color_error`.

### 5.5 Free zones

Logo, QR, Service и Custom - свободные зоны. Для каждой сохраняются:

- `enabled`;
- режим `mode`;
- `staticColor`;
- собственная `brightness` от 1 до 255;
- разреженный `customLayer` для режима custom.

Режимы:

- `static` - вся свободная зона получает один цвет;
- `custom` - каждый нарисованный пиксель берет свой цвет, остальные выключены;
- `rainbow` - цвет вычисляется в runtime и движется по физическим LED.

Custom layer хранится координатами `x-y`. При загрузке цвет используется только
если эта координата по текущему `zoneMap` все еще принадлежит нужной free zone.

### 5.6 FastLED и физический вывод

FastLED - библиотека, которая формирует сигнал для цепочки WS2811.

В `led_setup()` используется:

```cpp
FastLED.addLeds<WS2811, LED_PIN, BRG>(leds, NUM_IC_CHIPS);
```

То есть основной выход - `LED1`, GPIO22, а порядок цветовых каналов сейчас
фиксирован как `BRG`. После заполнения массива `leds[]` функция `FastLED.show()`
отправляет цвета на физическую матрицу.

`MAX_LED_VOLTS = 12` и `MAX_LED_MILLIAMPS = 3000` задают FastLED ограничение
потребления LED-матрицы. Это не лимит зарядного тока станции.

### 5.7 Startup-анимация

При включении `led_run_startup_animation()` кратко показывает всей матрицей:

1. красный;
2. синий;
3. зеленый.

Каждый цвет длится 180 мс, максимум около 540 мс. Перед каждым цветом и каждые
20 мс во время показа напрямую перечитываются входы. Если активен error любого
активного порта, анимация не начинается или немедленно прерывается, после чего
запускается обычный renderer.

Анимация не изменяет NVS, zoneMap, status layers или free-zone настройки.

## 6. Как работает topology

Topology - правило, которое связывает координату `x-y` с физическим номером LED
в змейке. Она нужна, потому что матрицу можно начинать паять с разных углов и
вести змейку по столбцам или строкам.

Функции находятся в `led_manager.cpp`:

- `getLEDIndexForTopology(x, y, topology)` работает с явно переданным вариантом;
- `getLEDIndex(x, y)` использует текущую `cfg.topology`.

| Код | UI | Физический порядок |
| ---: | --- | --- |
| 0 | Вертикальная, старт справа снизу | Змейка по столбцам от правого нижнего угла |
| 1 | Вертикальная, старт слева снизу | Змейка по столбцам от левого нижнего угла |
| 2 | Горизонтальная, старт слева сверху | Змейка по строкам от левого верхнего угла |
| 3 | Горизонтальная, старт справа сверху | Змейка по строкам от правого верхнего угла |

### Смена topology

`zoneMap` физический, поэтому простая смена числа topology переместила бы зоны
на экране. Endpoint `/save_topology` делает remap:

1. для каждой логической координаты находит старый физический index;
2. читает `zoneId` из старого index;
3. находит новый физический index по новой topology;
4. записывает тот же `zoneId` в новый index;
5. сохраняет topology и новую `zoneMap` вместе в blob `matrix_cfg`;
6. только после успешной записи применяет их в RAM.

Поэтому визуальные координаты зон в редакторе остаются на месте, но физический
порядок вывода меняется под монтаж.

Status layers и free custom layers не remap-ятся: они уже хранятся по `x-y` и
при следующей загрузке переводятся через новую topology. UI блокирует сохранение
topology, если есть несохраненные изменения зон или слоев, и просит подтверждение.

Если topology выбрана неправильно, логическая картинка в браузере может выглядеть
правильно, а физическая матрица будет идти не с того угла или в другом направлении.
Hardware test pattern пока не реализован, поэтому вариант нужно подтвердить на
реальной матрице.

## 7. Как работает web UI

Весь UI хранится в `src/gui_html.h` как строка `PAGE_MAIN[] PROGMEM`. `PROGMEM`
означает, что большая HTML-страница хранится во flash, а не постоянно как копия
в обычной RAM. `handleRoot()` отдает ее браузеру через `server.send_P()`.

HTML, CSS и JavaScript исполняются в браузере инженера. ESP32 предоставляет
данные и принимает команды через endpoints. Endpoint - это URL web API, который
выполняет одну конкретную операцию, например `/save_zones`.

Основные вкладки:

- **Зоны матрицы** - назначение пикселей Port1..Port4 и free zones;
- **Статусы портов** - цвета waiting/charging/error и общая яркость портов;
- **Свободные зоны** - static/custom/rainbow и яркость каждой free zone;
- **Сервис** - Сеть, Безопасность, Прошивка, Диагностика.

### Что делают действия UI

| Действие | Что происходит |
| --- | --- |
| Сохранить зоны | Отправляет текущую координатную разметку в `/save_zones`. Backend переводит координаты в physical `zoneMap`, проверяет каждый active port, сохраняет `matrix_cfg` и сразу применяет карту без reboot. |
| Сохранить слой статуса | Отправляет выбранный waiting/charging/error и его `x-y` цвета в `/save_status_colors`. Каждый статус сохраняется отдельно и сразу применяется к runtime. |
| Сохранить настройки свободной зоны | Отправляет mode, color, brightness и при custom - рисунок только выбранной free zone в `/save_free_zone`. После успешной записи renderer обновляется без reboot. |
| Яркость портов | Через 500 мс задержки после ввода (debounce) либо после отпускания ползунка отправляет только `bright_ports` в `/set_bright`. После ответа физическая яркость меняется сразу; кнопка сохранения слоя не нужна. |
| Яркость free zone | Аналогично автоматически отправляет только `zoneId` и `brightness` в `/save_free_zone`. Остальные несохраненные параметры не затрагиваются; новая физическая яркость применяется после успешного save. |
| Применить яркость ко всем | Последовательно отправляет только brightness для всех размеченных свободных зон и сразу обновляет их после успешных записей. |
| Сохранить topology | После проверки несохраненных изменений и подтверждения вызывает `/save_topology`; backend remap-ит `zoneMap` и сразу применяет результат. |
| Сохранить и подключиться | Сначала вызывает `/save_network`, затем `/network_reconnect`. Новая сеть применяется через 1,5 секунды без reboot, поэтому текущая web-сессия может оборваться. |
| Сохранить пароль | Вызывает `/save_auth`. Новые credentials действуют сразу; следующие запросы требуют новый Basic Auth. |
| Загрузить прошивку | Отправляет `.bin` multipart-запросом в `/update`; после успеха ESP32 перезагружается. |

Изменения редактора сначала существуют только в браузере. Dirty-индикатор
показывает, что они не сохранены. Status/free save блокируются, если `zoneMap`
изменена, но еще не сохранена, чтобы backend не применил слой к старой карте.

Яркость не затемняет browser preview: предпросмотр остается читаемым. Значение
яркости влияет на сохраненную и физическую матрицу после успешного запроса.

### Локальный mock mode

Если извлечь страницу и открыть ее как обычный `index.html` без ESP32,
`/get_config` недоступен. UI создает mock-конфигурацию 12x8 с двумя активными
портами. Изменения остаются в браузере, dirty-индикаторы не означают запись в
контроллер. Сообщения прямо говорят, что контроллер недоступен.

Mock mode выбирается только при первоначальной недоступности backend. Временная
ошибка `/diagnostics` на реальном устройстве показывает потерю связи, но не
переключает рабочую сессию навсегда в mock mode.

## 8. API / endpoints

Все перечисленные endpoints защищены Basic Auth. Для `/update` авторизация
проверяется до приема firmware bytes. UI вызывает чтение через GET, изменения -
через POST.

| Endpoint | Метод | Кто вызывает | Принимает / возвращает | Где реализован |
| --- | --- | --- | --- | --- |
| `/` | GET | Браузер | Возвращает `PAGE_MAIN` | `main.cpp`: `handleRoot()` |
| `/get_size` | GET | UI как fallback размера | Текст `cols:rows` | `handleGetMatrixSize()` |
| `/get_config` | GET | UI при загрузке/reload | Matrix size/topology, activePortCount, colors, layers, zone metadata, zoneMap и free zones | `handleGetConfig()` |
| `/diagnostics` | GET в UI | Header и вкладка Diagnostics | Raw Data1..8, active flags, ports/statuses, LED outputs, runtime mode | `handleDiagnostics()` |
| `/save_zones` | POST JSON | Кнопка «Сохранить зоны» | Объект `{"x-y": zoneId}`; сохраняет `matrix_cfg` | `handleSaveZones()` |
| `/save_topology` | POST form/JSON | Кнопка topology | `topology=0..3`; remap и сохраняет topology+zoneMap | `handleSaveTopology()` |
| `/set_bright` | POST form | Автосохранение яркости портов | `type=ports`, `val=1..255`; сохраняет `bright_ports` | `handleSetBright()` |
| `/save_status_colors` | POST JSON | Сохранение выбранного status layer | `status`, `colors`; сохраняет один слой и при наличии первый fallback-цвет | `handleSaveStatusColors()` |
| `/save_free_zone` | POST JSON | Полное сохранение зоны, автояркость, общая яркость | Обязательный `zoneId`; только brightness или полный config/customLayer | `handleSaveFreeZone()` |
| `/network_status` | GET | Network UI | Состояние connecting/sta/reconnecting/ap_fallback, IP, SSID, RSSI и config | `handleNetworkStatus()` |
| `/scan_networks` | GET | Кнопка scan | Сначала HTTP 202, затем JSON-массив SSID/RSSI/channel/security | `handleScanNetworks()` |
| `/save_network` | POST form | «Сохранить и подключиться» | STA, DHCP/static и AP fallback поля; сохраняет `net_cfg` | `handleSaveNetworkConfig()` |
| `/network_reconnect` | POST | Та же кнопка после save | Без payload; планирует применение через 1500 мс | `handleNetworkReconnect()` |
| `/auth_status` | GET | Security UI | Username и `defaultCredentials` | `handleAuthStatus()` |
| `/save_auth` | POST form | Security UI | `username`, `password`, `confirm`; сохраняет `auth_cfg` | `handleSaveAuth()` |
| `/update` | POST multipart | Firmware UI | Файл `.bin`; пишет OTA partition и планирует reboot | inline handlers в `setup()` |

### Validation и ошибки сохранения

- `/save_zones` отклоняет неверный JSON, координаты/zoneId вне диапазона,
  дубликаты и карту без хотя бы одного пикселя каждого активного порта.
- `/save_topology` принимает только `0..3` и применяет RAM только после успешной
  записи.
- brightness принимается только в диапазоне `1..255`.
- status/free layers ограничены текущими `MATRIX_X`, `VIRTUAL_Y`, количеством
  пикселей, валидным `#RRGGBB` и отсутствием повторных координат.
- custom free layer дополнительно ограничен пикселями выбранной free zone.
- storage-функции читают записанное значение обратно. При ошибке endpoints
  возвращают HTTP 500, а не ложный `OK`.
- Для multi-key операций status/free/network/auth есть best-effort rollback.
  NVS не дает общей транзакции между несколькими ключами, поэтому отключение
  питания ровно между двумя записями остается небольшим остаточным риском.

## 9. Где и как хранятся настройки

NVS (Non-Volatile Storage) - область flash-памяти ESP32 для небольших настроек.
Она сохраняется при выключении питания и перепрошивке OTA, если разметка flash
не стирается.

Настройки разделены на namespaces - независимые именованные группы.

### Namespace `led_settings`

| Key | Содержимое |
| --- | --- |
| `matrix_cfg` | Один бинарный `StoredMatrixConfig`: magic, version, size, matrixX/Y, `MatrixConfig` и физический `zoneMap[]`. |
| `free_zones_v1` | Бинарный массив настроек Logo/QR/Service/Custom с magic/version/size/dimensions. |
| `layer_wait` | JSON `x-y -> color` для waiting. |
| `layer_chg` | JSON для charging. |
| `layer_err` | JSON для error. |
| `fz5_layer` | Custom layer Logo. |
| `fz6_layer` | Custom layer QR. |
| `fz7_layer` | Custom layer Service. |
| `fz8_layer` | Custom layer Custom. |

`matrix_cfg` содержит:

- topology;
- `bright_ports`;
- fallback-цвета waiting/charging/error;
- zoneMap.

### Namespace `net_cfg`

| Key | Содержимое |
| --- | --- |
| `ssid`, `pass` | Сохраненная STA-сеть и пароль. |
| `dhcp` | Получать адрес автоматически или использовать static. |
| `ip`, `gw`, `mask`, `dns` | Static IP, gateway, subnet, DNS. |
| `ap_ssid`, `ap_pass` | Имя и пароль recovery AP. |

### Namespace `auth_cfg`

| Key | Содержимое |
| --- | --- |
| `version` | Версия auth config, сейчас 1. |
| `username` | Логин Basic Auth. |
| `password` | Пароль Basic Auth в открытом виде в NVS. |

### Пустая или невалидная память

Matrix config проверяется по magic/version/size, compile-time dimensions,
topology, brightness и zoneId. Если blob отсутствует, старый, поврежден или
создан для другого размера матрицы:

- удаляются `matrix_cfg`, `free_zones_v1`, три status layer и четыре free layer;
- в RAM создаются defaults;
- network и auth namespaces не затрагиваются.

Default `zoneMap` делит физическую цепочку на равные последовательные участки по
числу `DEFAULT_ACTIVE_PORT_COUNT`. Сейчас это Port1 и Port2. Default topology -
0, port brightness - 120, waiting - зеленый, charging - синий, error - красный.

Free-zone defaults: enabled, brightness 100; Logo белый, QR зеленый, Service
оранжевый, Custom фиолетовый. Их пиксели на первом старте не видны, потому что
default zoneMap полностью занят активными port zones.

Если только `free_zones_v1` невалиден, сбрасываются free configs и free custom
layers; matrix config и status layers остаются.

Auth config при отсутствии/ошибке использует `admin/admin` и пытается записать
эти defaults. Network config при отсутствии использует пустой STA SSID, DHCP и
уникальный AP default в RAM. Невалидный network config также заменяется defaults
в RAM до следующего корректного сохранения.

Defaults matrix/free не обязательно сразу записываются обратно при чтении. Они
работают в RAM и попадут в NVS при соответствующем успешном сохранении из UI.

### Что не сохраняется

Не сохраняются:

- текущие электрические уровни Data1..Data8;
- вычисленные PortStatus;
- фаза waiting pulse, rainbow и error blink;
- startup-анимация;
- dirty edits браузера до нажатия save;
- временное network runtime state;
- результаты Wi-Fi scan.

## 10. Что происходит после reboot

Полная последовательность:

1. ESP32 начинает `setup()`.
2. Загружает `auth_cfg`; при ошибке включает `admin/admin`.
3. Настраивает и читает Data1..Data8.
4. Создает mapping Port1..Port4 и вычисляет первый статус.
5. FastLED настраивает LED1/GPIO22.
6. `matrix_config_load()` загружает topology, brightness, colors и zoneMap.
7. `matrix_config_load_free_zones()` загружает free-zone config.
8. Загружаются три status layer и четыре free custom layer.
9. Startup-анимация выполняется, если нет активного error.
10. Выполняется первая runtime-отрисовка.
11. Запускается LED task.
12. Запускается input task с периодом 50 мс.
13. `network_setup()` загружает `net_cfg` и запускает STA или AP fallback.
14. Регистрируются endpoints и запускается HTTP server.

Все сохраненные matrix/network/auth настройки восстанавливаются. Реальные
PortStatus вычисляются заново по входам; они не берутся из NVS.

## 11. Как работает сеть

### Термины

- STA - режим клиента: ESP32 подключается к существующему роутеру.
- AP fallback - собственная Wi-Fi точка ESP32 для восстановления доступа.
- DHCP - роутер автоматически выдает ESP32 IP.
- Static IP - адрес задан вручную в настройках.

### Старт без SSID

Если `ssid` пустой, сразу поднимается AP fallback:

- имя по умолчанию `LED_MATRIX_XXXXXX`;
- `XXXXXX` - нижние 24 бита стабильного chip/eFuse MAC ID;
- пароль по умолчанию `12345678`;
- IP ESP32 в AP mode: `192.168.4.1`.

Сохраненный пользовательский `ap_ssid` не перетирается уникальным default.

### Старт с сохраненным SSID

`network_setup()` вызывает `WiFi.begin()` и сразу возвращается. Состояние -
`connecting`. LED/input tasks продолжают работать.

- при успехе состояние становится `sta`, UI доступен по STA IP;
- если подключения нет 12 секунд, включается `ap_fallback`;
- если в AP нет подключенного клиента, раз в 60 секунд выполняется новая
  12-секундная попытка STA.

Постоянный AP+STA не используется. Во время повторной STA-попытки AP временно
выключается; при неудаче возвращается.

### Потеря роутера

Watchdog проверяет сеть раз в 500 мс. При потере успешной STA:

1. состояние становится `reconnecting`;
2. `WiFi.reconnect()` повторяется примерно раз в 5 секунд;
3. на восстановление дается 45 секунд;
4. затем поднимается AP fallback с причиной `connection_lost`.

Если роутер вернулся в пределах 45 секунд, ESP32 снова становится STA. Runtime
матрицы не зависит от этого процесса.

### Асинхронный scan

`/scan_networks` запускает framework scan в неблокирующем режиме. Пока scan идет,
endpoint отвечает HTTP 202, а UI опрашивает результат примерно каждые 500 мс.
Backend отменяет зависший scan через 15 секунд; UI ожидает до 14 секунд.

В AP fallback только на время scan включается AP+STA, после завершения режим
возвращается в AP-only. Чтение Data продолжает отдельная task.

### DHCP и static

При DHCP роутер выбирает IP. При static перед `WiFi.begin()` вызывается
`WiFi.config(staticIp, gateway, subnet, dns)`. Неверные, но синтаксически
корректные адреса могут лишить инженера доступа через STA; recovery AP должен
вернуть локальный доступ после timeout.

Hostname задается как `led-controller`, но mDNS в проекте не запущен. Поэтому
`http://led-controller.local` не гарантируется.

После STA-подключения IP можно узнать в Network UI, Serial log, DHCP leases
роутера или заранее использовать static IP/DHCP reservation. Через AP fallback
UI и OTA доступны по `http://192.168.4.1`.

## 12. Авторизация и OTA

### Basic Auth

Basic Auth - встроенная в HTTP проверка логина и пароля. При первом открытии `/`
браузер показывает системный запрос credentials.

Defaults:

- login: `admin`;
- password: `admin`.

`requireHttpAuth()` вызывается во всех актуальных handlers. `/auth_status`
возвращает `defaultCredentials: true`, пока одновременно используются
`admin/admin`; UI показывает предупреждение.

После `/save_auth` новые credentials сразу находятся в RAM и NVS. Следующий
запрос со старыми данными получит новый запрос авторизации. Пароль должен иметь
6..64 символа, username - 1..32.

Ограничение: Basic Auth работает по обычному HTTP, а пароль лежит в NVS открытым
текстом. Для локальной/VPN MVP-сети это функционально, но не равно защите HTTPS.

### Web OTA

OTA - обновление прошивки без USB, через сеть.

1. Инженер открывает Сервис -> Прошивка.
2. Выбирает непустой файл `.bin`.
3. Браузер POST-ит его в `/update`.
4. Backend проверяет Basic Auth до записи bytes.
5. `Update.begin()`, `Update.write()` и `Update.end(true)` записывают firmware.
6. Только при полном успехе endpoint отвечает `OK` и через одну секунду вызывает
   `ESP.restart()`.

Это local web OTA. Из другого города оно работает только если инженер имеет
сетевой маршрут к самой ESP32, например через VPN объекта. Дискретные оптопары
Data1..Data8 не могут передавать firmware.

В отличие от более широкой `proj1`, здесь нет OCPP/cloud firmware update,
update-by-URL, MQTT-команды обновления или ArduinoOTA. Также пока нет signed
firmware и автоматического rollback.

## 13. Типовые сценарии

### 13.1 Первый запуск новой платы

1. Включить питание.
2. Матрица кратко покажет red/blue/green, если нет error.
3. Port1 и Port2 начнут показывать статусы на default-разметке.
4. Найти Wi-Fi `LED_MATRIX_XXXXXX`.
5. Подключиться с паролем `12345678`.
6. Открыть `http://192.168.4.1`.
7. Войти как `admin/admin` и затем сменить пароль.

### 13.2 Настройка Wi-Fi

1. В Сервис -> Сеть нажать «Сканировать сети».
2. Выбрать или ввести SSID.
3. Ввести Wi-Fi password; для обычного роутера оставить DHCP включенным.
4. Нажать «Сохранить и подключиться».
5. Через 1,5 секунды AP может исчезнуть, пока ESP32 переходит в STA.
6. Узнать STA IP в роутере/Serial либо из статуса до потери доступной сессии.
7. Открыть `http://<STA-IP>`.

Пустой password при том же SSID означает «не менять сохраненный пароль». При
смене SSID пустой password становится паролем новой сети, то есть подходит только
для открытой сети.

### 13.3 Разметка Port1/Port2

1. Открыть «Зоны матрицы».
2. Выбрать Port1 и нарисовать его пиксели.
3. Выбрать Port2 и нарисовать его пиксели.
4. Ластиком убрать лишнее в zone 0 «Без зоны».
5. Убедиться, что предупреждения об активных портах исчезли.
6. Нажать «Сохранить зоны».

Backend не примет карту, если хотя бы один активный порт не имеет ни одного
пикселя. Несохраненная разметка остается только в браузере и не меняет матрицу.

### 13.4 Настройка status colors

1. Открыть «Статусы портов».
2. Выбрать waiting, charging или error.
3. Выбрать цвет кисти и нарисовать слой на портовых пикселях.
4. Нажать «Сохранить слой статуса».
5. Повторить отдельно для остальных статусов.
6. Общую яркость портов менять slider/input - она сохраняется автоматически.

Runtime сразу использует сохраненный слой, когда входы дают соответствующий
статус. Error priority не меняется.

### 13.5 Настройка Logo/QR/free zone

1. Сначала назначить этой free zone пиксели в «Зоны матрицы» и сохранить карту.
2. Открыть «Свободные зоны» и выбрать размеченную зону.
3. Выбрать static, custom или rainbow.
4. Для static выбрать цвет; для custom нарисовать пиксели.
5. Нажать «Сохранить настройки свободной зоны».
6. Яркость можно менять отдельно - она сохранится автоматически.

Если zoneMap dirty или активные порты невалидны, UI требует сначала сохранить
корректную разметку.

### 13.6 Смена topology

1. Сохранить все dirty zone/status/free edits.
2. Открыть «Настройки матрицы».
3. Выбрать один из четырех вариантов и проверить browser numbering preview.
4. Нажать «Сохранить топологию» и подтвердить.
5. Backend remap-ит physical zoneMap, логические зоны останутся на местах.
6. Проверить реальную матрицу и reboot.

### 13.7 Смена пароля

1. Открыть Сервис -> Безопасность.
2. Ввести username, новый password и подтверждение.
3. Сохранить.
4. Обновить страницу и войти с новыми credentials.

Физического reset auth сейчас нет, поэтому сохраненный пароль нельзя забывать.

### 13.8 OTA

1. Открыть UI локально, через AP fallback либо через VPN объекта.
2. Открыть Сервис -> Прошивка.
3. Выбрать `firmware.bin` именно для `esp32dev` и текущей разметки flash.
4. Не отключать питание и сеть во время upload.
5. Дождаться сообщения об успехе и reboot.

### 13.9 Неверный Wi-Fi

1. После «Сохранить и подключиться» текущий AP временно пропадет.
2. Через 12 секунд неудачного старта ESP32 снова поднимет AP fallback.
3. Подключиться к AP и открыть `192.168.4.1`.
4. Исправить SSID/password или static IP.

### 13.10 Изменение activePortCount с 2 на 3

1. Изменить только `DEFAULT_ACTIVE_PORT_COUNT` в `board_config.h`.
2. Собрать и прошить firmware.
3. Status mapper, `/get_config`, diagnostics и UI автоматически покажут Port3
   активным; Data5/Data6 начнут определять его status.
4. Если NVS `matrix_cfg` от той же версии и размера валиден, старая zoneMap
   сохранится. Она может не содержать Port3.
5. В этом случае Port3 логически работает, но физически не виден. UI покажет
   warning, а `/save_zones` не позволит сохранить карту без Port3.
6. Инженер должен вручную выделить хотя бы один пиксель Port3 и сохранить зоны.

Новый default split на три порта создается только при пустом/невалидном
`matrix_cfg`, а не автоматически при одном изменении activePortCount.

## 14. Где что менять разработчику

| Задача | Где менять | Что проверить дополнительно |
| --- | --- | --- |
| Количество активных портов | `board_config.h`: `DEFAULT_ACTIVE_PORT_COUNT` | UI warning, zones для нового порта, Data mapping и железо. |
| GPIO Data/LED | `board_config.h`: `BOARD_INPUT_PINS`, `LED_OUTPUT_PINS` | Схему платы, boot-strapping GPIO, active-low. |
| Размер матрицы | `led_manager.h`: `MATRIX_X`, `VIRTUAL_Y` | NVS matrix/layers сбросятся; UI desktop/mobile; RAM для caches. |
| Тип LED/color order | `led_manager.cpp`: `FastLED.addLeds<WS2811,...,BRG>` | Реальную ленту и порядок RGB. |
| Лимит LED power | `led_manager.h`: `MAX_LED_VOLTS`, `MAX_LED_MILLIAMPS` | Блок питания, проводку, максимальную нагрузку. |
| Default status colors/port brightness | `matrix_config_storage.cpp`: `matrix_config_set_defaults()` | Mock defaults в `gui_html.h` должны соответствовать. |
| Default free-zone colors/brightness | `matrix_config_set_free_zone_defaults()` | Mock config и UI. |
| Default AP name/password | `network_config.cpp`: `DEFAULT_AP_SSID_PREFIX`, `DEFAULT_AP_PASSWORD` | Требования production security. |
| Network timeouts | `network_config.cpp`: `STA_*`, `AP_FALLBACK_*`, `NETWORK_*` | Реальный роутер и время recovery. |
| Input period | `main.cpp`: `INPUT_TASK_PERIOD_MS` | Нагрузка CPU и дребезг входов. |
| UI-тексты/layout/JS | `gui_html.h` | Browser syntax, desktop/mobile, mock и real backend. |
| Fixed zone IDs | `zone_model.h` | Backend, storage, renderer и UI зависят от ID; менять только отдельным этапом. |
| Новый free-zone mode | `zone_model.h`, storage validation, `led_manager.cpp`, backend и UI | Это не локальная правка одного enum. |

Команда сборки из корня `Test-led`:

```powershell
pio run -e esp32dev
```

После изменения compile-time размера старый matrix blob не проходит проверку
dimensions. Прошивка очистит matrix/status/free данные и загрузит defaults, но не
затронет `net_cfg` и `auth_cfg`.

## 15. Ограничения и риски

### Обязательно проверить на железе

- соответствие GPIO реальной плате и полярности оптопар;
- устойчивость уровней `INPUT_PULLUP` и необходимость внешней подтяжки;
- дребезг/помехи Data inputs и нужна ли временная фильтрация;
- все четыре комбинации charging/error для Port1 и Port2;
- реальный приоритет error и скорость реакции около 50 мс;
- topology, физический старт цепочки и порядок `BRG`;
- ток, нагрев и FastLED power cap на полной белой матрице;
- startup animation с error до включения и во время анимации;
- сохранение/reload/reboot всех NVS-настроек;
- STA/AP fallback, потерю роутера, reconnect и async scan;
- OTA по STA, AP и при необходимости через VPN.

### Текущие ограничения

- Программного debounce/filter входов нет. Добавлять его нужно только после
  измерений на реальных оптопарах, чтобы не задержать настоящую ошибку.
- Basic Auth идет без HTTPS; default `admin/admin` и AP password `12345678`
  необходимо менять перед эксплуатацией.
- Пароли хранятся в NVS открытым текстом.
- Нет физического factory reset и auth recovery.
- NVS - не полноценная транзакционная БД. Read-back и rollback уменьшают риск,
  но внезапное питание между multi-key commits может оставить несогласованность.
- Matrix size задается compile-time. Runtime resize и remap между размерами нет.
- При увеличении activePortCount существующая zoneMap не распределяется заново.
- Используется только LED1. Multi-matrix через LED2..LED4 пока нет.
- Brightness schedule по времени суток отложен до проверки реальной яркости.
- GUI большой и находится в одном header. Он занимает flash; JSON `String` и
  ответы `/get_config`/`diagnostics` используют heap. Для 12x8 это приемлемо по
  коду, но нужны hardware soak-тесты памяти и повторных запросов.
- OTA не проверяет цифровую подпись и не реализует application rollback.
- Нет captive portal и mDNS discovery.
- Нет cloud/OCPP/MQTT remote OTA. VPN дает маршрут к обычному web OTA, но не
  является функцией самой ESP32.
- Hardware test pattern topology пока сознательно отсутствует.

### Что не нужно переносить из `proj1`

Без отдельного требования не нужны EVSE control, OCPP, MQTT, RAPI, POS, legacy
ConfigJson и старый тяжелый UI. Они увеличат Flash/RAM и количество источников
истины, но не улучшат чтение Data1..Data8 или надежность LED-индикации.

## 16. Глоссарий

| Термин | Простое объяснение |
| --- | --- |
| Data-вход | GPIO ESP32, на который через оптопару приходит дискретный сигнал станции. |
| active-low | Сигнал считается активным при электрическом уровне LOW. |
| PortStatus | Итоговое состояние порта: waiting, charging, error или disabled. |
| renderer | Код `led_refresh_internal()`, который выбирает итоговый цвет каждого LED. |
| FastLED | Библиотека отправки массива цветов в физическую цепочку WS2811. |
| FreeRTOS task | Отдельный повторяющийся цикл внутри ESP32; здесь отдельно работают inputs и LEDs. |
| NVS | Flash-хранилище небольших настроек, переживающее reboot. |
| endpoint | URL функции backend, например POST `/save_zones`. |
| STA | ESP32 как клиент существующего Wi-Fi роутера. |
| AP fallback | Собственная recovery Wi-Fi сеть ESP32, когда STA недоступна. |
| DHCP | Автоматическая выдача IP роутером. |
| static IP | IP, gateway, mask и DNS, заданные вручную. |
| topology | Правило перевода координаты `x-y` в номер LED физической змейки. |
| zoneMap | Массив принадлежности каждого физического пикселя определенной зоне. |
| zoneId | Числовой ID зоны от 0 до 8. |
| status layer | Разреженная карта цветов `x-y` для waiting, charging или error. |
| free zone | Независимая зона Logo/QR/Service/Custom с static/custom/rainbow. |
| OTA | Загрузка новой `.bin` прошивки через web-интерфейс без USB. |

## Краткая схема

```text
Питание
  -> setup()
  -> auth + Data inputs + status mapper
  -> matrix/free/status config из NVS
  -> безопасная startup-анимация
  -> input task (50 мс) + LED task
  -> неблокирующий STA или AP fallback
  -> HTTP server и web UI

Data LOW/HIGH
  -> StationInputState
  -> StatusMapperState
  -> renderer + zoneMap + layers
  -> FastLED.show()
  -> физическая матрица

Изменение в UI
  -> dirty state в браузере
  -> POST endpoint
  -> validation
  -> NVS write + read-back
  -> применение в RAM/renderer
  -> reboot восстанавливает настройку из NVS
```
