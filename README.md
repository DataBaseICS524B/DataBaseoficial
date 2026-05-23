# CustomDB – Полная документация проекта

## CustomDB Project - Распределение задач между 5 ролями

| Компонент | Роль 1<br>Архитектор ядра | Роль 2<br>Парсер/Executor | Роль 3<br>Мост C++/C# | Роль 4<br>C# GUI | Роль 5<br>Тестирование |
|-----------|---------------------------|---------------------------|----------------------|------------------|------------------------|
| **C++ ядро (storage, catalog)** | ✅ | - | - | - | - |
| **C++ парсер/executor** | - | ✅ | - | - | - |
| **C++ клиентская библиотека** | ✅ | - | ✅ | - | - |
| **C++ сервер + сеть** | - | - | ✅ | - | - |
| **C++/C# мост (P/Invoke)** | - | - | ✅ | - | интеграция |
| **C# GUI (WPF/WinForms)** | - | - | - | ✅ | - |
| **Скрипты сборки** | помогает | - | - | - | ✅ |
| **Тестирование** | - | - | - | - | ✅ |

### Легенда

- **✅** - Основной исполнитель
- **-** - Не участвует
- **интеграция** - Участвует в интеграции компонента
- **помогает** - Помогает в разработке

## Коммуникация между ролями

Схема взаимодействия ролей в проекте: [communication_diagram.txt](communication_diagram.txt)

## Контакты и ответственность

| Роль | Имя (заполнить) | Telegram | Ответственность за сдачу |
|------|----------------|----------|--------------------------|
| Роль 1 |Бутузов Роман | @RomanButuzov | 25% |
| Роль 2 | Бутов Дмитрий | @dimetro_cr | 25% |
| Роль 3 | Бондаренко Данил | @Fenl1xs | 25% |
| Роль 4 | Жакыпбаев Асан | @asan1_1 | 15% |
| Роль 5 | Исааков Димитрий | @polemistis_1829 | 10% |

## Обзор

CustomDB — это учебная клиент-серверная СУБД с поддержкой:
- Метаданных (базы данных, таблицы, колонки)
- Постоянного хранилища (JSON-файлы)
- TCP-взаимодействия
- C API для P/Invoke из C#
- AUTO_INCREMENT, UNIQUE, массивов (TEXT[]), индексов
- Полнотекстового поиска и аудита
- Хранимых процедур
- Графического интерфейса (WPF)

---

## Архитектура взаимодействия компонентов

| № | Компонент | Технология | Назначение |
|---|-----------|------------|------------|
| 1 | **C# GUI** | WPF / .NET 8.0 | Графический интерфейс пользователя |
| 2 | **C API** | extern "C" | Мост для P/Invoke из C# |
| 3 | **C++ Client** | C++17 | TCP-клиент для подключения к серверу |
| 4 | **TCP Server** | C++ / POSIX sockets | Сервер с пулом потоков |
| 5 | **Catalog** | C++ (Singleton) | Управление метаданными БД |
| 6 | **StorageEngine** | C++ / JSON | Файловое хранение данных |

### Схема взаимодействия


### Паттерны проектирования

| Паттерн | Применение | Причина выбора |
|---------|-----------|----------------|
| Singleton | Catalog | Единственный экземпляр каталога БД на весь сервер |
| Repository | StorageEngine | Инкапсуляция операций с файловым хранилищем |
| Pimpl | DatabaseClient | Сокрытие деталей реализации от клиента |
| MVVM | WPF GUI | Разделение логики и представления |
| Command | RelayCommand | Привязка действий UI |

---

## 1. Ядро СУБД (C++)

### 1.1. Класс Catalog (Singleton)

```cpp
class Catalog {
public:
    static Catalog& getInstance();
    void initialize(const std::string& dataPath = "data");
    
    // Управление БД
    void createDatabase(const std::string& dbName);
    void dropDatabase(const std::string& dbName);
    bool databaseExists(const std::string& dbName) const;
    std::vector<std::string> listDatabases() const;
    Database* getDatabase(const std::string& dbName);
    
    // Управление таблицами
    void createTable(const std::string& dbName, 
                     const std::string& tableName, 
                     const std::vector<Column>& columns);
    void dropTable(const std::string& dbName, const std::string& tableName);
    Table* getTable(const std::string& dbName, const std::string& tableName);
    
    void saveAll();
};
```
### 1.2. Класс Database
```cpp
class Database {
public:
    Database(const std::string& name);
    const std::string& getName() const;
    void createTable(const std::string& name, const std::vector<Column>& columns);
    void dropTable(const std::string& name);
    Table* getTable(const std::string& name);
    bool tableExists(const std::string& name) const;
};
```
### 1.3. Класс Table
```cpp
class Table {
public:
    Table(const std::string& name);
    const std::string& getName() const;
    const std::vector<Column>& getColumns() const;
    void addColumn(const Column& column);
    
    void insertRow(const std::vector<std::string>& values);
    std::vector<std::vector<std::string>> getRows() const;
    void setRows(const std::vector<std::vector<std::string>>& rows);
    size_t getRowCount() const;
    
    void updateRow(size_t index, const std::vector<std::string>& values);
    void deleteRow(size_t index);
};
```
### 1.4. Класс Column
```cpp
class Column {
public:
    Column(const std::string& name, DataType type, int varcharLength = 0);
    const std::string& getName() const;
    DataType getType() const;
    int getVarcharLength() const;
    bool validateValue(const std::string& value) const;
};

enum class DataType {
    INT,      // Целые числа
    FLOAT,    // Числа с плавающей точкой
    BOOL,     // TRUE/FALSE
    TEXT,     // Произвольный текст
    VARCHAR   // Текст с ограничением длины
};
```
### 1.5. Дополнительные функции

| Функция | Описание | SQL Пример |
|---------|----------|------------|
| AUTO_INCREMENT | Автогенерация ID | `CREATE TABLE users (id INT AUTO_INCREMENT, name TEXT)` |
| UNIQUE | Запрет дубликатов | `CREATE TABLE users (email TEXT UNIQUE)` |
| Массивы (TEXT[]) | Массивы строк | `INSERT INTO users VALUES ('["admin","user"]')` |
| Индексы | Ускоренный поиск | (API, не SQL) |
| Полнотекстовый поиск | Поиск с ранжированием | (API, не SQL) |
| Аудит | Журнал изменений | Автоматически в `_audit_log.json` |
| Хранимые процедуры | Сохранение SQL | (API, не SQL) |

## 2. SQL Парсер (Роль 2)
### 2.1. Поддерживаемый синтаксис
### DDL (Data Definition Language):

```sql
CREATE DATABASE database_name;
DROP DATABASE database_name;
USE database_name;
CREATE TABLE table_name (column_definitions);
DROP TABLE table_name;
```
### DML (Data Manipulation Language):

```sql
SELECT * FROM table_name;
SELECT column1, column2 FROM table_name WHERE condition;
INSERT INTO table_name VALUES (value1, value2);
INSERT INTO table_name (col1, col2) VALUES (val1, val2);
UPDATE table_name SET column = value WHERE condition;
DELETE FROM table_name WHERE condition;
```
### Типы данных в CREATE TABLE:

INT / INTEGER

FLOAT / DOUBLE

BOOL / BOOLEAN

TEXT

VARCHAR(n)

TEXT[] — массив строк

Условия WHERE:

=, >, <, >=, <=, <>

AND, OR, NOT

### 2.2. Batch-запросы
Поддерживается выполнение нескольких запросов одной строкой, разделённых ;:

```sql
CREATE DATABASE test; USE test; CREATE TABLE users (id INT); INSERT INTO users VALUES (1); SELECT * FROM users;
```
### 2.3. Основные структуры AST
```cpp
class Value {
public:
    enum Type { INT_TYPE, DOUBLE_TYPE, STRING_TYPE, BOOL_TYPE, ARRAY_TYPE, NULL_TYPE };
    Value();                    // NULL
    Value(int v);
    Value(double v);
    Value(const std::string& v);
    Value(bool v);
    Value(const std::vector<Value>& v);  // массив
};

struct Query {
    enum Type { CREATE_DB, DROP_DB, CREATE_TABLE, DROP_TABLE,
                SELECT, INSERT, UPDATE, DELETE, USE_DB, UNKNOWN };
    Type type;
    std::variant<...> data;
};
```
## 3. Сетевой слой и C API (Роль 3)
### 3.1. Формат сетевого протокола
```text
[4 байта: длина запроса] [запрос]
[4 байта: статус] [4 байта: длина ответа] [ответ]
```
### 3.2. C API (для P/Invoke из C#)
```cpp
extern "C" {
    void* db_connect(const char* host, int port);
    const char* db_execute(void* connection, const char* query);
    void db_disconnect(void* connection);
    void db_free_string(const char* str);
}
```
### 3.3. Формат возвращаемых данных (JSON)
#### Успешный SELECT:

```json
{
  "type": "select",
  "columns": ["id", "name", "age"],
  "rows": [["1", "Alice", "25"], ["2", "Bob", "30"]],
  "affected_rows": 2
}
```
#### INSERT/UPDATE/DELETE:

```json
{
  "type": "modification",
  "affected_rows": 3,
  "message": "3 rows affected"
}
```
#### DDL (CREATE/DROP):

```json
{
  "type": "ddl",
  "success": true,
  "message": "Table users created"
}
```
#### Ошибка:

```json
{
  "type": "error",
  "message": "Database already exists"
}
```
### 3.4. Компоненты сетевого слоя
| Файл | Назначение |
|------|------------|
| `src/network/Server.h/cpp` | TCP сервер, пул потоков |
| `src/network/ClientSession.h/cpp` | Обработка клиентской сессии |
| `src/network/Protocol.h/cpp` | Протокол и JSON сериализация |
| `src/network/ThreadPool.h/cpp` | Пул потоков |
| `src/lib/Connection.cpp` | Клиентский TCP сокет |
| `src/lib/client.cpp` | C-обёртка для P/Invoke |
| `src/cli/main.cpp` | Точка входа сервера |

## 4. Графический интерфейс (WPF, Роль 4)
### 4.1. Компоненты GUI
```text
┌─────────────────────────────────────────────────────────────┐
│                     MainWindow.xaml                         │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Панель подключения (Host, Port, Connect/Disconnect) │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  ComboBox с историей запросов + кнопка Execute       │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  DataGrid для отображения результатов SELECT         │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  StatusBar с информацией о подключении и статусе     │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```
### 4.2. Основные классы
```csharp
public class MainViewModel : INotifyPropertyChanged
{
    // Свойства
    string Host { get; set; }
    string Port { get; set; }
    string CurrentQuery { get; set; }
    bool IsConnected { get; set; }
    ObservableCollection<string> QueryHistory { get; set; }
    ObservableCollection<DataRowView> QueryResults { get; set; }
    
    // Команды
    RelayCommand ConnectCommand { get; }
    RelayCommand ExecuteQueryCommand { get; }
}

public class DatabaseService
{
    Task<bool> Connect(string host, int port);
    Task<QueryResult> ExecuteQuery(string query);
    Task Disconnect();
}

public class QueryResult
{
    bool IsSuccess { get; set; }
    bool IsSelect { get; set; }
    DataTable? DataTable { get; set; }
    int AffectedRows { get; set; }
    string? ErrorMessage { get; set; }
}
```
### 4.3. Горячие клавиши
Комбинация	Действие
Ctrl+Enter / F5	Выполнить запрос
F6	Очистить поле ввода
Esc	Отключиться от сервера
## 5. Сборка, тестирование и запуск (Роль 5)
### 5.1. Требования
Windows 10/11: Visual Studio 2022, .NET 6.0 SDK, Docker Desktop

Linux: GCC, CMake, Docker

macOS: Clang, CMake, Docker

### 5.2. Сборка проекта
#### Windows:

```batch
scripts\build_full.bat
```
#### Linux/macOS:

```bash
./scripts/build_full.sh
```
### 5.3. Запуск сервера через Docker (рекомендуемый способ)
```bash
# Загрузка образа
docker load -i customdb-server.tar

# Запуск контейнера
docker run -d --name customdb-server -p 5432:5432 databaseoficial-customdb-server

# Проверка
docker ps
```
### 5.4. Запуск GUI
Распаковать папку win-x64

Запустить CustomDB.UI.exe

Нажать Connect (localhost:5432)

### 5.5. Запуск тестов
```bash
# Все тесты (Windows)
run_all_tests.bat

# Все тесты (Linux/macOS)
./run_all_tests.sh
Тип тестов	Файл	Количество
Unit-тесты	tests/unit/unit_tests_all.cpp	18 ✅
Интеграционные	tests/integration/test_catalog_storage.cpp	3 ✅
Функциональные	functional_test.py	6 ✅
```
### 5.6. Остановка сервера
```bash
docker stop customdb-server
docker rm customdb-server
```
## 6. Примеры использования
### 6.1. Полный сеанс работы
```sql
CREATE DATABASE full_test;
USE full_test;
CREATE TABLE employees (id INT AUTO_INCREMENT, name TEXT, email TEXT UNIQUE, department TEXT);
INSERT INTO employees (name, email, department) VALUES ('Alice', 'alice@mail.com', '["IT"]');
INSERT INTO employees (name, email, department) VALUES ('Bob', 'bob@mail.com', '["HR"]');
INSERT INTO employees (name, email, department) VALUES ('Charlie', 'charlie@mail.com', '["IT"]');
SELECT * FROM employees;
SELECT name, department FROM employees WHERE id = 2;
UPDATE employees SET department = '["IT","Management"]' WHERE name = 'Bob';
DELETE FROM employees WHERE name = 'Charlie';
SELECT * FROM employees;
DROP TABLE employees;
DROP DATABASE full_test;
```
### 6.2. Работа с массивами
```sql
CREATE TABLE products (id INT, name TEXT, tags TEXT[]);
INSERT INTO products VALUES (1, 'Laptop', '["electronics","computers"]');
INSERT INTO products VALUES (2, 'Book', '["education","books"]');
SELECT * FROM products;
```
### 6.3. Batch-запрос одной строкой
```sql
CREATE DATABASE demo; USE demo; CREATE TABLE users (id INT, name TEXT); INSERT INTO users VALUES (1, 'Alice'); INSERT INTO users VALUES (2, 'Bob'); SELECT * FROM users;
```
### 6.4. C++ клиент
```cpp
#include "customdb/client.h"

int main() {
    customdb::Client client;
    client.connect("localhost", 5432);
    client.execute("CREATE DATABASE testdb;");
    client.execute("CREATE TABLE users (id INT, name TEXT);");
    client.execute("INSERT INTO users VALUES (1, 'Alice'), (2, 'Bob');");
    std::string result = client.execute("SELECT * FROM users;");
    std::cout << result << std::endl;
    client.disconnect();
    return 0;
}
```
### 6.5. C# клиент (через P/Invoke)
```csharp
IntPtr conn = db_connect("localhost", 5432);
string result = Marshal.PtrToStringAnsi(db_execute(conn, "SELECT * FROM users;"));
db_disconnect(conn);
db_free_string(conn);
```
## 7. Возможные проблемы и решения

| Проблема | Решение |
|----------|---------|
| `docker: command not found` | Установите Docker Desktop |
| `Cannot connect to Docker daemon` | Запустите Docker Desktop |
| Порт 5432 занят | Используйте `-p 5433:5432` |
| GUI показывает `disconnected` | Убедитесь, что контейнер запущен (`docker ps`) |
| Ошибка в запросе | Проверьте синтаксис, не забудьте `;` |
| `customdb.dll` не найдена | Скопируйте DLL в папку с `CustomDB.UI.exe` |

## 8. Полученные артефакты
| Артефакт | Назначение |
|----------|------------|
| `publish/win-x64/CustomDB.UI.exe` | GUI для Windows (самодостаточный) |
| `build/Release/customdb_server.exe` | Нативный сервер Windows |
| `databaseoficial-customdb-server:latest` | Docker-образ сервера |
| `publish/linux-x64/customdb_server` | Нативный сервер Linux |
| `customdb-server.tar` | Экспортированный Docker-образ |

## Заключение
Проект CustomDB представляет собой полностью рабочую клиент-серверную СУБД с:
Полноценным SQL-парсером
TCP-сервером с пулом потоков
Графическим интерфейсом на WPF
Кроссплатформенной сборкой (Windows/Linux/macOS)
Docker-контейнеризацией
Автоматическими тестами (unit, интеграционные, функциональные)
Статус: ✅ Готов к демонстрации и использованию.
