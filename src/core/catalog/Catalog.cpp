// src/core/catalog/Catalog.cpp
#include "Catalog.h"
#include "../storage/StorageEngine.h"
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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
    loadProcedures();          // <-- добавлено
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
    
    storageEngine_->deleteDatabase(dbName);
    if (currentDatabaseName_ == dbName) {
        currentDatabaseName_.clear();
    }
    databases_.erase(it);
    std::cout << "[Catalog] Dropped database: " << dbName << std::endl;
}

Database* Catalog::getDatabase(const std::string& dbName) {
    auto it = databases_.find(dbName);
    return it != databases_.end() ? it->second.get() : nullptr;
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

Table* Catalog::getTable(const std::string& dbName, const std::string& tableName) {
    Database* db = getDatabase(dbName);
    return db ? db->getTable(tableName) : nullptr;
}

// ----- NEW: Audit log -----
void Catalog::logChange(const std::string& tableName, const std::string& operation,
                        const std::string& oldData, const std::string& newData) {
    std::string auditPath = storageEngine_->getDataPath() + "/_audit_log.json";
    json audit;
    std::ifstream infile(auditPath);
    if (infile.is_open()) {
        infile >> audit;
        infile.close();
    }
    json entry;
    entry["timestamp"] = std::time(nullptr);
    entry["table"] = tableName;
    entry["operation"] = operation;
    entry["old_data"] = oldData;
    entry["new_data"] = newData;
    audit["logs"].push_back(entry);
    std::ofstream outfile(auditPath);
    outfile << audit.dump(4);
    outfile.close();
}

// ----- NEW: Stored procedures -----
void Catalog::loadProcedures() {
    std::string procPath = storageEngine_->getDataPath() + "/_procedures.json";
    json j;
    std::ifstream infile(procPath);
    if (infile.is_open()) {
        infile >> j;
        infile.close();
        for (auto& [name, sql] : j.items()) {
            procedures_[name] = sql;
        }
    }
}

void Catalog::saveProcedures() {
    std::string procPath = storageEngine_->getDataPath() + "/_procedures.json";
    json j;
    for (const auto& [name, sql] : procedures_) {
        j[name] = sql;
    }
    std::ofstream outfile(procPath);
    outfile << j.dump(4);
    outfile.close();
}

void Catalog::createProcedure(const std::string& name, const std::string& sql) {
    procedures_[name] = sql;
    saveProcedures();
}

std::string Catalog::getProcedure(const std::string& name) const {
    auto it = procedures_.find(name);
    return (it != procedures_.end()) ? it->second : "";
}

std::vector<std::string> Catalog::listProcedures() const {
    std::vector<std::string> res;
    for (const auto& [name, _] : procedures_) {
        res.push_back(name);
    }
    return res;
}

} // namespace customdb