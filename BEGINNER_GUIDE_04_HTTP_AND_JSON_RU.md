# HTTP-запросы и JSON в `main.cpp`

Этот документ объясняет, как браузер обменивается данными с ESP32 и как функции из `src/main.cpp` вручную разбирают и собирают JSON.

JSON здесь означает текст с именованными полями. Например:

```json
{
  "zoneId": 5,
  "brightness": 100
}
```

Важно: `/get_config` создаёт HTTP-ответ, а не файл. Функция собирает временную строку `String json` в оперативной памяти ESP32, отправляет её браузеру и после завершения запроса освобождает локальную строку.

Код проекта этим документом не изменяется.

---

## 1. Как браузер вызывает ESP32

### 1.1. Адрес запроса и функция-обработчик

Адрес программного запроса, например `/get_config`, сначала будем называть адресом HTTP API, а технический термин укажем в скобках: конечная точка (`endpoint`).

Функцию C++, которая принимает этот запрос, сначала будем называть функцией-обработчиком, а технический термин укажем в скобках: обработчик (`handler`).

Связь регистрируется в `setup()`:

```cpp
server.on("/get_config", handleGetConfig);
server.on("/save_zones", HTTP_POST, handleSaveZones);
```

Когда браузер запрашивает `/get_config`, объект `WebServer server(80)` вызывает `handleGetConfig()`.

### 1.2. Чем GET отличается от POST

Запрос чтения (`GET`) обычно просит ESP32 вернуть уже существующие данные:

```text
GET /get_config
```

У такого запроса обычно нет тела.

Запрос изменения (`POST`) передаёт новые данные:

```text
POST /save_free_zone
Content-Type: application/json

{"zoneId":5,"brightness":100}
```

Браузерная функция `fetch()` отправляет этот текст. Серверная часть получает необработанное тело так:

```cpp
String body = server.arg("plain");
```

Если POST отправлен как набор полей формы, обработчик читает отдельные значения:

```cpp
server.arg("topology")
server.arg("val")
```

`/get_config` вызывается браузером как GET и ничего не сохраняет. Регистрация `server.on("/get_config", handleGetConfig)` не ограничивает метод отдельным параметром, но фактический интерфейс использует GET.

### 1.3. Коды HTTP

| Код | Русский смысл | Что означает в этой прошивке |
|---:|---|---|
| 200 | запрос выполнен | данные возвращены или сохранение завершено |
| 202 | запрос принят, работа продолжается | асинхронный поиск Wi-Fi ещё идёт |
| 400 | неверный запрос | поле отсутствует, JSON или значение недопустимы |
| 401 | требуется авторизация | имя или пароль не переданы либо неверны |
| 500 | внутренняя ошибка | NVS, OTA или другое внутреннее действие не завершилось |

### 1.4. Почему каждый адрес проверяет авторизацию

В начале обработчика вызывается:

```cpp
if (!requireHttpAuth()) return;
```

`requireHttpAuth()` проверяет имя и пароль через базовую HTTP-авторизацию (Basic Auth). Если данные неверны, `server.requestAuthentication()` формирует ответ 401, а обработчик немедленно завершается.

Проверка нужна не только на странице `/`. Если защитить страницу, но не защитить `/save_zones` или `/update`, злоумышленник смог бы вызвать адрес напрямую без интерфейса.

---

## 2. `parseHexColor()`

### 2.1. Полный актуальный код

```cpp
void parseHexColor(String hex, uint8_t* targetArray) {
    if (hex.startsWith("#")) hex = hex.substring(1);
    if (hex.length() == 6) {
        targetArray[0] = strtol(hex.substring(0, 2).c_str(), NULL, 16); // R
        targetArray[1] = strtol(hex.substring(2, 4).c_str(), NULL, 16); // G
        targetArray[2] = strtol(hex.substring(4, 6).c_str(), NULL, 16); // B
    }
}
```

### 2.2. Реальные вызывающие

- `handleSaveStatusColors()` преобразует первый цвет слоя в fallback-цвет статуса.
- `handleSaveFreeZone()` преобразует `staticColor`.

Оба обработчика сначала проверяют строку либо получают её из уже нормализованного слоя.

### 2.3. Построчное объяснение

```cpp
void parseHexColor(String hex, uint8_t* targetArray) {
```

- `hex` передаётся копией, поэтому функция может удалить `#`, не меняя исходную строку.
- `targetArray` является указателем на массив из трёх байтов.
- Функция имеет тип `void` и не возвращает признак ошибки.

```cpp
if (hex.startsWith("#")) hex = hex.substring(1);
```

- Проверяется первый символ.
- Для `"#34c759"` создаётся подстрока `"34c759"`.
- Локальная `hex` меняется с 7 до 6 символов.

```cpp
if (hex.length() == 6) {
```

- Преобразование выполняется только для шести символов.
- При длине 5 или 7 массив не меняется.

```cpp
targetArray[0] = strtol(hex.substring(0, 2).c_str(), NULL, 16);
```

- `substring(0,2)` даёт `"34"`.
- `c_str()` предоставляет указатель на обычную строку C.
- `strtol(...,16)` читает число в шестнадцатеричной системе.
- `"34"` становится десятичным 52.
- Значение записывается в красный канал.

Следующие две строки аналогично заполняют зелёный и синий:

```text
"c7" -> 199
"59" -> 89
```

Итог:

```text
вход: "#34c759"
targetArray: [52, 199, 89]
```

### 2.4. Неверный ввод

Функция сама не проверяет, являются ли шесть символов допустимыми цифрами цвета. `strtol()` может частично разобрать неправильную строку.

Поэтому прямой вызывающий код должен сначала использовать `isValidHexColor()` или нормализованный слой. При неверной длине функция молча оставляет массив без изменений.

---

## 3. `rgbToHex()`

### 3.1. Полный актуальный код

```cpp
String rgbToHex(const uint8_t *rgb) {
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", rgb[0], rgb[1], rgb[2]);
    return String(buf);
}
```

### 3.2. Реальные вызывающие

- `handleGetConfig()` для waiting, charging и error.
- `appendFreeZoneConfigsJson()` для `staticColor` каждой свободной зоны.

### 3.3. Построчное объяснение

```cpp
String rgbToHex(const uint8_t *rgb) {
```

- `rgb` указывает на три байта.
- `const` запрещает функции менять исходный массив.
- Результатом будет объект Arduino `String`.

```cpp
char buf[8];
```

Создаётся локальный массив:

```text
# + 6 цифр + завершающий нулевой символ = 8 байт
```

```cpp
snprintf(buf, sizeof(buf), "#%02x%02x%02x", rgb[0], rgb[1], rgb[2]);
```

- `%02x` печатает байт двумя шестнадцатеричными цифрами.
- Ведущий ноль добавляется автоматически.
- Размер `sizeof(buf)` защищает от записи длиннее массива.

Пример изменения:

```text
rgb = [0, 80, 255]
buf после snprintf = "#0050ff"
```

```cpp
return String(buf);
```

Локальный массив преобразуется в безопасно возвращаемую строку.

### 3.4. Неверный ввод

Функция предполагает, что `rgb` не равен `nullptr` и указывает минимум на три элемента. В текущих вызовах передаются реальные массивы структур.

Любой `uint8_t` уже находится в диапазоне 0..255, поэтому дополнительная проверка каналов не нужна.

---

## 4. `isValidHexColor()`

### 4.1. Полный актуальный код

```cpp
bool isValidHexColor(const String &value) {
    if (value.length() != 7 || value[0] != '#') return false;
    for (int i = 1; i < value.length(); i++) {
        if (!isxdigit((unsigned char)value[i])) return false;
    }
    return true;
}
```

### 4.2. Реальный вызывающий

`handleSaveFreeZone()` проверяет поле `staticColor`.

### 4.3. Построчное объяснение

```cpp
const String &value
```

Строка передаётся по ссылке без копирования, а `const` запрещает изменение.

```cpp
if (value.length() != 7 || value[0] != '#') return false;
```

Цвет обязан иметь вид `#RRGGBB`: ровно 7 символов и `#` первым.

```cpp
for (int i = 1; i < value.length(); i++) {
```

Цикл проверяет индексы 1..6.

```cpp
if (!isxdigit((unsigned char)value[i])) return false;
```

`isxdigit()` принимает только `0..9`, `a..f`, `A..F`.

Преобразование к `unsigned char` исключает неправильную передачу отрицательного значения в системную функцию.

```cpp
return true;
```

До этой строки дошли только все шесть правильных цифр.

### 4.4. Примеры

```text
"#34c759" -> true
"34c759"  -> false, нет #
"#34cz59" -> false, z недопустим
"#fff"    -> false, неверная длина
```

При false обработчик отвечает HTTP 400 `BAD_COLOR`.

---

## 5. `jsonEscape()`

### 5.1. Полный актуальный код

```cpp
String jsonEscape(String value) {
    value.replace("\\", "\\\\");
    value.replace("\"", "\\\"");
    value.replace("\n", "\\n");
    value.replace("\r", "\\r");
    return value;
}
```

### 5.2. Реальные вызывающие

- `handleNetworkStatus()` для сохранённого SSID, подключённого SSID и AP SSID.
- `handleScanNetworks()` для имён найденных Wi-Fi.
- `handleAuthStatus()` для имени пользователя.

### 5.3. Построчное изменение строки

Вход:

```text
Office "A"\Guest
```

```cpp
value.replace("\\", "\\\\");
```

Обратная косая черта удваивается:

```text
Office "A"\\Guest
```

```cpp
value.replace("\"", "\\\"");
```

Кавычки получают защитную обратную черту:

```text
Office \"A\"\\Guest
```

Следующие строки заменяют реальные переводы строк на текстовые `\n` и `\r`.

```cpp
return value;
```

Результат можно безопасно поместить между JSON-кавычками.

### 5.4. Неверный ввод и ограничения

Функция не сообщает об ошибке. Она обрабатывает символы, реально ожидаемые в SSID и имени пользователя.

Она не является полным универсальным кодировщиком JSON: например, отдельно не обрабатывает табуляцию и все управляющие байты. Ограничения длин и допустимый источник строк уменьшают риск, но для произвольного текста правильнее было бы использовать структурированную JSON-библиотеку.

---

## 6. `extractJsonStringField()`

### 6.1. Полный актуальный код

```cpp
String extractJsonStringField(const String &json, const char *fieldName) {
    String pattern = "\"";
    pattern += fieldName;
    pattern += "\"";

    int fieldPos = json.indexOf(pattern);
    if (fieldPos < 0) return "";

    int colonPos = json.indexOf(':', fieldPos + pattern.length());
    if (colonPos < 0) return "";

    int quoteStart = colonPos + 1;
    while (quoteStart < json.length() && isspace((unsigned char)json[quoteStart])) quoteStart++;
    if (quoteStart >= json.length() || json[quoteStart] != '"') return "";

    int quoteEnd = json.indexOf('"', quoteStart + 1);
    if (quoteEnd < 0) return "";

    return json.substring(quoteStart + 1, quoteEnd);
}
```

### 6.2. Реальные вызывающие

- `handleSaveStatusColors()`: поле `"status"`.
- `handleSaveFreeZone()`: поля `"mode"` и `"staticColor"`.

### 6.3. Пошаговый пример

Вход:

```json
{"zoneId":5,"mode":"rainbow","brightness":100}
```

`fieldName = "mode"`.

```cpp
String pattern = "\"";
pattern += fieldName;
pattern += "\"";
```

Изменение:

```text
pattern = "\""
pattern = "\"mode"
pattern = "\"mode\""
```

```cpp
int fieldPos = json.indexOf(pattern);
```

Находится начало `"mode"`.

```cpp
if (fieldPos < 0) return "";
```

При отсутствии поля возвращается пустая строка.

```cpp
int colonPos = json.indexOf(':', fieldPos + pattern.length());
```

Ищется двоеточие после имени.

```cpp
int quoteStart = colonPos + 1;
while (...) quoteStart++;
```

Индекс проходит возможные пробелы после `:`.

```cpp
if (... || json[quoteStart] != '"') return "";
```

Значение обязано начинаться с кавычки.

```cpp
int quoteEnd = json.indexOf('"', quoteStart + 1);
```

Ищется следующая кавычка.

```cpp
return json.substring(quoteStart + 1, quoteEnd);
```

Результат: `"rainbow"` без кавычек.

### 6.4. Неверный ввод и ограничения

Возвращается `""`, если:

- поле отсутствует;
- нет двоеточия;
- значение не строковое;
- нет закрывающей кавычки.

Пустое корректное значение и ошибка выглядят одинаково.

Функция не понимает защищённую кавычку `\"` внутри значения: она примет её кавычку за конец. Текущие поля содержат короткие фиксированные слова и цвета, поэтому это ограничение приемлемо для текущего контракта.

---

## 7. `jsonHasField()`

### 7.1. Полный актуальный код

```cpp
bool jsonHasField(const String &json, const char *fieldName) {
    String pattern = "\"";
    pattern += fieldName;
    pattern += "\"";
    return json.indexOf(pattern) >= 0;
}
```

### 7.2. Реальные вызывающие

Только `handleSaveFreeZone()` проверяет наличие:

- `enabled`;
- `mode`;
- `brightness`;
- `staticColor`;
- `customLayer`.

### 7.3. Построчный пример

Для:

```json
{"zoneId":5,"brightness":100}
```

и `fieldName = "brightness"`:

```text
pattern -> "\"brightness\""
indexOf -> позиция больше или равна 0
результат -> true
```

Для `"mode"` результат false.

### 7.4. Неверный ввод и ограничение

Функция ищет текст, а не разбирает структуру JSON. Теоретически совпадение может находиться внутри строкового значения.

В текущем небольшом контролируемом payload это используется только как предварительная проверка. После неё значение отдельно извлекается и валидируется.

---

## 8. `parseStrictInt()`

### 8.1. Полный актуальный код

```cpp
bool parseStrictInt(const String &value, int &result) {
    String clean = value;
    clean.trim();
    if (clean.length() == 0) return false;

    int position = 0;
    bool negative = false;
    if (clean[position] == '-') {
        negative = true;
        position++;
    }
    if (position >= clean.length()) return false;

    long parsed = 0;
    for (; position < clean.length(); position++) {
        if (!isdigit((unsigned char)clean[position])) return false;
        const int digit = clean[position] - '0';
        if (parsed > (2147483647L - digit) / 10L) return false;
        parsed = (parsed * 10) + digit;
    }

    result = negative ? -(int)parsed : (int)parsed;
    return true;
}
```

### 8.2. Все реальные вызывающие

- `extractJsonIntField()`.
- `parseZoneCoordinate()` для `x` и `y`.
- `parseZoneMapPayload()` для `zoneId`.
- `handleSaveTopology()` для поля формы.
- `handleSetBright()` для яркости портов.

### 8.3. Пошаговый пример `" 120 "`

```cpp
String clean = value;
```

Создаётся копия `" 120 "`.

```cpp
clean.trim();
```

Пробелы по краям удаляются: `"120"`.

```cpp
if (clean.length() == 0) return false;
```

Пустая строка запрещена.

```cpp
int position = 0;
bool negative = false;
```

Начальные значения:

```text
position = 0
negative = false
```

Проверка `'-'` не срабатывает.

```cpp
long parsed = 0;
```

Накопитель начинается с нуля.

Цикл:

| `position` | символ | `digit` | `parsed` после строки |
|---:|---|---:|---:|
| 0 | `1` | 1 | 1 |
| 1 | `2` | 2 | 12 |
| 2 | `0` | 0 | 120 |

Проверка:

```cpp
if (parsed > (2147483647L - digit) / 10L) return false;
```

предотвращает переполнение положительного 32-битного `int`.

```cpp
result = negative ? -(int)parsed : (int)parsed;
```

`result = 120`.

Функция возвращает `true`.

### 8.4. Неверный ввод

```text
""           -> false
"-"          -> false
"12x"        -> false
"1.5"        -> false
"2147483648" -> false
"-12"        -> true, result = -12
```

Функция не принимает самый нижний возможный `int` `-2147483648`, потому что сначала ограничивает абсолютное значение числом 2147483647. Для текущих небольших параметров это не важно.

---

## 9. `extractJsonIntField()`

### 9.1. Полный актуальный код

```cpp
bool extractJsonIntField(const String &json, const char *fieldName, int &result, bool allowBoolean = false) {
    String pattern = "\"";
    pattern += fieldName;
    pattern += "\"";

    int fieldPos = json.indexOf(pattern);
    if (fieldPos < 0) return false;

    int colonPos = json.indexOf(':', fieldPos + pattern.length());
    if (colonPos < 0) return false;

    int valueStart = colonPos + 1;
    while (valueStart < json.length() && isspace((unsigned char)json[valueStart])) valueStart++;

    if (allowBoolean) {
        const bool trueValue = json.substring(valueStart, valueStart + 4) == "true";
        const bool falseValue = json.substring(valueStart, valueStart + 5) == "false";
        if (trueValue || falseValue) {
            int trailing = valueStart + (trueValue ? 4 : 5);
            while (trailing < json.length() && isspace((unsigned char)json[trailing])) trailing++;
            if (trailing >= json.length() || (json[trailing] != ',' && json[trailing] != '}')) return false;
            result = trueValue ? 1 : 0;
            return true;
        }
    }

    int valueEnd = valueStart;
    if (valueEnd < json.length() && json[valueEnd] == '-') valueEnd++;
    while (valueEnd < json.length() && isdigit((unsigned char)json[valueEnd])) valueEnd++;

    if (valueEnd <= valueStart) return false;
    int trailing = valueEnd;
    while (trailing < json.length() && isspace((unsigned char)json[trailing])) trailing++;
    if (trailing < json.length() && json[trailing] != ',' && json[trailing] != '}') return false;
    return parseStrictInt(json.substring(valueStart, valueEnd), result);
}
```

### 9.2. Реальные вызывающие

- `handleSaveTopology()`: `topology`.
- `handleSaveFreeZone()`: `zoneId`, `enabled`, `brightness`.

Для `enabled` передаётся `allowBoolean = true`.

### 9.3. Числовой пример

Вход:

```json
{"zoneId":5,"brightness":100}
```

Поле `"brightness"`:

```text
pattern = "\"brightness\""
fieldPos = начало имени
colonPos = позиция :
valueStart = позиция символа 1
valueEnd проходит 1, 0, 0
substring = "100"
parseStrictInt -> result = 100
```

### 9.4. Логический пример

Вход:

```json
{"enabled":true}
```

При `allowBoolean = true`:

```text
trueValue = true
falseValue = false
trailing указывает на }
result = 1
return true
```

Для `false` результат 0.

### 9.5. Неверный ввод

Функция возвращает false, если:

- поля нет;
- нет `:`;
- значение не число и не разрешённый boolean;
- после значения находится неожиданный символ;
- число не проходит строгий разбор.

Пример `"brightness":12px` отклоняется.

Это ручной разборщик, а не полный JSON-парсер. Он рассчитан на простые поля текущего API.

---

## 10. `extractJsonObjectField()`

### 10.1. Полный актуальный код

```cpp
bool extractJsonObjectField(const String &json, const char *fieldName, String &result) {
    String pattern = "\"";
    pattern += fieldName;
    pattern += "\"";

    int fieldPos = json.indexOf(pattern);
    if (fieldPos < 0) return false;

    int colonPos = json.indexOf(':', fieldPos + pattern.length());
    if (colonPos < 0) return false;

    int objectStart = colonPos + 1;
    while (objectStart < json.length() && isspace((unsigned char)json[objectStart])) objectStart++;
    if (objectStart >= json.length() || json[objectStart] != '{') return false;

    int depth = 0;
    for (int i = objectStart; i < json.length(); i++) {
        if (json[i] == '{') depth++;
        if (json[i] == '}') {
            depth--;
            if (depth == 0) {
                result = json.substring(objectStart, i + 1);
                return true;
            }
        }
    }

    return false;
}
```

### 10.2. Реальные вызывающие

- `handleSaveStatusColors()` извлекает объект `"colors"`.
- `handleSaveFreeZone()` извлекает `"customLayer"`.

### 10.3. Пошаговый пример

Вход:

```json
{"status":"error","colors":{"0-0":"#ff0000","1-0":"#000000"}}
```

После поиска имени и двоеточия:

```text
objectStart -> символ {
depth = 0
```

Цикл:

1. На первой `{` объекта colors: `depth = 1`.
2. Обычные символы не меняют depth.
3. На закрывающей `}`: `depth = 0`.
4. В `result` копируется:

```json
{"0-0":"#ff0000","1-0":"#000000"}
```

### 10.4. Неверный ввод и ограничение

Возвращается false при отсутствии поля, двоеточия, открывающей или закрывающей скобки.

Счётчик учитывает вложенные объекты.

Он не понимает, что фигурная скобка внутри строкового значения не должна менять глубину. Текущие объекты слоёв содержат только координаты и цвета, поэтому скобок внутри строк не ожидается.

---

## 11. `findFirstHexColor()`

### 11.1. Полный актуальный код

```cpp
bool findFirstHexColor(const String &json, String &hexColor) {
    int hashPos = json.indexOf("#");
    while (hashPos >= 0) {
        if (hashPos + 7 <= json.length()) {
            String candidate = json.substring(hashPos, hashPos + 7);
            bool valid = true;
            for (int i = 1; i < 7; i++) {
                char c = candidate[i];
                if (!isxdigit((unsigned char)c)) {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                hexColor = candidate;
                return true;
            }
        }
        hashPos = json.indexOf("#", hashPos + 1);
    }
    return false;
}
```

### 11.2. Реальный вызывающий

Только `handleSaveStatusColors()`. Первый цвет нормализованного слоя становится общим fallback-цветом статуса.

### 11.3. Пошаговый пример

Вход:

```json
{"0-0":"bad","1-0":"#ff0000","2-0":"#00ff00"}
```

```cpp
int hashPos = json.indexOf("#");
```

Находится `#` перед `ff0000`.

```cpp
String candidate = json.substring(hashPos, hashPos + 7);
```

`candidate = "#ff0000"`.

Цикл проверяет шесть символов после `#`. Все допустимы.

```cpp
hexColor = candidate;
return true;
```

Результат:

```text
hexColor = "#ff0000"
```

### 11.4. Неверный ввод

Если ни одной последовательности `#` плюс шесть hex-цифр нет, функция возвращает false и не обязана менять `hexColor`.

Она не проверяет кавычки или границу после седьмого символа. Вызов выполняется после строгой нормализации слоя, поэтому вход уже имеет ожидаемый формат.

---

## 12. `parseFreeZoneMode()`

### 12.1. Полный актуальный код

```cpp
bool parseFreeZoneMode(String mode, FreeZoneMode &result) {
    mode.toLowerCase();
    if (mode == "static") result = FREE_ZONE_STATIC;
    else if (mode == "custom") result = FREE_ZONE_CUSTOM;
    else if (mode == "rainbow") result = FREE_ZONE_RAINBOW;
    else return false;
    return true;
}
```

### 12.2. Реальный вызывающий

`handleSaveFreeZone()` обрабатывает поле `"mode"`.

### 12.3. Построчный пример

Вход: `"RAINBOW"`.

```cpp
mode.toLowerCase();
```

Локальная копия становится `"rainbow"`.

Условия static и custom ложны. Третье условие записывает:

```text
result = FREE_ZONE_RAINBOW
```

Функция возвращает true.

### 12.4. Неверный ввод

`"blink"` возвращает false. Обработчик отвечает 400 `BAD_MODE`.

Функция не вызывает `trim()`, поэтому `" rainbow "` будет отклонено. Браузер отправляет точное значение без пробелов.

---

## 13. `canonicalStatusName()`

### 13.1. Полный актуальный код

```cpp
bool canonicalStatusName(const String &value, String &canonical) {
    if (value == "waiting" || value == "wait") canonical = "waiting";
    else if (value == "charging" || value == "charge") canonical = "charging";
    else if (value == "error" || value == "err") canonical = "error";
    else return false;
    return true;
}
```

### 13.2. Реальный вызывающий

`handleSaveStatusColors()` нормализует поле `"status"`.

### 13.3. Примеры изменения

```text
value = "wait"     -> canonical = "waiting", true
value = "charging" -> canonical = "charging", true
value = "err"      -> canonical = "error", true
value = "paused"   -> canonical не используется, false
```

Проверка чувствительна к регистру и не удаляет пробелы.

При false обработчик отвечает 400 `BAD_STATUS`.

---

## 14. `appendHardwareMapJson()`

### 14.1. Полный актуальный код

```cpp
void appendHardwareMapJson(String &json) {
    json += ",\"hardware_map\":{";
    bool first = true;

    for (int y = 0; y < VIRTUAL_Y; y++) {
        for (int x = 0; x < MATRIX_X; x++) {
            int index = getLEDIndex(x, y);
            if (index < 0) continue;

            uint8_t zoneId = zoneMap[index];
            if (zoneId == 0) continue;

            if (!first) json += ",";
            first = false;

            json += "\"";
            json += String(x);
            json += "-";
            json += String(y);
            json += "\":\"";
            json += String((int)zoneId);
            json += "\"";
        }
    }

    json += "}";
}
```

### 14.2. Реальный вызывающий

Только `handleGetConfig()`.

### 14.3. Входные данные

- `json` уже содержит начало корневого объекта и matrix.
- `zoneMap[]` хранит зоны по физическим индексам.
- `cfg.topology` используется внутри `getLEDIndex()`.

Пример до вызова:

```text
{"topo":0,...,"matrix":{"cols":12,"rows":8,"total":96,"topology":0}}
```

### 14.4. Построчное изменение

```cpp
json += ",\"hardware_map\":{";
```

Добавляется новое поле и открывающая `{`.

```cpp
bool first = true;
```

Переменная определяет, нужна ли запятая перед следующей записью.

Два цикла `for` перебирают:

```text
y = 0..7
x = 0..11
```

```cpp
int index = getLEDIndex(x, y);
```

Логическая координата переводится в физический индекс.

```cpp
if (index < 0) continue;
```

Неверная координата пропускается. При текущих границах она должна быть валидной.

```cpp
uint8_t zoneId = zoneMap[index];
if (zoneId == 0) continue;
```

Пиксели «Без зоны» не включаются в JSON. Браузер считает отсутствующий ключ зоной 0.

```cpp
if (!first) json += ",";
first = false;
```

Перед первой записью запятая не нужна. Перед последующими нужна.

Оставшиеся строки строят:

```json
"3-2":"1"
```

Координата и `zoneId` отправляются строками.

В конце:

```cpp
json += "}";
```

закрывает только объект `hardware_map`.

### 14.5. Результат и ошибки

Пример добавленного фрагмента:

```json
,"hardware_map":{"0-0":"2","1-0":"2","6-0":"1"}
```

Функция не возвращает ошибку. Она полагается на валидные размеры, топологию и `zoneMap`.

---

## 15. `appendStatusLayersJson()`

### 15.1. Полный актуальный код

```cpp
void appendStatusLayersJson(String &json) {
    json += ",\"layers\":{\"wait\":";
    json += matrix_config_load_status_layer("waiting");
    json += ",\"charge\":";
    json += matrix_config_load_status_layer("charging");
    json += ",\"err\":";
    json += matrix_config_load_status_layer("error");
    json += "}";
}
```

### 15.2. Реальный вызывающий

Только `handleGetConfig()`.

### 15.3. Пошаговое изменение

Пусть NVS содержит:

```text
waiting  -> {"0-0":"#00ff00"}
charging -> {}
error    -> {"0-0":"#ff0000"}
```

Изменение `json`:

```text
до:
...,"matrix":{...}

после первой строки:
...,"matrix":{...},"layers":{"wait":

после waiting:
...,"layers":{"wait":{"0-0":"#00ff00"}

после charging:
...,"layers":{"wait":{...},"charge":{}

после error и закрытия:
...,"layers":{"wait":{...},"charge":{},"err":{"0-0":"#ff0000"}}
```

Функция читает слои непосредственно из storage при каждом `/get_config`, а не сериализует LED-кэш.

При отсутствии слоя storage-функция возвращает пустой объект `{}`.

---

## 16. `appendFreeZoneConfigsJson()`

### 16.1. Полный актуальный код

```cpp
void appendFreeZoneConfigsJson(String &json) {
    json += ",\"free_zones\":[";

    for (size_t i = 0; i < FREE_ZONE_COUNT; i++) {
        if (i > 0) json += ",";

        const FreeZoneConfig &freeZone = freeZoneConfigs[i];
        json += "{\"zoneId\":";
        json += String((int)freeZone.zoneId);
        json += ",\"enabled\":";
        json += (freeZone.enabled ? "true" : "false");
        json += ",\"mode\":\"";
        json += free_zone_mode_to_string(freeZone.mode);
        json += "\",\"staticColor\":\"";
        json += rgbToHex(freeZone.staticColor);
        json += "\",\"brightness\":";
        json += String((int)freeZone.brightness);
        json += ",\"customLayer\":";
        json += matrix_config_load_free_zone_layer(freeZone.zoneId);
        json += "}";
    }

    json += "]";
}
```

### 16.2. Реальный вызывающий

Только `handleGetConfig()`.

### 16.3. Пошаговый пример одного элемента

Пусть:

```text
freeZoneConfigs[0]:
zoneId = 5
enabled = 1
mode = FREE_ZONE_STATIC
staticColor = [255,255,255]
brightness = 100
custom layer = {}
```

После открытия массива:

```text
,"free_zones":[
```

Цикл выполняется четыре раза.

```cpp
if (i > 0) json += ",";
```

Добавляет разделитель между объектами, но не перед первым.

```cpp
const FreeZoneConfig &freeZone = freeZoneConfigs[i];
```

Создаётся неизменяемая ссылка на рабочую конфигурацию в RAM.

Последовательные `+=` строят:

```json
{
  "zoneId": 5,
  "enabled": true,
  "mode": "static",
  "staticColor": "#ffffff",
  "brightness": 100,
  "customLayer": {}
}
```

`matrix_config_load_free_zone_layer()` читает пользовательский рисунок этой зоны из NVS.

После четырёх объектов `json += "]"` закрывает массив.

Полное соответствие строк и результата:

| Инструкция | Что добавляет или изменяет |
|---|---|
| `json += ",\"free_zones\":["` | имя поля и начало массива |
| `for (...)` | перебор четырёх рабочих конфигураций |
| `if (i > 0) json += ","` | запятая между объектами |
| `const FreeZoneConfig &freeZone = ...` | ссылка на текущую конфигурацию без копии |
| `json += "{\"zoneId\":"` | начало объекта и имя zoneId |
| `json += String((int)freeZone.zoneId)` | числовой zoneId |
| `json += ",\"enabled\":"` | имя enabled |
| тернарный оператор `freeZone.enabled ? ...` | логическое JSON-значение true/false без кавычек |
| `json += ",\"mode\":\""` | начало строкового mode |
| `free_zone_mode_to_string(...)` | `static`, `custom` или `rainbow` |
| `json += "\",\"staticColor\":\""` | закрывает mode, открывает staticColor |
| `rgbToHex(...)` | цвет `#RRGGBB` |
| `json += "\",\"brightness\":"` | закрывает цвет, открывает brightness |
| `String((int)freeZone.brightness)` | число 1..255 |
| `json += ",\"customLayer\":"` | имя объекта рисунка |
| `matrix_config_load_free_zone_layer(...)` | готовый JSON-объект из NVS |
| `json += "}"` | конец одной свободной зоны |
| `json += "]"` | конец массива |

### 16.4. Ошибки

Функция не возвращает ошибку.

Рабочие `freeZoneConfigs[]` уже были проверены при загрузке или сохранении. Невалидный либо отсутствующий custom layer storage возвращает как пустой объект.

---

## 17. `appendZoneMetadataJson()`

### 17.1. Полный актуальный код

```cpp
void appendZoneMetadataJson(String &json) {
    StatusMapperState status;
    status_mapper_get_snapshot(status);
    const uint8_t activePortCount = status.activePortCount > MAX_PORT_COUNT
        ? MAX_PORT_COUNT
        : status.activePortCount;

    json += ",\"zones\":[";

    for (size_t i = 0; i < ZONE_METADATA_COUNT; i++) {
        if (i > 0) json += ",";

        const ZoneMetadata &zone = ZONE_METADATA[i];
        bool enabled = zone.enabled;
        bool reserved = zone.reserved;

        if (zone.type == ZONE_TYPE_PORT) {
            const bool activePort = zone.linkedPort >= 0 && zone.linkedPort < activePortCount;
            enabled = activePort;
            reserved = !activePort;
        }

        json += "{\"id\":";
        json += String((int)zone.id);
        json += ",\"name\":\"";
        json += zone.name;
        json += "\",\"type\":\"";
        json += zone_type_to_string(zone.type);
        json += "\",\"enabled\":";
        json += (enabled ? "true" : "false");
        json += ",\"reserved\":";
        json += (reserved ? "true" : "false");

        if (zone.type == ZONE_TYPE_PORT) {
            json += ",\"linked_port\":";
            json += String((int)zone.linkedPort + 1);
        } else {
            json += ",\"linked_port\":null";
        }

        if (zone.type == ZONE_TYPE_FREE) {
            json += ",\"mode\":\"";
            json += free_zone_mode_to_string(currentFreeZoneMode(zone.id));
            json += "\"";
        } else {
            json += ",\"mode\":null";
        }

        json += "}";
    }

    json += "]";
}
```

### 17.2. Реальный вызывающий

Только `handleGetConfig()`.

### 17.3. Начальные локальные данные

```cpp
StatusMapperState status;
status_mapper_get_snapshot(status);
```

Получается согласованный снимок activePortCount.

```cpp
const uint8_t activePortCount = ...
```

Значение ограничивается `MAX_PORT_COUNT`.

При текущем проекте:

```text
status.activePortCount = 2
activePortCount = 2
```

### 17.4. Цикл метаданных

Цикл проходит по зонам 0..8.

```cpp
const ZoneMetadata &zone = ZONE_METADATA[i];
```

Создаётся неизменяемая ссылка на текущую запись.

```cpp
bool enabled = zone.enabled;
bool reserved = zone.reserved;
```

Сначала копируются статические defaults.

Для port zone они заменяются динамически:

```cpp
const bool activePort =
    zone.linkedPort >= 0 &&
    zone.linkedPort < activePortCount;
```

Примеры:

```text
Port1 linkedPort=0: 0 < 2 -> active
Port2 linkedPort=1: 1 < 2 -> active
Port3 linkedPort=2: 2 < 2 -> reserved
Port4 linkedPort=3: 3 < 2 -> reserved
```

Затем строка собирает поля:

- `id`;
- `name`;
- `type`;
- `enabled`;
- `reserved`;
- `linked_port`;
- `mode`.

Для портовой зоны `linked_port` равен 1..4. Для остальных используется JSON-значение `null`.

Для свободной зоны `currentFreeZoneMode()` читает текущий режим из `freeZoneConfigs[]`. Для остальных `mode = null`.

Построение одного объекта по строкам:

| Инструкция | Результат |
|---|---|
| `json += "{\"id\":"` | открывает объект и поле id |
| `String((int)zone.id)` | добавляет номер |
| `json += ",\"name\":\""` | открывает строку name |
| `json += zone.name` | добавляет постоянное имя из `ZONE_METADATA` |
| `json += "\",\"type\":\""` | закрывает name и открывает type |
| `zone_type_to_string(...)` | добавляет `off`, `port` или `free` |
| `json += "\",\"enabled\":"` | открывает enabled |
| тернарный оператор для `enabled` | добавляет true/false без кавычек |
| `json += ",\"reserved\":"` | открывает reserved |
| тернарный оператор для `reserved` | добавляет true/false |
| ветка `ZONE_TYPE_PORT` | добавляет linked_port 1..4 |
| ветка `else` | добавляет linked_port null |
| ветка `ZONE_TYPE_FREE` | добавляет строковый mode |
| её `else` | добавляет mode null |
| `json += "}"` | закрывает текущую зону |
| итоговый `json += "]"` | закрывает массив всех зон |

### 17.5. Пример результата Port2

```json
{
  "id": 2,
  "name": "Port 2",
  "type": "port",
  "enabled": true,
  "reserved": false,
  "linked_port": 2,
  "mode": null
}
```

Port3 отличается:

```json
{
  "id": 3,
  "name": "Port 3",
  "type": "port",
  "enabled": false,
  "reserved": true,
  "linked_port": 3,
  "mode": null
}
```

Функция не сохраняет active/reserved в NVS. Она вычисляет их для каждого ответа.

---

## 18. `handleGetConfig()`

### 18.1. Полный актуальный код

```cpp
void handleGetConfig() {
    if (!requireHttpAuth()) return;

    String json;
    json.reserve(7600);
    json += "{\"topo\":";
    json += String((int)cfg.topology);
    json += ",\"b_ports\":";
    json += String((int)cfg.bright_ports);
    json += ",\"c_wait\":\"";
    json += rgbToHex(cfg.color_wait);
    json += "\",\"c_charge\":\"";
    json += rgbToHex(cfg.color_charge);
    json += "\",\"c_error\":\"";
    json += rgbToHex(cfg.color_error);
    json += "\",\"activePortCount\":";
    StatusMapperState status;
    status_mapper_get_snapshot(status);
    json += String((int)status.activePortCount);
    json += ",\"matrix\":{\"cols\":";
    json += String((int)MATRIX_X);
    json += ",\"rows\":";
    json += String((int)VIRTUAL_Y);
    json += ",\"total\":";
    json += String((int)NUM_IC_CHIPS);
    json += ",\"topology\":";
    json += String((int)cfg.topology);
    json += "}";
    appendStatusLayersJson(json);
    appendHardwareMapJson(json);
    appendZoneMetadataJson(json);
    appendFreeZoneConfigsJson(json);
    json += "}";

    server.send(200, "application/json", json);
}
```

### 18.2. Кто вызывает

`setup()` регистрирует:

```cpp
server.on("/get_config", handleGetConfig);
```

Браузерная функция `loadConfig()` выполняет GET `/get_config` при первой загрузке страницы и после действий, требующих повторной синхронизации.

### 18.3. Авторизация

```cpp
if (!requireHttpAuth()) return;
```

При неверных данных формируется 401. Никакой JSON конфигурации не раскрывается.

### 18.4. Создание строки

```cpp
String json;
json.reserve(7600);
```

Создаётся пустая локальная строка.

`reserve(7600)` заранее просит около 7600 байт вместимости, чтобы многочисленные `+=` реже перераспределяли heap, то есть динамическую область RAM.

Это не создаёт файл размером 7600 байт и не заставляет ответ иметь такую длину.

### 18.5. Построчное построение основы

Пусть:

```text
cfg.topology = 0
cfg.bright_ports = 120
color_wait = [0,255,0]
color_charge = [0,80,255]
color_error = [255,0,0]
activePortCount = 2
```

Изменение:

```text
json = ""

после "{\"topo\":":
{"topo":

после topology:
{"topo":0

после bright_ports:
{"topo":0,"b_ports":120

после трёх цветов:
{"topo":0,"b_ports":120,"c_wait":"#00ff00","c_charge":"#0050ff","c_error":"#ff0000"

после activePortCount:
...,"activePortCount":2

после matrix:
...,"matrix":{"cols":12,"rows":8,"total":96,"topology":0}
```

`StatusMapperState status` является локальным снимком. Он нужен только для `activePortCount`.

Соответствие каждой инструкции основы:

| Инструкция | Что происходит |
|---|---|
| `String json` | создаётся пустая локальная строка |
| `json.reserve(7600)` | заранее резервируется вместимость RAM |
| `json += "{\"topo\":"` | открывается корневой объект |
| `String((int)cfg.topology)` | добавляется основное значение топологии |
| `json += ",\"b_ports\":"` | открывается поле яркости |
| `String((int)cfg.bright_ports)` | добавляется яркость портов |
| `json += ",\"c_wait\":\""` | открывается цвет ожидания |
| `rgbToHex(cfg.color_wait)` | добавляется `#RRGGBB` |
| строка с `c_charge` и `rgbToHex` | добавляется цвет зарядки |
| строка с `c_error` и `rgbToHex` | добавляется цвет ошибки |
| `json += "\",\"activePortCount\":"` | открывается поле числа портов |
| `StatusMapperState status` | создаётся локальный получатель |
| `status_mapper_get_snapshot(status)` | копируется согласованный статус |
| `String((int)status.activePortCount)` | добавляется число портов |
| `json += ",\"matrix\":{\"cols\":"` | открывается вложенный matrix |
| `MATRIX_X` | добавляется 12 |
| `VIRTUAL_Y` | добавляется 8 |
| `NUM_IC_CHIPS` | добавляется 96 |
| `cfg.topology` | повторяется topology внутри canonical matrix |
| `json += "}"` | закрывается только вложенный matrix |
| четыре `append...` | добавляют layers, hardware_map, zones, free_zones |
| финальный `json += "}"` | закрывается корневой объект |
| `server.send(...)` | строка становится телом HTTP-ответа |

### 18.6. Как строка меняется после append-функций

Для читаемости ниже `BASE` означает уже собранную основу:

```text
BASE =
{"topo":0,"b_ports":120,"c_wait":"#00ff00",
"c_charge":"#0050ff","c_error":"#ff0000",
"activePortCount":2,
"matrix":{"cols":12,"rows":8,"total":96,"topology":0}
```

После:

```cpp
appendStatusLayersJson(json);
```

```text
BASE
,"layers":{"wait":{},"charge":{},"err":{}}
```

После:

```cpp
appendHardwareMapJson(json);
```

```text
BASE
,"layers":{...}
,"hardware_map":{"0-0":"2","1-0":"2",...,"11-7":"1"}
```

Многоточие здесь и далее добавлено только в документации. Реальный HTTP-ответ содержит все назначенные координаты и не содержит `...`.

После:

```cpp
appendZoneMetadataJson(json);
```

```text
BASE
,"layers":{...}
,"hardware_map":{...}
,"zones":[
  {"id":0,...},
  {"id":1,...},
  ...
  {"id":8,...}
]
```

После:

```cpp
appendFreeZoneConfigsJson(json);
```

```text
BASE
,"layers":{...}
,"hardware_map":{...}
,"zones":[...]
,"free_zones":[
  {"zoneId":5,...},
  {"zoneId":6,...},
  {"zoneId":7,...},
  {"zoneId":8,...}
]
```

После:

```cpp
json += "}";
```

закрывается корневой объект.

### 18.7. Отправка HTTP-ответа

```cpp
server.send(200, "application/json", json);
```

- 200 означает успех.
- `application/json` сообщает браузеру тип содержимого.
- `json` становится телом HTTP-ответа.

Ни `json`, ни ответ не сохраняются как файл в NVS, flash или файловой системе.

### 18.8. Ошибки

Явного ответа 500 внутри `handleGetConfig()` нет.

- Авторизация может завершить запрос кодом 401.
- Невалидные storage-слои загружаются как пустые объекты.
- Метод не проверяет успешность выделения `String.reserve()`. При критической нехватке heap поведение Arduino String требует отдельного нагрузочного теста.

---

## 19. Реалистичный итоговый `/get_config`

Ниже показана форма ответа для 12x8, topology 0, двух активных портов и начальных цветов.

`hardware_map` намеренно сокращён только в документации. Комментарий и многоточие не являются допустимым JSON и никогда не отправляются ESP32.

```jsonc
{
  "topo": 0,
  "b_ports": 120,
  "c_wait": "#00ff00",
  "c_charge": "#0050ff",
  "c_error": "#ff0000",
  "activePortCount": 2,
  "matrix": {
    "cols": 12,
    "rows": 8,
    "total": 96,
    "topology": 0
  },
  "layers": {
    "wait": {},
    "charge": {},
    "err": {}
  },
  "hardware_map": {
    "0-0": "2",
    "1-0": "2",
    "2-0": "2",
    "3-0": "2",
    "4-0": "2",
    "5-0": "2",
    "6-0": "1",
    "7-0": "1",
    "8-0": "1",
    "9-0": "1",
    "10-0": "1",
    "11-0": "1"
    /* строки y=1..7 сокращены только здесь */
  },
  "zones": [
    {"id":0,"name":"Off","type":"off","enabled":true,"reserved":false,"linked_port":null,"mode":null},
    {"id":1,"name":"Port 1","type":"port","enabled":true,"reserved":false,"linked_port":1,"mode":null},
    {"id":2,"name":"Port 2","type":"port","enabled":true,"reserved":false,"linked_port":2,"mode":null},
    {"id":3,"name":"Port 3","type":"port","enabled":false,"reserved":true,"linked_port":3,"mode":null},
    {"id":4,"name":"Port 4","type":"port","enabled":false,"reserved":true,"linked_port":4,"mode":null},
    {"id":5,"name":"Logo","type":"free","enabled":true,"reserved":false,"linked_port":null,"mode":"static"},
    {"id":6,"name":"QR","type":"free","enabled":true,"reserved":false,"linked_port":null,"mode":"static"},
    {"id":7,"name":"Service","type":"free","enabled":true,"reserved":false,"linked_port":null,"mode":"static"},
    {"id":8,"name":"Custom","type":"free","enabled":true,"reserved":false,"linked_port":null,"mode":"custom"}
  ],
  "free_zones": [
    {"zoneId":5,"enabled":true,"mode":"static","staticColor":"#ffffff","brightness":100,"customLayer":{}},
    {"zoneId":6,"enabled":true,"mode":"static","staticColor":"#34c759","brightness":100,"customLayer":{}},
    {"zoneId":7,"enabled":true,"mode":"static","staticColor":"#ff9500","brightness":100,"customLayer":{}},
    {"zoneId":8,"enabled":true,"mode":"custom","staticColor":"#af52de","brightness":100,"customLayer":{}}
  ]
}
```

При реальной пользовательской разметке, слоях или цветах значения будут другими.

---

## 20. Таблица HTTP API, storage, RAM и UI

В колонке «Storage» указана функция постоянного хранения. Прочерк означает, что запрос ничего не сохраняет.

| Endpoint | Метод UI | Функция `main.cpp` | Основная проверка | Storage-функция | Применение в RAM/устройстве | Действие UI |
|---|---|---|---|---|---|---|
| `/` | GET | `handleRoot()` | Basic Auth | - | отдаёт `PAGE_MAIN` | открывает страницу |
| `/get_size` | GET, fallback | `handleGetMatrixSize()` | Basic Auth | - | читает compile-time размер | запасная загрузка размера |
| `/get_config` | GET | `handleGetConfig()` | Basic Auth | `matrix_config_load_status_layer()`, `matrix_config_load_free_zone_layer()` | только читает RAM и хранилище | строит все редакторы |
| `/diagnostics` | GET | `handleDiagnostics()` | Basic Auth | - | читает snapshots входов/статусов | обновляет диагностику |
| `/save_zones` | POST JSON | `handleSaveZones()` | JSON, координаты, zoneId, active ports | `matrix_config_save()` | `led_replace_zone_map_safe()`, reload free layers | снимает dirty после OK |
| `/save_topology` | POST | `handleSaveTopology()` | topology 0..3, remap | `matrix_config_save()` | `led_apply_matrix_config_safe()` | перезагружает config |
| `/set_bright` | POST form | `handleSetBright()` | type ports, value 1..255 | `matrix_config_save()` | `cfg = nextConfig`, `led_refresh_safe()` | autosave яркости портов |
| `/save_status_colors` | POST JSON | `handleSaveStatusColors()` | status, размер, нормализация layer | `matrix_config_save_status_layer()`, иногда `matrix_config_save()` | иногда новый `cfg`, reload status caches | сохраняет выбранный слой |
| `/save_free_zone` | POST JSON | `handleSaveFreeZone()` | zone, partial fields, mode/color/layer | `matrix_config_save_free_zones()`, при custom `matrix_config_save_free_zone_layer()` | обновляет `freeZoneConfigs[]`, reload/refresh | сохраняет зону или brightness |
| `/network_status` | GET | `handleNetworkStatus()` | Basic Auth | - | читает `networkConfig` и Wi-Fi | показывает сеть |
| `/scan_networks` | GET | `handleScanNetworks()` | Basic Auth, состояние поиска | - | запускает/читает неблокирующий поиск | показывает найденные SSID |
| `/save_network` | POST form | `handleSaveNetworkConfig()` | SSID, пароли, DHCP/static IP | `network_config_save()` | обновляет `networkConfig` после verified save | затем вызывает reconnect |
| `/network_reconnect` | POST | `handleNetworkReconnect()` | Basic Auth | - | `network_schedule_apply(1500)` | показывает подключение |
| `/auth_status` | GET | `handleAuthStatus()` | Basic Auth | - | читает текущего пользователя | показывает warning defaults |
| `/save_auth` | POST form | `handleSaveAuth()` | username, password, confirm | `auth_config_save_credentials()` | обновляет current credentials | просит войти заново |
| `/update` | POST multipart | OTA callbacks | Basic Auth, `.bin`, Update result | библиотека `Update` пишет раздел firmware | планирует `ESP.restart()` | показывает результат OTA |

Все endpoints сначала требуют авторизацию. Дополнительные проверки выполняются до записи storage. При ошибке записи актуальные save-обработчики возвращают 500, а не ложный 200.

---

## 21. Как входной и выходной JSON связаны

### Входной путь POST

```text
JavaScript собирает объект
        ↓
JSON.stringify(...)
        ↓
fetch('/save_free_zone', {method:'POST', body: ...})
        ↓
WebServer принимает HTTP body
        ↓
server.arg("plain")
        ↓
extract/jsonHas/parse helpers
        ↓
полная проверка
        ↓
storage save
        ↓
обновление RAM
        ↓
ответ 200 или ошибка 400/500
```

### Выходной путь GET

```text
fetch('/get_config')
        ↓
handleGetConfig()
        ↓
чтение cfg/status/zoneMap/freeZoneConfigs
        ↓
чтение строк layers из storage
        ↓
последовательные String +=
        ↓
server.send(200, "application/json", json)
        ↓
браузер разбирает ответ
        ↓
applyConfig() заполняет UI
```

Входные helper-функции не являются универсальным JSON-парсером. Они безопасны только вместе с ограничениями размера, нормализацией слоёв и проверками текущих обработчиков. При расширении API сложными вложенными данными следует повторно оценить переход на структурированную JSON-библиотеку.
