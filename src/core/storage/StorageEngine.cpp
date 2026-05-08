// src/core/storage/StorageEngine.cpp
#include "StorageEngine.h"
#include "../catalog/Database.h"
#include "../catalog/Table.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>  // Потребуется установить через vcpkg

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace customdb {

StorageEngine& StorageEngine::getInstance() {
    static StorageEngine instance;
    return instance;
}

void StorageEngine::initialize(const std::string& dataPath) {
    dataPath_ = dataPath;
    ensureDirectoryExists(dataPath_);
}

void StorageEngine::ensureDirectoryExists(const std::string& path) {
    if (!fs::exists(path)) {
        fs::create_directories(path);
    }
}

std::string StorageEngine::getDatabasePath(const std::string& dbName) const {
    return dataPath_ + "/" + dbName;
}

std::string StorageEngine::getTablePath(const std::string& dbName, const std::string& tableName) const {
    return getDatabasePath(dbName) + "/" + tableName + ".json";
}

void StorageEngine::saveDatabase(const Database& database) {
    std::string dbPath = getDatabasePath(database.getName());
    ensureDirectoryExists(dbPath);
    
    // Сохраняем метаданные базы данных
    json dbMeta;
    dbMeta["name"] = database.getName();
    dbMeta["created"] = fs::file_time_type::clock::now().time_since_epoch().count();
    
    std::string metaPath = dbPath + "/metadata.json";
    std::ofstream metaFile(metaPath);
    metaFile << dbMeta.dump(4);
    metaFile.close();
}

std::unique_ptr<Database> StorageEngine::loadDatabase(const std::string& dbName) {
    std::string dbPath = getDatabasePath(dbName);
    std::string metaPath = dbPath + "/metadata.json";
    
    if (!fs::exists(metaPath)) {
        return nullptr;
    }
    
    auto database = std::make_unique<Database>(dbName);
    
    // Загружаем все таблицы в этой базе данных
    for (const auto& entry : fs::directory_iterator(dbPath)) {
        if (entry.path().extension() == ".json" && entry.path().filename() != "metadata.json") {
            std::string tableName = entry.path().stem().string();
            auto table = loadTable(dbName, tableName);
            if (table) {
                // Добавляем таблицу в базу данных (нужно добавить метод addTable в Database)
                // Пока просто выводим
                std::cout << "Loaded table: " << tableName << std::endl;
            }
        }
    }
    
    return database;
}

void StorageEngine::deleteDatabase(const std::string& dbName) {
    std::string dbPath = getDatabasePath(dbName);
    if (fs::exists(dbPath)) {
        fs::remove_all(dbPath);
    }
}

void StorageEngine::saveTable(const Table& table, const std::string& dbName) {
    std::string dbPath = getDatabasePath(dbName);
    ensureDirectoryExists(dbPath);
    
    json tableJson;
    tableJson["name"] = table.getName();
    
    // Сохраняем схему таблицы
    json columnsJson = json::array();
    for (const auto& col : table.getColumns()) {
        json colJson;
        colJson["name"] = col.getName();
        colJson["type"] = static_cast<int>(col.getType());
        colJson["varchar_length"] = col.getVarcharLength();
        columnsJson.push_back(colJson);
    }
    tableJson["columns"] = columnsJson;
    
    // Сохраняем данные
    json rowsJson = json::array();
    for (const auto& row : table.getRows()) {
        json rowJson = json::array();
        for (const auto& value : row) {
            rowJson.push_back(value);
        }
        rowsJson.push_back(rowJson);
    }
    tableJson["rows"] = rowsJson;
    
    std::string tablePath = getTablePath(dbName, table.getName());
    std::ofstream file(tablePath);
    file << tableJson.dump(4);
    file.close();
}

std::unique_ptr<Table> StorageEngine::loadTable(const std::string& dbName, const std::string& tableName) {
    std::string tablePath = getTablePath(dbName, tableName);
    
    if (!fs::exists(tablePath)) {
        return nullptr;
    }
    
    std::ifstream file(tablePath);
    json tableJson;
    file >> tableJson;
    file.close();
    
    // Восстанавливаем колонки
    std::vector<Column> columns;
    for (const auto& colJson : tableJson["columns"]) {
        std::string name = colJson["name"];
        DataType type = static_cast<DataType>(colJson["type"]);
        int varcharLen = colJson.value("varchar_length", 0);
        columns.emplace_back(name, type, varcharLen);
    }
    
    auto table = std::make_unique<Table>(tableJson["name"]);
    for (const auto& col : columns) {
        table->addColumn(col);
    }
    
    // Восстанавливаем строки
    std::vector<std::vector<std::string>> rows;
    for (const auto& rowJson : tableJson["rows"]) {
        std::vector<std::string> row;
        for (const auto& value : rowJson) {
            row.push_back(value.get<std::string>());
        }
        rows.push_back(row);
    }
    table->setRows(rows);
    
    return table;
}

void StorageEngine::deleteTable(const std::string& dbName, const std::string& tableName) {
    std::string tablePath = getTablePath(dbName, tableName);
    if (fs::exists(tablePath)) {
        fs::remove(tablePath);
    }
}

std::vector<std::string> StorageEngine::listDatabases() {
    std::vector<std::string> databases;
    
    if (!fs::exists(dataPath_)) {
        return databases;
    }
    
    for (const auto& entry : fs::directory_iterator(dataPath_)) {
        if (entry.is_directory()) {
            std::string metaPath = entry.path().string() + "/metadata.json";
            if (fs::exists(metaPath)) {
                databases.push_back(entry.path().filename().string());
            }
        }
    }
    
    return databases;
}

std::vector<std::string> StorageEngine::listTables(const std::string& dbName) {
    std::vector<std::string> tables;
    std::string dbPath = getDatabasePath(dbName);
    
    if (!fs::exists(dbPath)) {
        return tables;
    }
    
    for (const auto& entry : fs::directory_iterator(dbPath)) {
        if (entry.path().extension() == ".json" && entry.path().filename() != "metadata.json") {
            tables.push_back(entry.path().stem().string());
        }
    }
    
    return tables;
}

} // namespace customdb