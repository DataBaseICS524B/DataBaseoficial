# Документация парсера SQL-подобных запросов

**Автор:** Роль 2 (Бутов Дмитрий)
**Статус:** ✅ Готов к интеграции
**Дата:** 2026-05-14

---

## 1. Обзор

Парсер преобразует SQL-подобные запросы в **AST (Abstract Syntax Tree)** — структурированное представление, которое могут использовать другие компоненты (Executor, сетевой слой).

**Файлы парсера:**
src/core/query/
├── ast/ASTNodes.h # Структуры данных (Value, Column, Query и др.)
├── parser/
│ ├── Lexer.h / .cpp # Токенизация
│ ├── Parser.h / .cpp # Синтаксический анализ
└── tests/test_parser.cpp # Пример использования

text

---

## 2. Основной API

### 2.1 Класс `Parser`

```cpp
#include "parser/Parser.h"

Parser parser(const std::string& sql);
std::unique_ptr<Query> parse();
std::string getLastError() const;
Пример использования:

cpp
Parser parser("SELECT * FROM users WHERE age > 18;");
auto query = parser.parse();
if (query) {
    // Работаем с AST
} else {
    std::cerr << parser.getLastError() << std::endl;
}
3. Структуры AST (из ASTNodes.h)
3.1 Value — универсальный тип данных
cpp
class Value {
public:
    enum Type { INT_TYPE, DOUBLE_TYPE, STRING_TYPE, BOOL_TYPE, NULL_TYPE };

    Value();                    // NULL
    Value(int v);
    Value(double v);
    Value(const std::string& v);
    Value(bool v);

    Type getType() const;
    int asInt() const;
    double asDouble() const;
    std::string asString() const;
    bool asBool() const;
    bool isNull() const;
};
3.2 Column — определение колонки
cpp
struct Column {
    std::string name;
    DataType type;      // INT, DOUBLE, STRING, BOOL, UNKNOWN
    bool nullable = true;
    bool primaryKey = false;
};
3.3 Query — корневой узел AST
cpp
struct Query {
    enum Type {
        CREATE_DB, DROP_DB,     // DDL для БД
        CREATE_TABLE, DROP_TABLE, // DDL для таблиц
        SELECT, INSERT, UPDATE, DELETE, // DML
        UNKNOWN
    };
    Type type;

    // Хранит один из типов запросов (см. ниже)
    std::variant<CreateDatabaseQuery, DropDatabaseQuery,
                 CreateTableQuery, DropTableQuery,
                 SelectQuery, InsertQuery, UpdateQuery, DeleteQuery> data;
};
Как получить конкретный тип запроса:

cpp
if (query->type == Query::SELECT) {
    SelectQuery& select = std::get<SelectQuery>(query->data);
    // работаем с select
}
4. DDL запросы
4.1 CreateDatabaseQuery
cpp
struct CreateDatabaseQuery {
    std::string databaseName;
    bool ifNotExists = false;  // true для "IF NOT EXISTS"
};
Пример запроса: CREATE DATABASE mydb; → databaseName = "mydb", ifNotExists = false

4.2 DropDatabaseQuery
cpp
struct DropDatabaseQuery {
    std::string databaseName;
    bool ifExists = false;  // true для "IF EXISTS"
};
4.3 CreateTableQuery
cpp
struct CreateTableQuery {
    std::string databaseName;   // может быть пустым
    std::string tableName;
    std::vector<Column> columns;
    bool ifNotExists = false;
};
Поддерживаемые синтаксисы:

CREATE TABLE users (id INT, name STRING); → databaseName = ""

CREATE TABLE mydb.users (id INT, name STRING); → databaseName = "mydb"

4.4 DropTableQuery
cpp
struct DropTableQuery {
    std::string databaseName;   // может быть пустым
    std::string tableName;
    bool ifExists = false;
};
5. DML запросы
5.1 SelectQuery
cpp
struct SelectQuery {
    std::vector<std::string> columns;     // пустой вектор = SELECT *
    std::string databaseName;             // может быть пустым
    std::string tableName;
    std::unique_ptr<Condition> where;     // nullptr если нет WHERE
};
5.2 InsertQuery
cpp
struct InsertQuery {
    std::string databaseName;   // может быть пустым
    std::string tableName;
    std::vector<std::string> columns;           // пустой = все колонки
    std::vector<std::vector<Value>> values;     // поддержка множественной вставки
};
5.3 UpdateQuery
cpp
struct UpdateQuery {
    std::string databaseName;   // может быть пустым
    std::string tableName;
    std::map<std::string, Value> setValues;   // колонка → новое значение
    std::unique_ptr<Condition> where;         // nullptr если нет WHERE
};
5.4 DeleteQuery
cpp
struct DeleteQuery {
    std::string databaseName;   // может быть пустым
    std::string tableName;
    std::unique_ptr<Condition> where;   // nullptr если нет WHERE
};
5.5 Condition — условие WHERE
cpp
struct Condition {
    std::string column;
    std::string op;    // "=", ">", "<", ">=", "<=", "<>"
    Value value;
};
Поддерживаемые операторы:

Оператор	Значение
=	Равно
>	Больше
<	Меньше
>=	Больше или равно
<=	Меньше или равно
<>	Не равно
6. Поддерживаемый синтаксис
DDL (Data Definition Language)
sql
CREATE DATABASE database_name;
CREATE DATABASE IF NOT EXISTS database_name;
DROP DATABASE database_name;
DROP DATABASE IF EXISTS database_name;

CREATE TABLE table_name (column_definitions);
CREATE TABLE database_name.table_name (column_definitions);
CREATE TABLE IF NOT EXISTS table_name (column_definitions);

DROP TABLE table_name;
DROP TABLE database_name.table_name;
DROP TABLE IF EXISTS table_name;
DML (Data Manipulation Language)
sql
SELECT * FROM table_name;
SELECT column1, column2 FROM table_name;
SELECT * FROM table_name WHERE column = value;
SELECT * FROM database_name.table_name WHERE column > value;

INSERT INTO table_name VALUES (value1, value2);
INSERT INTO table_name (col1, col2) VALUES (val1, val2);
INSERT INTO table_name VALUES (val1, val2), (val3, val4);

UPDATE table_name SET column = value WHERE column = value;
UPDATE database_name.table_name SET col1 = val1, col2 = val2 WHERE col > val;

DELETE FROM table_name;
DELETE FROM table_name WHERE column = value;
Типы данных в CREATE TABLE
sql
INT / INTEGER
DOUBLE / FLOAT / REAL
STRING / VARCHAR(n) / TEXT
BOOL / BOOLEAN
Ограничения (парсятся, но не проверяются)
sql
NOT NULL
PRIMARY KEY
7. Интеграция с другими ролями
Для Роли 1 (Архитектор ядра)
Что нужно предоставить парсеру:

Интерфейсы Catalog и Table для выполнения запросов

Что парсер предоставляет:

Parser::parse() → std::unique_ptr<Query> с заполненной структурой

Для Роли 3 (C++/C# мост)
Пример вызова из сетевого слоя:

cpp
std::string handleQuery(const std::string& sql) {
    Parser parser(sql);
    auto query = parser.parse();

    if (!query) {
        return "{\"error\": \"" + parser.getLastError() + "\"}";
    }

    // Передать query в Executor (Роль 1 + вы)
    auto result = executor->execute(std::move(query));

    // result уже в JSON
    return result;
}
Для Роли 5 (Тестирование)
Тесты, которые можно написать:

cpp
#include "query/parser/Parser.h"

TEST(ParserTest, CreateDatabaseWithIfNotExists) {
    Parser parser("CREATE DATABASE IF NOT EXISTS testdb;");
    auto query = parser.parse();
    ASSERT_EQ(query->type, Query::CREATE_DB);
    auto& db = std::get<CreateDatabaseQuery>(query->data);
    EXPECT_EQ(db.databaseName, "testdb");
    EXPECT_TRUE(db.ifNotExists);
}
8. Обработка ошибок
Парсер возвращает nullptr при ошибке. Сообщение об ошибке можно получить через getLastError():

cpp
Parser parser("SELEC * FROM users;");  // опечатка
auto query = parser.parse();
if (!query) {
    std::cout << parser.getLastError() << std::endl;
    // Вывод: "Unknown query type at line 1, column 1"
}
9. Ограничения текущей версии
Ограничение	Статус
WHERE только с одним условием	Будет исправлено в Днях 3-4
Нет подзапросов	Не требуется по ТЗ
Нет JOIN	Не требуется по ТЗ
Нет ORDER BY/GROUP BY	Не требуется по ТЗ
10. Файлы для включения в сборку
Для CMakeLists.txt (Роль 1):

cmake
include_directories(src/core)

set(PARSER_SOURCES
    src/core/query/parser/Lexer.cpp
    src/core/query/parser/Parser.cpp
)
Для тестов (Роль 5):

cpp
#include "query/parser/Parser.h"
#include "query/ast/ASTNodes.h"
