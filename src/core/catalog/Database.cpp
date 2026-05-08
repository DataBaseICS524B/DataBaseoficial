// src/core/catalog/Database.cpp
#include "Database.h"
#include <stdexcept>

namespace customdb {

Database::Database(const std::string& name) : name_(name) {}

const std::string& Database::getName() const {
    return name_;
}

void Database::createTable(const std::string& tableName, const std::vector<Column>& columns) {
    if (tableExists(tableName)) {
        throw std::runtime_error("Table " + tableName + " already exists");
    }
    
    auto table = std::make_unique<Table>(tableName);
    for (const auto& column : columns) {
        table->addColumn(column);
    }
    
    tables_[tableName] = std::move(table);
}

void Database::dropTable(const std::string& tableName) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        throw std::runtime_error("Table " + tableName + " does not exist");
    }
    tables_.erase(it);
}

Table* Database::getTable(const std::string& tableName) {
    auto it = tables_.find(tableName);
    if (it != tables_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const std::unordered_map<std::string, std::unique_ptr<Table>>& Database::getTables() const {
    return tables_;
}

bool Database::tableExists(const std::string& tableName) const {
    return tables_.find(tableName) != tables_.end();
}

void Database::addTable(std::unique_ptr<Table> table) {
    std::string name = table->getName();
    tables_[name] = std::move(table);
}

} // namespace customdb