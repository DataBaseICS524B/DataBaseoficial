# Роль 3: Реализация C++/C# моста и сетевого слоя

## Список созданных и изменённых файлов

### Файлы которые добавились или были изменены

| Файл | Назначение |
|------|------------|
| `src/network/Server.h` | Заголовок TCP сервера |
| `src/network/Server.cpp` | Реализация TCP сервера с пулом потоков |
| `src/network/ClientSession.h` | Заголовок обработчика клиентской сессии |
| `src/network/ClientSession.cpp` | Реализация парсинга и выполнения SQL запросов |
| `src/network/Protocol.h` | Заголовок сетевого протокола и JSON сериализации |
| `src/network/Protocol.cpp` | Реализация протокола (длина→запрос→статус→данные) |
| `src/network/ThreadPool.h` | Заголовок пула потоков |
| `src/network/ThreadPool.cpp` | Реализация пула потоков |
| `src/lib/Connection.cpp` | Реализация TCP сокетов (клиентская часть) |
| `src/lib/client.cpp` | C-обёртка для P/Invoke: `db_connect`, `db_execute`, `db_disconnect`, `db_free_string` |
| `src/cli/main.cpp` | Точка входа сервера (парсинг аргументов, запуск) |
| `CMakeLists.txt` | Добавлены сетевые файлы в сборку сервера |
| `scripts/run.sh` | Обновлён запуск сервера |

## Подробное описание файлов

### 📁 src/network/Server.h
```cpp
// TCP сервер. Слушает порт, принимает подключения,
// передаёт клиентов в пул потоков для обработки
```

### 📁 src/network/Server.cpp
```cpp
// Реализация:
// - socket() -> bind() -> listen() -> accept()
// - Запуск пула потоков
// - При подключении создаёт ClientSession и кидает в пул
```

### 📁 src/network/ClientSession.h
```cpp
// Обработка одного клиента:
// - Читает 4 байта (длина запроса)
// - Читает сам запрос
// - Вызывает processQuery()
// - Отправляет ответ (статус + длина + данные)
```

### 📁 src/network/ClientSession.cpp
```cpp
// Парсит SQL запросы с помощью regex:
// - CREATE DATABASE name
// - DROP DATABASE name
// - CREATE TABLE name (col TYPE)
// - INSERT INTO name VALUES (...)
// - SELECT * FROM name
// - UPDATE name SET col=val
// - DELETE FROM name
// - USE DATABASE name
// Вызывает методы Catalog для выполнения
```

### 📁 src/network/Protocol.h
```cpp
// Формат пакета:
// [4 bytes: длина запроса] [запрос]
// [4 bytes: статус] [4 bytes: длина ответа] [ответ]
//
// JSON сериализация результатов:
// - createSelectResult()  // для SELECT
// - createDMLResult()     // для INSERT/UPDATE/DELETE
// - createDDLResult()     // для CREATE/DROP
// - createErrorResult()   // для ошибок
```

### 📁 src/network/Protocol.cpp
```cpp
// Реализация:
// - encodeQuery() / decodeQuery()
// - encodeResponse() / decodeResponse()
// - Формирование JSON строк с результатами
```

### 📁 src/network/ThreadPool.h
```cpp
// Пул потоков. Работает по принципу:
// - Создаём N потоков при старте
// - Задачи складываем в очередь
// - Потоки разбирают очередь и выполняют
```

### 📁 src/network/ThreadPool.cpp
```cpp
// Реализация пула потоков:
// - workers_ (вектор потоков)
// - tasks_ (очередь задач)
// - enqueue() (добавить задачу)
// - stop() (остановка всех потоков)
```

### 📁 src/lib/Connection.cpp
```cpp
// Клиентский TCP сокет:
// - connect()    // подключение к серверу
// - execute()    // отправить запрос, получить ответ
// - disconnect() // закрыть соединение
//
// Кроссплатформенность:
// - Windows: winsock2.h
// - Linux/macOS: sys/socket.h
```

### 📁 src/lib/client.cpp
```cpp
// C-обёртка для вызова из C#:
// - db_connect()    // возвращает указатель на Client
// - db_execute()    // выполняет запрос, возвращает JSON
// - db_disconnect() // закрывает соединение
// - db_free_string()// освобождает память
```

### 📁 src/cli/main.cpp
```cpp
// Точка входа сервера:
// - Парсинг --host и --port
// - Инициализация Catalog (Singleton)
// - Запуск Server
// - Обработка Ctrl+C для graceful shutdown
```

---

## Архитектура взаимодействия

```
C# GUI (P/Invoke)
       │
       ▼
db_execute() ──────► C-обёртка (client.cpp)
                           │
                           ▼
                    DatabaseClient
                           │
                           ▼
                    Connection (TCP сокет)
                           │
                           ▼
┌──────────────────────────────────────────────────────┐
│                      СЕРВЕР                           │
│  ┌─────────┐    ┌──────────────┐    ┌─────────────┐  │
│  │ Server  │───►│ ThreadPool   │───►│ClientSession│  │
│  └─────────┘    └──────────────┘    └──────┬──────┘  │
│                                            │         │
│                                            ▼         │
│                                     ┌─────────────┐  │
│                                     │  Protocol   │  │
│                                     │ (JSON)      │  │
│                                     └─────────────┘  │
│                                            │         │
│                                            ▼         │
│                                     ┌─────────────┐  │
│                                     │  Catalog    │  │
│                                     │ (Singleton) │  │
│                                     └─────────────┘  │
│                                            │         │
│                                            ▼         │
│                                     ┌─────────────┐  │
│                                     │StorageEngine│  │
│                                     └─────────────┘  │
└──────────────────────────────────────────────────────┘
```

---

## Запуск

```bash
# 1. Собрать проект
./scripts/build.sh

# 2. Запустить сервер
./scripts/run.sh --host 127.0.0.1 --port 5433

# 3. Проверить через Python
python3 -c "
import socket
s = socket.socket()
s.connect(('127.0.0.1', 5433))
q = b'\x00\x00\x00\x11CREATE DATABASE test;'
s.send(q)
print(s.recv(1024))
"
```