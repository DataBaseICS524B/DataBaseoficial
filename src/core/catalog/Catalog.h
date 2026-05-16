// src/core/catalog/Catalog.h
#ifndef CATALOG_H
#define CATALOG_H

#include "Database.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace customdb {

class StorageEngine;

// Singleton паттерн для каталога всех баз данных
class Catalog {
public:
    // Получение единственного экземпляра
    static Catalog& getInstance();
    
    // Запрещаем копирование и присваивание
    Catalog(const Catalog&) = delete;
    Catalog& operator=(const Catalog&) = delete;
    
    // Инициализация (загрузка сохранённых данных)
    void initialize(const std::string& dataPath = "data");
    
    // Сохранение всех данных
    void saveAll();
    
    // Работа с базами данных
    void createDatabase(const std::string& dbName);
    void dropDatabase(const std::string& dbName);
    Database* getDatabase(const std::string& dbName);
    const std::unordered_map<std::string, std::unique_ptr<Database>>& getDatabases() const;
    bool databaseExists(const std::string& dbName) const;
    std::vector<std::string> listDatabases() const;
    
    // Текущая активная база данных
    void setCurrentDatabase(const std::string& dbName);
    Database* getCurrentDatabase();
    std::string getCurrentDatabaseName() const;
    
    // Работа с таблицами (с автоматическим сохранением)
    void createTable(const std::string& dbName, const std::string& tableName, const std::vector<Column>& columns);
    void dropTable(const std::string& dbName, const std::string& tableName);
    Table* getTable(const std::string& dbName, const std::string& tableName);
    
    // NEW: Аудит
    void logChange(const std::string& tableName, const std::string& operation,
                   const std::string& oldData, const std::string& newData);
    
    // NEW: Хранимые процедуры
    void createProcedure(const std::string& name, const std::string& sql);
    std::string getProcedure(const std::string& name) const;
    std::vector<std::string> listProcedures() const;

private:
    // Приватный конструктор (Singleton)
    Catalog() : storageEngine_(nullptr), initialized_(false) {}
    
    void loadAllDatabases();
    void saveDatabase(const std::string& dbName);
    void loadProcedures();
    void saveProcedures();
    
private:
    std::unordered_map<std::string, std::unique_ptr<Database>> databases_;
    std::string currentDatabaseName_;
    StorageEngine* storageEngine_;
    bool initialized_;
    
    // NEW: хранилище процедур
    std::unordered_map<std::string, std::string> procedures_;
};

} // namespace customdb

#endif // CATALOG_H