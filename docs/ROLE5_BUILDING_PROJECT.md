# CustomDB – Интеграция, тестирование и запуск

## Часть A. Отчёт по интеграции сборки, кроссплатформенности, контейнеризации и тестированию (роль №5)

### 1. Задача

- Обеспечить сборку C++ сервера на Windows (MSVC) и Linux (GCC).
- Устранить зависимости от POSIX-специфичных API в сетевом слое.
- Автоматизировать процесс получения конечных артефактов: нативный сервер для Windows, самодостаточный GUI для Windows, Docker-образ сервера для Linux.
- Предоставить универсальный способ запуска сервера на любой ОС через Docker.
- Разработать и внедрить автоматические тесты (unit, интеграционные, функциональные) для проверки корректности сборки, работы сервера, персистентности и GUI.
- Создать единый скрипт для запуска всех тестов.

### 2. Реализованные решения

#### 2.1. Кроссплатформенная адаптация сетевого кода

**Проблема:** использование `<arpa/inet.h>`, `<sys/socket.h>`, функций `close()`, `sleep()` делает код несовместимым с Windows.

**Решение:** условная компиляция (`#ifdef _WIN32`) с заменой на эквиваленты WinSock2.

**Изменённые файлы:**
- `src/lib/Connection.cpp`
- `src/network/ClientSession.cpp`
- `src/network/Protocol.cpp`

**Фрагмент реализации:**
```cpp
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define CLOSE_SOCKET close
#endif
```

**Результат:** код компилируется и выполняется на обеих платформах без изменений логики.

#### 2.2. Docker-контейнеризация сервера

**Dockerfile (многостадийная сборка):**
```dockerfile
FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y build-essential cmake git nlohmann-json3-dev
WORKDIR /app
COPY . .
RUN rm -rf build && cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake --build build

FROM ubuntu:22.04
COPY --from=builder /app/build/customdb_server .
EXPOSE 5432
CMD ["./customdb_server"]
```

**docker-compose.yml:**
```yaml
services:
  customdb-server:
    build: .
    ports:
      - "5432:5432"
    volumes:
      - customdb-data:/app/data
volumes:
  customdb-data:
```

#### 2.3. Автоматизация сборки

**`scripts/build_full.bat` (Windows):**
```batch
@echo off
if exist build rmdir /s /q build
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
cd ..
cd src\gui
dotnet publish CustomDB.UI.csproj -c Release --self-contained -r win-x64 -o ..\..\publish\win-x64
cd ..\..
copy build\Release\customdb_server.exe publish\win-x64\
docker build -t databaseoficial-customdb-server .
```

**`scripts/build_full.sh` (Linux / WSL):**
```bash
#!/bin/bash
set -e
OS=$(uname -s)
if [ "$OS" = "Linux" ]; then
    sudo apt-get install -y nlohmann-json3-dev
elif [ "$OS" = "Darwin" ]; then
    brew install nlohmann-json
fi
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..
mkdir -p publish/linux-x64
cp build/customdb_server publish/linux-x64/
docker build -t databaseoficial-customdb-server .
```

#### 2.4. Настройка GUI и автоматическое подключение

В `MainWindow.xaml.cs` добавлена инициализация `DatabaseService` с подключением к `localhost:5432` при загрузке окна. Установлен `<OutputType>WinExe</OutputType>` для исключения консольного окна.

#### 2.5. Оптимизация `.gitignore`

Исправлены правила: `/lib/` вместо `lib/` (чтобы не блокировать `src/lib`), добавлены `publish/`, `build/`, `*.tar`, `*.exe`.

#### 2.6. Работа с Git

Ветка `feature/integration-build`, использование `noreply`-адреса, Pull Request.

### 3. Автоматизированное тестирование

#### 3.1. Unit-тесты (C++ / Google Test) – `tests/unit/unit_tests_all.cpp`
- 18 тестов (Column, Table, Database). Все проходят.

#### 3.2. Интеграционные тесты (C++ / Google Test) – `tests/integration/test_catalog_storage.cpp`
- 3 теста на персистентность. Все проходят.

#### 3.3. Функциональные тесты (Python) – `functional_test.py`
- Проверка сборки, работы нативного и Docker‑сервера, SQL, GUI.
- Персистентность нативного сервера падает (дефект в обвязке, не в ядре). Docker‑версия работает стабильно.

#### 3.4. Единые скрипты запуска всех тестов
- `run_all_tests.bat` (Windows)
- `run_all_tests.sh` (Linux / WSL)

### 4. Полученные артефакты

| Артефакт | Назначение |
|----------|------------|
| `publish/win-x64/CustomDB.UI.exe` | GUI для Windows (самодостаточный) |
| `build/Release/customdb_server.exe` | Нативный сервер Windows |
| `databaseoficial-customdb-server:latest` | Docker-образ сервера |
| `publish/linux-x64/customdb_server` | Нативный сервер Linux |
| `tests/unit/unit_tests_all.cpp` | Unit-тесты |
| `tests/integration/test_catalog_storage.cpp` | Интеграционные тесты |
| `functional_test.py` | Функциональные тесты |
| `run_all_tests.bat` / `.sh` | Запуск всех тестов |

### 5. Результаты тестирования

- **Unit:** 18/18 ✅
- **Integration:** 3/3 ✅
- **Functional:** кроме персистентности нативного сервера ✅ (дефект передан разработчикам)

### 6. Заключение

Реализованная инфраструктура сборки и тестирования обеспечивает:
- Однокомандное получение всех необходимых бинарных файлов.
- Полную кроссплатформенность C++ кода (MSVC/GCC).
- Независимость GUI от .NET Runtime благодаря `self-contained` публикации.
- Универсальный способ запуска сервера через Docker, исключающий проблемы с зависимостями.
- Трёхуровневую автоматическую проверку (unit, интеграционные, функциональные тесты).

---

## Часть B. Руководство для жюри по запуску CustomDB

### Что нужно для запуска

- **Docker Desktop** (скачать с [docker.com](https://www.docker.com/products/docker-desktop/))
- **Windows 10/11** (для графического клиента) или любая ОС с Docker (только для сервера)

**В поставку входят:**
- файл `customdb-server.tar` – Docker-образ сервера
- папка `win-x64` – GUI (`CustomDB.UI.exe`)
- данная инструкция

---

### 1. Запуск сервера (один раз)

1. Установите и запустите Docker Desktop (дождитесь зелёной иконки кита в трее).
2. Откройте командную строку (cmd, PowerShell или терминал) в папке с `customdb-server.tar`.
3. Выполните:
   ```bash
   docker load -i customdb-server.tar
   docker run -d --name customdb-server -p 5432:5432 databaseoficial-customdb-server
   ```
   > Если порт 5432 занят, замените левый порт, например `-p 5433:5432`. Тогда в GUI указывайте порт `5433`.

4. Проверьте, что контейнер работает:
   ```bash
   docker ps
   ```

---

### 2. Запуск GUI (только Windows)

1. Распакуйте папку `win-x64`.
2. Запустите `CustomDB.UI.exe`.
3. Нажмите кнопку **Connect** (по умолчанию `localhost:5432` – если вы не меняли порт).
4. Статус изменится на `Connected`.

---

### 3. Выполнение SQL-запросов

**Пример полного сеанса:**
```sql
CREATE DATABASE company;
USE company;
CREATE TABLE employees (id INT, name TEXT, salary FLOAT);
INSERT INTO employees VALUES (1, 'Alice', 50000);
INSERT INTO employees VALUES (2, 'Bob', 60000);
SELECT * FROM employees;
```

**Поддерживаемые операции:**
- `CREATE/DROP DATABASE`
- `CREATE/DROP TABLE`
- `INSERT INTO`, `SELECT`, `UPDATE`, `DELETE`
- типы данных: `INT`, `FLOAT`, `BOOL`, `TEXT`, `VARCHAR(N)`

---

### 4. Остановка сервера (после демонстрации)

```bash
docker stop customdb-server
docker rm customdb-server    # если больше не нужен
```

---

### 5. Возможные проблемы и их решение

| Проблема | Решение |
|----------|---------|
| `docker: command not found` | Установите Docker Desktop и запустите. |
| `Cannot connect to the Docker daemon` | Запустите Docker Desktop. |
| Порт 5432 занят | Остановите другой сервис или используйте `-p 5433:5432`. |
| GUI `disconnected` | Убедитесь, что контейнер запущен (`docker ps`), и нажмите **Connect**. |
| Ошибка при запросе | Проверьте синтаксис, не забудьте `;`. Сначала создайте БД и выполните `USE`. |

---

### Заключение

- Сервер работает в изолированном контейнере, не требует установки компиляторов.
- GUI для Windows полностью автономен (не требует .NET Runtime).
- Все базовые функции СУБД (создание БД/таблиц, CRUD) работают стабильно.
- Проект успешно протестирован на Windows, Linux (WSL) и в Docker.