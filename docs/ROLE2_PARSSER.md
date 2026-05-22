# Документация парсера SQL-подобных запросов

**Автор:** Роль 2 (Бутов Дмитрий)
**Статус:** ✅ Готов к интеграции
**Дата:** 2026-05-22

# Обзор
Парсер преобразует SQL-подобные запросы в AST и поддерживает batch-режим (несколько запросов одной строкой через ;). Executor выполняет запросы через API, предоставленный Ролью 1.

## Файлы парсера:
```bash
src/core/query/
├── ast/ASTNodes.h          # Структуры данных
├── parser/
│   ├── Lexer.h / .cpp      # Токенизация
│   ├── Parser.h / .cpp     # Синтаксический анализ
└── tests/test_parser.cpp   # Тесты
```
## Поддерживаемый синтаксис
### DDL (Data Definition Language)
```sql
CREATE DATABASE database_name;
DROP DATABASE database_name;
USE database_name;
CREATE TABLE table_name (column_definitions);
DROP TABLE table_name;
```
### DML (Data Manipulation Language)
```sql
SELECT * FROM table_name;
SELECT column1, column2 FROM table_name WHERE condition;
INSERT INTO table_name VALUES (value1, value2);
INSERT INTO table_name (col1, col2) VALUES (val1, val2);
UPDATE table_name SET column = value WHERE condition;
DELETE FROM table_name WHERE condition;
```
### Типы данных в CREATE TABLE
```sql
INT / INTEGER
FLOAT / DOUBLE
BOOL / BOOLEAN
TEXT
VARCHAR(n)
TEXT[]    -- массив строк
```
### Условия WHERE
```sql
WHERE column = value
WHERE column > value
WHERE column < value
WHERE column >= value
WHERE column <= value
WHERE column <> value
WHERE condition1 AND condition2
WHERE condition1 OR condition2
WHERE NOT condition
```
## Batch-запросы (ключевая особенность)
Поддерживается выполнение нескольких запросов одной строкой, разделённых ;:

```sql
CREATE DATABASE test; USE test; CREATE TABLE users (id INT); INSERT INTO users VALUES (1); SELECT * FROM users;
```
## Основные структуры AST
### Value
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
    // ... геттеры
};
```
### Query
```cpp
struct Query {
    enum Type { CREATE_DB, DROP_DB, CREATE_TABLE, DROP_TABLE,
                SELECT, INSERT, UPDATE, DELETE, USE_DB, UNKNOWN };
    Type type;
    std::variant<...> data;  // одна из структур запроса
};
```
## Интеграция с Executor
```cpp
// В ClientSession.cpp или Executor
std::string processQuery(const std::string& sql) {
    Parser parser(sql);
    auto queries = parser.parseBatch();  // поддержка batch
    
    for (auto& query : queries) {
        switch (query->type) {
            case Query::CREATE_DB: {
                auto& q = std::get<CreateDatabaseQuery>(query->data);
                catalog.createDatabase(q.databaseName);
                break;
            }
            case Query::USE_DB: {
                auto& q = std::get<UseDatabaseQuery>(query->data);
                catalog.setCurrentDatabase(q.databaseName);
                break;
            }
            // ... остальные типы
        }
    }
    return jsonResult;
}
```
## Примеры запросов
### Пример 1: AUTO_INCREMENT и UNIQUE
```sql
CREATE DATABASE company;
USE company;
CREATE TABLE employees (id INT AUTO_INCREMENT, name TEXT, email TEXT UNIQUE);
INSERT INTO employees (name, email) VALUES ('Alice', 'alice@company.com');
INSERT INTO employees (name, email) VALUES ('Bob', 'bob@company.com');
SELECT * FROM employees;
```
### Пример 2: Массивы (TEXT[])
```sql
CREATE DATABASE shop;
USE shop;
CREATE TABLE products (id INT, name TEXT, tags TEXT[]);
INSERT INTO products VALUES (1, 'Laptop', '["electronics","computers"]');
INSERT INTO products VALUES (2, 'Book', '["education","books"]');
SELECT * FROM products;
```
### Пример 3: Batch-запрос одной строкой
```sql
CREATE DATABASE demo; USE demo; CREATE TABLE users (id INT, name TEXT); INSERT INTO users VALUES (1, 'Alice'); INSERT INTO users VALUES (2, 'Bob'); SELECT * FROM users;
```
## Обработка ошибок
```cpp
Parser parser(sql);
auto query = parser.parse();
if (!query) {
    std::cout << parser.getLastError() << std::endl;
    // "Unknown query type at line 1, column 1"
}
```