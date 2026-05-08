// src/core/catalog/Catalog.cpp
#include "Catalog.h"
#include "../storage/StorageEngine.h"
#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace customdb {

Catalog& Catalog::getInstance() {
    static Catalog instance;
    return instance;
}

void Catalog::initialize(const std::string& dataPath) {
    if (initialized_) {
        return;
    }
    
    storageEngine_ = &StorageEngine::getInstance();
    storageEngine_->initialize(dataPath);
    
    loadAllDatabases();
    initialized_ = true;
}

void Catalog::loadAllDatabases() {
    std::vector<std::string> dbNames = storageEngine_->listDatabases();
    
    for (const auto& dbName : dbNames) {
        auto database = storageEngine_->loadDatabase(dbName);
        if (database) {
            databases_[dbName] = std::move(database);
            std::cout << "[Catalog] Loaded database: " << dbName << std::endl;
        }
    }
}

void Catalog::saveAll() {
    for (const auto& [dbName, database] : databases_) {
        saveDatabase(dbName);
    }
}

void Catalog::saveDatabase(const std::string& dbName) {
    auto it = databases_.find(dbName);
    if (it != databases_.end()) {
        storageEngine_->saveDatabase(*it->second);
        
        // Сохраняем каждую таблицу
        for (const auto& [tableName, table] : it->second->getTables()) {
            storageEngine_->saveTable(*table, dbName);
        }
    }
}

void Catalog::createDatabase(const std::string& dbName) {
    if (databaseExists(dbName)) {
        throw std::runtime_error("Database " + dbName + " already exists");
    }
    
    databases_[dbName] = std::make_unique<Database>(dbName);
    storageEngine_->saveDatabase(*databases_[dbName]);
    std::cout << "[Catalog] Created database: " << dbName << std::endl;
}

void Catalog::dropDatabase(const std::string& dbName) {
    auto it = databases_.find(dbName);
    if (it == databases_.end()) {
        throw std::runtime_error("Database " + dbName + " does not exist");
    }
    
    // Удаляем из storage
    storageEngine_->deleteDatabase(dbName);
    
    // Если удаляем текущую БД, сбрасываем currentDatabase
    if (currentDatabaseName_ == dbName) {
        currentDatabaseName_.clear();
    }
    
    databases_.erase(it);
    std::cout << "[Catalog] Dropped database: " << dbName << std::endl;
}

Database* Catalog::getDatabase(const std::string& dbName) {
    auto it = databases_.find(dbName);
    if (it != databases_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const std::unordered_map<std::string, std::unique_ptr<Database>>& Catalog::getDatabases() const {
    return databases_;
}

bool Catalog::databaseExists(const std::string& dbName) const {
    return databases_.find(dbName) != databases_.end();
}

std::vector<std::string> Catalog::listDatabases() const {
    std::vector<std::string> result;
    for (const auto& [name, _] : databases_) {
        result.push_back(name);
    }
    return result;
}

void Catalog::setCurrentDatabase(const std::string& dbName) {
    if (!databaseExists(dbName)) {
        throw std::runtime_error("Database " + dbName + " does not exist");
    }
    currentDatabaseName_ = dbName;
}

Database* Catalog::getCurrentDatabase() {
    if (currentDatabaseName_.empty()) {
        return nullptr;
    }
    return getDatabase(currentDatabaseName_);
}

std::string Catalog::getCurrentDatabaseName() const {
    return currentDatabaseName_;
}

// ОДНА реализация createTable (удали дубликат)
void Catalog::createTable(const std::string& dbName, const std::string& tableName, const std::vector<Column>& columns) {
    Database* db = getDatabase(dbName);
    if (!db) {
        throw std::runtime_error("Database " + dbName + " does not exist");
    }
    
    db->createTable(tableName, columns);
    Table* table = db->getTable(tableName);
    if (table && storageEngine_) {
        storageEngine_->saveTable(*table, dbName);
    }
    
    std::cout << "[Catalog] Created table: " << tableName << " in database: " << dbName << std::endl;
}

// ОДНА реализация dropTable (удали дубликат)
void Catalog::dropTable(const std::string& dbName, const std::string& tableName) {
    Database* db = getDatabase(dbName);
    if (!db) {
        throw std::runtime_error("Database " + dbName + " does not exist");
    }
    
    if (storageEngine_) {
        storageEngine_->deleteTable(dbName, tableName);
    }
    db->dropTable(tableName);
    
    std::cout << "[Catalog] Dropped table: " << tableName << " from database: " << dbName << std::endl;
}

// ОДНА реализация getTable (удали дубликат)
Table* Catalog::getTable(const std::string& dbName, const std::string& tableName) {
    Database* db = getDatabase(dbName);
    if (!db) {
        return nullptr;
    }
    return db->getTable(tableName);
}

} // namespace customdb