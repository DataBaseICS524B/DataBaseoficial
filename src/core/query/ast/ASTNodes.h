#ifndef AST_NODES_H
#define AST_NODES_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <variant>

enum class DataType { INT, DOUBLE, STRING, BOOL, ARRAY, UNKNOWN };

class Value {
public:
    enum Type { INT_TYPE, DOUBLE_TYPE, STRING_TYPE, BOOL_TYPE, ARRAY_TYPE, NULL_TYPE };
private:
    Type type;
    std::variant<int, double, std::string, bool, std::vector<Value>> data;
public:
    Value() : type(NULL_TYPE) {}
    Value(int v) : type(INT_TYPE), data(v) {}
    Value(double v) : type(DOUBLE_TYPE), data(v) {}
    Value(const std::string& v) : type(STRING_TYPE), data(v) {}
    Value(bool v) : type(BOOL_TYPE), data(v) {}
    Value(const std::vector<Value>& v) : type(ARRAY_TYPE), data(v) {}
    Type getType() const { return type; }
    int asInt() const { return std::get<int>(data); }
    double asDouble() const { return std::get<double>(data); }
    std::string asString() const { return std::get<std::string>(data); }
    bool asBool() const { return std::get<bool>(data); }
    std::vector<Value> asArray() const { return std::get<std::vector<Value>>(data); }
    bool isNull() const { return type == NULL_TYPE; }
    bool isArray() const { return type == ARRAY_TYPE; }
};

// Forward declarations
struct Condition;
struct SelectQuery;
struct UpdateQuery;
struct DeleteQuery;

struct Column { 
    std::string name; 
    DataType type; 
    bool nullable = true; 
    bool primaryKey = false; 
};

struct CreateDatabaseQuery { 
    std::string databaseName; 
    bool ifNotExists = false; 
};

struct DropDatabaseQuery { 
    std::string databaseName; 
    bool ifExists = false; 
};

struct CreateTableQuery { 
    std::string databaseName; 
    std::string tableName; 
    std::vector<Column> columns; 
    bool ifNotExists = false; 
};

struct DropTableQuery { 
    std::string databaseName; 
    std::string tableName; 
    bool ifExists = false; 
};

struct InsertQuery { 
    std::string databaseName; 
    std::string tableName; 
    std::vector<std::string> columns; 
    std::vector<std::vector<Value>> values; 
};

struct Condition { 
    std::string column; 
    std::string op; 
    Value value; 
};

struct SelectQuery { 
    std::vector<std::string> columns; 
    std::string databaseName; 
    std::string tableName; 
    std::unique_ptr<Condition> where; 
};

struct UpdateQuery { 
    std::string databaseName; 
    std::string tableName; 
    std::map<std::string, Value> setValues; 
    std::unique_ptr<Condition> where; 
};

struct DeleteQuery { 
    std::string databaseName; 
    std::string tableName; 
    std::unique_ptr<Condition> where; 
};

// НОВАЯ СТРУКТУРА ДЛЯ USE
struct UseDatabaseQuery {
    std::string databaseName;
};

struct Query {
    enum Type { 
        CREATE_DB, DROP_DB, CREATE_TABLE, DROP_TABLE, 
        SELECT, INSERT, UPDATE, DELETE, USE_DB, UNKNOWN  // ДОБАВЛЕН USE_DB
    };
    Type type;
    std::variant<CreateDatabaseQuery, DropDatabaseQuery, CreateTableQuery, 
                 DropTableQuery, SelectQuery, InsertQuery, UpdateQuery, 
                 DeleteQuery, UseDatabaseQuery> data;  // ДОБАВЛЕН UseDatabaseQuery
    
    Query() : type(UNKNOWN) {}
};

#endif