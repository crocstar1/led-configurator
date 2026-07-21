# Методический разбор функций прошивки LED-контроллера

Этот документ объясняет, как работает код текущей прошивки `Test-led` на уровне
конкретных функций, переменных, условий и вызовов. Он привязан к актуальному
состоянию исходников на момент создания документа.

## 1. Как читать этот документ

Файл [`FIRMWARE_WALKTHROUGH_RU.md`](FIRMWARE_WALKTHROUGH_RU.md) объясняет общую
архитектуру и путь данных через устройство. Этот документ идет глубже: он
показывает, что происходит внутри методов, какие временные переменные создаются,
какие глобальные данные читаются и когда управление переходит в другой файл.

Новичку лучше читать разделы в таком порядке:

1. Карта вызовов и `main.cpp`.
2. `station_inputs` и `status_mapper`.
3. `led_manager` и `matrix_config_storage`.
4. Сеть, авторизация и UI.
5. Сквозные сценарии и синтаксис.

### Термины

- **`setup()`** - функция Arduino, которая выполняется один раз после включения
  или перезагрузки ESP32.
- **`loop()`** - функция Arduino, которая затем вызывается снова и снова.
- **Задача FreeRTOS (`FreeRTOS task`)** - отдельный постоянный цикл внутри ESP32.
  В проекте отдельно работают задача входов и задача LED.
- **Снимок (`snapshot`)** - цельная копия текущего состояния. Читатель получает
  копию, а не ссылку на структуру, которую другая задача может менять.
- **Мьютекс (`mutex`)** - программный замок: пока одна задача меняет LED-данные,
  другая не может одновременно использовать их.
- **Обработчик (`handler`)** - C++-функция, которую web-сервер вызывает для
  конкретного HTTP-запроса.
- **Endpoint** - адрес такого запроса, например POST `/save_zones`.
- **Callback** - функция, переданная другой системе для вызова позже. Например,
  upload callback вызывается web-сервером при получении очередной части `.bin`.
- **Renderer** - `led_refresh_internal()`, механизм выбора итогового цвета
  каждого физического LED.
- **NVS** - область flash-памяти ESP32 для настроек, переживающих reboot.
- **Heap** - динамическая оперативная память, из которой создаются `String` и
  крупные JSON-ответы.
- **Backend** - C++-код внутри ESP32, который читает входы, хранит настройки и
  отвечает на HTTP-запросы.
- **Frontend/UI** - HTML/CSS/JavaScript, выполняющиеся в браузере инженера.
- **RAM** - оперативная память, содержимое которой теряется при reboot.
- **Flash** - постоянная память с прошивкой и NVS.
- **Cache** - подготовленная копия данных в RAM для быстрого renderer без чтения
  NVS на каждом кадре.
- **State machine** - логика, которая хранит текущее состояние и разрешенные
  переходы; в проекте так работает сеть.

## 2. Карта вызовов верхнего уровня

```text
Arduino запускает setup() один раз
  |
  +-> auth_config_setup()
  +-> station_inputs_setup()
  +-> status_mapper_setup()
  +-> updatePortStatusFromInputs()
  +-> led_setup()
  |     +-> matrix/free/layers load из NVS
  |     +-> startup animation
  |     +-> LED_Task
  +-> startInputTask()
  |     +-> Input_Task каждые 50 мс
  +-> network_setup()
  +-> регистрация server.on(...)
  +-> server.begin()

После setup() Arduino постоянно вызывает loop()
  |
  +-> server.handleClient() -> HTTP handler при запросе UI
  +-> network_loop() -> watchdog/reconnect/apply
  +-> fallback чтения входов, если Input_Task не создалась
  +-> ESP.restart() после успешной OTA

Параллельно:
  Input_Task -> station_inputs -> status_mapper
  LED_Task   -> status snapshot -> renderer -> FastLED.show()
```

Сборку описывает `platformio.ini`: environment `esp32dev`, Arduino framework и
единственная внешняя библиотека `FastLED ^3.6.0`. Wi-Fi, WebServer, Update,
Preferences и FreeRTOS приходят из ESP32 Arduino framework.

### Какие функции вызываются один раз

- `auth_config_setup()`;
- `station_inputs_setup()`;
- `status_mapper_setup()`;
- `led_setup()`;
- `startInputTask()`;
- `network_setup()`;
- регистрация endpoints и `server.begin()`.

### Какие функции работают постоянно

- `loop()`;
- `inputTaskWorker()` с периодом 50 мс;
- `ledTaskWorker()` с задержкой 15 мс;
- `network_loop()` внутри Arduino loop;
- JavaScript timers в открытом браузере.

### Что запускается запросом браузера

`server.handleClient()` распознает URL и вызывает зарегистрированный handler.
Например, POST `/save_zones` вызывает `handleSaveZones()`. Handler выполняется в
контексте Arduino loop, но чтение Data и LED renderer продолжаются в отдельных
задачах.

## 3. Разбор `src/main.cpp`

`main.cpp` соединяет все модули. Здесь находится `WebServer server(80)`, запуск
задачи входов, JSON helpers, HTTP handlers, OTA, `setup()` и `loop()`.

### 3.1 Глобальные переменные OTA

В начале файла объявлены:

```cpp
bool otaUploadAuthorized = false;
bool otaUpdateSuccess = false;
bool otaRestartPending = false;
String otaUpdateError = "";
unsigned long otaRestartAt = 0;
```

Они живут все время работы прошивки. Upload callback меняет их по мере загрузки
файла, финальный handler формирует HTTP-ответ, а `loop()` выполняет отложенный
reboot. Это связь между несколькими callback одного OTA-запроса.

### 3.2 `updatePortStatusFromInputs()`

Функция в `main.cpp:26` выполняет один полный цикл входов:

1. `station_inputs_update()` читает восемь GPIO.
2. Локально создается `StationInputState inputs`.
3. `station_inputs_get_snapshot(inputs)` копирует в него цельное состояние.
4. `status_mapper_update(inputs)` вычисляет статусы портов.

Локальная `inputs` уничтожается после выхода из функции. Глобальное состояние
хранят модули `station_inputs` и `status_mapper`.

### 3.3 `inputTaskWorker()` и `startInputTask()`

`INPUT_TASK_PERIOD_MS = 50` задает период чтения.

`inputTaskWorker(void *)`:

- получает текущий FreeRTOS tick в `lastWake`;
- переводит 50 мс в ticks через `pdMS_TO_TICKS()`;
- входит в бесконечный `while (true)`;
- вызывает `updatePortStatusFromInputs()`;
- `vTaskDelayUntil()` выдерживает стабильный период относительно `lastWake`.

`startInputTask()` вызывает `xTaskCreatePinnedToCore()`:

- имя задачи: `Input_Task`;
- stack: 2048 bytes;
- priority: 2;
- core: 1.

Результат записывается в `inputTaskStarted`. Если создание не удалось,
`loop()` выполняет ту же работу раз в 50 мс. Это резервный путь, а не второй
одновременный читатель.

### 3.4 Basic Auth helpers

`authRequestAuthenticated()` передает текущие login/password из `auth_config` в
`server.authenticate()`.

`requireHttpAuth()`:

```text
если credentials верны -> true
иначе server.requestAuthentication() -> false
```

Каждый handler начинает с `if (!requireHttpAuth()) return;`. Поэтому при
неверных credentials остальной код метода вообще не выполняется.

### 3.5 JSON и цветовые helpers

В `main.cpp:69-263` находятся небольшие парсеры:

- `parseHexColor()` переводит `#RRGGBB` в три `uint8_t`;
- `rgbToHex()` делает обратное;
- `isValidHexColor()` проверяет формат;
- `jsonEscape()` экранирует внешние строки для JSON;
- `extractJsonStringField()`, `extractJsonIntField()` и
  `extractJsonObjectField()` извлекают ожидаемые поля;
- `parseStrictInt()` не принимает мусор после числа;
- `canonicalStatusName()` сводит aliases к `waiting/charging/error`;
- `parseFreeZoneMode()` принимает только три поддерживаемых режима.

Это не универсальный JSON parser. Helpers рассчитаны на небольшие, строго
ограниченные payload текущего API. Перед сохранением storage выполняет повторную
нормализацию слоев.

### 3.6 Формирование `/get_config`

Четыре функции дополняют один JSON:

- `appendHardwareMapJson()` перебирает все `x/y`, переводит их через
  `getLEDIndex()` и возвращает ненулевые зоны как `"x-y":"zoneId"`;
- `appendStatusLayersJson()` загружает три status layer из NVS;
- `appendFreeZoneConfigsJson()` добавляет config и custom layer каждой free zone;
- `appendZoneMetadataJson()` строит active/reserved metadata от актуального
  `StatusMapperState`, а не от статических флагов Port2..4.

`handleGetConfig()` создает локальный `String json` и заранее резервирует 7600
bytes через `json.reserve(7600)`. Это уменьшает количество перераспределений
heap. Затем метод читает глобальные `cfg`, `zoneMap`, `freeZoneConfigs` и snapshot
status mapper, добавляет поля и отвечает HTTP 200 `application/json`.

Он ничего не меняет и ничего не сохраняет.

### 3.7 `/` - `handleRoot()`

- **Кто вызывает:** браузер при открытии IP контроллера.
- **Вход:** только HTTP GET и Basic Auth.
- **Локальные переменные:** нет.
- **Читает:** `PAGE_MAIN` из `gui_html.h`.
- **Меняет:** ничего.
- **Вызывает:** `server.send_P(200, "text/html", PAGE_MAIN)`.
- **Ошибка:** неверная авторизация дает запрос Basic Auth.

`send_P` читает HTML из flash/PROGMEM, а не создает постоянную RAM-копию всей
страницы.

### 3.8 `/get_size` - `handleGetMatrixSize()`

Это небольшой compatibility/fallback endpoint. Локальный `String sizeStr`
строится как `MATRIX_X:VIRTUAL_Y`, например `12:8`. UI использует его, только
если основной `/get_config` не дал полноценный `matrix` объект.

### 3.9 `/diagnostics` - `handleDiagnostics()`

Метод создает два локальных snapshots:

```cpp
StationInputState inputs;
StatusMapperState status;
```

Затем:

1. Копирует raw/active входы и статусы.
2. Создает `String json`, резервируя 1400 bytes.
3. Добавляет `activePortCount`, `runtimeMode`, primary LED output.
4. Циклом `for` по `LED_OUTPUT_COUNT` добавляет LED1..LED4.
5. Циклом по `BOARD_INPUT_COUNT` добавляет Data1..Data8, GPIO, LOW/HIGH и active.
6. Циклом по `MAX_PORT_COUNT` добавляет mapping и status Port1..Port4.
7. Возвращает HTTP 200 JSON.

Метод не перечитывает GPIO сам: он показывает последний безопасный snapshot от
Input_Task. Временная ошибка diagnostics не включает mock mode в UI.

### 3.10 `/save_zones` - `handleSaveZones()`

- **Кто вызывает:** кнопка «Сохранить зоны».
- **Payload:** JSON вида `{"0-0":"1","1-0":"2"}`. Отсутствующая координата
  означает zone 0.
- **Локальные данные:** `String body` и стековый массив
  `uint8_t nextZoneMap[NUM_IC_CHIPS] = {}`.

Последовательность:

1. `parseZoneMapPayload()` проверяет JSON, координаты, zoneId и дубликаты.
2. Каждая координата переводится в физический index текущей topology.
3. `zoneMapHasRequiredActivePortZones()` проверяет хотя бы один пиксель каждого
   активного порта.
4. `matrix_config_save(cfg, nextZoneMap, ...)` записывает общий `matrix_cfg` и
   читает его обратно.
5. Только после успешной NVS-записи `led_replace_zone_map_safe()` меняет RAM.
6. `led_reload_free_zone_layers_safe()` заново фильтрует custom free layers по
   новой принадлежности зон.
7. Возвращается `OK`.

Ошибки: HTTP 400 `BAD_JSON`/`ACTIVE_PORT_ZONE_EMPTY`; HTTP 500 `SAVE_FAILED`.
Старый глобальный `zoneMap` остается рабочим, если сохранение не удалось.

### 3.11 `/save_topology` - `handleSaveTopology()`

Handler принимает topology как form field либо JSON. Локальные переменные:

- `int topology = -1`;
- `MatrixConfig nextConfig = cfg` - копия текущего config;
- `uint8_t nextZoneMap[NUM_IC_CHIPS] = {}` - новая физическая карта.

После проверки диапазона `0..3`:

1. В копии меняется только `nextConfig.topology`.
2. `led_remap_zone_map_for_topology()` переносит каждый zoneId через логические
   координаты из старого physical index в новый.
3. `matrix_config_save(nextConfig, nextZoneMap)` одним blob сохраняет topology и
   карту.
4. `led_apply_matrix_config_safe()` под LED mutex заменяет `cfg`, `zoneMap`,
   перечитывает caches слоев и перерисовывает матрицу.

Status/free layers не переписываются в NVS: они уже хранят ключи `x-y`.
Ошибки: `BAD_TOPOLOGY`, `REMAP_FAILED`, `SAVE_FAILED`.

### 3.12 `/set_bright` - `handleSetBright()`

UI POST-ит form fields `type=ports` и `val=1..255`.

Handler создает `const String type`, `int parsedValue` и копию
`MatrixConfig nextConfig = cfg`. Он меняет только `nextConfig.bright_ports`,
записывает полный `matrix_cfg`, затем присваивает `cfg = nextConfig` и вызывает
`led_refresh_safe()`.

Если NVS save не прошел, глобальный `cfg` не меняется. Endpoint не зависит от
кнопки сохранения status layer.

### 3.13 `/save_status_colors` - `handleSaveStatusColors()`

Payload:

```json
{"status":"charging","colors":{"0-0":"#0055ff"}}
```

Важные локальные переменные:

- `body` - исходный JSON;
- `status` - canonical status;
- `colorsJson` - вложенный объект;
- `normalizedLayer` - проверенная компактная версия слоя;
- `nextConfig` - копия `cfg`;
- `firstColor` и `parsedColor[3]` - первый цвет слоя для общего fallback;
- `previousLayer` - предыдущая NVS-версия для rollback.

Метод сначала полностью проверяет размер и структуру payload. Затем сохраняет
слой выбранного статуса. Если слой содержит хотя бы один цвет, первый цвет также
становится global fallback `color_wait/color_charge/color_error`. Для этого может
потребоваться второе сохранение `matrix_cfg`.

Если запись слоя успешна, а запись config нет, handler пытается вернуть
`previousLayer`. Это best-effort rollback: попытка вернуть предыдущее значение,
но не полноценная транзакция базы данных.

После успеха обновляется `cfg` и вызывается `led_reload_status_layers_safe()`.
Каждый waiting/charging/error сохраняется отдельным нажатием.

### 3.14 `/save_free_zone` - `handleSaveFreeZone()`

Этот handler поддерживает два сценария:

1. Полный payload выбранной зоны: enabled, mode, staticColor, brightness и
   customLayer.
2. Частичный payload `{zoneId, brightness}` для autosave яркости.

Сначала извлекается и проверяется `zoneId`. Затем создается локальная копия всех
конфигов:

```cpp
FreeZoneConfig nextConfigs[FREE_ZONE_COUNT];
FreeZoneConfig &nextZone = nextConfigs[zoneIndex];
```

Все поля применяются только к `nextZone`. `hasConfigPayload` отмечает наличие
обычных полей. Если есть `customLayer`, он полностью нормализуется и проверяется
против текущего `zoneMap` до первой NVS-записи.

Custom layer сохраняется только когда итоговый mode равен `FREE_ZONE_CUSTOM`.
Затем сохраняется массив free-zone configs. Если второй save не удался, handler
пытается вернуть предыдущий custom layer. Только после успеха копия
`nextConfigs` переносится в глобальный `freeZoneConfigs`.

Brightness-only payload не содержит mode/color/layer и поэтому не может
случайно сохранить другие browser edits.

### 3.15 `/network_status` - `handleNetworkStatus()`

Handler получает `const NetworkConfig &net = network_config_get()`. Символ `&`
означает ссылку: большая структура не копируется, а доступна только для чтения.

Он формирует JSON с:

- runtime mode и connection status;
- configured/connected;
- сохраненным и реально подключенным SSID;
- IP/RSSI;
- DHCP/static fields;
- AP SSID, AP IP и числом клиентов.

SSID проходит `jsonEscape()`. Метод не возвращает Wi-Fi/AP passwords.

### 3.16 `/scan_networks` - `handleScanNetworks()`

Первый запрос:

1. Видит, что scan не активен.
2. Вызывает `network_scan_start()`.
3. Отвечает HTTP 202 `{"status":"scanning"}` и `Retry-After: 1`.

Следующие запросы читают `WiFi.scanComplete()`:

- `WIFI_SCAN_RUNNING` -> снова 202;
- `WIFI_SCAN_FAILED` -> завершение и HTTP 500;
- число `>= 0` -> цикл по найденным сетям.

В цикле создаются JSON-поля SSID, RSSI, channel, secure. После ответа
`network_scan_finish()` освобождает результаты scan.

### 3.17 `/save_network` - `handleSaveNetworkConfig()`

Сначала создается копия текущей конфигурации:

```cpp
NetworkConfig next = network_config_get();
```

Handler читает form fields. Пустой Wi-Fi password сохраняет старый пароль, если
SSID не изменился. При новом SSID пустой пароль становится пустым паролем новой
сети. Для static mode каждое поле парсится в локальный `IPAddress ip`.

После полной проверки вызывается `network_config_save(next, error)`. Эта функция
проверяет NVS read-back и при сбое пытается вернуть предыдущую config. Endpoint
возвращает HTTP 500 для NVS errors и HTTP 400 для пользовательских данных.

Важно: этот handler только сохраняет. Текущая кнопка UI затем отдельно вызывает
`/network_reconnect`.

### 3.18 `/network_reconnect` - `handleNetworkReconnect()`

Метод не получает payload. Он вызывает `network_schedule_apply(1500)` и сразу
отвечает `RECONNECT_SCHEDULED`. Через 1,5 секунды `network_loop()` применит
конфигурацию. Задержка дает браузеру время получить HTTP-ответ до смены Wi-Fi.

### 3.19 `/auth_status` и `/save_auth`

`handleAuthStatus()` формирует небольшой JSON из текущего username и результата
`auth_config_uses_default_credentials()`.

`handleSaveAuth()` читает form fields `username`, `password`, `confirm`,
проверяет непустые значения и совпадение password. Подробную длину и NVS save
проверяет `auth_config_save_credentials()`. После успеха следующие HTTP-запросы
уже требуют новые credentials.

### 3.20 `/update` - OTA handlers

`server.on("/update", HTTP_POST, finalHandler, uploadCallback)` регистрирует две
функции-callback.

Upload callback получает `HTTPUpload &upload` и реагирует на состояния:

- `UPLOAD_FILE_START`: проверяет auth, имя и `.bin`, вызывает `Update.begin()`;
- `UPLOAD_FILE_WRITE`: пишет очередной `upload.buf` через `Update.write()`;
- `UPLOAD_FILE_END`: проверяет totalSize и вызывает `Update.end(true)`;
- `UPLOAD_FILE_ABORTED`: фиксирует ошибку и вызывает `Update.abort()`.

Final handler после завершения возвращает HTTP 200 только если
`otaUpdateSuccess && !Update.hasError()`. Затем ставит
`otaRestartPending = true` и время `millis() + 1000`. Reboot выполняет `loop()`,
а не upload callback.

### 3.21 `setup()`

`setup()` в `main.cpp:1087` выполняется строго сверху вниз:

1. `Serial.begin(115200)`.
2. `auth_config_setup()`.
3. `station_inputs_setup()`.
4. `status_mapper_setup()`.
5. Первое `updatePortStatusFromInputs()`.
6. `led_setup()` с NVS load и startup animation.
7. `startInputTask()`.
8. `network_setup()`.
9. Регистрация всех handlers.
10. `server.begin()`.

LED runtime и input task запускаются до ожидания результата STA. Сам
`network_setup()` не содержит 12-секундного blocking loop.

### 3.22 `loop()`

Каждый проход:

1. `server.handleClient()` обслуживает HTTP.
2. `network_loop()` делает короткий шаг state machine.
3. При `!inputTaskStarted` с локальной `static` переменной времени выполняется
   резервное чтение входов.
4. При наступлении `otaRestartAt` вызывается `ESP.restart()`.

`static unsigned long lastFallbackInputUpdate` сохраняет значение между
вызовами `loop()`. Обычная локальная переменная обнулилась бы на каждом проходе.

## 4. Разбор `src/board_config.h`

Этот header содержит аппаратные compile-time константы. Compile-time означает:
значения становятся частью `.bin`; изменить их через UI нельзя.

### Константы количества

- `BOARD_INPUT_COUNT = 8` - число Data inputs.
- `LED_OUTPUT_COUNT = 4` - LED1..LED4 на плате.
- `MAX_PORT_COUNT = 4` - максимальное число портов в массивах.
- `DEFAULT_ACTIVE_PORT_COUNT = 2` - реально активны Port1/Port2.

Для трех портов меняется одна константа на `3`, затем нужна новая сборка и
прошивка. Mapping автоматически включит Data5/Data6. Но существующая NVS-zoneMap
не перераспределится: инженер должен разметить Port3.

### GPIO arrays

`BOARD_INPUT_PINS[8]` хранит GPIO16,17,18,19,21,25,26,27. Индекс массива важен:
index 0 - Data1, index 1 - Data2 и так далее.

`LED_OUTPUT_PINS[4]` хранит GPIO22,23,32,33. Сейчас
`PRIMARY_LED_OUTPUT_INDEX = 0`, поэтому `PRIMARY_LED_PIN` равен GPIO22.

`BOARD_INPUT_ACTIVE_LOW = true` сообщает `station_inputs`, что LOW надо
преобразовать в `active=true`.

### Где находится размер матрицы

Размер **не находится** в `board_config.h`. Он задан в `led_manager.h`:

```cpp
#define MATRIX_X 12
#define VIRTUAL_Y 8
#define NUM_IC_CHIPS (MATRIX_X * VIRTUAL_Y)
```

При изменении размера пересобираются массивы `leds`, `zoneMap` и layer caches.
Старый `matrix_cfg` другого размера будет признан несовместимым и matrix/status/
free storage сбросится к defaults. Network/auth не очищаются.

## 5. Разбор `station_inputs`

### 5.1 `StationInputState`

Структура в `station_inputs.h` содержит два массива по восемь элементов:

```cpp
bool active[BOARD_INPUT_COUNT];
uint8_t rawLevel[BOARD_INPUT_COUNT];
```

`rawLevel` хранит электрический `LOW/HIGH`. `active` хранит удобный логический
результат после учета active-low.

### 5.2 Модульное состояние и защита

В `station_inputs.cpp`:

```cpp
static StationInputState inputState = {};
static portMUX_TYPE inputStateMux = portMUX_INITIALIZER_UNLOCKED;
```

`static` здесь скрывает переменные внутри этого `.cpp`. Другие файлы не могут
напрямую изменить `inputState`.

`portMUX_TYPE` используется с `portENTER_CRITICAL`/`portEXIT_CRITICAL`. Это
короткая защита копирования структуры между задачами. Здесь нет обычного LED
mutex, потому что защищенная операция очень короткая и не должна ждать.

### 5.3 `station_inputs_setup()`

Цикл `for` проходит индексы `0..7` и вызывает:

```cpp
pinMode(BOARD_INPUT_PINS[i], INPUT_PULLUP);
```

После настройки сразу вызывается `station_inputs_update()`, поэтому до запуска
задачи уже существует реальное первое состояние.

### 5.4 `station_inputs_update()`

Сначала создается локальная нулевая структура `nextState`. Затем цикл:

1. Читает `level = digitalRead(pin)`.
2. Вычисляет `active`: при active-low это `level == LOW`.
3. Записывает оба значения в `nextState[i]`.

После завершения всех восьми чтений критическая секция одним присваиванием
заменяет `inputState = nextState`. Читатель никогда не видит ситуацию, когда
Data1 уже новый, а Data2 еще от прошлого цикла.

### 5.5 `station_inputs_get_snapshot()`

Функция принимает `StationInputState &snapshot`. `&` означает ссылку на
переменную вызывающего кода. Под защитой она копирует туда весь `inputState`.

После выхода вызывающая задача работает со своей копией. Она не держит lock во
время JSON-сборки или renderer logic.

### 5.6 Пример Data1/Data2

Если Data1 LOW, Data2 HIGH:

```text
nextState.rawLevel[0] = LOW
nextState.active[0]   = true
nextState.rawLevel[1] = HIGH
nextState.active[1]   = false
```

Если Data2 также станет LOW, `active[1]` станет true в следующем цикле максимум
примерно через 50 мс.

### 5.7 Почему чтение не осталось в `loop()`

HTTP upload, JSON и network handlers могут занять Arduino loop дольше обычного.
Отдельная priority-2 task продолжает читать Data независимо. Это особенно важно
для error. Если task не создалась, loop fallback сохраняет работоспособность, но
уже не дает той же изоляции от долгих handlers.

## 6. Разбор `status_mapper`

### 6.1 Enum и структуры

`PortStatus` может быть:

- `PORT_STATUS_WAITING`;
- `PORT_STATUS_CHARGING`;
- `PORT_STATUS_ERROR`;
- `PORT_STATUS_DISABLED`.

`PortInputMapping` хранит для одного порта индексы charging/error входов,
`zoneId` и `enabled`.

`StatusMapperState` содержит:

- `activePortCount`;
- массив `ports[MAX_PORT_COUNT]`;
- массив `statuses[MAX_PORT_COUNT]`.

Как и inputs, модуль хранит `static mapperState` и защищает его собственным
`portMUX_TYPE mapperStateMux`.

### 6.2 `status_mapper_setup()`

Локальная `nextState` обнуляется, затем получает
`activePortCount = DEFAULT_ACTIVE_PORT_COUNT`.

Цикл по четырем портам вычисляет mapping формулами:

```cpp
chargingInput = i * 2;
errorInput    = i * 2 + 1;
zoneId        = i + 1;
enabled       = i < activePortCount;
```

Для `i=0` получаются Data1/Data2 и zone1. Для `i=1` - Data3/Data4 и zone2.
Активные порты начинают с waiting, резервные - disabled. Затем структура целиком
публикуется под critical section.

### 6.3 `status_mapper_update()`

Метод сначала получает snapshot текущего mapper, чтобы сохранить mapping и
activePortCount. Для каждого порта:

1. Пересчитывает `enabled`.
2. Если порт disabled, записывает `PORT_STATUS_DISABLED` и переходит к следующему.
3. Читает `inputs.active[errorInput]`.
4. Читает `inputs.active[chargingInput]`.
5. Сначала проверяет error, затем charging, иначе waiting.

Порядок `if` и создает safety priority:

```cpp
if (errorActive) ERROR;
else if (chargingActive) CHARGING;
else WAITING;
```

После цикла новый state публикуется целиком.

### 6.4 Кто читает mapper

- `led_refresh_internal()` берет статусы для renderer;
- `/diagnostics` показывает их инженеру;
- `/get_config` получает `activePortCount` и строит zone metadata;
- `/save_zones` проверяет зоны всех active ports;
- startup animation проверяет error.

`status_mapper_status_to_string()` переводит enum в JSON-строки.

## 7. Разбор `led_manager`

`led_manager` владеет физическим LED-выводом и текущим матричным состоянием.

### 7.1 Глобальные данные

В `led_manager.cpp` находятся:

- `CRGB leds[NUM_IC_CHIPS]` - итоговый цвет каждого физического LED;
- `uint8_t zoneMap[NUM_IC_CHIPS]` - zoneId каждого physical index;
- `MatrixConfig cfg` - topology, port brightness и три fallback-цвета;
- `FreeZoneConfig freeZoneConfigs[FREE_ZONE_COUNT]`;
- `StatusLayerCache waitLayer/chargeLayer/errorLayer`;
- `FreeZoneLayerCache freeZoneLayers[FREE_ZONE_COUNT]`;
- `SemaphoreHandle_t ledMutex` - основной замок для согласованного изменения
  LED state и renderer caches.

Каждый cache содержит два массива: `hasColor[index]` и `colors[index]`. Первый
показывает, задан ли индивидуальный цвет, второй хранит сам `CRGB`.

Topology/zoneMap применяются через полностью mutex-защищенный метод. В текущих
handlers port brightness, fallback status colors и массив free configs сначала
копируются в globals, после чего вызывается безопасный reload/refresh. Это очень
короткое окно, но формально не все такие присваивания находятся внутри mutex.
Перед промышленной приемкой этот участок стоит проверить отдельным concurrency
аудитом; документ не скрывает это фактическое поведение.

### 7.2 `led_setup()`

Последовательность:

1. `xSemaphoreCreateMutex()` создает LED mutex.
2. `FastLED.addLeds<WS2811, LED_PIN, BRG>()` настраивает LED1/GPIO22, тип WS2811
   и порядок цветовых каналов BRG.
3. `FastLED.setMaxPowerInVoltsAndMilliamps(12, 3000)` задает power cap матрицы.
4. `led_load_config_from_flash()` загружает NVS и очищает физический output.
5. `led_run_startup_animation()` показывает red/blue/green.
6. `led_refresh_internal()` сразу рисует реальный runtime.
7. `xTaskCreatePinnedToCore()` запускает `LED_Task`: stack 4096, priority 1,
   core 1.

Power cap относится к потреблению LED, а не к зарядному току EVSE.

### 7.3 `led_load_config_from_flash()`

Это `static` функция, доступная только внутри `led_manager.cpp`. Она вызывает:

1. `matrix_config_load(cfg, zoneMap, ...)`;
2. `matrix_config_load_free_zones(freeZoneConfigs, ...)`;
3. `led_reload_status_layers_unlocked()`;
4. `led_reload_free_zone_layers_unlocked()`.

Затем цикл делает все `leds[i] = CRGB::Black` и вызывает `FastLED.show()`.
Слово `unlocked` означает: функция сама не берет mutex; вызывающий код должен
знать, что конкурентная LED task еще не стартовала или lock уже удерживается.

### 7.4 Startup animation

`startup_error_active()` прямо перечитывает station inputs, обновляет mapper и
циклом проверяет statuses активных портов. Она не ждет будущий Input_Task,
потому что animation выполняется до ее запуска.

`startup_show_color(color)`:

1. Сначала проверяет error.
2. `fill_solid()` записывает один цвет во весь `leds`.
3. `FastLED.show()` отправляет его.
4. До 180 мс выполняет короткие `delay(20)`.
5. После каждой задержки снова проверяет error.

`led_run_startup_animation()` последовательно вызывает ее для Red, Blue, Green.
Возврат `false` немедленно прекращает цепочку. После этого `led_setup()` все
равно вызывает runtime renderer, поэтому error получает рабочую индикацию.

### 7.5 `getLEDIndexForTopology()`

Входы: логические `x`, `virtualY` и код topology. При координате вне размера
возвращается `-1`.

Функция использует временные `tx` и `ty`, чтобы развернуть оси, и `switch` по
четырем topology:

- 0: vertical snake, старт справа снизу;
- 1: vertical snake, старт слева снизу;
- 2: horizontal snake, старт слева сверху;
- 3: horizontal snake, старт справа сверху.

Проверка четности столбца/строки (`% 2`) меняет направление каждого следующего
прохода и создает змейку. `getLEDIndex(x,y)` - короткая обертка, которая передает
текущую `cfg.topology`.

### 7.6 `led_remap_zone_map_for_topology()`

Метод принимает исходный и целевой массивы указателями. В начале он проверяет:

- указатели не `nullptr`;
- source и target не один массив;
- оба размера равны `NUM_IC_CHIPS`;
- topology находятся в `0..3`.

Локальный `bool assigned[NUM_IC_CHIPS]` отслеживает, что каждый новый physical
index заполнен ровно один раз. Вложенные циклы `y/x` делают:

```text
oldIndex = coordinate по старой topology
newIndex = та же coordinate по новой topology
targetZones[newIndex] = sourceZones[oldIndex]
```

Второй цикл проверяет все `assigned`. Метод не меняет globals и NVS; он только
готовит target. Применение выполняет handler после успешного save.

### 7.7 Загрузка слоев в caches

`parseStatusLayerJson()` очищает cache, перебирает пары `"x-y":"#RRGGBB"`,
переводит coordinate через текущую topology и ставит `hasColor[index] = true`.

`parseFreeZoneLayerJson()` делает то же, но дополнительно проверяет
`zoneMap[index] == zoneId`. Запись custom color вне своей free zone остается
неиспользованной.

`led_reload_status_layers_unlocked()` загружает три NVS key. Аналогичная
`led_reload_free_zone_layers_unlocked()` проходит циклом четыре free zones.

### 7.8 `ledTaskWorker()`

Задача работает в `while (true)`:

1. Сохраняет `now = millis()`.
2. Пытается взять `ledMutex` максимум на 10 ticks.
3. Если прошло больше 25 мс, меняет `breatheScale` на `0.012` вверх или вниз.
4. При достижении `1.0`/`0.15` переключает `breatheDirectionUp`.
5. Вызывает `led_refresh_internal()`.
6. Освобождает mutex.
7. Делает `vTaskDelay(15 ms)`.

Если lock занят HTTP handler, задача пропускает один проход, а не читает
наполовину обновленный config.

### 7.9 `led_refresh_internal()` - renderer

Это центральная функция физической индикации.

В начале:

- `rainbowHue` - локальная `static` фаза rainbow, увеличивается на каждом render;
- `pulseBright` вычисляется из `breatheScale`, минимум 40;
- создаются `CRGB customWait/customCharge/customError` из `cfg`;
- ко всем трем применяется `cfg.bright_ports`;
- создается snapshot `StatusMapperState portState`.

Затем цикл `for (int i = 0; i < NUM_IC_CHIPS; i++)` рассматривает каждый physical
LED. Для одного `i` решение выглядит так.

#### Случай zone 0

`zoneMap[i] == 0` -> `leds[i] = CRGB::Black`. Такой пиксель физически выключен.

#### Случай port zone

1. `portIndex = zone - ZONE_ID_PORT1`.
2. Из snapshot берется `zoneStatus`.
3. Disabled -> Black.
4. `getStatusLayerColorForStatus()` ищет per-pixel цвет текущего статуса.
5. Если он есть, применяется `bright_ports`.
6. Для error цвет проходит `visibleErrorColor()` и мигает по
   `(millis()/500)%2`.
7. Для waiting дополнительно применяется `pulseBright`.
8. Charging показывается постоянно.
9. Если layer color нет, используется уже масштабированный global fallback.

`visibleErrorColor()` суммирует RGB. Если сумма меньше 40, подставляется чистый
красный, чтобы ошибка не стала практически черной.

#### Случай free zone

`freeZoneConfigFor(zone)` находит config:

- config отсутствует или disabled -> Black;
- rainbow -> `CHSV(rainbowHue + i*4, 255, brightness)`;
- custom -> цвет из free cache, затем per-zone brightness; отсутствующий pixel
  layer -> Black;
- static -> `staticColor`, масштабированный per-zone brightness.

После обработки всех 96 элементов один `FastLED.show()` отправляет целый кадр.
Он не вызывается отдельно на каждом pixel.

### 7.10 Безопасные методы изменения LED state

- `led_refresh_safe()` берет mutex и вызывает renderer.
- `led_replace_zone_map_safe()` меняет только zoneMap и перерисовывает.
- `led_apply_matrix_config_safe()` одновременно меняет cfg+zoneMap, перечитывает
  оба вида caches и перерисовывает.
- `led_reload_status_layers_safe()` перечитывает status caches.
- `led_reload_free_zone_layers_safe()` перечитывает free caches.

Если mutex еще `NULL`, методы выполняют минимальную операцию без lock. Это нужно
для ранней инициализации до запуска LED task.

## 8. Разбор `matrix_config_storage`

### 8.1 Что именно хранится

Namespace NVS: `led_settings`.

Keys:

| Key | Данные |
| --- | --- |
| `matrix_cfg` | `StoredMatrixConfig`: metadata, `MatrixConfig`, physical zoneMap |
| `free_zones_v1` | `StoredFreeZoneConfigV1` с четырьмя `FreeZoneConfig` |
| `layer_wait` | waiting status layer JSON |
| `layer_chg` | charging layer JSON |
| `layer_err` | error layer JSON |
| `fz5_layer`...`fz8_layer` | custom layers free zones |

`MatrixConfig` объявлен в `led_manager.h` и содержит topology, `bright_ports` и
три RGB arrays. `FreeZoneConfig` содержит zoneId, enabled, mode, staticColor[3]
и brightness.

Storage wrappers добавляют к ним magic, version, struct size и matrixX/Y.

### 8.2 Defaults

`matrix_config_set_defaults()`:

- обнуляет config через `memset`;
- topology = 0;
- bright_ports = 120;
- делит physical zoneMap на равные последовательные участки active ports;
- waiting = green, charging = blue, error = red.

Формула `(i * activePorts) / zoneCount` выбирает номер участка. При 96 pixels и
2 ports первые 48 physical indices получают zone1, вторые 48 - zone2.

`matrix_config_set_free_zone_defaults()` циклом создает четыре enabled config с
brightness 100. Logo white, QR green, Service orange, Custom purple; default mode
берется из `ZONE_METADATA`.

### 8.3 Validation matrix/free config

`zones_are_valid()` требует точный размер `NUM_IC_CHIPS` и zoneId не выше 8.

`matrix_config_validate()` дополнительно требует topology `0..3` и ненулевую
port brightness.

`free_zone_configs_are_valid()` проверяет:

- ровно четыре записи;
- ID и порядок совпадают с `FREE_ZONE_IDS`;
- enabled равен 0/1;
- brightness не 0;
- mode входит в static/custom/rainbow.

### 8.4 Нормализация layer JSON

`normalize_layer_json()` - строгий parser небольшого объекта.

Временные данные:

- `String source` - trimmed input;
- `bool seen[NUM_IC_CHIPS]` - защита от повторной physical coordinate;
- `size_t entryCount`;
- `int position` - текущий символ parser;
- `String normalizedJson` - результат.

Цикл вручную читает quoted key, двоеточие и quoted color. Каждая coordinate:

1. Парсится как два неотрицательных числа.
2. Проверяется относительно MATRIX_X/VIRTUAL_Y.
3. Переводится `getLEDIndex()`.
4. Проверяется на duplicate и максимум NUM pixels.
5. При free zone сверяется с physical zoneMap.
6. Записывается в canonical compact JSON.

Это ограничивает размер payload и не оставляет координаты за пределами матрицы.

### 8.5 `matrix_config_load()`

Метод ожидает target zone array точного размера. Через `Preferences` открывает
`led_settings` read-only и вызывает `load_current_config()`.

Тот читает `StoredMatrixConfig stored` через `getBytes()` и проверяет:

- прочитан полный struct;
- magic/version/size;
- matrixX/Y совпадают с compile-time;
- внутренний config и zones валидны.

При успехе копирует `stored.config` и `stored.zones` в RAM. При любой ошибке:

1. `clear_incompatible_matrix_storage()` удаляет matrix, free config, status
   layers и free layers;
2. создаются defaults в RAM;
3. возвращается `false`.

Network/auth namespaces не открываются и не затрагиваются.

### 8.6 `matrix_config_save()`

Сначала валидирует RAM data. Затем локальный `StoredMatrixConfig stored = {}`
получает metadata, config и memcpy zoneMap.

Запись:

1. `preferences.putBytes(KEY_MATRIX_CONFIG, &stored, sizeof(stored))`;
2. локальный `verified` читается обратно `getBytes()`;
3. сравниваются размеры и весь struct через `memcmp()`.

Возврат `true` означает, что read-back совпал. Это один NVS key, поэтому topology
и zoneMap сохраняются вместе в одном blob.

### 8.7 Free-zone config load/save

`matrix_config_load_free_zones()` читает `StoredFreeZoneConfigV1`. При неверной
metadata/dimensions/config он удаляет только `free_zones_v1` и free custom layers,
затем создает defaults.

`matrix_config_save_free_zones()` строит struct, пишет `putBytes`, читает обратно
и сравнивает `memcmp`, как matrix save.

### 8.8 Status layer load/save

`status_layer_key(status)` превращает waiting/charge/error aliases в один из
трех NVS keys.

`matrix_config_load_status_layer()` читает строку или `{}` и нормализует ее.
Невалидная сохраненная строка не попадает в renderer: возвращается `{}`.

`matrix_config_save_status_layer()` нормализует вход, пишет `putString`, читает
строку обратно и требует точное совпадение.

### 8.9 Free custom layer load/save

`free_zone_layer_key(zoneId)` строит ключ `fz<id>_layer` только для ID5..8.

Load нормализует coordinates и colors. Save дополнительно получает актуальный
zoneMap и через `matrix_config_normalize_free_zone_layer()` запрещает пиксели,
которые не принадлежат выбранной free zone.

### 8.10 Read-back и rollback

Read-back есть внутри каждой storage save. Rollback между несколькими keys
организован уровнем выше:

- status handler возвращает предыдущий layer, если matrix config save failed;
- free handler возвращает custom layer, если free config save failed;
- network/auth сохраняют предыдущие config/credentials при ошибке.

Это best-effort: если питание исчезло между физическими NVS commits, общей
транзакции нет. При следующем boot validation и defaults уменьшают последствия,
но hardware power-loss tests все равно нужны.

## 9. Разбор `network_config`

### 9.1 `NetworkConfig` и состояния

Структура хранит SSID/password, DHCP, четыре IP-поля и AP credentials.

`NetworkRuntimeMode`:

- `NETWORK_MODE_CONNECTING`;
- `NETWORK_MODE_STA`;
- `NETWORK_MODE_RECONNECTING`;
- `NETWORK_MODE_AP_FALLBACK`.

Модульные `static` переменные хранят config, mode, status и времена переходов.
Это простая state machine: `network_loop()` делает один короткий шаг, а не ждет
подключения внутри blocking цикла.

### 9.2 Defaults и уникальный AP

`defaultApSsid()` читает `ESP.getEfuseMac()`, берет младшие 24 bits и форматирует
шесть hex symbols: `LED_MATRIX_XXXXXX`. Этот suffix стабилен для конкретного chip.

`setDefaults()` задает пустой STA, DHCP, static fallback values, уникальный AP и
password `12345678`.

### 9.3 `loadConfig()`

Сначала выставляет defaults, затем читает namespace `net_cfg`:

`ssid`, `pass`, `dhcp`, `ip`, `gw`, `mask`, `dns`, `ap_ssid`, `ap_pass`.

`preferences.isKey("ap_ssid")` отличает пользовательский AP SSID от отсутствия
key. Сохраненное имя не заменяется новым default. После чтения весь config
валидируется; при ошибке используются defaults в RAM.

### 9.4 `writeConfigToNvs()` и `network_config_save()`

`writeConfigToNvs()` записывает все keys и тут же сравнивает каждое прочитанное
значение. `network_config_save()` сначала валидирует длины и static fields,
сохраняет `previous`, а при failed write пытается записать previous обратно.
Только после успешного read-back глобальный `networkConfig = config`.

### 9.5 STA-first и AP fallback

`network_setup()` загружает config, отключает framework persistence и включает
auto reconnect. `applyConfig()`:

- пустой SSID -> `startApFallback("not_configured", false)`;
- есть SSID -> `startStaAttempt(CONNECTING, false)`.

`startStaAttempt()` выключает AP, включает `WIFI_STA`, при static вызывает
`WiFi.config()`, затем `WiFi.begin()`. Функция сразу возвращается.

`startApFallback()` завершает scan, отключает STA, включает `WIFI_AP`, задает
`192.168.4.1`, запускает AP и обновляет mode/status/timers.

### 9.6 `network_loop()` и watchdog

Сначала метод:

- завершает scan старше 15 секунд;
- применяет pending config, когда наступил `pendingApplyAt`;
- пропускает watchdog, если не прошло 500 мс.

После этого `switch(runtimeMode)`:

- STA без connection -> `startStaReconnect()`;
- CONNECTING дольше 12 секунд -> AP fallback;
- RECONNECTING повторяет `WiFi.reconnect()` каждые 5 секунд;
- потерянная рабочая STA получает 45 секунд на recovery;
- повтор из fallback получает 12 секунд;
- AP fallback без клиентов раз в 60 секунд пытается STA, если сохранена сеть.

Постоянного AP+STA нет. Во время новой STA attempt AP временно выключается.

### 9.7 Async scan

`network_scan_start()` удаляет прошлые результаты. Если текущий mode AP fallback,
переключает radio во временный `WIFI_AP_STA`, чтобы AP оставался доступен во время
scan. `WiFi.scanNetworks(true, true)` запускает асинхронный scan.

Флаг `scanActive`, время `scanStartedAt` и `scanRestoreApOnly` описывают процесс.
`network_scan_finish()` удаляет результаты и возвращает AP-only, если нужно.

Сам scan не читает Data и не меняет mapper. Input_Task продолжает работу.

### 9.8 «Сохранить и подключиться»

Это два последовательных запроса UI:

1. `/save_network` -> validation + NVS + RAM config.
2. `/network_reconnect` -> `pendingApplyAt = millis() + 1500`.

Через 1,5 секунды `network_loop()` вызывает `applyConfig()`. UI может потерять
текущий адрес, потому что AP выключается или STA получает другой IP.

## 10. Разбор `auth_config`

### 10.1 Defaults и module state

Namespace: `auth_cfg`. Keys: `version`, `username`, `password`.

`currentUsername` и `currentPassword` - `static String`, по умолчанию
`admin/admin`. Пароль хранится в NVS открытым текстом.

### 10.2 `saveRawCredentials()`

Метод пишет version/username/password, затем читает их обратно. Он проверяет и
размеры результата `put*`, и равенство read-back. Возвращает только bool, потому
что это внутренний helper.

### 10.3 `auth_config_setup()`

Открывает NVS read-only и читает три локальные переменные. Если namespace не
открылся, оставляет defaults в RAM. Если version/credentials невалидны, включает
defaults и пытается сохранить их. При успехе копирует NVS strings в current state.

### 10.4 `auth_config_save_credentials()`

Создает `cleanUsername`, убирает пробелы по краям, проверяет username 1..32 и
password 6..64. Сохраняет previous credentials для rollback.

Только после успешной NVS-записи меняет `currentUsername/currentPassword`.
`error` передается по ссылке, поэтому вызывающий handler получает точный code.

### 10.5 `defaultCredentials`

`auth_config_uses_default_credentials()` возвращает true только если одновременно
username и password равны `admin`. `/auth_status` передает это UI.

### 10.6 Риски

- Basic Auth без HTTPS передает credentials без криптографической защиты канала.
- Password хранится plaintext в NVS.
- Нет физического/auth reset.
- Default credentials и AP password надо менять перед эксплуатацией.

## 11. Разбор `zone_model.h`

### 11.1 Типы и ID

`ZoneType` различает off, port и free. `FreeZoneMode` различает static, custom и
rainbow.

Фиксированные ID:

| ID | Константа | Назначение |
| ---: | --- | --- |
| 0 | `ZONE_ID_OFF` | Без зоны, физически Black |
| 1 | `ZONE_ID_PORT1` | Port1 runtime status |
| 2 | `ZONE_ID_PORT2` | Port2 runtime status |
| 3 | `ZONE_ID_PORT3` | Port3 runtime status/reserved |
| 4 | `ZONE_ID_PORT4` | Port4 runtime status/reserved |
| 5 | `ZONE_ID_LOGO` | Free Logo |
| 6 | `ZONE_ID_QR` | Free QR highlight |
| 7 | `ZONE_ID_SERVICE` | Free Service |
| 8 | `ZONE_ID_CUSTOM` | Free Custom |

`FREE_ZONE_IDS[]` задает стабильный порядок четырех free configs. Storage
проверяет не только ID, но и этот порядок.

### 11.2 `ZoneMetadata`

Структура хранит id, имя backend, type, default enabled/reserved, linkedPort и
default free mode. Константный массив `ZONE_METADATA[]` - каталог известных зон.

Статические значения Port2..4 в этом массиве не являются окончательной правдой.
`appendZoneMetadataJson()` динамически заменяет enabled/reserved по
`activePortCount`. Поэтому при compile-time count=3 UI автоматически откроет
Port1..Port3.

### 11.3 Inline helpers

- `zone_id_is_valid()` проверяет ID `0..8`;
- `zone_is_port()` проверяет диапазон `1..4`;
- `zone_is_free()` проверяет `5..8`;
- `free_zone_index()` циклом находит позицию free config;
- `zone_metadata_for()` ищет metadata;
- `zone_type_to_string()` и `free_zone_mode_to_string()` готовят JSON strings.

`inline` позволяет определить маленькую функцию в header без отдельного `.cpp`.

## 12. Разбор `gui_html.h`

`PAGE_MAIN[] PROGMEM` содержит HTML, CSS и JavaScript в одной flash-строке. После
загрузки страницы JavaScript выполняется в браузере, а не на CPU ESP32. ESP32
тратит flash на текст и heap на HTTP/JSON, но DOM, drawing и большинство UI-state
живут в памяти телефона или компьютера.

### 12.1 HTML-структура

Главная страница содержит:

- верхнюю панель и кнопку «Сервис»;
- вкладки «Зоны матрицы», «Статусы портов», «Свободные зоны»;
- общий `matrix_grid`;
- раскрываемые «Настройки матрицы» с topology;
- service modal;
- service sections Network, Security, Firmware, Diagnostics.

Один grid переиспользуется тремя редакторами. JavaScript меняет его цвет и
доступность в зависимости от `activeTab`.

### 12.2 CSS

CSS задает:

- variables цветов и размеров;
- `.panel`, `.toolbar`, `.field`, `.notice`, `.dirty-indicator`;
- grid матрицы и `.pixel`;
- tabs и service modal;
- responsive правила для narrow desktop/mobile;
- scroll/compact topology preview.

CSS не участвует в физическом renderer. Даже если browser pixel выглядит
тусклее/ярче, physical brightness определяется backend config.

### 12.3 Главные JS state variables

| Переменная | Смысл |
| --- | --- |
| `COLS`, `ROWS` | Текущий размер от backend или mock |
| `activeTab` | zones/status/free |
| `activeZoneId`, `activeStatus`, `activeFreeZone` | Что редактируется |
| `hardwareZonesMap` | Browser-карта `x-y -> zoneId` |
| `zoneMeta` | Metadata zones от `/get_config` |
| `statusColorLayers` | Три browser status layers |
| `freeModes`, `freeStaticColors`, `freeCustomLayers` | Free edit state |
| `freeBrightness` | Per-zone brightness |
| `matrixConfig`, `pendingTopology` | Сохраненная и выбранная topology |
| `activePortCount` | Значение backend |
| `backendAvailable`, `mockMode` | Real или local preview |
| `zoneMapDirty` | Zone edits не сохранены |
| `statusLayerDirty` | Dirty каждого status |
| `freeZoneDirty` | Dirty content каждой free zone |
| `portsBrightnessRevision`, `freeBrightnessRevisions` | Защита autosave от старых ответов |

### 12.4 `buildMockConfig()` и mock mode

Mock config содержит 12x8, два active ports, default zones/colors/free configs.
Он нужен для открытия извлеченного `index.html` без ESP32.

`loadConfig()` сначала пытается GET `/get_config`. При успехе:

- `backendAvailable = true`;
- `mockMode = false`;
- JSON передается в `applyConfig()`.

При первоначальной ошибке:

- используется `buildMockConfig()`;
- `backendAvailable = false`;
- `mockMode = true`.

Временная ошибка diagnostics позже не включает mock mode.

### 12.5 `applyConfig(cfg)`

Функция переносит backend JSON в browser state:

- clamp activePortCount;
- берет canonical `matrix.topology`;
- обновляет rows/cols/total;
- sanitizes hardware map и layers по размеру;
- берет fallback colors из `c_wait/c_charge/c_error`;
- переносит free configs/modes/colors/brightness/customLayer;
- сбрасывает dirty flags и autosave timers;
- обновляет selects, legends, grid и context.

Sanitize functions удаляют browser entries с координатами за пределами текущей
матрицы. Это frontend safety, backend выполняет свою независимую validation.

### 12.6 Построение матрицы

`buildMatrix()` очищает grid и двумя циклами создает `COLS * ROWS` кнопок-pixel.
Каждый DOM element получает `dataset.x`, `dataset.y` и key `x-y`.

Y перебирается от `ROWS-1` к 0, чтобы экранная ориентация соответствовала
редактору. Grid dimensions задаются CSS custom properties.

`redrawMatrix()` не создает elements заново. Она перебирает существующие pixels:

1. Находит zoneId из `hardwareZonesMap` или 0.
2. Вызывает `pixelStatusColor()`/`freeZoneColor()` либо zone color.
3. Устанавливает CSS background.
4. Добавляет locked/selected classes.
5. Обновляет title, hint, active-port warning и dirty indicators.

Brightness sliders намеренно не затемняют preview: редактор остается читаемым.

### 12.7 Drawing и pointer events

`paintPixel(pixel)` читает `dataset`, строит key и действует по active tab:

- zones + brush -> `hardwareZonesMap[key] = activeZoneId`;
- zones + eraser -> `delete hardwareZonesMap[key]`, то есть zone0;
- status -> `statusColorLayers[activeStatus][key] = selectedColor`;
- free custom -> `freeCustomLayers[activeFreeZone][key] = brushColor`.

При изменении выставляется соответствующий dirty flag и вызывается redraw.

`pointerdown` включает `drawing`, захватывает pointer и красит первый pixel.
`pointermove` красит следующие pixels при зажатой мыши/пальце. `pointerup`,
`pointercancel`, `pointerleave` выключают drawing. Pointer Events дают один путь
для desktop mouse и mobile touch.

### 12.8 Validation active ports и dirty flags

`missingActivePortZones()` циклом от 1 до `activePortCount` считает pixels каждой
port zone. `validateActivePortZones()` формирует singular/plural message.

`updateDirtyIndicators()` показывает три независимых индикатора:

- zoneMap;
- selected status/port brightness;
- free content/brightness.

Status/free save functions сначала проверяют active ports и `zoneMapDirty`.
Это предотвращает сохранение слоя относительно новой browser map, пока backend
еще использует старую физическую карту.

### 12.9 Сохранение зон в JS

Handler кнопки `save_zones_btn`:

1. Вызывает `validateActivePortZones()`.
2. Проверяет `backendAvailable`.
3. Делает `fetch('/save_zones', POST JSON.stringify(hardwareZonesMap))`.
4. Читает text ответа.
5. Только при `res.ok && text === 'OK'` сбрасывает `zoneMapDirty`.
6. Sanitizes browser free custom layers и запускает pending brightness saves.

При mock/offline dirty остается, потому что контроллер ничего не сохранил.

### 12.10 Status layer UI

`activeStatus` выбирается кнопками waiting/charging/error. `paintPixel()` пишет в
соответствующий объект. «Очистить слой» делает этот объект пустым и оставляет
dirty=true: физический слой изменится только после save.

`save_status_btn` блокируется логически при invalid/dirty zoneMap, затем POST-ит:

```js
{ status: activeStatus, colors: statusColorLayers[activeStatus] }
```

Только успешный `OK` сбрасывает dirty выбранного статуса.

### 12.11 Port brightness autosave

`updatePortsBrightness()` синхронизирует range/number, увеличивает revision и
ставит dirty. `schedulePortsBrightnessSave()` ставит timer на 500 мс, а событие
`change` отправляет сразу.

`persistPortsBrightness(value, revision)` POST-ит только `type=ports&val=...`.
Dirty сбрасывается только если response относится к все еще актуальной revision.
Если пользователь успел передвинуть slider еще раз, старый response не отменяет
dirty нового значения.

### 12.12 Free-zone editor

`buildSelects()` оставляет все free zones видимыми, но disables неразмеченные.
`updateFreeModeControls()` включает/выключает color/custom controls по mode.

`buildFreeZonePayload()` создает полный объект выбранной зоны. Для custom
добавляет customLayer; static/rainbow не обязаны его перезаписывать.

`apply_free_btn` вызывает `ensureFreeZoneSaveAllowed()`, отменяет pending
brightness timer и отправляет полный payload. Только успех сбрасывает
`freeZoneDirty`.

### 12.13 Free brightness autosave и batch

`buildFreeZoneBrightnessPayload()` содержит только `zoneId` и `brightness`.
Debounce/revision работают аналогично port brightness.

Кнопка «Применить яркость ко всем» получает `getMappedFreeZoneIds()` и циклом
вызывает brightness-only save для каждой размеченной free zone. Mode, color и
customLayer не отправляются.

### 12.14 Topology UI

`ledIndexForTopology()` повторяет четыре формулы backend только для browser
preview. Он не меняет физическую матрицу.

`renderTopologyControls()`:

- строит select из `TOPOLOGY_OPTIONS`;
- для матриц до 256 pixels показывает grid номеров;
- для больших показывает compact start/end/direction summary.

Save handler проверяет dirty zone/status/free layers, сравнивает pending с
сохраненной topology, показывает confirmation и POST-ит `/save_topology`.
После `OK` вызывает `loadConfig()`, чтобы получить remapped map от backend.

### 12.15 Service modal

`setServiceOpen(open)` меняет CSS class/`aria-hidden`. При открытии первой активна
Network. Клик вне окна не является close action; используется кнопка «Закрыть».

Service tab listeners меняют `activeServiceSection` и при необходимости вызывают
`loadNetworkStatus()`, `loadAuthStatus()` или `loadDiagnostics()`.

### 12.16 Network UI

`loadNetworkStatus()` GET-ит endpoint и сохраняет `networkStatus`. При временной
ошибке оставляет последнее известное реальное состояние.

`renderNetworkStatus()` создает безопасные DOM nodes через `textContent`, поэтому
SSID не вставляется как HTML.

`scanNetworks()`:

1. Ставит `networkScanInFlight` и disables button.
2. До 14 секунд вызывает `/scan_networks`.
3. При 202 ждет Promise 500 мс через `await`.
4. При 200 отображает массив.
5. В `finally` всегда снимает in-flight и включает кнопку.

В mock mode показываются две явно подписанные примерные сети; backend не
вызывается.

`saveNetworkConfig()` собирает `URLSearchParams`, POST-ит `/save_network`, затем
POST `/network_reconnect` и предупреждает о временной потере интерфейса.

### 12.17 Auth UI

`loadAuthStatus()` очищает password fields и GET-ит `/auth_status`. При
`defaultCredentials=true` показывает warning.

`saveAuthConfig()` проверяет login, password length и confirmation в браузере,
создает `URLSearchParams` и POST-ит `/save_auth`. Backend повторяет validation.

### 12.18 OTA UI

`updateFirmwareFileState()` проверяет, что выбран непустой `.bin`.

`uploadFirmware()` использует `XMLHttpRequest`, а не `fetch`, потому что XHR дает
`xhr.upload.onprogress`. Файл помещается в `FormData`. Callbacks `onload`,
`onerror`, `onabort` обновляют progress/message.

### 12.19 Diagnostics polling

`loadDiagnostics()` защищен `diagnosticsRequestInFlight`, чтобы два запроса не
накладывались. При успехе сохраняет data, отмечает backend доступным и вызывает
`renderDiagnostics()`.

Timers:

- каждые 2,5 секунды только при открытой Diagnostics;
- каждые 30 секунд вне Diagnostics для header/reconnect visibility;
- кнопка ручного обновления остается.

Временная ошибка меняет runtime label на «связь потеряна», но не переводит UI в
mock mode.

## 13. Сквозные сценарии с переменными

### A. Data1 LOW -> Port1 charging

1. Input_Task вызывает `station_inputs_update()`.
2. Для `i=0`: `level=LOW`, поэтому `nextState.active[0]=true`.
3. Для Data2 при HIGH: `nextState.active[1]=false`.
4. Цельный `inputState` заменяется под critical section.
5. Snapshot передается в `status_mapper_update()`.
6. Port1 mapping: `chargingInput=0`, `errorInput=1`.
7. `errorActive=false`, `chargingActive=true` -> `statuses[0]=CHARGING`.
8. LED_Task берет mapper snapshot.
9. Для каждого physical pixel с `zoneMap[i]==1` renderer ищет chargeLayer color.
10. Если его нет, берет `cfg.color_charge`, применяет `cfg.bright_ports`.
11. `leds[i]` получает постоянный charging color.
12. После цикла `FastLED.show()` отправляет кадр.

### B. Data2 LOW -> Port1 error

1. `active[1]` становится true.
2. Mapper первым проверяет `errorActive`, поэтому выбирает ERROR независимо от
   Data1.
3. Renderer берет error layer либо `cfg.color_error`.
4. Слишком темный color заменяется видимым red.
5. Выражение `(millis()/500)%2` чередует color/Black.

Если Data1 и Data2 оба LOW, результат все равно ERROR.

### C. Инженер нажал «Сохранить зоны»

1. Browser `hardwareZonesMap` уже содержит локальные edits.
2. JS проверяет pixels Port1/Port2 и backend availability.
3. `JSON.stringify()` создает coordinate object.
4. POST `/save_zones` приходит в `handleSaveZones()`.
5. `nextZoneMap` изначально весь zone0.
6. Parser переводит каждую `x-y` в physical index по текущей topology.
7. Backend повторно проверяет active ports.
8. `matrix_config_save()` записывает config+map в `matrix_cfg` и читает назад.
9. Только успех меняет global RAM zoneMap под LED mutex.
10. Renderer сразу использует карту.
11. JS получает `OK`, снимает dirty.
12. После reboot `matrix_config_load()` восстанавливает тот же blob.

### D. Инженер поменял topology

1. Select меняет только `pendingTopology` и browser preview.
2. Save проверяет zoneMap/status/free dirty.
3. Confirmation объясняет physical order.
4. POST передает число `0..3`.
5. Backend создает `nextConfig` и `nextZoneMap`.
6. Для каждой logical coordinate oldIndex переводится в newIndex.
7. Один NVS blob сохраняет новую пару topology+zoneMap.
8. Под mutex обновляются globals и перечитываются coordinate layers в caches.
9. UI снова вызывает `/get_config`.
10. Логическая разметка остается на месте; физическая цепочка меняется.

### E. Инженер запускает Wi-Fi scan

1. JS disables кнопку и GET-ит `/scan_networks`.
2. Backend вызывает async `WiFi.scanNetworks(true,true)` и отвечает 202.
3. JS не блокирует браузер: `await` освобождает event loop на 500 мс.
4. Input_Task ESP32 продолжает каждые 50 мс читать Data.
5. Следующий GET получает 202 или готовый count.
6. Backend циклом строит array сетей, очищает framework scan results.
7. UI безопасно создает text DOM cards.
8. В AP fallback radio временно AP+STA только на время scan.

### F. Плата перезагрузилась

1. Auth загружается или становится `admin/admin`.
2. Inputs впервые читаются, mapper создается и обновляется.
3. Matrix blob валидируется по magic/version/size/dimensions.
4. Free config и семь layer keys загружаются.
5. При error startup animation пропускается/прерывается.
6. Запускаются LED/Input tasks.
7. Network config загружается; STA начинается неблокирующе либо включается AP.
8. Web handlers регистрируются и server starts.
9. Временные status, pulse/rainbow phases и browser dirty state не
   восстанавливаются; они вычисляются заново.

## 14. Таблица «где что менять»

| Хочу изменить | Файл/символ | Что обязательно проверить |
| --- | --- | --- |
| GPIO Data1..8 | `board_config.h`, `BOARD_INPUT_PINS` | Схема, active-low, boot GPIO, hardware inputs |
| GPIO LED outputs | `board_config.h`, `LED_OUTPUT_PINS` | Плата, FastLED output, питание |
| Active ports | `board_config.h`, `DEFAULT_ACTIVE_PORT_COUNT` | Новые port zones, Data pairs, diagnostics, old NVS map |
| Matrix size | `led_manager.h`, `MATRIX_X/VIRTUAL_Y` | Storage reset, RAM, UI/mobile, topology, real matrix |
| LED type/color order | `led_manager.cpp`, `FastLED.addLeds<...,BRG>` | Реальная WS2811 и RGB test |
| Default status colors | `matrix_config_storage.cpp`, `matrix_config_set_defaults()` | Mock defaults в `gui_html.h` |
| Default free colors | `matrix_config_set_free_zone_defaults()` | UI mock и physical result |
| AP SSID/password | `network_config.cpp`, `DEFAULT_AP_*` | Recovery procedure/security |
| Network timeout | `network_config.cpp`, `STA_*`, `NETWORK_*` | STA loss/recovery/AP behavior |
| Input period | `main.cpp`, `INPUT_TASK_PERIOD_MS` | Error latency, noise, CPU |
| UI text/layout | `gui_html.h`, HTML/CSS/JS block | Desktop/mobile, JS syntax, mock/real |
| Новый endpoint | `main.cpp`: похожий handler + `server.on()`; JS `fetch()` | Auth, validation, truthful errors, storage read-back |
| Новый zone ID/mode | `zone_model.h` плюс storage/renderer/backend/UI | NVS compatibility и safety |

## 15. Синтаксис C++/JS на примерах проекта

### `struct`

`struct StationInputState` объединяет два массива в одно цельное значение.
Присваивание `inputState = nextState` копирует всю структуру.

### Массив

`BOARD_INPUT_PINS[8]` - восемь значений одного типа. Индекс начинается с нуля,
поэтому Data1 хранится в `[0]`.

### `for`

```cpp
for (uint8_t i = 0; i < BOARD_INPUT_COUNT; i++)
```

Создает `i=0`, выполняет тело, увеличивает `i`, останавливается перед 8. Так код
читает все входы без восьми одинаковых строк.

### `if / else if / else`

В mapper порядок условий задает приоритет. Как только true-ветка выполнена,
следующие не проверяются. Поэтому error должен стоять перед charging.

### `static`

У функции/глобальной переменной в `.cpp` ограничивает видимость модулем. У
локальной `static rainbowHue` сохраняет значение между вызовами renderer.

### `const` и `constexpr`

`const` запрещает изменение через данное имя/ссылку. `constexpr` задает значение,
известное при сборке, например `DEFAULT_ACTIVE_PORT_COUNT`.

### `String`

Arduino `String` управляет динамическим текстом. Он удобен для JSON, но использует
heap. Поэтому большие ответы делают `reserve()`, а layer payload ограничивается.

### Указатель

`uint8_t *zones` хранит адрес первого элемента массива. Отдельный `zoneCount`
нужен, потому что указатель сам не знает длину. Проверка `nullptr` защищает от
отсутствующего массива.

### Ссылка

`const NetworkConfig &net` не копирует struct и запрещает изменение. `String
&error` позволяет функции записать error code в переменную вызывающего кода.

### `volatile`

В текущих модулях shared state не объявлен `volatile`. `volatile` запретил бы
часть оптимизаций, но не сделал бы копирование нескольких полей атомарным.
Поэтому inputs/status используют critical snapshot, а LED state - mutex.

### Mutex и critical section

Mutex можно ждать и держать во время перечитывания caches/render. Critical
section должна быть очень короткой; здесь она защищает только копию небольшого
state.

### FreeRTOS task

`xTaskCreatePinnedToCore()` запускает функцию, которая обычно содержит
`while(true)` и `vTaskDelay`. Delay отдает CPU другим задачам.

### Callback

Upload callback не вызывается нашим кодом напрямую. WebServer вызывает его на
каждом START/WRITE/END событии файла.

### JSON

JSON передает структурированные текстовые данные. Например zone map:

```json
{"0-0":"1","1-0":"2"}
```

Backend обязан заново проверить JSON, даже если UI уже проверил его.

### `fetch`, Promise и `async/await`

`fetch()` сразу возвращает Promise - обещание будущего HTTP-результата.
`await fetch(...)` приостанавливает только данную async JS-функцию, но не весь
браузер. `try/catch/finally` разделяет success/error/обязательную cleanup.

`scanNetworks()` использует это, чтобы опрашивать 202 без замораживания UI.

## 16. Осторожность при изменениях

### Safety-критичные места

- Порядок `error -> charging -> waiting` в `status_mapper_update()`.
- Active-low conversion в `station_inputs_update()`.
- Mapping Data pairs и `DEFAULT_ACTIVE_PORT_COUNT`.
- Renderer ветки port/free и `visibleErrorColor()`.
- Backend validation active port zones.
- Изоляция Input_Task от network/HTTP.

Изменение этих мест требует tests всех LOW/HIGH combinations на железе.

### Изменения, требующие hardware test

- GPIO и `INPUT_PULLUP`;
- input period/debounce;
- topology и BRG order;
- FastLED voltage/current cap;
- matrix dimensions;
- startup animation/error interruption;
- network reconnect/AP radio behavior;
- OTA и power-loss behavior.

### Изменения, требующие save/reload/reboot test

- Любой `MatrixConfig`/`FreeZoneConfig` field;
- NVS key/magic/version/struct layout;
- zoneMap/topology remap;
- status/free layers;
- network/auth write/rollback;
- browser autosave и dirty flags.

### Почему нельзя возвращать простой live hardware preview

Если browser drawing напрямую перехватывает physical output, runtime error может
оказаться скрыт. Возвращать hardware preview можно только отдельным проектным
этапом с коротким timeout, немедленным error preemption, независимым input path и
явным возвратом renderer. Сейчас client-side preview безопаснее и проще.

### Что пока сознательно не добавлено

- runtime matrix resize;
- multi-matrix LED2..LED4;
- brightness schedule;
- HTTPS и signed OTA;
- captive portal/mDNS;
- cloud/OCPP/MQTT;
- physical/auth factory reset.

### Финальная проверка после правки кода

Минимальный порядок:

1. Посмотреть `git diff` и убедиться, что нет unrelated изменений.
2. Выполнить `pio run -e esp32dev`.
3. Проверить Data1..Data4 и priority error.
4. Проверить zones/status/free/topology save и reboot restore.
5. Проверить STA/AP fallback и scan во время изменения Data.
6. Проверить auth и OTA только подходящим `.bin`.
