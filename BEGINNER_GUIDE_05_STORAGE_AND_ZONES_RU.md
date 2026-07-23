# Хранение настроек и модель зон

Этот документ объясняет, какие данные существуют в прошивке, где они находятся,
как записываются во flash-память ESP32 и как восстанавливаются после
перезагрузки. Разбор относится к текущему коду проекта, прежде всего к файлам:

- `src/led_manager.h`;
- `src/led_manager.cpp`;
- `src/matrix_config_storage.h`;
- `src/matrix_config_storage.cpp`;
- `src/zone_model.h`;
- `src/main.cpp`;
- `src/network_config.cpp`;
- `src/auth_config.cpp`.

Слово «хранилище настроек» далее означает специальную область flash-памяти
ESP32 (NVS, Non-Volatile Storage). Она сохраняет значения после отключения
питания.

---

## 1. Четыре уровня данных

Одна из причин, по которой код сначала кажется сложным: похожая настройка
проходит через несколько представлений. Это не четыре независимых источника
истины. Это четыре этапа жизни одних данных.

### 1.1. Константы сборки

Константы сборки задаются в исходном коде. Они становятся частью файла
прошивки при компиляции и не меняются из web-интерфейса.

Примеры:

```cpp
#define MATRIX_X        12
#define VIRTUAL_Y       8
#define NUM_IC_CHIPS    (MATRIX_X * VIRTUAL_Y)

static constexpr uint8_t DEFAULT_ACTIVE_PORT_COUNT = 2;
```

| Значение | Файл | Кто задаёт | Кто читает | После reboot | Пример |
|---|---|---|---|---|---|
| `MATRIX_X` | `led_manager.h` | разработчик до сборки | LED-код, storage, HTTP API | остаётся частью прошивки | `12` столбцов |
| `VIRTUAL_Y` | `led_manager.h` | разработчик до сборки | LED-код, storage, HTTP API | остаётся частью прошивки | `8` строк |
| `NUM_IC_CHIPS` | `led_manager.h` | вычисляет компилятор | размеры массивов | остаётся частью прошивки | `12 * 8 = 96` |
| `DEFAULT_ACTIVE_PORT_COUNT` | `board_config.h` | разработчик | defaults и `status_mapper` | остаётся частью прошивки | `2` порта |
| `ZONE_ID_*` | `zone_model.h` | разработчик | backend, storage, LED-код, UI JSON | остаются частью прошивки | Port1 имеет ID `1` |

Если изменить `MATRIX_X` с 12 на другое значение, простая перезагрузка ничего
не изменит. Нужно собрать и прошить новый бинарный файл.

### 1.2. Рабочие данные в RAM

Оперативная память (RAM) хранит значения, с которыми прошивка работает прямо
сейчас. При отключении питания RAM очищается.

Главные рабочие переменные определены в `led_manager.cpp`:

```cpp
CRGB leds[NUM_IC_CHIPS];
uint8_t zoneMap[NUM_IC_CHIPS];
MatrixConfig cfg;
FreeZoneConfig freeZoneConfigs[FREE_ZONE_COUNT];
```

| Переменная | Кто создаёт | Кто меняет | Кто читает | Сохраняется сама после reboot | Пример |
|---|---|---|---|---|---|
| `leds[]` | статически создаёт `led_manager.cpp` | `led_refresh_internal()` | `FastLED.show()` | нет | физический пиксель 0 сейчас красный |
| `zoneMap[]` | статически создаёт `led_manager.cpp` | загрузка NVS и safe-функции | отрисовка и HTTP API | нет, но копия хранится в `matrix_cfg` | индекс 0 принадлежит Port1 |
| `cfg` | статически создаёт `led_manager.cpp` | загрузка NVS и save handlers | отрисовка и `/get_config` | нет, но копия хранится в `matrix_cfg` | topology 0, brightness 120 |
| `freeZoneConfigs[]` | статически создаёт `led_manager.cpp` | загрузка NVS и `/save_free_zone` | отрисовка и `/get_config` | нет, но копия хранится в `free_zones_v1` | Logo static, brightness 100 |
| `waitLayer`, `chargeLayer`, `errorLayer` | `led_manager.cpp` | функции reload | отрисовка | нет, исходный JSON хранится в NVS | у координаты `2-3` задан цвет |
| `freeZoneLayers[]` | `led_manager.cpp` | функции reload | отрисовка custom-зон | нет, исходный JSON хранится в NVS | custom-цвет Logo |

После reboot эти переменные создаются заново, затем заполняются из NVS.

### 1.3. Упаковка для NVS

NVS умеет хранить отдельные числа, строки и массив байтов. Для связанных
матричных настроек код создаёт структуры-«контейнеры».

```cpp
struct StoredMatrixConfig {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint16_t matrixX;
    uint16_t matrixY;
    MatrixConfig config;
    uint8_t zones[NUM_IC_CHIPS];
};
```

`StoredMatrixConfig` содержит не только рабочий `MatrixConfig`, но и служебные
поля для проверки совместимости, а также `zoneMap`.

Аналогично:

```cpp
struct StoredFreeZoneConfigV1 {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint16_t matrixX;
    uint16_t matrixY;
    FreeZoneConfig zones[FREE_ZONE_COUNT];
};
```

Буквы `V1` означают первую и текущую версию формата хранения free zones. Это
не доказательство ненужного старого кода. Имя показывает, как интерпретировать
байты в NVS и позволяет в будущем ввести V2 без неоднозначности.

### 1.4. JSON браузера

Браузер не получает прямой доступ к C++-структурам и NVS. Он обменивается с
ESP32 текстом JSON:

```json
{
  "0-0": 1,
  "1-0": 1,
  "2-0": 2
}
```

В этом примере:

- ключ `"0-0"` означает логическую координату `x=0, y=0`;
- значение `1` означает `ZONE_ID_PORT1`;
- значение `2` означает `ZONE_ID_PORT2`.

В браузере есть своё временное состояние JavaScript. Пока инженер не нажал
«Сохранить зоны», изменённая карта существует только в браузере. После POST
`/save_zones` backend проверяет JSON, переводит координаты в физические индексы,
сохраняет данные в NVS и только затем обновляет рабочий `zoneMap[]`.

### 1.5. Один пример на всех четырёх уровнях

Пусть инженер назначил координату `0-0` зоне Port1:

1. Константа `ZONE_ID_PORT1` в коде равна `1`.
2. В browser JSON появляется `"0-0":1`.
3. Backend вычисляет физический индекс через `getLEDIndex(0, 0)` и записывает
   `1` в соответствующий элемент временного массива.
4. `StoredMatrixConfig.zones[]` сохраняется под ключом `matrix_cfg`.
5. После успешной записи рабочий `zoneMap[]` в RAM получает ту же карту.
6. После reboot `matrix_config_load()` восстанавливает её из `matrix_cfg`.

---

## 2. `MatrixConfig`: рабочие настройки портовых зон

Структура объявлена в `src/led_manager.h`:

```cpp
struct MatrixConfig {
    uint8_t topology;
    uint8_t bright_ports;

    uint8_t color_wait[3];
    uint8_t color_charge[3];
    uint8_t color_error[3];
};
```

### 2.1. Поля

#### `uint8_t topology`

Однобайтовое число от 0 до 3. Оно выбирает один из четырёх способов перевода
логической координаты `x-y` в физический номер светодиода.

Пример:

```cpp
cfg.topology = 0;
```

#### `uint8_t bright_ports`

Общая яркость всех портовых зон. Допустимое рабочее значение в текущем storage
равно 1..255. Ноль считается невалидным.

Пример:

```cpp
cfg.bright_ports = 120;
```

#### `color_wait[3]`

Три байта RGB: красный, зелёный, синий для fallback-цвета ожидания.

```cpp
{0, 255, 0}
```

Это зелёный цвет.

#### `color_charge[3]`

Fallback-цвет зарядки:

```cpp
{0, 80, 255}
```

#### `color_error[3]`

Fallback-цвет ошибки:

```cpp
{255, 0, 0}
```

Слово fallback здесь означает запасной цвет. Если для конкретной координаты
не задан цвет в выбранном status layer, функция отрисовки использует этот
общий цвет.

### 2.2. Кто создаёт, меняет и читает

- Глобальный объект `cfg` создаёт `led_manager.cpp`.
- `matrix_config_load()` заполняет его при запуске.
- `/set_bright`, `/save_status_colors` и `/save_topology` готовят изменённую
  копию и записывают её.
- LED-код читает `cfg` при каждом вычислении цветов.
- `/get_config` передаёт значения браузеру.

---

## 3. `FreeZoneConfig`: рабочая настройка одной свободной зоны

Структура объявлена в `src/led_manager.h`:

```cpp
struct FreeZoneConfig {
    uint8_t zoneId;
    uint8_t enabled;
    FreeZoneMode mode;
    uint8_t staticColor[3];
    uint8_t brightness;
};
```

### 3.1. Поля

| Поле | Смысл | Пример |
|---|---|---|
| `zoneId` | ID конкретной free zone | `ZONE_ID_LOGO`, то есть 5 |
| `enabled` | разрешено ли отображение | `1` |
| `mode` | static, custom или rainbow | `FREE_ZONE_STATIC` |
| `staticColor[3]` | RGB для static и запасной цвет | `{255,255,255}` |
| `brightness` | индивидуальная яркость зоны | `100` |

В RAM существует массив:

```cpp
FreeZoneConfig freeZoneConfigs[FREE_ZONE_COUNT];
```

Порядок элементов соответствует `FREE_ZONE_IDS`:

1. Logo;
2. QR;
3. Service;
4. Custom.

Сам пиксельный custom-рисунок не помещён внутрь `FreeZoneConfig`. Он хранится
отдельной JSON-строкой. Благодаря этому небольшой основной config не раздувается
до размера всей матрицы.

---

## 4. `StoredMatrixConfig`: контейнер постоянного хранения

Актуальный код:

```cpp
struct StoredMatrixConfig {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint16_t matrixX;
    uint16_t matrixY;
    MatrixConfig config;
    uint8_t zones[NUM_IC_CHIPS];
};
```

### 4.1. Построчный смысл

```cpp
uint32_t magic;
```

Контрольная сигнатура. Текущий код пишет `0x314D444C`, комментарий обозначает
её как `"LDM1"`. Если в ключе лежат случайные байты или данные другого типа,
значение почти наверняка не совпадёт.

```cpp
uint16_t version;
```

Версия формата. Сейчас `MATRIX_CONFIG_VERSION = 1`.

```cpp
uint16_t size;
```

Размер всей структуры во время записи. После обновления структуры старое
содержимое с другим размером можно отвергнуть.

```cpp
uint16_t matrixX;
uint16_t matrixY;
```

Размер матрицы, для которого сохранена конфигурация. При compile-time переходе
с 12x8 на другой размер старая карта не применяется.

```cpp
MatrixConfig config;
```

Рабочие topology, port brightness и fallback-цвета.

```cpp
uint8_t zones[NUM_IC_CHIPS];
```

Физическая карта зон. Каждый элемент соответствует физическому индексу LED,
а значение является `zoneId`.

### 4.2. Почему это не дублирование `MatrixConfig`

`MatrixConfig` нужен работающей прошивке: это небольшой понятный объект с
настройками отображения.

`StoredMatrixConfig` нужен только границе хранения. Он добавляет:

- сигнатуру;
- версию;
- размер структуры;
- размеры матрицы;
- `zoneMap`.

Если заставить LED-код работать прямо со `StoredMatrixConfig`, служебные поля
NVS проникнут в отрисовку. Если сохранять только `MatrixConfig`, код не сможет
проверить размер и восстановить `zoneMap`. Поэтому структуры выполняют разные
обязанности.

---

## 5. `StoredFreeZoneConfigV1`

Актуальный код:

```cpp
struct StoredFreeZoneConfigV1 {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint16_t matrixX;
    uint16_t matrixY;
    FreeZoneConfig zones[FREE_ZONE_COUNT];
};
```

Поля `magic`, `version`, `size`, `matrixX`, `matrixY` решают те же задачи, что
и у matrix config. Массив `zones[]` содержит четыре настройки
`FreeZoneConfig`.

`V1` обозначает текущую версию двоичного формата. Удалять эту структуру как
«legacy» нельзя: ключ `free_zones_v1` прямо сейчас записывается и читается
этим типом.

Custom layers не входят в эту структуру. Они хранятся в отдельных ключах
`fz5_layer`...`fz8_layer`.

---

## 6. Default-настройки матрицы

Функция находится в `matrix_config_storage.cpp` и является внутренней:
слово `static` ограничивает её видимость этим `.cpp`.

```cpp
static void matrix_config_set_defaults(MatrixConfig &config, uint8_t *zones, size_t zoneCount) {
    memset(&config, 0, sizeof(config));

    config.topology = 0;
    config.bright_ports = 120;

    if (zones != nullptr && zoneCount > 0) {
        const uint8_t activePorts = DEFAULT_ACTIVE_PORT_COUNT == 0
            ? 1
            : (DEFAULT_ACTIVE_PORT_COUNT > MAX_PORT_COUNT
                ? MAX_PORT_COUNT
                : DEFAULT_ACTIVE_PORT_COUNT);

        for (size_t i = 0; i < zoneCount; i++) {
            const uint8_t portOffset = (uint8_t)((i * activePorts) / zoneCount);
            zones[i] = (uint8_t)(ZONE_ID_PORT1 + portOffset);
        }
    }

    config.color_wait[0] = 0;
    config.color_wait[1] = 255;
    config.color_wait[2] = 0;

    config.color_charge[0] = 0;
    config.color_charge[1] = 80;
    config.color_charge[2] = 255;

    config.color_error[0] = 255;
    config.color_error[1] = 0;
    config.color_error[2] = 0;
}
```

### 6.1. Пошаговый разбор

1. `memset()` заполняет весь `config` нулями. Это исключает случайные
   неинициализированные байты.
2. Topology становится 0.
3. Общая яркость портовых зон становится 120.
4. Проверяется, передан ли настоящий массив зон и не равен ли его размер нулю.
5. `activePorts` ограничивается диапазоном 1..`MAX_PORT_COUNT`.
6. Цикл проходит по всем физическим индексам.
7. Формула `(i * activePorts) / zoneCount` делит массив на равные
   последовательные части.
8. К `ZONE_ID_PORT1` добавляется номер части.
9. Устанавливаются зелёный waiting, синий charging и красный error.

Для 96 пикселей и двух активных портов:

- `i=0`: `(0*2)/96 = 0`, зона Port1;
- `i=47`: `(47*2)/96 = 0`, зона Port1;
- `i=48`: `(48*2)/96 = 1`, зона Port2;
- `i=95`: `(95*2)/96 = 1`, зона Port2.

Это деление выполняется по физическим индексам. Визуальный вид половин зависит
от topology, с которой затем координаты переводятся в эти индексы.

---

## 7. Default-настройки свободных зон

```cpp
static void matrix_config_set_free_zone_defaults(FreeZoneConfig *freeZones, size_t freeZoneCount) {
    if (freeZones == nullptr || freeZoneCount != FREE_ZONE_COUNT) {
        return;
    }

    for (size_t i = 0; i < FREE_ZONE_COUNT; i++) {
        const uint8_t zoneId = FREE_ZONE_IDS[i];
        const ZoneMetadata *metadata = zone_metadata_for(zoneId);
        freeZones[i].zoneId = zoneId;
        freeZones[i].enabled = 1;
        freeZones[i].mode = metadata ? metadata->defaultMode : FREE_ZONE_STATIC;
        freeZones[i].brightness = 100;
        freeZones[i].staticColor[0] = 255;
        freeZones[i].staticColor[1] = 255;
        freeZones[i].staticColor[2] = 255;
    }

    freeZones[free_zone_index(ZONE_ID_QR)].staticColor[0] = 52;
    freeZones[free_zone_index(ZONE_ID_QR)].staticColor[1] = 199;
    freeZones[free_zone_index(ZONE_ID_QR)].staticColor[2] = 89;

    freeZones[free_zone_index(ZONE_ID_SERVICE)].staticColor[0] = 255;
    freeZones[free_zone_index(ZONE_ID_SERVICE)].staticColor[1] = 149;
    freeZones[free_zone_index(ZONE_ID_SERVICE)].staticColor[2] = 0;

    freeZones[free_zone_index(ZONE_ID_CUSTOM)].staticColor[0] = 175;
    freeZones[free_zone_index(ZONE_ID_CUSTOM)].staticColor[1] = 82;
    freeZones[free_zone_index(ZONE_ID_CUSTOM)].staticColor[2] = 222;
}
```

### 7.1. Что происходит

- Неверный указатель или неверное число зон немедленно прекращает функцию.
- Цикл создаёт четыре настройки в строгом порядке `FREE_ZONE_IDS`.
- Все зоны включаются.
- Default mode берётся из `ZONE_METADATA`.
- Яркость каждой становится 100.
- Базовый static color сначала белый.
- Затем QR получает RGB `52,199,89`.
- Service получает `255,149,0`.
- Custom получает `175,82,222`.
- Logo остаётся белым.

На чистой NVS эти конфиги существуют в RAM, но free zones не видны, пока
`zoneMap` не содержит пиксели с ID 5..8.

---

## 8. Проверка `matrix_config_validate()`

```cpp
static bool matrix_config_validate(const MatrixConfig &config, const uint8_t *zones, size_t zoneCount) {
    if (!zones_are_valid(zones, zoneCount)) {
        return false;
    }

    if (config.topology > 3) {
        return false;
    }

    if (config.bright_ports == 0) {
        return false;
    }

    return true;
}
```

Проверка выполняется и перед записью, и после чтения сохранённого контейнера.

1. `zones_are_valid()` требует настоящий указатель, ровно `NUM_IC_CHIPS`
   элементов и zone ID не выше `ZONE_ID_MAX`.
2. Допустимы topology 0, 1, 2, 3.
3. Нулевая port brightness запрещена.
4. Если всё допустимо, возвращается `true`.

Эта функция не проверяет, что каждый активный порт имеет хотя бы один пиксель.
Такое safety-правило проверяет HTTP handler `/save_zones` функцией
`zoneMapHasRequiredActivePortZones()`. Storage остаётся универсальным и
проверяет структурную корректность данных.

---

## 9. Загрузка matrix config

### 9.1. Внутренняя функция `load_current_config()`

```cpp
static bool load_current_config(Preferences &preferences, MatrixConfig &config, uint8_t *zones, size_t zoneCount) {
    StoredMatrixConfig stored = {};
    const size_t bytesRead = preferences.getBytes(KEY_MATRIX_CONFIG, &stored, sizeof(stored));

    if (bytesRead != sizeof(stored) ||
        stored.magic != MATRIX_CONFIG_MAGIC ||
        stored.version != MATRIX_CONFIG_VERSION ||
        stored.size != sizeof(stored) ||
        stored.matrixX != MATRIX_X ||
        stored.matrixY != VIRTUAL_Y) {
        return false;
    }

    if (!matrix_config_validate(stored.config, stored.zones, sizeof(stored.zones))) {
        return false;
    }

    config = stored.config;
    memcpy(zones, stored.zones, zoneCount);
    return true;
}
```

#### Порядок

1. `stored = {}` создаёт локальный контейнер и обнуляет его.
2. `getBytes()` читает ключ `matrix_cfg`.
3. Сверяется фактическое число прочитанных байтов.
4. Проверяются magic, version и сохранённый size.
5. Сохранённые `matrixX` и `matrixY` сравниваются с текущей прошивкой.
6. Проверяются внутренний config и все zone IDs.
7. Только после всех проверок данные копируются в рабочие аргументы.

Это важно: невалидный объект не успевает частично изменить `cfg` или
`zoneMap`.

### 9.2. Публичная функция `matrix_config_load()`

```cpp
bool matrix_config_load(MatrixConfig &config, uint8_t *zones, size_t zoneCount) {
    if (zones == nullptr || zoneCount != NUM_IC_CHIPS) {
        matrix_config_set_defaults(config, zones, zoneCount);
        return false;
    }

    Preferences preferences;
    preferences.begin(NVS_NAMESPACE, true);
    const bool loaded = load_current_config(preferences, config, zones, zoneCount);
    preferences.end();

    if (!loaded) {
        clear_incompatible_matrix_storage();
        matrix_config_set_defaults(config, zones, zoneCount);
    }

    return loaded;
}
```

#### Порядок

1. Проверяет адрес и размер массива назначения.
2. Открывает namespace `led_settings` только для чтения. Второй аргумент
   `true` у `begin()` означает read-only.
3. Вызывает строгую загрузку.
4. Закрывает Preferences.
5. При любой несовместимости вызывает
   `clear_incompatible_matrix_storage()`.
6. Устанавливает defaults в RAM.
7. Возвращает `true`, только если NVS действительно загружена.

#### Что очищает несовместимость

`clear_incompatible_matrix_storage()` удаляет:

- `matrix_cfg`;
- `free_zones_v1`;
- `layer_wait`;
- `layer_chg`;
- `layer_err`;
- `fz5_layer`;
- `fz6_layer`;
- `fz7_layer`;
- `fz8_layer`.

Network и auth находятся в других namespaces и не затрагиваются.

Такое поведение специально защищает смену compile-time размера матрицы:
слои со старыми координатами не остаются рядом с новой картой.

---

## 10. Сохранение matrix config

```cpp
bool matrix_config_save(const MatrixConfig &config, const uint8_t *zones, size_t zoneCount) {
    if (!matrix_config_validate(config, zones, zoneCount)) {
        return false;
    }

    StoredMatrixConfig stored = {};
    stored.magic = MATRIX_CONFIG_MAGIC;
    stored.version = MATRIX_CONFIG_VERSION;
    stored.size = sizeof(stored);
    stored.matrixX = MATRIX_X;
    stored.matrixY = VIRTUAL_Y;
    stored.config = config;
    memcpy(stored.zones, zones, sizeof(stored.zones));

    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    const size_t bytesWritten = preferences.putBytes(KEY_MATRIX_CONFIG, &stored, sizeof(stored));
    StoredMatrixConfig verified = {};
    const size_t bytesRead = preferences.getBytes(KEY_MATRIX_CONFIG, &verified, sizeof(verified));
    preferences.end();

    return bytesWritten == sizeof(stored) && bytesRead == sizeof(verified) &&
        memcmp(&stored, &verified, sizeof(stored)) == 0;
}
```

### 10.1. Пошагово

1. Невалидные данные не допускаются к записи.
2. Локальный `stored` обнуляется.
3. Заполняются служебные поля.
4. Рабочий `MatrixConfig` копируется внутрь.
5. Весь массив zones копируется внутрь.
6. Namespace открывается на запись: `false` означает не read-only.
7. `putBytes()` пишет единый blob, то есть один массив байтов.
8. Сразу создаётся второй локальный объект `verified`.
9. Только что записанный ключ читается обратно.
10. `memcmp()` сравнивает все байты ожидаемого и прочитанного объектов.
11. Функция возвращает успех только при совпадении размера записи, размера
    чтения и содержимого.

### 10.2. Когда обновляется RAM

Эта storage-функция сама не меняет глобальные `cfg` и `zoneMap`. Handler
сначала вызывает save, проверяет `true`, а затем применяет значения:

- `/save_zones` вызывает `led_replace_zone_map_safe()`;
- `/save_topology` вызывает `led_apply_matrix_config_safe()`;
- `/set_bright` присваивает `cfg = nextConfig`.

Так RAM не сообщает об успешном изменении, если NVS-save вернул ошибку.

### 10.3. Ограничение

Read-back verification обнаруживает несовпадение, но `matrix_config_save()`
не хранит и не восстанавливает предыдущий blob при неудаче. Обычно `putBytes`
записывает один NVS-key как одну операцию, однако это не полноценная
транзакционная база данных. После аппаратного сбоя питания во время записи
следующая загрузка всё равно проверит контейнер и перейдёт к defaults, если
он повреждён.

---

## 11. Загрузка и сохранение free zone configs

### 11.1. `matrix_config_load_free_zones()`

Функция:

1. требует массив ровно из `FREE_ZONE_COUNT` элементов;
2. читает blob `free_zones_v1`;
3. проверяет размер, magic, version, размер структуры и размеры матрицы;
4. проверяет порядок zone IDs, enabled, mode и ненулевую brightness;
5. копирует данные в RAM только после полной проверки.

При ошибке:

- удаляется `free_zones_v1`;
- удаляются `fz5_layer`...`fz8_layer`;
- в RAM создаются default free configs;
- status layers и matrix config не удаляются этой функцией.

### 11.2. `matrix_config_save_free_zones()`

Функция создаёт `StoredFreeZoneConfigV1`, записывает его в
`free_zones_v1`, читает обратно и сравнивает байты через `memcmp()`.

Как и matrix save, она не обновляет глобальный массив сама. Handler
`handleSaveFreeZone()` присваивает новые элементы `freeZoneConfigs[]` только
после успешной записи.

---

## 12. Нормализация слоёв `normalize_layer_json()`

Нормализация означает: принять входной JSON, строго проверить его и собрать
каноническую строку одного формата.

Сигнатура:

```cpp
static bool normalize_layer_json(
    const String &colorsJson,
    String &normalizedJson,
    bool enforceZone,
    uint8_t zoneId,
    const uint8_t *zones,
    size_t zoneCount
)
```

### 12.1. Аргументы

| Аргумент | Смысл |
|---|---|
| `colorsJson` | входная строка вида `{"0-0":"#ff0000"}` |
| `normalizedJson` | ссылка на строку, куда будет записан проверенный результат |
| `enforceZone` | требовать ли принадлежность координаты конкретной free zone |
| `zoneId` | ID free zone при `enforceZone=true` |
| `zones` | текущая физическая zoneMap |
| `zoneCount` | размер zoneMap |

Для status layer `enforceZone=false`: один общий status layer может содержать
цвета координат разных портовых зон.

Для free custom layer `enforceZone=true`: рисунок Logo не может записать цвет
на координату, которая в `zoneMap` принадлежит QR или Port1.

### 12.2. Подготовка

```cpp
String source = colorsJson;
source.trim();
```

Создаётся локальная копия и убираются пробелы по краям.

```cpp
if (source.length() < 2 || source.length() > MAX_LAYER_JSON_BYTES ||
    source[0] != '{' || source[source.length() - 1] != '}') {
    return false;
}
```

Пустой объект `{}` допустим. Слишком короткая, слишком длинная или не
обрамлённая фигурными скобками строка отвергается.

```cpp
if (enforceZone && (!zone_is_free(zoneId) || !zones_are_valid(zones, zoneCount))) {
    return false;
}
```

Для custom free layer обязательно проверяются ID зоны и полная zoneMap.

### 12.3. Защита от повторов

```cpp
bool seen[NUM_IC_CHIPS] = {};
size_t entryCount = 0;
int position = 1;
normalizedJson = "{";
normalizedJson.reserve(source.length());
```

- `seen[]` отмечает уже использованные физические LED.
- `entryCount` считает записи.
- `position` указывает на текущий символ входной строки.
- выход начинается с `{`.
- `reserve()` заранее выделяет примерно нужную память и уменьшает число
  перераспределений `String`.

### 12.4. Разбор одной записи

Для:

```json
{"2-3":"#ff0000"}
```

цикл делает следующее:

1. требует открывающую кавычку ключа;
2. находит закрывающую кавычку;
3. получает `coordinate = "2-3"`;
4. требует двоеточие;
5. требует строковое значение;
6. получает `color = "#ff0000"`;
7. `parse_layer_coordinate()` превращает координату в `x=2`, `y=3`;
8. `is_valid_hex_color()` проверяет семь символов и hex-цифры;
9. `getLEDIndex(2,3)` получает физический индекс по текущей topology;
10. проверяется диапазон индекса и отсутствие повтора;
11. для free zone проверяется `zones[index] == zoneId`;
12. координата и цвет добавляются в выход без лишних пробелов.

### 12.5. Что считается ошибкой

- координата без одного дефиса;
- отрицательная или выходящая за матрицу координата;
- цвет не формата `#RRGGBB`;
- повтор одного физического пикселя;
- больше `NUM_IC_CHIPS` записей;
- лишняя запятая;
- отсутствующая кавычка или двоеточие;
- пиксель custom layer не принадлежит указанной free zone.

При любой ошибке функция возвращает `false`. Handler отвечает HTTP 400 и не
начинает запись.

### 12.6. Публичные обёртки

```cpp
bool matrix_config_normalize_status_layer(...)
```

вызывает общую функцию с `enforceZone=false`.

```cpp
bool matrix_config_normalize_free_zone_layer(...)
```

вызывает её с `enforceZone=true`.

---

## 13. Status layers

### 13.1. Ключи

| Статус | Основное имя | Допустимый alias | NVS key |
|---|---|---|---|
| ожидание | `waiting` | `wait` | `layer_wait` |
| зарядка | `charging` | `charge` | `layer_chg` |
| ошибка | `error` | `err` | `layer_err` |

Функция `status_layer_key()` переводит имя в ключ. Неизвестное имя даёт
`nullptr`.

### 13.2. Загрузка

```cpp
String matrix_config_load_status_layer(const char *status)
```

1. Определяет ключ.
2. При неизвестном status возвращает `{}`.
3. Читает строку из `led_settings`.
4. Если ключ отсутствует, получает `{}`.
5. Повторно нормализует прочитанную строку.
6. При повреждении возвращает безопасный пустой слой `{}`.

Повреждённая строка не проходит в LED cache.

### 13.3. Сохранение

```cpp
bool matrix_config_save_status_layer(const char *status, const String &colorsJson)
```

1. Проверяет status.
2. Полностью нормализует JSON до открытия NVS на запись.
3. Записывает строку.
4. Читает её обратно.
5. Сравнивает длину записи и точный текст.

После успешного handler-save вызывается
`led_reload_status_layers_safe()`: JSON один раз переводится в быстрые массивы
цветов RAM. Он не разбирается каждый LED-кадр.

---

## 14. Free custom layers

### 14.1. Имена ключей

Функция:

```cpp
static String free_zone_layer_key(uint8_t zoneId)
```

для ID 5..8 создаёт:

- `fz5_layer`;
- `fz6_layer`;
- `fz7_layer`;
- `fz8_layer`.

Для Port1, Port2 или zone 0 возвращается пустая строка.

### 14.2. Загрузка

`matrix_config_load_free_zone_layer(zoneId)`:

1. проверяет, что ID является free zone;
2. читает JSON или `{}`;
3. проверяет синтаксис, координаты и цвета;
4. на этапе load не требует принадлежность текущей zoneMap;
5. при ошибке возвращает `{}`.

При следующей загрузке в LED cache дополнительно учитывается актуальная
zoneMap: физически free layer применяется только там, где пиксель относится к
этой free zone.

### 14.3. Сохранение

`matrix_config_save_free_zone_layer()`:

1. проверяет ID и zoneMap;
2. нормализует с `enforceZone=true`;
3. отклоняет координаты вне выбранной зоны;
4. записывает строку;
5. читает её обратно;
6. сравнивает точный результат.

---

## 15. Read-back verification

«Проверка чтением обратно» (read-back verification) означает:

```text
подготовили ожидаемое значение
        ↓
записали в NVS
        ↓
сразу прочитали тот же key
        ↓
сравнили с ожидаемым
```

Она применяется к:

- `matrix_cfg`;
- `free_zones_v1`;
- всем status layers;
- всем free custom layers;
- network keys;
- auth keys.

Польза: endpoint не отвечает `OK`, если storage-функция обнаружила
несовпадение.

Ограничение: проверка подтверждает результат после записи, но не превращает
несколько разных NVS keys в одну общую неделимую операцию.

---

## 16. Rollback: возврат предыдущих данных

Откат (rollback) означает попытку вернуть предыдущее значение, если первая
часть составного сохранения прошла, а вторая нет.

### 16.1. Status layer + fallback color

`handleSaveStatusColors()` может менять:

1. JSON status layer;
2. fallback-цвет внутри `matrix_cfg`.

Порядок:

```text
загрузить previousLayer
        ↓
сохранить новый layer
        ↓
если изменился fallback, сохранить matrix_cfg
        ↓
если matrix_cfg не сохранился, вернуть previousLayer
```

При успешном rollback ответ: `SAVE_CONFIG_FAILED`.

Если и rollback не удался:

```text
SAVE_CONFIG_FAILED_ROLLBACK_FAILED
```

### 16.2. Free custom layer + FreeZoneConfig

`handleSaveFreeZone()` сначала полностью проверяет payload. Затем:

1. при необходимости сохраняет custom layer;
2. сохраняет `free_zones_v1`;
3. если второй шаг не удался, пытается вернуть previousLayer;
4. обновляет RAM только после успеха.

Partial brightness payload содержит только `zoneId` и `brightness`; он не
меняет mode, staticColor или custom layer.

### 16.3. Network и auth

`network_config_save()` сохраняет предыдущий `NetworkConfig` и при ошибке
пытается записать его обратно.

`auth_config_save_credentials()` делает то же для предыдущего username и
password.

### 16.4. Остаточный риск

NVS не предоставляет этому коду одну транзакцию сразу для двух произвольных
keys. Если питание исчезнет точно между двумя записями или между ошибкой и
rollback, возможна комбинация старого и нового значения.

Защита проекта:

- полная проверка до записи;
- запись маленького числа ключей;
- read-back каждого ключа;
- best-effort rollback, то есть обязательная попытка вернуть прошлое;
- строгая проверка при следующей загрузке;
- безопасные defaults или пустой layer при повреждении.

---

## 17. Полная таблица NVS

### 17.1. Матрица и зоны: namespace `led_settings`

| Namespace | Key | Тип | Кто сохраняет | Кто загружает | Когда применяется | При повреждении |
|---|---|---|---|---|---|---|
| `led_settings` | `matrix_cfg` | blob `StoredMatrixConfig` | `matrix_config_save()` | `matrix_config_load()` | при `led_setup()`, после save zones/topology/brightness/colors | очищаются все matrix/free/layer keys, используются defaults |
| `led_settings` | `free_zones_v1` | blob `StoredFreeZoneConfigV1` | `matrix_config_save_free_zones()` | `matrix_config_load_free_zones()` | при `led_setup()`, после save free zone | удаляется этот key и free layers, используются free defaults |
| `led_settings` | `layer_wait` | JSON String | `matrix_config_save_status_layer()` | `matrix_config_load_status_layer()` | reload status cache | возвращается `{}`, fallback-цвет остаётся |
| `led_settings` | `layer_chg` | JSON String | та же функция | та же функция | reload charging cache | возвращается `{}` |
| `led_settings` | `layer_err` | JSON String | та же функция | та же функция | reload error cache | возвращается `{}` |
| `led_settings` | `fz5_layer` | JSON String Logo | `matrix_config_save_free_zone_layer()` | `matrix_config_load_free_zone_layer()` | reload free caches | возвращается `{}` |
| `led_settings` | `fz6_layer` | JSON String QR | та же функция | та же функция | reload free caches | возвращается `{}` |
| `led_settings` | `fz7_layer` | JSON String Service | та же функция | та же функция | reload free caches | возвращается `{}` |
| `led_settings` | `fz8_layer` | JSON String Custom | та же функция | та же функция | reload free caches | возвращается `{}` |

### 17.2. Сеть: namespace `net_cfg`

| Namespace | Key | Тип | Кто сохраняет | Кто загружает | Когда применяется | При повреждении |
|---|---|---|---|---|---|---|
| `net_cfg` | `ssid` | String | `writeConfigToNvs()` | `loadConfig()` | `network_setup()`/reconnect | невалидный общий config заменяется network defaults |
| `net_cfg` | `pass` | String | та же функция | та же функция | подключение STA | defaults |
| `net_cfg` | `dhcp` | bool | та же функция | та же функция | настройка IP | defaults |
| `net_cfg` | `ip` | String IPv4 | та же функция | та же функция | static mode | defaults |
| `net_cfg` | `gw` | String IPv4 | та же функция | та же функция | static mode | defaults |
| `net_cfg` | `mask` | String IPv4 | та же функция | та же функция | static mode | defaults |
| `net_cfg` | `dns` | String IPv4 | та же функция | та же функция | static mode | defaults |
| `net_cfg` | `ap_ssid` | String | та же функция | та же функция | AP fallback | если key отсутствует, уникальный `LED_MATRIX_XXXXXX`; невалидный config даёт defaults |
| `net_cfg` | `ap_pass` | String | та же функция | та же функция | AP fallback | default AP password |

Network save проверяет все значения чтением обратно. При невалидной
совокупности `validateConfig()` вызывает `setDefaults(networkConfig)`.

### 17.3. Авторизация: namespace `auth_cfg`

| Namespace | Key | Тип | Кто сохраняет | Кто загружает | Когда применяется | При повреждении |
|---|---|---|---|---|---|---|
| `auth_cfg` | `version` | `uint16_t` | `saveRawCredentials()` | `auth_config_setup()` | при startup | defaults `admin/admin`, затем попытка сохранить defaults |
| `auth_cfg` | `username` | String | та же функция | та же функция | каждый Basic Auth check | defaults |
| `auth_cfg` | `password` | String | та же функция | та же функция | каждый Basic Auth check | defaults |

Пароль хранится как строка, не как криптографический hash. Это отдельный
production-риск Basic Auth без HTTPS, но формат здесь описан без его изменения.

---

## 18. `zone_model.h`: построчный разбор

### 18.1. Защита header-файла

```cpp
#ifndef ZONE_MODEL_H
#define ZONE_MODEL_H
```

Если header включён из нескольких `.cpp`, эти строки не дают компилятору
объявить всё повторно.

```cpp
#include <Arduino.h>
```

Подключает типы Arduino, включая `uint8_t`.

### 18.2. `ZoneType`

```cpp
enum ZoneType : uint8_t {
    ZONE_TYPE_OFF = 0,
    ZONE_TYPE_PORT,
    ZONE_TYPE_FREE
};
```

- `enum` задаёт ограниченный набор именованных значений.
- `: uint8_t` просит хранить значение в одном байте.
- `ZONE_TYPE_OFF` означает пиксель без рабочей зоны.
- `ZONE_TYPE_PORT` означает Port1..Port4.
- `ZONE_TYPE_FREE` означает Logo/QR/Service/Custom.
- Неуказанные числа увеличиваются автоматически: PORT=1, FREE=2.

### 18.3. `FreeZoneMode`

```cpp
enum FreeZoneMode : uint8_t {
    FREE_ZONE_STATIC = 0,
    FREE_ZONE_CUSTOM,
    FREE_ZONE_RAINBOW
};
```

- STATIC: вся зона использует `staticColor`.
- CUSTOM: отдельные координаты могут иметь свои цвета.
- RAINBOW: цвет вычисляется во времени.

### 18.4. Все zone IDs

```cpp
static constexpr uint8_t ZONE_ID_OFF = 0;
```

Zone 0 «Без зоны». Пиксель становится чёрным, если не занят другой логикой.

```cpp
static constexpr uint8_t ZONE_ID_PORT1 = 1;
static constexpr uint8_t ZONE_ID_PORT2 = 2;
static constexpr uint8_t ZONE_ID_PORT3 = 3;
static constexpr uint8_t ZONE_ID_PORT4 = 4;
```

ID четырёх портовых зон.

```cpp
static constexpr uint8_t ZONE_ID_LOGO = 5;
static constexpr uint8_t ZONE_ID_QR = 6;
static constexpr uint8_t ZONE_ID_SERVICE = 7;
static constexpr uint8_t ZONE_ID_CUSTOM = 8;
```

ID четырёх свободных зон.

```cpp
static constexpr uint8_t ZONE_ID_MAX = ZONE_ID_CUSTOM;
```

Максимальный допустимый ID равен 8.

Слова `static constexpr` здесь означают compile-time константу, локальную для
каждой единицы компиляции, которая подключает header. Это не изменяемая
runtime-настройка.

### 18.5. `FREE_ZONE_IDS`

```cpp
static constexpr uint8_t FREE_ZONE_IDS[] = {
    ZONE_ID_LOGO,
    ZONE_ID_QR,
    ZONE_ID_SERVICE,
    ZONE_ID_CUSTOM
};
```

Массив задаёт официальный порядок free zones.

```cpp
static constexpr size_t FREE_ZONE_COUNT =
    sizeof(FREE_ZONE_IDS) / sizeof(FREE_ZONE_IDS[0]);
```

Размер всего массива делится на размер одного элемента. Результат равен 4.
При добавлении пятого ID счётчик обновится автоматически.

### 18.6. `ZoneMetadata`

```cpp
struct ZoneMetadata {
    uint8_t id;
    const char *name;
    ZoneType type;
    bool enabled;
    bool reserved;
    int8_t linkedPort;
    FreeZoneMode defaultMode;
};
```

| Поле | Что означает |
|---|---|
| `id` | числовой zone ID |
| `name` | базовое имя для JSON |
| `type` | off, port или free |
| `enabled` | статический базовый признак |
| `reserved` | статический базовый признак резерва |
| `linkedPort` | индекс порта 0..3 или -1 |
| `defaultMode` | начальный режим free zone |

`const char *name` является адресом неизменяемой C-строки. `int8_t` у
`linkedPort` выбран потому, что требуется значение `-1`.

### 18.7. Каждая строка `ZONE_METADATA[]`

```cpp
{ZONE_ID_OFF, "Off", ZONE_TYPE_OFF, true, false, -1, FREE_ZONE_STATIC},
```

- ID 0;
- системное имя `"Off"`;
- тип OFF;
- базово enabled;
- не reserved;
- не связан с портом;
- поле defaultMode формально заполнено STATIC, хотя для OFF не используется.

```cpp
{ZONE_ID_PORT1, "Port 1", ZONE_TYPE_PORT, true, false, 0, FREE_ZONE_STATIC},
```

- ID 1;
- имя Port 1;
- port type;
- статически enabled;
- не reserved;
- связан с элементом 0 массива статусов, то есть физическим Port1;
- free mode для port не используется.

```cpp
{ZONE_ID_PORT2, "Port 2", ZONE_TYPE_PORT, false, true, 1, FREE_ZONE_STATIC},
```

- ID 2;
- связан с индексом порта 1;
- статические поля говорят disabled/reserved;
- однако при текущем `activePortCount=2` HTTP metadata динамически заменяет
  это на active=true и reserved=false.

```cpp
{ZONE_ID_PORT3, "Port 3", ZONE_TYPE_PORT, false, true, 2, FREE_ZONE_STATIC},
```

Port3 связан с индексом 2 и базово зарезервирован. При
`activePortCount >= 3` UI/backend выдадут его активным.

```cpp
{ZONE_ID_PORT4, "Port 4", ZONE_TYPE_PORT, false, true, 3, FREE_ZONE_STATIC},
```

Port4 связан с индексом 3. Он становится активным при
`activePortCount == 4`.

```cpp
{ZONE_ID_LOGO, "Logo", ZONE_TYPE_FREE, true, false, -1, FREE_ZONE_STATIC},
```

Free zone ID 5, включена, не связана с портом, default mode static.

```cpp
{ZONE_ID_QR, "QR", ZONE_TYPE_FREE, true, false, -1, FREE_ZONE_STATIC},
```

Free zone ID 6, default mode static.

```cpp
{ZONE_ID_SERVICE, "Service", ZONE_TYPE_FREE, true, false, -1, FREE_ZONE_STATIC},
```

Free zone ID 7, default mode static.

```cpp
{ZONE_ID_CUSTOM, "Custom", ZONE_TYPE_FREE, true, false, -1, FREE_ZONE_CUSTOM},
```

Free zone ID 8, default mode custom.

### 18.8. Важное правило activePortCount

Статические `enabled` и `reserved` внутри `ZONE_METADATA` не являются
окончательным runtime-решением для Port1..Port4.

`appendZoneMetadataJson()` в `main.cpp` получает snapshot
`StatusMapperState`, читает `activePortCount` и для port zone вычисляет:

```cpp
enabled = linkedPort >= 0 && linkedPort < activePortCount;
reserved = !enabled;
```

При текущем `DEFAULT_ACTIVE_PORT_COUNT = 2`:

- Port1 active;
- Port2 active;
- Port3 reserved;
- Port4 reserved.

Поэтому статическая строка Port2 с `enabled=false` не блокирует Port2 в
текущем UI. Таблица задаёт базовые metadata, а activePortCount задаёт
фактическую доступность портов.

### 18.9. Число metadata-элементов

```cpp
static constexpr size_t ZONE_METADATA_COUNT =
    sizeof(ZONE_METADATA) / sizeof(ZONE_METADATA[0]);
```

Сейчас результат равен 9: zone 0 плюс восемь рабочих зон.

### 18.10. `zone_id_is_valid()`

```cpp
inline bool zone_id_is_valid(uint8_t zoneId) {
    return zoneId <= ZONE_ID_MAX;
}
```

- `inline` позволяет определить небольшую функцию в header.
- Для `uint8_t` нижняя граница всегда 0.
- ID 0..8 даёт `true`, 9..255 даёт `false`.

### 18.11. `zone_is_port()`

```cpp
inline bool zone_is_port(uint8_t zoneId) {
    return zoneId >= ZONE_ID_PORT1 && zoneId <= ZONE_ID_PORT4;
}
```

Возвращает `true` только для 1..4.

### 18.12. `zone_is_free()`

```cpp
inline bool zone_is_free(uint8_t zoneId) {
    return zoneId >= ZONE_ID_LOGO && zoneId <= ZONE_ID_CUSTOM;
}
```

Возвращает `true` только для 5..8.

### 18.13. `free_zone_index()`

```cpp
inline int free_zone_index(uint8_t zoneId) {
    for (size_t i = 0; i < FREE_ZONE_COUNT; i++) {
        if (FREE_ZONE_IDS[i] == zoneId) {
            return (int)i;
        }
    }
    return -1;
}
```

1. Цикл идёт по четырём free IDs.
2. При совпадении возвращается индекс массива 0..3.
3. Для Port1 или неизвестной зоны возвращается -1.

Примеры:

- `free_zone_index(5)` → 0;
- `free_zone_index(6)` → 1;
- `free_zone_index(8)` → 3;
- `free_zone_index(1)` → -1.

### 18.14. `zone_metadata_for()`

```cpp
inline const ZoneMetadata *zone_metadata_for(uint8_t zoneId) {
    for (size_t i = 0; i < ZONE_METADATA_COUNT; i++) {
        if (ZONE_METADATA[i].id == zoneId) {
            return &ZONE_METADATA[i];
        }
    }
    return nullptr;
}
```

Функция ищет metadata по ID.

`&ZONE_METADATA[i]` означает адрес найденной структуры.
`const ZoneMetadata *` означает указатель, через который нельзя менять
константную таблицу.
`nullptr` означает «не найдено».

### 18.15. `zone_type_to_string()`

```cpp
inline const char *zone_type_to_string(ZoneType type) {
    switch (type) {
        case ZONE_TYPE_PORT:
            return "port";
        case ZONE_TYPE_FREE:
            return "free";
        case ZONE_TYPE_OFF:
        default:
            return "off";
    }
}
```

`switch` выбирает строку для JSON. Неизвестное значение безопасно становится
`"off"`.

### 18.16. `free_zone_mode_to_string()`

```cpp
inline const char *free_zone_mode_to_string(FreeZoneMode mode) {
    switch (mode) {
        case FREE_ZONE_CUSTOM:
            return "custom";
        case FREE_ZONE_RAINBOW:
            return "rainbow";
        case FREE_ZONE_STATIC:
        default:
            return "static";
    }
}
```

Переводит C++ enum в строку, понятную JavaScript. Неизвестное значение
безопасно представляется как `"static"`.

```cpp
#endif
```

Закрывает защиту header-файла.

---

## 19. Сценарий 1: первый запуск с пустой NVS

### Шаг 1. Создание RAM

До `setup()` глобальные `cfg`, `zoneMap[]` и `freeZoneConfigs[]` существуют,
но ещё не содержат загруженных пользовательских настроек.

### Шаг 2. Auth

`auth_config_setup()` не находит корректные `version`, `username`, `password`.
В RAM устанавливается `admin/admin`, затем код пытается записать эти defaults.

### Шаг 3. LED storage

`led_setup()` вызывает `led_load_config_from_flash()`.

```cpp
matrix_config_load(cfg, zoneMap, sizeof(zoneMap));
```

Ключ `matrix_cfg` отсутствует:

- bytesRead не равен размеру структуры;
- load возвращает false;
- несовместимые matrix keys очищаются;
- `cfg` получает topology 0, brightness 120 и default colors;
- 96 физических элементов zoneMap делятся между Port1 и Port2.

### Шаг 4. Free configs

```cpp
matrix_config_load_free_zones(freeZoneConfigs, FREE_ZONE_COUNT);
```

Ключ отсутствует:

- free config и free layers очищаются;
- в RAM создаются четыре defaults.

Эти defaults не обязательно немедленно записываются. Они существуют в RAM и
попадут в NVS при соответствующем сохранении из UI.

### Шаг 5. Layers

Все status и free layer keys отсутствуют, поэтому loaders возвращают `{}`.
LED caches становятся пустыми.

### Шаг 6. Отрисовка

Матрица уже имеет default Port1/Port2 zoneMap. Цвет каждой портовой зоны
определяется текущим status mapper и fallback colors.

---

## 20. Сценарий 2: сохранение новой разметки зон

Пусть инженер изменил карту и нажал «Сохранить зоны».

### Шаг 1. Browser JSON

JavaScript создаёт объект логических координат:

```json
{
  "0-0": 1,
  "1-0": 1,
  "2-0": 2
}
```

Полный реальный объект содержит нужные координаты всей разметки.

### Шаг 2. HTTP handler

`handleSaveZones()` получает:

```cpp
String body = server.arg("plain");
```

Создаётся временный:

```cpp
uint8_t nextZoneMap[NUM_IC_CHIPS] = {};
```

### Шаг 3. Перевод координат

`parseZoneMapPayload()`:

- сначала заполняет все элементы zone 0;
- проверяет JSON и каждый zoneId;
- для каждой координаты вызывает `getLEDIndex(x,y)`;
- пишет zoneId по физическому индексу;
- отклоняет повторы и выход за матрицу.

### Шаг 4. Safety validation

`zoneMapHasRequiredActivePortZones()` проверяет, что для каждого активного
порта есть хотя бы один пиксель.

При activePortCount 2 нужны Port1 и Port2.

### Шаг 5. NVS

```cpp
matrix_config_save(cfg, nextZoneMap, sizeof(nextZoneMap))
```

создаёт единый `StoredMatrixConfig`, пишет `matrix_cfg` и проверяет чтением
обратно.

### Шаг 6. RAM

Только после успеха:

```cpp
led_replace_zone_map_safe(nextZoneMap, sizeof(nextZoneMap));
```

копирует карту в глобальный `zoneMap[]` под защитой LED mutex и обновляет
матрицу.

Затем `led_reload_free_zone_layers_safe()` перечитывает custom layers, потому
что принадлежность координат free zones могла измениться.

### Шаг 7. Ответ

- успех: HTTP 200 `OK`;
- неверный JSON: HTTP 400;
- нет зоны активного порта: HTTP 400;
- NVS не подтвердила запись: HTTP 500.

---

## 21. Сценарий 3: reboot и полное восстановление

### 21.1. После подачи питания

1. Статические RAM-переменные создаются заново.
2. `auth_config_setup()` загружает auth.
3. `station_inputs_setup()` и `status_mapper_setup()` готовят runtime-входы.
4. `led_setup()` начинает загрузку матрицы.

### 21.2. Matrix config

`matrix_config_load()` читает `matrix_cfg` и проверяет:

- blob size;
- magic;
- version;
- struct size;
- matrix 12x8;
- topology;
- brightness;
- каждый zoneId.

При успехе:

- `cfg` восстанавливается;
- `zoneMap[]` восстанавливается.

### 21.3. Free configs

`matrix_config_load_free_zones()` восстанавливает:

- enabled;
- mode;
- staticColor;
- brightness

для Logo, QR, Service и Custom.

### 21.4. Layers

Три status layer и четыре free custom layer читаются как JSON, повторно
проверяются и превращаются в RAM caches.

### 21.5. Что не хранится

Не восстанавливаются как настройки:

- текущий физический статус charging/error: он заново читается с Data;
- текущая фаза breathing/rainbow;
- содержимое `leds[]`;
- открытая вкладка браузера;
- несохранённые browser edits;
- временное состояние Wi-Fi scan;
- runtime state reconnect watchdog.

### 21.6. Сеть

`network_setup()` отдельно читает `net_cfg` и запускает STA либо AP fallback.
Это не влияет на восстановление matrix config.

### 21.7. Итог

После reboot постоянные настройки восстанавливаются из трёх namespaces, а
динамическое состояние вычисляется заново:

```text
led_settings → cfg + zoneMap + free configs + layers
net_cfg      → параметры Wi-Fi и AP fallback
auth_cfg     → username/password
Data GPIO    → текущие статусы портов
```

---

## 22. Практические правила изменения storage

1. Нельзя менять порядок или поля `StoredMatrixConfig` без изменения версии
   формата и продуманной совместимости.
2. Нельзя переименовывать NVS keys как обычные C++-символы: пользовательские
   настройки перестанут находиться.
3. `StoredFreeZoneConfigV1` является актуальным форматом, а не мусорным
   compatibility-кодом.
4. Новое поле рабочего `MatrixConfig` влияет на двоичный размер
   `StoredMatrixConfig`.
5. При изменении размеров матрицы текущий код намеренно сбрасывает matrix,
   free и layer data, но не network/auth.
6. Save handler должен сначала проверить весь payload, затем сохранить, затем
   обновить RAM.
7. Ответ HTTP 200 допустим только после подтверждённой записи.
8. Изменения rollback-цепочки требуют тестов с save/reload/reboot и
   искусственной ошибкой NVS.

