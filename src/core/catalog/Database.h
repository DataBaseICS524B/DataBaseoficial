// src/core/catalog/Database.h
#ifndef DATABASE_H
#define DATABASE_H

#include "Table.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace customdb {

class Database {
public:
    Database(const std::string& name);
    
    // Getters
    const std::string& getName() const;
    
    // Работа с таблицами
    void createTable(const std::string& tableName, const std::vector<Column>& columns);
    void dropTable(const std::string& tableName);
    Table* getTable(const std::string& tableName);
    const std::unordered_map<std::string, std::unique_ptr<Table>>& getTables() const;
    bool tableExists(const std::string& tableName) const;
    void addTable(std::unique_ptr<Table> table);
    
private:
    std::string name_;
    std::unordered_map<std::string, std::unique_ptr<Table>> tables_;
};

} // namespace customdb

#endif