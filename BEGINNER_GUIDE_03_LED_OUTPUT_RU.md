# Как прошивка вычисляет и выводит цвета на LED-матрицу

Этот документ подробно разбирает модуль `src/led_manager.cpp`: загрузку настроек, отдельную задачу светодиодов, вычисление цвета каждого физического пикселя и безопасное применение новых настроек.

Главная функция главы - `led_refresh_internal()`, которая вычисляет цвет каждого физического светодиода (renderer). Ниже она приведена целиком и разобрана буквально по строкам.

Исходный код проекта этим документом не изменяется.

---

## 1. Общая цепочка LED-модуля

```text
setup() в main.cpp
        ↓
led_setup()
        ↓
создание ledMutex
        ↓
настройка FastLED и физического выхода GPIO 22
        ↓
led_load_config_from_flash()
        ↓
загрузка cfg, zoneMap, freeZoneConfigs и кэшей слоёв
        ↓
короткая стартовая последовательность
        ↓
первый вызов led_refresh_internal()
        ↓
создание отдельной задачи LED_Task
        ↓
ledTaskWorker() работает постоянно
        ↓
обновляет breatheScale
        ↓
led_refresh_internal() заполняет leds[]
        ↓
FastLED.show() отправляет цвета физической матрице
```

Обработчики запросов из `src/main.cpp` могут менять настройки во время работы. Они вызывают функции с окончанием `_safe`, которые сначала получают блокировку `ledMutex`, а затем обновляют общие данные.

---

## 2. Основные данные LED-модуля

### 2.1. Массив `leds[]`

Актуальное объявление:

```cpp
CRGB leds[NUM_IC_CHIPS];
```

`NUM_IC_CHIPS` вычисляется как `MATRIX_X * VIRTUAL_Y`. При размере 12x8 массив содержит 96 элементов.

Каждый элемент `leds[i]` хранит итоговый цвет одного физического светодиода с номером `i`.

Тип `CRGB` предоставляется библиотекой FastLED. Он содержит три цветовых канала:

```text
r - красный, 0..255
g - зелёный, 0..255
b - синий, 0..255
```

Примеры:

```cpp
CRGB::Black        // (0, 0, 0), светодиод выключен
CRGB(255, 0, 0)   // красный
CRGB(0, 80, 255)  // сине-голубой
```

Изменение `leds[i]` само по себе ещё не отправляет данные на провод. Это только изменение рабочей копии в оперативной памяти.

Функция `FastLED.show()` передаёт весь массив `leds[]` физической цепочке светодиодов.

### 2.2. Массив `zoneMap[]`

Актуальное объявление:

```cpp
uint8_t zoneMap[NUM_IC_CHIPS];
```

Для каждого физического номера `i` массив хранит номер зоны:

| `zoneMap[i]` | Значение |
|---:|---|
| 0 | без зоны, пиксель выключен |
| 1 | Port1 |
| 2 | Port2 |
| 3 | Port3 |
| 4 | Port4 |
| 5 | Logo |
| 6 | QR |
| 7 | Service |
| 8 | Custom |

Пример:

```text
zoneMap[17] = 1
```

Это означает: физический светодиод 17 принадлежит Port1.

`zoneMap[]` хранится по физическим номерам. При сохранении разметки координаты редактора переводятся в физические номера через текущую схему нумерации змейки, то есть топологию (`topology`).

### 2.3. Рабочая структура `cfg`

Актуальное объявление:

```cpp
MatrixConfig cfg;
```

Структура определена в `src/led_manager.h`:

```cpp
struct MatrixConfig {
    uint8_t topology;
    uint8_t bright_ports;
    uint8_t color_wait[3];
    uint8_t color_charge[3];
    uint8_t color_error[3];
};
```

Она содержит:

- `topology` - физический порядок змейки;
- `bright_ports` - общую яркость всех портовых зон, от 1 до 255;
- `color_wait` - запасной цвет ожидания;
- `color_charge` - запасной цвет зарядки;
- `color_error` - запасной цвет ошибки.

Запасной цвет (`fallback color`) используется для пикселя порта, если в слое выбранного статуса для этого пикселя нет индивидуального цвета.

Пример:

```text
cfg.bright_ports = 120
cfg.color_charge = [0, 80, 255]
```

### 2.4. Массив `freeZoneConfigs[]`

Актуальное объявление:

```cpp
FreeZoneConfig freeZoneConfigs[FREE_ZONE_COUNT];
```

В массиве четыре элемента: Logo, QR, Service и Custom.

```cpp
struct FreeZoneConfig {
    uint8_t zoneId;
    uint8_t enabled;
    FreeZoneMode mode;
    uint8_t staticColor[3];
    uint8_t brightness;
};
```

Для каждой свободной зоны хранятся:

- номер зоны;
- включена она или нет;
- режим постоянного цвета `static`, пользовательского рисунка `custom` или радуги `rainbow`;
- постоянный цвет;
- собственная яркость от 1 до 255.

Яркость свободной зоны не зависит от `cfg.bright_ports`.

### 2.5. `breatheScale`

Актуальное объявление:

```cpp
float breatheScale = 0.2;
```

Это множитель плавного пульса ожидания. LED-задача постепенно изменяет его примерно от 0.15 до 1.0 и обратно.

Пример:

```text
breatheScale = 0.50
pulseBright = 220 * 0.50 = 110
```

Он применяется только к порту со статусом ожидания.

### 2.6. `rainbowHue`

Внутри `led_refresh_internal()` объявлена локальная сохраняющаяся переменная:

```cpp
static uint8_t rainbowHue = 0;
```

Слово `static` внутри функции означает, что значение не создаётся заново при каждом вызове. Оно сохраняется между кадрами.

Каждый кадр выполняет:

```cpp
rainbowHue += 1;
```

После 255 восьмибитное значение переходит к 0. Так цветовой круг повторяется.

### 2.7. `CRGB` и `CHSV`

`CRGB` задаёт цвет непосредственно через красный, зелёный и синий каналы.

`CHSV` задаёт цвет через:

- оттенок;
- насыщенность;
- яркость.

Строка:

```cpp
CHSV(rainbowHue + (i * 4), 255, freeConfig->brightness)
```

создаёт насыщенный радужный цвет. FastLED преобразует его в итоговые красный, зелёный и синий каналы при присваивании в `leds[i]`.

---

## 3. Кэши цветных слоёв

### 3.1. `StatusLayerCache`

Актуальная структура:

```cpp
struct StatusLayerCache {
    bool hasColor[NUM_IC_CHIPS];
    CRGB colors[NUM_IC_CHIPS];
};
```

Созданы три экземпляра:

```cpp
StatusLayerCache waitLayer;
StatusLayerCache chargeLayer;
StatusLayerCache errorLayer;
```

Для каждого физического пикселя:

- `hasColor[i]` сообщает, задан ли индивидуальный цвет;
- `colors[i]` хранит уже готовый `CRGB`.

Пример:

```text
chargeLayer.hasColor[17] = true
chargeLayer.colors[17] = CRGB(0, 120, 255)
```

Это означает, что пиксель 17 имеет индивидуальный цвет для статуса зарядки.

Набор индивидуальных цветов одного состояния порта называется слоем статуса (`status layer`).

### 3.2. `FreeZoneLayerCache`

Актуальная структура:

```cpp
struct FreeZoneLayerCache {
    bool hasColor[NUM_IC_CHIPS];
    CRGB colors[NUM_IC_CHIPS];
};
```

Массив:

```cpp
FreeZoneLayerCache freeZoneLayers[FREE_ZONE_COUNT];
```

содержит отдельный кэш для Logo, QR, Service и Custom.

В режиме `custom`:

- `hasColor[i] == true` означает, что для пикселя нарисован цвет;
- `colors[i]` содержит этот цвет;
- отсутствие цвета приводит к чёрному пикселю.

Такой нарисованный пользователем рисунок называется пользовательским слоем (`custom layer`).

### 3.3. Откуда загружаются данные

Слои хранятся в постоянной памяти настроек (NVS) как строки JSON. JSON здесь является текстовым объектом вида:

```json
{
  "0-0": "#FF0000",
  "1-0": "#00FF00"
}
```

Ключ `"0-0"` содержит логические координаты `x-y`.

Функции:

```cpp
parseStatusLayerJson(...)
parseFreeZoneLayerJson(...)
```

разбирают текст, переводят координату через `getLEDIndex()` в физический номер и заполняют массивы кэша.

### 3.4. Почему JSON не разбирается каждый кадр

Разбор текста требует поиска кавычек, выделения временных строк, преобразования координат и цветов. Если делать это для каждого из примерно 30-40 кадров в секунду, ESP32 будет зря расходовать процессорное время и временную память.

Поэтому JSON разбирается:

1. один раз при запуске;
2. повторно после сохранения слоя;
3. повторно после изменения разметки или топологии, если изменился физический индекс координаты.

Точное число разборов при одном событии:

| Событие | Что разбирается |
|---|---|
| запуск | 3 статусных строки и 4 строки свободных зон |
| сохранение status layer | заново все 3 статусных строки |
| сохранение free zone | заново все 4 строки свободных зон |
| сохранение topology | заново 3 статусных и 4 свободных слоя |
| сохранение zoneMap | заново 4 слоя свободных зон |

Перестроение всех небольших кэшей при редком сохранении проще и надёжнее, чем пытаться частично обновлять один элемент.

В каждом обычном кадре используется быстрый доступ:

```cpp
layer.hasColor[i]
layer.colors[i]
```

### 3.5. Функции загрузки кэшей без собственной блокировки

```cpp
void led_reload_status_layers_unlocked() {
    parseStatusLayerJson(matrix_config_load_status_layer("waiting"), waitLayer);
    parseStatusLayerJson(matrix_config_load_status_layer("charging"), chargeLayer);
    parseStatusLayerJson(matrix_config_load_status_layer("error"), errorLayer);
}
```

Эта функция читает три строки из NVS и полностью перестраивает три кэша статусов.

```cpp
void led_reload_free_zone_layers_unlocked() {
    for (size_t i = 0; i < FREE_ZONE_COUNT; i++) {
        parseFreeZoneLayerJson(freeZoneConfigs[i].zoneId, matrix_config_load_free_zone_layer(freeZoneConfigs[i].zoneId), freeZoneLayers[i]);
    }
}
```

Она четыре раза проходит по свободным зонам и строит их кэши.

Слово `unlocked` в имени означает внутреннюю функцию без собственной блокировки. Её разрешено вызывать:

- до запуска параллельной LED-задачи;
- либо внутри участка, где вызывающий код уже получил `ledMutex`.

---

## 4. Зачем нужен `ledMutex`

Актуальное объявление:

```cpp
SemaphoreHandle_t ledMutex = NULL;
```

Функция:

```cpp
ledMutex = xSemaphoreCreateMutex();
```

создаёт объект взаимного исключения, который называется мьютексом (`mutex`).

Он не делает переменные безопасными автоматически. Он не даёт LED-задаче и функциям `_safe` одновременно выполнять участки, в которых они явно получили этот мьютекс.

Функция с окончанием `_safe` является безопасной внешней обёрткой: она сама получает `ledMutex`. В текущем коде под мьютексом гарантированно выполняются:

- `leds[]`;
- вычисление кадра и `FastLED.show()`;
- замена `zoneMap[]` через безопасную функцию `_safe`;
- совместное применение `cfg` и `zoneMap[]` при смене топологии;
- перестроение кэшей статусных и свободных слоёв через безопасные функции `_safe`.

Без мьютекса возможна ситуация:

1. LED-задача начала перебирать `zoneMap[]`.
2. HTTP-обработчик заменил половину массива.
3. Один кадр получился из смеси старой и новой разметки.

Безопасные функции сначала получают `ledMutex`, выполняют согласованное изменение и только потом отпускают его.

Мьютекс не защищает `StatusMapperState`. Для него существует отдельная короткая защита в `status_mapper.cpp`.

Есть важная особенность фактического кода: `handleSetBright()` и `handleSaveStatusColors()` присваивают новый `cfg`, а `handleSaveFreeZone()` присваивает `freeZoneConfigs[]` до вызова безопасной функции `_safe`. Эти конкретные присваивания происходят вне `ledMutex`. Следующий кадр строится уже под мьютексом, но формально остаётся короткое окно одновременного чтения и записи. Это остаточный риск для отдельного этапа проверки надёжности.

---

## 5. `led_setup()`

Файл: `src/led_manager.cpp`.

### 5.1. Полный актуальный код

```cpp
void led_setup() {
    ledMutex = xSemaphoreCreateMutex();

    FastLED.addLeds<WS2811, LED_PIN, BRG>(leds, NUM_IC_CHIPS);
    FastLED.setMaxPowerInVoltsAndMilliamps(MAX_LED_VOLTS, MAX_LED_MILLIAMPS);

    led_load_config_from_flash();
    led_run_startup_animation();
    led_refresh_internal();

    xTaskCreatePinnedToCore(ledTaskWorker, "LED_Task", 4096, NULL, 1, NULL, 1);
}
```

### 5.2. Кто вызывает и что существует до входа

`setup()` в `src/main.cpp` вызывает `led_setup()` один раз после первого вычисления статусов портов и до запуска постоянной Data-задачи.

До входа:

- GPIO входов настроены;
- первый `StatusMapperState` уже вычислен;
- глобальные массивы LED-модуля существуют, но сохранённые настройки ещё не загружены;
- физический выход FastLED ещё не настроен.

### 5.3. Построчное объяснение

```cpp
ledMutex = xSemaphoreCreateMutex();
```

Создаётся мьютекс и его адрес сохраняется в `ledMutex`.

```cpp
FastLED.addLeds<WS2811, LED_PIN, BRG>(leds, NUM_IC_CHIPS);
```

FastLED настраивается для:

- контроллера светодиодов WS2811;
- основного GPIO `LED_PIN`, сейчас GPIO 22;
- порядка цветовых каналов `BRG`;
- массива `leds`;
- 96 физических пикселей.

Порядок `BRG` означает, что физическая лента ожидает сначала синий, затем красный, затем зелёный байт. Внутри `CRGB` программа всё равно работает с обычными R, G и B.

```cpp
FastLED.setMaxPowerInVoltsAndMilliamps(MAX_LED_VOLTS, MAX_LED_MILLIAMPS);
```

FastLED получает программный бюджет питания: 12 В и 3000 мА. Библиотека может уменьшить общий вывод, если расчётное потребление кадра превышает бюджет.

Это ограничение потребления LED-матрицы, а не тока зарядной станции.

```cpp
led_load_config_from_flash();
```

Загружаются `cfg`, `zoneMap`, свободные зоны и кэши.

```cpp
led_run_startup_animation();
```

Матрица коротко показывает красный, синий и зелёный. Перед каждым цветом и во время него проверяется аппаратный сигнал ошибки. При ошибке анимация прерывается.

```cpp
led_refresh_internal();
```

Сразу строится первый рабочий кадр. Это убирает остаточный цвет стартовой последовательности.

```cpp
xTaskCreatePinnedToCore(ledTaskWorker, "LED_Task", 4096, NULL, 1, NULL, 1);
```

Создаётся отдельная постоянно выполняемая работа ESP32, то есть задача FreeRTOS (`FreeRTOS task`).

Параметры:

| Параметр | Значение | Смысл |
|---|---|---|
| функция | `ledTaskWorker` | что выполнять |
| имя | `"LED_Task"` | служебное имя |
| стек | 4096 | 4096 байт временной памяти задачи |
| параметр | `NULL` | дополнительные данные не передаются |
| приоритет | 1 | ниже Data-задачи с приоритетом 2 |
| дескриптор | `NULL` | отдельный объект управления не сохраняется |
| ядро | 1 | выполнять на ядре процессора номер 1 |

Стек (`stack`) 4096 нужен не только для самого цикла, но и для всех вызванных функций, локальных `CRGB`, снимка статусов и операций FastLED. Код не содержит комментария с измерением минимально необходимого размера, поэтому 4096 является запасом текущей реализации. На железе полезно проверить минимальный остаток стека.

Приоритет (`priority`) 1 позволяет задаче Data-входов с приоритетом 2 прервать LED-задачу и быстро обновить важный статус. После короткого чтения Data-задача снова засыпает, и LED-задача продолжает работу.

Обе задачи закреплены за ядром (`core`) 1. Планировщик FreeRTOS сначала даёт время готовой задаче с более высоким приоритетом.

### 5.4. Что меняется глобально

- `ledMutex` получает созданный объект;
- FastLED получает ссылку на `leds[]`;
- загружаются `cfg`, `zoneMap`, `freeZoneConfigs[]` и кэши;
- физическая матрица получает стартовые и первый рабочий кадры;
- создаётся LED-задача.

### 5.5. Ошибки текущей реализации

Результат `xSemaphoreCreateMutex()` не проверяется. Если мьютекс не создан из-за нехватки памяти, `ledMutex` останется `NULL`, а запущенная LED-задача попытается использовать его. Это остаточный риск текущего кода.

Результат `xTaskCreatePinnedToCore()` также не проверяется. Если задача не создана, первый рабочий кадр останется на матрице, но постоянное обновление прекратится.

Обычная нехватка или повреждение сохранённых настроек обрабатывается загрузчиками через начальные значения.

---

## 6. `led_load_config_from_flash()`

### 6.1. Полный актуальный код

```cpp
static void led_load_config_from_flash() {
    matrix_config_load(cfg, zoneMap, sizeof(zoneMap));
    matrix_config_load_free_zones(freeZoneConfigs, FREE_ZONE_COUNT);
    led_reload_status_layers_unlocked();
    led_reload_free_zone_layers_unlocked();

    for (int i = 0; i < NUM_IC_CHIPS; i++) leds[i] = CRGB::Black;
    FastLED.show();
}
```

### 6.2. Кто вызывает и когда

Только `led_setup()` вызывает эту внутреннюю функцию при старте.

Слово `static` перед функцией ограничивает её видимость файлом `led_manager.cpp`.

В этот момент LED-задача ещё не создана, поэтому внутренние функции без собственной блокировки можно вызывать безопасно.

### 6.3. Построчное объяснение

```cpp
matrix_config_load(cfg, zoneMap, sizeof(zoneMap));
```

Из ключа `matrix_cfg` в NVS загружаются:

- `cfg`;
- весь массив `zoneMap`;
- служебно проверяются версия, размер структуры и размер матрицы.

Если запись отсутствует или невалидна, загрузчик заполняет начальные значения.

```cpp
matrix_config_load_free_zones(freeZoneConfigs, FREE_ZONE_COUNT);
```

Загружаются четыре `FreeZoneConfig`. При ошибке используются начальные режимы, цвета и яркости.

```cpp
led_reload_status_layers_unlocked();
```

Строки ожидания `"waiting"`, зарядки `"charging"` и ошибки `"error"` преобразуются в три кэша.

```cpp
led_reload_free_zone_layers_unlocked();
```

Строки рисунков четырёх свободных зон преобразуются в четыре кэша.

```cpp
for (int i = 0; i < NUM_IC_CHIPS; i++) leds[i] = CRGB::Black;
```

Цикл `for` проходит от 0 до 95 и делает рабочий массив чёрным.

```cpp
FastLED.show();
```

Чёрный массив отправляется матрице. Это очищает неизвестное состояние до стартовой последовательности.

### 6.4. Что возвращается и что происходит при ошибке

Функция ничего не возвращает.

Она не проверяет возвращаемые `bool` от двух основных загрузчиков. Это допустимо только потому, что сами загрузчики при ошибке заполняют переданные структуры начальными значениями. Слои при отсутствии возвращаются пустыми.

---

## 7. `ledTaskWorker()`

### 7.1. Полный актуальный код

```cpp
void ledTaskWorker(void * parameter) {
    while (true) {
        unsigned long now = millis();
        if (xSemaphoreTake(ledMutex, (TickType_t)10) == pdTRUE) {

            if (now - lastBreatheTime > 25) {
                lastBreatheTime = now;
                if (breatheDirectionUp) {
                    breatheScale += 0.012;
                    if (breatheScale >= 1.0) breatheDirectionUp = false;
                } else {
                    breatheScale -= 0.012;
                    if (breatheScale <= 0.15) breatheDirectionUp = true;
                }
                led_refresh_internal();
            }

            xSemaphoreGive(ledMutex);
        }
        vTaskDelay(15 / portTICK_PERIOD_MS);
    }
}
```

### 7.2. Кто вызывает и какие данные существуют

FreeRTOS запускает функцию после `xTaskCreatePinnedToCore()` в `led_setup()`.

До старта:

- мьютекс должен быть создан;
- конфигурация и кэши загружены;
- FastLED настроен;
- первый рабочий кадр уже показан.

Параметр `void * parameter` не используется, потому что при создании передаётся `NULL`.

### 7.3. Построчное объяснение

```cpp
while (true) {
```

Бесконечный цикл означает, что задача работает до перезагрузки ESP32.

```cpp
unsigned long now = millis();
```

`now` получает число миллисекунд с момента запуска.

```cpp
if (xSemaphoreTake(ledMutex, (TickType_t)10) == pdTRUE) {
```

Задача пытается получить `ledMutex`.

Максимальное ожидание равно 10 тикам (`ticks`) планировщика. Тик является внутренней единицей времени FreeRTOS. В этом месте число 10 передано напрямую, а не через `pdMS_TO_TICKS()`, поэтому точная длительность зависит от настройки частоты тиков.

Если мьютекс получен, `xSemaphoreTake()` возвращает `pdTRUE`.

```cpp
if (now - lastBreatheTime > 25) {
```

Новый кадр строится, только если после последнего шага пульса прошло больше 25 мс.

```cpp
lastBreatheTime = now;
```

Запоминается время текущего шага.

```cpp
if (breatheDirectionUp) {
```

Проверяется направление изменения пульса.

```cpp
breatheScale += 0.012;
```

При движении вверх множитель увеличивается.

```cpp
if (breatheScale >= 1.0) breatheDirectionUp = false;
```

При достижении верхней границы следующий кадр начнёт уменьшение.

```cpp
} else {
    breatheScale -= 0.012;
```

При движении вниз множитель уменьшается.

```cpp
if (breatheScale <= 0.15) breatheDirectionUp = true;
```

При достижении нижней границы направление меняется вверх.

```cpp
led_refresh_internal();
```

Вычисляется и отправляется новый кадр. Мьютекс всё это время удерживается.

```cpp
xSemaphoreGive(ledMutex);
```

Мьютекс освобождается, и HTTP-обработчик может применить настройки.

```cpp
vTaskDelay(15 / portTICK_PERIOD_MS);
```

Задача добровольно засыпает примерно на 15 мс. `portTICK_PERIOD_MS` переводит миллисекунды в тики текущей среды.

### 7.4. Почему при неудаче обновление пропускается

Если за 10 тиков мьютекс получить не удалось, тело `if` не выполняется:

- `breatheScale` в этой итерации не меняется;
- `led_refresh_internal()` не вызывается;
- старый физический кадр продолжает гореть;
- задача делает короткую задержку и пробует снова.

Это безопаснее, чем одновременно читать наполовину изменённые `zoneMap`, `cfg` или кэши.

Пропуск одного кадра обычно незаметен. Важный статус Data-задача уже опубликует независимо, а LED-задача покажет его после освобождения мьютекса.

### 7.5. Почему Data-задача имеет приоритет 2

Data-задача должна своевременно прочитать ошибку и обновить `StatusMapperState`. Она выполняет короткую работу каждые 50 мс.

LED-задача имеет приоритет 1, потому что задержка одного визуального кадра менее опасна, чем задержка чтения входа ошибки.

После завершения короткой Data-задачи LED-задача продолжает вычисление кадра.

### 7.6. Ошибки

Если `ledMutex == NULL`, текущая функция не имеет проверки перед `xSemaphoreTake()`. Это причина, по которой создание мьютекса должно быть успешным.

Если вычисление кадра станет дольше ожидаемого, частота кадров уменьшится. Data-задача с более высоким приоритетом всё равно сможет получить процессорное время.

---

## 8. `led_refresh_internal()`: полный код

```cpp
void led_refresh_internal() {
    static uint8_t rainbowHue = 0;
    rainbowHue += 1;

    // Вычисляем треугольный пульс яркости для режима ожидания.
    uint8_t pulseBright = (uint8_t)(220 * breatheScale);
    if (pulseBright < 40) pulseBright = 40;

    // Извлекаем кастомные цвета портов из конфига и масштабируем их под яркость bright_ports
    CRGB customWait   = CRGB(cfg.color_wait[0], cfg.color_wait[1], cfg.color_wait[2]);
    CRGB customCharge = CRGB(cfg.color_charge[0], cfg.color_charge[1], cfg.color_charge[2]);
    CRGB customError  = CRGB(cfg.color_error[0], cfg.color_error[1], cfg.color_error[2]);

    // Применяем общую яркость ко всем портовым зонам.
    customWait.nscale8_video(cfg.bright_ports);
    customCharge.nscale8_video(cfg.bright_ports);
    customError.nscale8_video(cfg.bright_ports);

    StatusMapperState portState;
    status_mapper_get_snapshot(portState);

    for (int i = 0; i < NUM_IC_CHIPS; i++) {
        uint8_t zone = zoneMap[i];

        if (zone == 0) {
            leds[i] = CRGB::Black;
        }
        else {
            CRGB layerColor;
            if (zone_is_port(zone)) {
                const uint8_t portIndex = zone - ZONE_ID_PORT1;
                const PortStatus zoneStatus = (portIndex < MAX_PORT_COUNT)
                    ? portState.statuses[portIndex]
                    : PORT_STATUS_DISABLED;

                if (zoneStatus == PORT_STATUS_DISABLED) {
                    leds[i] = CRGB::Black;
                    continue;
                }

                if (getStatusLayerColorForStatus(i, zoneStatus, layerColor)) {
                    layerColor.nscale8_video(cfg.bright_ports);

                    if (zoneStatus == PORT_STATUS_ERROR) {
                        layerColor = visibleErrorColor(layerColor);
                        leds[i] = ((millis() / 500) % 2 == 0) ? layerColor : CRGB::Black;
                    } else if (zoneStatus == PORT_STATUS_WAITING) {
                        layerColor.nscale8_video(pulseBright);
                        leds[i] = layerColor;
                    } else {
                        leds[i] = layerColor;
                    }
                    continue;
                }
            }
        }
        // Port zones 1..4 use the current mapped runtime status.
        if (zone_is_port(zone)) {
            const uint8_t portIndex = zone - ZONE_ID_PORT1;
            const PortStatus zoneStatus = (portIndex < MAX_PORT_COUNT)
                ? portState.statuses[portIndex]
                : PORT_STATUS_DISABLED;

            if (zoneStatus == PORT_STATUS_DISABLED) {
                leds[i] = CRGB::Black;
            } else if (zoneStatus == PORT_STATUS_ERROR) {
                // Моргаем кастомным цветом ошибки (раз в 500 мс)
                leds[i] = ((millis() / 500) % 2 == 0) ? visibleErrorColor(customError) : CRGB::Black;
            } else if (zoneStatus == PORT_STATUS_CHARGING) {
                leds[i] = customCharge; // Горит кастомным цветом зарядки
            } else {
                CRGB dynamicWait = customWait;
                dynamicWait.nscale8_video(pulseBright);
                leds[i] = dynamicWait;
            }
        }
        // Free zones: Logo, QR highlight, Service, Custom.
        else if (zone_is_free(zone)) {
            FreeZoneConfig *freeConfig = freeZoneConfigFor(zone);
            if (freeConfig == nullptr || freeConfig->enabled == 0) {
                leds[i] = CRGB::Black;
            } else if (freeConfig->mode == FREE_ZONE_RAINBOW) {
                leds[i] = CHSV(rainbowHue + (i * 4), 255, freeConfig->brightness);
            } else if (freeConfig->mode == FREE_ZONE_CUSTOM) {
                CRGB layerColor;
                if (getFreeZoneLayerColor(zone, i, layerColor)) {
                    layerColor.nscale8_video(freeConfig->brightness);
                    leds[i] = layerColor;
                } else {
                    leds[i] = CRGB::Black;
                }
            } else {
                CRGB staticColor = CRGB(freeConfig->staticColor[0], freeConfig->staticColor[1], freeConfig->staticColor[2]);
                staticColor.nscale8_video(freeConfig->brightness);
                leds[i] = staticColor;
            }
        }
    }
    FastLED.show();
}
```

### 8.1. Кто вызывает и какой у функции результат

Функцию вызывают:

- `led_setup()` один раз до запуска параллельной LED-задачи;
- `ledTaskWorker()` при обычном обновлении;
- безопасные функции `_safe`, уже получившие `ledMutex`.

Сама `led_refresh_internal()` не получает мьютекс. Вызывающий код обязан обеспечить безопасный момент.

Она читает:

- `zoneMap[]`;
- `cfg`;
- `freeZoneConfigs[]`;
- кэши слоёв;
- `breatheScale`;
- снимок статусов портов;
- текущее время.

Она изменяет:

- сохраняющийся `rainbowHue`;
- каждый элемент `leds[]`;
- физическую матрицу через `FastLED.show()`.

Функция имеет тип `void` и не возвращает код успеха. Она предполагает, что конфигурация и номера зон заранее проверены. Неизвестный `zoneId`, попавший в оперативную память в обход проверок, не имеет отдельной безопасной ветки и отмечен в разделе рисков.

---

## 9. `led_refresh_internal()`: буквальный построчный разбор

Для примеров ниже используется физический пиксель `i = 17`.

### 9.1. Начало кадра и радуга

```cpp
void led_refresh_internal() {
```

- Читает: ничего.
- Вычисляет: начало нового кадра.
- Изменяет: пока ничего.
- Пример: функция вызвана LED-задачей.
- Для пикселя 17: цвет ещё не выбран.

```cpp
static uint8_t rainbowHue = 0;
```

- Читает: сохранённое значение предыдущего кадра.
- Вычисляет: ничего.
- Изменяет: при первом вызове создаёт `rainbowHue = 0`.
- Пример: перед текущим кадром значение 41.
- Для пикселя 17: это базовый оттенок, если пиксель окажется в режиме радуги.

```cpp
rainbowHue += 1;
```

- Читает: старый `rainbowHue`.
- Вычисляет: старое значение плюс 1.
- Изменяет: `rainbowHue`.
- Пример: 41 становится 42.
- Для пикселя 17: будущий оттенок станет `42 + 17 * 4 = 110`.

### 9.2. Яркость пульса ожидания

```cpp
uint8_t pulseBright = (uint8_t)(220 * breatheScale);
```

- Читает: глобальный `breatheScale`.
- Вычисляет: множитель пульса от 0 до примерно 220.
- Изменяет: создаёт локальный `pulseBright`.
- Пример: `breatheScale = 0.5`, значит `pulseBright = 110`.
- Для пикселя 17: значение будет применено только при статусе ожидания `PORT_STATUS_WAITING`.

```cpp
if (pulseBright < 40) pulseBright = 40;
```

- Читает: `pulseBright`.
- Вычисляет: ниже ли значение минимальной видимой границы.
- Изменяет: при необходимости поднимает его до 40.
- Пример: 33 становится 40; 110 остаётся 110.
- Для пикселя 17: ожидание не станет полностью чёрным в нижней точке.

### 9.3. Запасные цвета портов

```cpp
CRGB customWait = CRGB(cfg.color_wait[0], cfg.color_wait[1], cfg.color_wait[2]);
```

- Читает: три канала `cfg.color_wait`.
- Вычисляет: объект цвета ожидания.
- Изменяет: создаёт локальный `customWait`.
- Пример: `[0,255,0]` становится `CRGB(0,255,0)`.
- Для пикселя 17: это запасной цвет ожидания, если индивидуального слоя нет.

```cpp
CRGB customCharge = CRGB(cfg.color_charge[0], cfg.color_charge[1], cfg.color_charge[2]);
```

- Читает: `cfg.color_charge`.
- Вычисляет: запасной цвет зарядки.
- Изменяет: создаёт `customCharge`.
- Пример: `[0,80,255]`.
- Для пикселя 17: используется при статусе зарядки `PORT_STATUS_CHARGING` без индивидуального цвета.

```cpp
CRGB customError = CRGB(cfg.color_error[0], cfg.color_error[1], cfg.color_error[2]);
```

- Читает: `cfg.color_error`.
- Вычисляет: запасной цвет ошибки.
- Изменяет: создаёт `customError`.
- Пример: `[255,0,0]`.
- Для пикселя 17: используется при статусе ошибки `PORT_STATUS_ERROR` без индивидуального цвета.

### 9.4. Общая яркость портов

```cpp
customWait.nscale8_video(cfg.bright_ports);
```

- Читает: `customWait` и `cfg.bright_ports`.
- Вычисляет: уменьшенный RGB.
- Изменяет: каналы `customWait`.
- Пример: зелёный 255 при яркости 120 становится примерно 120.
- Для пикселя 17: запасной цвет ожидания уже учитывает общую яркость портов.

```cpp
customCharge.nscale8_video(cfg.bright_ports);
```

- Читает: запасной цвет зарядки и общую яркость.
- Вычисляет: масштабированный цвет.
- Изменяет: `customCharge`.
- Пример: синий 255 при 120 становится примерно 120.
- Для пикселя 17: статус зарядки не использует яркость свободных зон.

```cpp
customError.nscale8_video(cfg.bright_ports);
```

- Читает: запасной цвет ошибки и общую яркость.
- Вычисляет: масштабированный цвет ошибки.
- Изменяет: `customError`.
- Пример: красный 255 при 120 становится примерно 120.
- Для пикселя 17: функция `visibleErrorColor()` позже не даст слишком тёмной ошибке исчезнуть.

Функция `nscale8_video()` масштабирует каждый RGB-канал коэффициентом 0..255 и старается не превращать ненулевой канал в ноль при малой яркости.

### 9.5. Снимок статусов

```cpp
StatusMapperState portState;
```

- Читает: ничего.
- Вычисляет: ничего.
- Изменяет: создаёт локальную структуру.
- Пример: поля ещё не следует использовать.
- Для пикселя 17: статус порта ещё неизвестен.

```cpp
status_mapper_get_snapshot(portState);
```

- Читает: последний защищённый `mapperState`.
- Вычисляет: согласованную копию.
- Изменяет: заполняет `portState`.
- Пример: `statuses = [PORT_STATUS_CHARGING, PORT_STATUS_WAITING, PORT_STATUS_DISABLED, PORT_STATUS_DISABLED]`.
- Для пикселя 17: если он Port1, будет прочитан `statuses[0]`.

### 9.6. Цикл по физическим светодиодам

```cpp
for (int i = 0; i < NUM_IC_CHIPS; i++) {
```

- Читает: `NUM_IC_CHIPS`, сейчас 96.
- Вычисляет: очередной физический номер от 0 до 95.
- Изменяет: счётчик `i`.
- Пример: рассматриваем итерацию `i = 17`.
- Для пикселя 17: именно в этой итерации будет записан `leds[17]`.

```cpp
uint8_t zone = zoneMap[i];
```

- Читает: `zoneMap[17]`.
- Вычисляет: номер зоны текущего физического пикселя.
- Изменяет: создаёт локальную `zone`.
- Пример: `zone = 1` для Port1 или `zone = 5` для Logo.
- Для пикселя 17: выбор дальнейшей ветки полностью зависит от этого значения.

### 9.7. Ветка zone 0

```cpp
if (zone == 0) {
```

- Читает: `zone`.
- Вычисляет: принадлежит ли пиксель зоне «Без зоны».
- Изменяет: ничего.
- Пример: `zoneMap[17] = 0`.
- Для пикселя 17: условие истинно.

```cpp
leds[i] = CRGB::Black;
```

- Читает: физический индекс `i`.
- Вычисляет: чёрный цвет.
- Изменяет: `leds[17] = (0,0,0)`.
- Пример: пиксель выключен.
- Для пикселя 17: этот цвет останется, потому что zone 0 не является ни port, ни free.

В этой ветке нет `continue`, но последующие проверки `zone_is_port(0)` и `zone_is_free(0)` ложны. Поэтому чёрный цвет не перезаписывается.

### 9.8. Подготовка индивидуального слоя статуса (`status layer`)

```cpp
else {
```

- Читает: результат `zone == 0`.
- Вычисляет: зона отлична от нуля.
- Изменяет: ничего.
- Пример: `zone = 1`.
- Для пикселя 17: начинается поиск индивидуального цвета.

```cpp
CRGB layerColor;
```

- Читает: ничего.
- Вычисляет: ничего.
- Изменяет: создаёт локальный цвет.
- Пример: он будет заполнен только при успешном поиске.
- Для пикселя 17: нельзя использовать его до результата функции поиска.

```cpp
if (zone_is_port(zone)) {
```

- Читает: `zone`.
- Вычисляет: находится ли номер между Port1 и Port4.
- Изменяет: ничего.
- Пример: zone 1 даёт true, zone 5 даёт false.
- Для пикселя 17: индивидуальный слой статуса ищется только для портовой зоны.

```cpp
const uint8_t portIndex = zone - ZONE_ID_PORT1;
```

- Читает: `zone` и константу 1.
- Вычисляет: индекс порта 0..3.
- Изменяет: создаёт неизменяемый `portIndex`.
- Пример: zone 1 даёт index 0.
- Для пикселя 17: статус берётся из `portState.statuses[0]`.

```cpp
const PortStatus zoneStatus = (portIndex < MAX_PORT_COUNT)
    ? portState.statuses[portIndex]
    : PORT_STATUS_DISABLED;
```

- Читает: `portIndex`, максимум 4 и снимок статусов.
- Вычисляет: безопасный статус.
- Изменяет: создаёт `zoneStatus`.
- Пример: `PORT_STATUS_WAITING`.
- Для пикселя 17: определяет, какой из трёх кэшей читать.

```cpp
if (zoneStatus == PORT_STATUS_DISABLED) {
```

- Читает: `zoneStatus`.
- Вычисляет: зарезервирован ли порт.
- Изменяет: ничего.
- Пример: Port3 при activePortCount 2.
- Для пикселя 17: если это зона отключённого порта `PORT_STATUS_DISABLED`, индивидуальный слой игнорируется.

```cpp
leds[i] = CRGB::Black;
continue;
```

- Читает: `i`.
- Вычисляет: чёрный цвет.
- Изменяет: `leds[i]`.
- `continue` сразу переходит к следующему физическому пикселю.
- Для пикселя 17: никакая следующая ветка не сможет его зажечь.

```cpp
if (getStatusLayerColorForStatus(i, zoneStatus, layerColor)) {
```

- Читает: физический индекс, статус и соответствующий кэш.
- Вычисляет: есть ли индивидуальный цвет.
- Изменяет: при успехе заполняет `layerColor`.
- Пример: `chargeLayer.hasColor[17] = true`.
- Для пикселя 17: при false код позже использует запасной цвет.

```cpp
layerColor.nscale8_video(cfg.bright_ports);
```

- Читает: индивидуальный цвет и общую яркость портов.
- Вычисляет: масштабированный цвет.
- Изменяет: `layerColor`.
- Пример: `(255,100,0)` при 120 становится примерно `(120,47,0)`.
- Для пикселя 17: индивидуальный цвет не имеет собственной отдельной яркости.

### 9.9. Индивидуальная ошибка

```cpp
if (zoneStatus == PORT_STATUS_ERROR) {
```

- Читает: статус.
- Вычисляет: нужна ли аварийная ветка.
- Изменяет: ничего.
- Для пикселя 17: условие истинно при `PORT_STATUS_ERROR`.

```cpp
layerColor = visibleErrorColor(layerColor);
```

- Читает: уже масштабированный цвет.
- Вычисляет: сумму R+G+B.
- Изменяет: `layerColor`.
- Пример: слишком тёмный `(5,5,5)` заменяется на `(255,0,0)`.
- Для пикселя 17: ошибка не исчезает из-за почти чёрной настройки.

```cpp
leds[i] = ((millis() / 500) % 2 == 0) ? layerColor : CRGB::Black;
```

- Читает: текущее время и `layerColor`.
- Вычисляет: чётную или нечётную половину секунды.
- Изменяет: `leds[17]`.
- Пример: на 1200 мс горит, на 1700 мс выключен.
- Для пикселя 17: получается мигание с переключением каждые 500 мс.

### 9.10. Индивидуальное ожидание и зарядка

```cpp
} else if (zoneStatus == PORT_STATUS_WAITING) {
```

- Читает: статус.
- Вычисляет: является ли он статусом ожидания `PORT_STATUS_WAITING`.
- Изменяет: ничего.
- Для пикселя 17: определяет применение пульса.

```cpp
layerColor.nscale8_video(pulseBright);
leds[i] = layerColor;
```

- Читает: индивидуальный цвет и `pulseBright`.
- Вычисляет: дополнительное плавное затемнение.
- Изменяет: сначала `layerColor`, затем `leds[17]`.
- Пример: яркость порта дала зелёный 120, пульс 110 уменьшает его примерно до 52.
- Для пикселя 17: ожидание плавно светлеет и темнеет.

```cpp
} else {
    leds[i] = layerColor;
}
```

- Читает: цвет после общей яркости.
- Вычисляет: это оставшийся активный статус, обычно зарядка `PORT_STATUS_CHARGING`.
- Изменяет: `leds[17]`.
- Пример: зарядка горит постоянно цветом слоя.

```cpp
continue;
```

- Читает: ничего.
- Вычисляет: индивидуальный цвет полностью обработан.
- Изменяет: счётчик цикла перейдёт к следующему пикселю.
- Для пикселя 17: запасной цвет и свободные зоны больше не проверяются.

### 9.11. Ветка запасного цвета портовой зоны

Если слой статуса не содержал цвета, выполнение доходит сюда:

```cpp
if (zone_is_port(zone)) {
```

- Читает: `zone`.
- Вычисляет: портовая ли это зона.
- Изменяет: ничего.
- Для пикселя 17: true для Port1.

```cpp
const uint8_t portIndex = zone - ZONE_ID_PORT1;
```

- Читает: номер зоны.
- Вычисляет: индекс порта.
- Изменяет: локальный `portIndex`.
- Пример: Port1 -> 0.

```cpp
const PortStatus zoneStatus = (portIndex < MAX_PORT_COUNT)
    ? portState.statuses[portIndex]
    : PORT_STATUS_DISABLED;
```

- Читает: снимок статусов.
- Вычисляет: статус или безопасное состояние отключённого порта `PORT_STATUS_DISABLED`.
- Изменяет: локальный `zoneStatus`.
- Пример: ожидание `PORT_STATUS_WAITING`.

```cpp
if (zoneStatus == PORT_STATUS_DISABLED) {
    leds[i] = CRGB::Black;
```

- Читает: статус.
- Вычисляет: порт зарезервирован.
- Изменяет: делает текущий пиксель чёрным.

```cpp
} else if (zoneStatus == PORT_STATUS_ERROR) {
    leds[i] = ((millis() / 500) % 2 == 0) ? visibleErrorColor(customError) : CRGB::Black;
```

- Читает: время и `customError`.
- Вычисляет: видимый цвет ошибки и фазу мигания.
- Изменяет: `leds[i]`.
- Пример: красный/чёрный каждые 500 мс.

```cpp
} else if (zoneStatus == PORT_STATUS_CHARGING) {
    leds[i] = customCharge;
```

- Читает: заранее масштабированный `customCharge`.
- Вычисляет: ничего дополнительного.
- Изменяет: `leds[i]`.
- Пример: `(0,38,120)` при исходном `(0,80,255)` и яркости 120.

```cpp
} else {
    CRGB dynamicWait = customWait;
```

- Читает: `customWait`.
- Вычисляет: копию, которую можно дополнительно изменить.
- Изменяет: создаёт `dynamicWait`.
- Для пикселя 17: общая `customWait` не портится для других пикселей.

```cpp
dynamicWait.nscale8_video(pulseBright);
leds[i] = dynamicWait;
```

- Читает: `pulseBright`.
- Вычисляет: пульсирующий цвет ожидания.
- Изменяет: `dynamicWait`, затем `leds[i]`.

### 9.12. Ветка свободной зоны

```cpp
else if (zone_is_free(zone)) {
```

- Читает: `zone`.
- Вычисляет: Logo/QR/Service/Custom ли это.
- Изменяет: ничего.
- Пример: zone 5, Logo.

```cpp
FreeZoneConfig *freeConfig = freeZoneConfigFor(zone);
```

- Читает: `freeZoneConfigs[]`.
- Вычисляет: адрес конфигурации нужной зоны.
- Изменяет: создаёт указатель `freeConfig`.
- Пример: адрес `freeZoneConfigs[0]` для Logo.
- Символ `*` означает, что переменная хранит адрес объекта.

```cpp
if (freeConfig == nullptr || freeConfig->enabled == 0) {
```

- Читает: указатель и `enabled`.
- Вычисляет: существует ли конфигурация и включена ли зона.
- Изменяет: ничего.
- Оператор `||` означает «или».

```cpp
leds[i] = CRGB::Black;
```

- При отсутствии или выключении конфигурации делает пиксель чёрным.

```cpp
} else if (freeConfig->mode == FREE_ZONE_RAINBOW) {
```

- Читает: режим.
- Вычисляет: включена ли радуга.

```cpp
leds[i] = CHSV(rainbowHue + (i * 4), 255, freeConfig->brightness);
```

- Читает: базовый оттенок, физический индекс и яркость зоны.
- Вычисляет: оттенок со сдвигом для пикселя.
- Изменяет: `leds[i]`.
- Пример: `rainbowHue=42`, `i=17`, оттенок 110, насыщенность 255, яркость 100.

```cpp
} else if (freeConfig->mode == FREE_ZONE_CUSTOM) {
```

- Читает: режим.
- Вычисляет: нужен ли нарисованный слой.

```cpp
CRGB layerColor;
```

- Создаёт временный цвет.

```cpp
if (getFreeZoneLayerColor(zone, i, layerColor)) {
```

- Читает: кэш выбранной свободной зоны.
- Вычисляет: есть ли цвет именно для этого физического пикселя.
- Изменяет: при успехе заполняет `layerColor`.

```cpp
layerColor.nscale8_video(freeConfig->brightness);
leds[i] = layerColor;
```

- Читает: яркость выбранной свободной зоны.
- Вычисляет: уменьшенный цвет пользовательского рисунка.
- Изменяет: `layerColor`, затем `leds[i]`.

```cpp
} else {
    leds[i] = CRGB::Black;
}
```

- Если пользовательский слой не содержит пиксель, он становится чёрным.

```cpp
} else {
```

- Это оставшийся режим `FREE_ZONE_STATIC`.

```cpp
CRGB staticColor = CRGB(freeConfig->staticColor[0], freeConfig->staticColor[1], freeConfig->staticColor[2]);
```

- Читает: три канала постоянного цвета.
- Вычисляет: объект `CRGB`.
- Изменяет: создаёт `staticColor`.
- Пример: Logo `[255,255,255]`.

```cpp
staticColor.nscale8_video(freeConfig->brightness);
```

- Читает: яркость конкретной свободной зоны.
- Вычисляет: масштабированный цвет.
- Изменяет: `staticColor`.

```cpp
leds[i] = staticColor;
```

- Записывает итоговый постоянный цвет текущему физическому пикселю.

### 9.13. Отправка кадра

```cpp
FastLED.show();
```

- Читает: все 96 элементов `leds[]`.
- Вычисляет: последовательность данных с учётом настроенного типа и порядка BRG; библиотека также учитывает лимит питания.
- Изменяет: физическое состояние всей матрицы.
- Пример: `leds[17] = CRGB(120,0,0)` приводит к красному физическому пикселю 17.

Функция вызывается один раз после расчёта всего массива, а не после каждого пикселя. Поэтому физическая матрица получает согласованный кадр.

---

## 10. Четыре полных примера одного пикселя

### 10.1. Пиксель zone 0

Условия:

```text
i = 17
zoneMap[17] = 0
старое leds[17] = синий
```

Путь:

1. `zone = 0`.
2. Выполняется `leds[17] = CRGB::Black`.
3. `zone_is_port(0)` возвращает false.
4. `zone_is_free(0)` возвращает false.
5. В конце `FastLED.show()` отправляет чёрный.

Результат: физический пиксель 17 выключен независимо от статусов и свободных настроек.

### 10.2. Port1 при ожидании

Условия:

```text
i = 17
zoneMap[17] = 1
portState.statuses[0] = PORT_STATUS_WAITING
cfg.bright_ports = 120
breatheScale = 0.5
pulseBright = 110
```

Вариант со слоем статуса:

```text
waitLayer.hasColor[17] = true
waitLayer.colors[17] = CRGB(0,255,0)
```

1. Цвет слоя масштабируется 120/255: примерно `(0,120,0)`.
2. Затем применяется пульс 110/255: примерно `(0,52,0)`.
3. `leds[17]` получает зелёный `(0,52,0)`.
4. Следующие кадры меняют `breatheScale`, поэтому яркость плавно меняется.

Вариант без слоя статуса:

1. `getStatusLayerColorForStatus()` возвращает false.
2. Используется `cfg.color_wait`.
3. Сначала применяется `bright_ports`.
4. Затем `pulseBright`.

Результат: ожидание всегда использует общую яркость портов и дополнительный пульс.

### 10.3. Port1 при ошибке

Условия:

```text
i = 17
zoneMap[17] = 1
portState.statuses[0] = PORT_STATUS_ERROR
cfg.bright_ports = 120
```

Если в `errorLayer` есть яркий красный:

1. Берётся индивидуальный цвет.
2. Применяется `bright_ports`.
3. `visibleErrorColor()` проверяет сумму каналов.
4. Каждые 500 мс `leds[17]` переключается между цветом и чёрным.

Если пользователь сохранил почти чёрный цвет ошибки:

```text
после яркости layerColor = (5, 0, 0)
сумма каналов = 5 < 40
```

`visibleErrorColor()` заменяет его на `(255,0,0)`.

Результат: ошибка мигает и не исчезает из-за слишком тёмной пользовательской настройки. Общий лимит питания FastLED всё равно остаётся активным.

### 10.4. Logo в режимах постоянного цвета, пользовательского рисунка и радуги

Общие условия:

```text
i = 17
zoneMap[17] = ZONE_ID_LOGO = 5
freeConfig->enabled = 1
freeConfig->brightness = 100
```

#### Static

```text
mode = FREE_ZONE_STATIC
staticColor = [255,255,255]
```

1. Создаётся белый `CRGB`.
2. Применяется яркость 100/255.
3. `leds[17]` становится примерно `(100,100,100)`.

#### Custom

```text
mode = FREE_ZONE_CUSTOM
freeZoneLayers[Logo].hasColor[17] = true
colors[17] = CRGB(255,0,100)
```

1. Цвет берётся из кэша рисунка.
2. Применяется яркость Logo 100.
3. `leds[17]` получает примерно `(100,0,39)`.

Если `hasColor[17] == false`, пиксель будет чёрным.

#### Rainbow

```text
mode = FREE_ZONE_RAINBOW
rainbowHue = 42
```

1. Оттенок равен `42 + 17*4 = 110`.
2. Насыщенность равна 255.
3. Яркость равна 100.
4. FastLED преобразует `CHSV(110,255,100)` в RGB.
5. В следующем кадре `rainbowHue` увеличится, и цвет изменится.

---

## 11. `led_refresh_safe()`

### 11.1. Полный актуальный код

```cpp
void led_refresh_safe() {
    if (ledMutex == NULL) return;
    if (xSemaphoreTake(ledMutex, portMAX_DELAY) == pdTRUE) {
        led_refresh_internal();
        xSemaphoreGive(ledMutex);
    }
}
```

### 11.2. Работа

1. Если мьютекс ещё не создан, функция ничего не делает.
2. `portMAX_DELAY` означает ждать освобождения мьютекса без заданного короткого предела.
3. После получения блокировки строится кадр.
4. Мьютекс освобождается.

Функция вызывается:

- после успешного изменения общей яркости портов;
- после успешного сохранения свободной зоны.

Она ничего не возвращает. При `ledMutex == NULL` обновление тихо пропускается.

---

## 12. `led_replace_zone_map_safe()`

### 12.1. Полный актуальный код

```cpp
void led_replace_zone_map_safe(const uint8_t *zones, size_t zoneCount) {
    if (zones == nullptr || zoneCount != NUM_IC_CHIPS) return;

    if (ledMutex == NULL) {
        memcpy(zoneMap, zones, sizeof(zoneMap));
        return;
    }

    if (xSemaphoreTake(ledMutex, portMAX_DELAY) == pdTRUE) {
        memcpy(zoneMap, zones, sizeof(zoneMap));
        led_refresh_internal();
        xSemaphoreGive(ledMutex);
    }
}
```

### 12.2. Работа

- Проверяется указатель и точное число 96 элементов.
- Если LED-модуль ещё не запущен, массив просто копируется.
- При работающей задаче мьютекс удерживается во время копирования и первого нового кадра.

`memcpy()` копирует все байты массива `zones` в глобальный `zoneMap`.

Реальное место вызова: `handleSaveZones()` после успешной записи NVS.

При неправильном указателе или размере функция ничего не меняет и не возвращает сообщение об ошибке. HTTP-обработчик заранее создаёт массив правильного размера.

---

## 13. `led_apply_matrix_config_safe()`

### 13.1. Полный актуальный код

```cpp
void led_apply_matrix_config_safe(const MatrixConfig &config, const uint8_t *zones, size_t zoneCount) {
    if (zones == nullptr || zoneCount != NUM_IC_CHIPS) return;

    if (ledMutex == NULL) {
        cfg = config;
        memcpy(zoneMap, zones, sizeof(zoneMap));
        led_reload_status_layers_unlocked();
        led_reload_free_zone_layers_unlocked();
        return;
    }

    if (xSemaphoreTake(ledMutex, portMAX_DELAY) == pdTRUE) {
        cfg = config;
        memcpy(zoneMap, zones, sizeof(zoneMap));
        led_reload_status_layers_unlocked();
        led_reload_free_zone_layers_unlocked();
        led_refresh_internal();
        xSemaphoreGive(ledMutex);
    }
}
```

### 13.2. Работа

Параметр `const MatrixConfig &config` передаёт структуру по ссылке и запрещает функции менять объект вызывающего кода.

Функция согласованно:

1. заменяет `cfg`;
2. заменяет физический `zoneMap`;
3. перестраивает слой статуса по новой топологии;
4. перестраивает пользовательский слой свободных зон;
5. показывает новый кадр.

Реальное место вызова: `handleSaveTopology()` после успешного пересчёта (`remap`) и сохранения.

Слои хранятся по координатам, поэтому их JSON не переписывается. Меняется только перевод координат в физический индекс внутри кэшей.

При неправильном размере функция тихо возвращается. При `ledMutex == NULL` данные применяются без кадра, потому что постоянная задача ещё не должна работать.

---

## 14. `led_reload_status_layers_safe()`

### 14.1. Полный актуальный код

```cpp
void led_reload_status_layers_safe() {
    if (ledMutex == NULL) {
        led_reload_status_layers_unlocked();
        return;
    }

    if (xSemaphoreTake(ledMutex, portMAX_DELAY) == pdTRUE) {
        led_reload_status_layers_unlocked();
        led_refresh_internal();
        xSemaphoreGive(ledMutex);
    }
}
```

### 14.2. Работа

До старта задачи она просто загружает кэши.

Во время работы:

1. ждёт `ledMutex`;
2. перечитывает слои ожидания, зарядки и ошибки;
3. сразу показывает новый слой;
4. освобождает мьютекс.

Реальное место вызова: `handleSaveStatusColors()` после успешного сохранения слоя и возможного запасного цвета.

---

## 15. `led_reload_free_zone_layers_safe()`

### 15.1. Полный актуальный код

```cpp
void led_reload_free_zone_layers_safe() {
    if (ledMutex == NULL) {
        led_reload_free_zone_layers_unlocked();
        return;
    }

    if (xSemaphoreTake(ledMutex, portMAX_DELAY) == pdTRUE) {
        led_reload_free_zone_layers_unlocked();
        led_refresh_internal();
        xSemaphoreGive(ledMutex);
    }
}
```

### 15.2. Работа

Функция перестраивает кэши Logo, QR, Service и Custom под текущие `freeZoneConfigs[]`, `zoneMap[]` и топологию.

Реальные места вызова:

- `handleSaveZones()` после замены разметки;
- `handleSaveFreeZone()` после сохранения свободной зоны.

В `handleSaveFreeZone()` после этой функции дополнительно вызывается `led_refresh_safe()`. Это приводит к ещё одному кадру. Функционально результат тот же, но второй refresh является избыточным с точки зрения частоты обновления.

---

## 16. Различие безопасных и внутренних методов

Слово `safe` в именах означает безопасную внешнюю обёртку: функция сама организует доступ через `ledMutex`.

Слово `unlocked` означает внутреннюю функцию без собственной блокировки: она доверяет вызывающему коду.

### Правило использования

```text
внешний HTTP-код
        ↓
безопасная функция safe получает ledMutex
        ↓
внутренняя функция unlocked меняет кэш
        ↓
led_refresh_internal() строит кадр
        ↓
безопасная функция safe освобождает ledMutex
```

При старте параллельной LED-задачи ещё нет:

```text
led_load_config_from_flash()
        ↓
внутренние функции unlocked разрешены без ledMutex
```

Нельзя вызвать внутреннюю функцию `unlocked` напрямую из HTTP-обработчика во время работы. Иначе LED-задача может читать кэш в момент его очистки и заполнения.

---

## 17. Реальные места вызова

| Функция | Где вызывается | Зачем |
|---|---|---|
| `led_setup()` | `setup()` в `main.cpp` | полностью запустить матрицу |
| `led_load_config_from_flash()` | только `led_setup()` | загрузить настройки до старта задачи |
| `ledTaskWorker()` | передаётся в `xTaskCreatePinnedToCore()` | постоянно обновлять пульс и кадр |
| `led_refresh_internal()` | `led_setup()`, `ledTaskWorker()`, все safe-обёртки | вычислить и отправить кадр |
| `led_refresh_safe()` | `handleSetBright()`, `handleSaveFreeZone()` | показать уже применённые настройки |
| `led_replace_zone_map_safe()` | `handleSaveZones()` | заменить только карту зон |
| `led_apply_matrix_config_safe()` | `handleSaveTopology()` | согласованно применить топологию и пересчитанный `zoneMap` |
| `led_reload_status_layers_safe()` | `handleSaveStatusColors()` | перечитать статусные слои |
| `led_reload_free_zone_layers_safe()` | `handleSaveZones()`, `handleSaveFreeZone()` | перечитать пользовательские слои свободных зон |
| `led_reload_status_layers_unlocked()` | загрузка при старте, `led_apply_matrix_config_safe()`, `led_reload_status_layers_safe()` | перестроить кэш без собственной блокировки |
| `led_reload_free_zone_layers_unlocked()` | загрузка при старте, `led_apply_matrix_config_safe()`, `led_reload_free_zone_layers_safe()` | перестроить кэш без собственной блокировки |

---

## 18. Что в итоге определяет цвет физического пикселя

Для физического номера `i` порядок решений такой:

```text
zoneMap[i] == 0?
    да -> чёрный
    нет
      ↓
это Port1..Port4?
    да
      ↓
    порт отключён (disabled)?
        да -> чёрный
        нет
          ↓
        есть цвет слоя статуса?
            да -> этот цвет + bright_ports
            нет -> запасной цвет cfg + bright_ports
          ↓
        ожидание waiting -> дополнительный breatheScale
        зарядка charging -> постоянный цвет
        ошибка error -> visibleErrorColor + мигание 500 мс

это Logo/QR/Service/Custom?
    да
      ↓
    зона выключена? -> чёрный
    постоянный цвет static -> staticColor + brightness зоны
    пользовательский рисунок custom -> custom layer + brightness зоны
    радуга rainbow -> CHSV + brightness зоны
```

После обработки всех 96 физических номеров один вызов `FastLED.show()` передаёт согласованный кадр матрице.

---

## 19. Технические риски, видимые по текущему коду

1. `led_setup()` не проверяет результат `xSemaphoreCreateMutex()`.
2. `led_setup()` не проверяет результат создания `LED_Task`.
3. Ожидание 10 передано как тики напрямую. Для явно заданных миллисекунд понятнее было бы использовать `pdMS_TO_TICKS()`, но это уже отдельное изменение кода.
4. `handleSaveFreeZone()` после `led_reload_free_zone_layers_safe()` вызывает ещё один `led_refresh_safe()`.
5. Если в оперативный `zoneMap` каким-то обходным путём попадёт неизвестный zoneId, ветки port/free его не обработают и пиксель может сохранить цвет прошлого кадра. Текущие storage и HTTP-проверки не должны пропускать такой zoneId.
6. Размер стека 4096 и частоту кадров нужно подтвердить длительным тестом на ESP32.

Эти пункты не меняют описанную штатную работу. Они являются предметом отдельного аудита надёжности, а не основанием менять код в рамках документационного этапа.
