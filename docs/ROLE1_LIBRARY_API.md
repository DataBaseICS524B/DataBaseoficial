# Библиотека CustomDB - Документация API

## Обзор

Библиотека `libcustomdb` предоставляет ядро СУБД с поддержкой:
- Метаданных (базы данных, таблицы, колонки)
- Постоянного хранилища (сохранение между запусками)
- Клиент-серверного взаимодействия
- C API для интеграции с другими языками (C#)

## Архитектура

### Паттерны проектирования

1. **Singleton** - `Catalog` (единственный экземпляр каталога БД)
2. **Repository** - `StorageEngine` (инкапсуляция операций с хранилищем)

### Компоненты
```text
┌─────────────────────────────────────────────────────┐
│ Клиентский код │
│ (C++ / C# через P/Invoke) │
└─────────────────────┬───────────────────────────────┘
│
┌─────────────────────▼───────────────────────────────┐
│ C API (extern "C") │
│ db_connect / db_execute / db_disconnect │
└─────────────────────┬───────────────────────────────┘
│
┌─────────────────────▼───────────────────────────────┐
│ DatabaseClient (C++ класс) │
└─────────────────────┬───────────────────────────────┘
│
┌─────────────────────▼───────────────────────────────┐
│ Connection │
│ (TCP сокеты) │
└─────────────────────┬───────────────────────────────┘
│
┌─────────────────────▼───────────────────────────────┐
│ Server │
│ (обработка клиентских запросов) │
└─────────────────────┬───────────────────────────────┘
│
┌─────────────────────▼───────────────────────────────┐
│ Catalog │
│ (Singleton - управление БД/таблицами) │
└─────────────────────┬───────────────────────────────┘
│
┌─────────────────────▼───────────────────────────────┐
│ StorageEngine │
│ (сохранение/загрузка из файлов) │
└─────────────────────────────────────────────────────┘
```

## Классы и методы

### 1. Catalog (Singleton)

```cpp
class Catalog {
public:
    // Получение экземпляра
    static Catalog& getInstance();
    
    // Инициализация
    void initialize(const std::string& dataPath = "data");
    
    // Работа с базами данных
    void createDatabase(const std::string& dbName);
    void dropDatabase(const std::string& dbName);
    bool databaseExists(const std::string& dbName) const;
    std::vector<std::string> listDatabases() const;
    Database* getDatabase(const std::string& dbName);
    
    // Работа с таблицами
    void createTable(const std::string& dbName, 
                     const std::string& tableName, 
                     const std::vector<Column>& columns);
    void dropTable(const std::string& dbName, const std::string& tableName);
    Table* getTable(const std::string& dbName, const std::string& tableName);
    
    // Сохранение
    void saveAll();
};
```

### 2. Database
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
### 3. Table
```cpp
class Table {
public:
    Table(const std::string& name);
    
    const std::string& getName() const;
    const std::vector<Column>& getColumns() const;
    void addColumn(const Column& column);
    
    // Операции с данными
    void insertRow(const std::vector<std::string>& values);
    std::vector<std::vector<std::string>> getRows() const;
    void setRows(const std::vector<std::vector<std::string>>& rows);
    size_t getRowCount() const;
    
    // UPDATE и DELETE
    void updateRow(size_t index, const std::vector<std::string>& values);
    void deleteRow(size_t index);
};
```
### 4. Column
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
### 5. C API (для P/Invoke из C#)
```cpp
extern "C" {
    // Подключение к серверу
    void* db_connect(const char* host, int port);
    
    // Выполнение запроса (возвращает JSON строку)
    const char* db_execute(void* connection, const char* query);
    
    // Отключение
    void db_disconnect(void* connection);
    
    // Освобождение памяти
    void db_free_string(const char* str);
}
```
## Формат возвращаемых данных
### Успешный SELECT
```json
{
  "type": "select",
  "columns": ["id", "name", "age"],
  "rows": [
    ["1", "Alice", "25"],
    ["2", "Bob", "30"]
  ],
  "affected_rows": 2
}
```
### INSERT/UPDATE/DELETE
```json
{
  "type": "modification",
  "affected_rows": 3,
  "message": "3 rows affected"
}
```
### DDL (CREATE/DROP)
```json
{
  "type": "ddl",
  "success": true,
  "message": "Table users created"
}
```
### Ошибка
```json
{
  "type": "error",
  "message": "Database already exists"
}
```
## Примеры использования
### C++ клиент
```cpp
#include "customdb/client.h"

int main() {
    customdb::Client client;
    
    // Подключение
    if (!client.connect("localhost", 5432)) {
        std::cerr << "Connection failed" << std::endl;
        return 1;
    }
    
    // CREATE DATABASE
    std::string result = client.execute("CREATE DATABASE testdb;");
    std::cout << result << std::endl;
    
    // CREATE TABLE
    result = client.execute("CREATE TABLE users (id INT, name TEXT);");
    
    // INSERT
    result = client.execute("INSERT INTO users VALUES (1, 'Alice'), (2, 'Bob');");
    
    // SELECT
    result = client.execute("SELECT * FROM users;");
    std::cout << result << std::endl;
    
    client.disconnect();
    return 0;
}
```
### C# клиент (P/Invoke)
```csharp
using System.Runtime.InteropServices;

public class CustomDBClient {
    [DllImport("libcustomdb")]
    private static extern IntPtr db_connect(string host, int port);
    
    [DllImport("libcustomdb")]
    private static extern IntPtr db_execute(IntPtr conn, string query);
    
    [DllImport("libcustomdb")]
    private static extern void db_disconnect(IntPtr conn);
    
    [DllImport("libcustomdb")]
    private static extern void db_free_string(IntPtr str);
    
    private IntPtr _connection;
    
    public void Connect(string host, int port) {
        _connection = db_connect(host, port);
        if (_connection == IntPtr.Zero)
            throw new Exception("Connection failed");
    }
    
    public string Execute(string query) {
        IntPtr result = db_execute(_connection, query);
        string json = Marshal.PtrToStringAnsi(result);
        db_free_string(result);
        return json;
    }
    
    public void Disconnect() {
        if (_connection != IntPtr.Zero) {
            db_disconnect(_connection);
            _connection = IntPtr.Zero;
        }
    }
}
```
## Сборка
```bash
./scripts/build.sh
Результат:

build/libcustomdb.so (Linux) или build/customdb.dll (Windows)

Тестирование
bash
cd build
./simple_tests      # Базовые тесты
./full_test         # Полные тесты
```