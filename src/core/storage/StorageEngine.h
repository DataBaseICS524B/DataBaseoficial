// src/core/storage/StorageEngine.h
#ifndef STORAGE_ENGINE_H
#define STORAGE_ENGINE_H

#include <string>
#include <vector>
#include <memory>

namespace customdb {

class Database;
class Table;

// Паттерн Repository - для работы с хранилищем
class StorageEngine {
public:
    static StorageEngine& getInstance();
    
    // Запрещаем копирование
    StorageEngine(const StorageEngine&) = delete;
    StorageEngine& operator=(const StorageEngine&) = delete;
    
    // Инициализация (создание папки data/)
    void initialize(const std::string& dataPath = "data");
    
    // Сохранение и загрузка баз данных
    void saveDatabase(const Database& database);
    std::unique_ptr<Database> loadDatabase(const std::string& dbName);
    void deleteDatabase(const std::string& dbName);
    
    // Сохранение и загрузка таблиц
    void saveTable(const Table& table, const std::string& dbName);
    std::unique_ptr<Table> loadTable(const std::string& dbName, const std::string& tableName);
    void deleteTable(const std::string& dbName, const std::string& tableName);
    
    // Список всех баз данных
    std::vector<std::string> listDatabases();
    
    // Список таблиц в базе данных
    std::vector<std::string> listTables(const std::string& dbName);
    
private:
    StorageEngine() = default;
    
    std::string getDatabasePath(const std::string& dbName) const;
    std::string getTablePath(const std::string& dbName, const std::string& tableName) const;
    void ensureDirectoryExists(const std::string& path);
    
    std::string dataPath_;
};

} // namespace customdb

#endif