// tests/test_catalog.cpp
#include <iostream>
#include <string>
#include <vector>
#include "core/catalog/Catalog.h"
#include "core/storage/StorageEngine.h"

using namespace customdb;

void printHeader(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << "========================================" << std::endl;
}

void testCreateDatabase() {
    printHeader("Test 1: CREATE DATABASE");
    
    auto& catalog = Catalog::getInstance();
    
    try {
        catalog.createDatabase("test_db");
        std::cout << "✓ CREATE DATABASE test_db - SUCCESS" << std::endl;
        
        if (catalog.databaseExists("test_db")) {
            std::cout << "✓ Database test_db exists - SUCCESS" << std::endl;
        }
        
        auto databases = catalog.listDatabases();
        std::cout << "✓ Databases count: " << databases.size() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "✗ FAILED: " << e.what() << std::endl;
    }
}

void testCreateTable() {
    printHeader("Test 2: CREATE TABLE");
    
    auto& catalog = Catalog::getInstance();
    
    try {
        std::vector<Column> columns = {
            Column("id", DataType::INT),
            Column("name", DataType::VARCHAR, 100),
            Column("age", DataType::INT),
            Column("active", DataType::BOOL)
        };
        
        catalog.createTable("test_db", "users", columns);
        std::cout << "✓ CREATE TABLE users - SUCCESS" << std::endl;
        
        auto* table = catalog.getTable("test_db", "users");
        if (table) {
            std::cout << "✓ Table users found with " << table->getColumns().size() << " columns" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "✗ FAILED: " << e.what() << std::endl;
    }
}

void testDropTable() {
    printHeader("Test 3: DROP TABLE");
    
    auto& catalog = Catalog::getInstance();
    
    try {
        catalog.dropTable("test_db", "users");
        std::cout << "✓ DROP TABLE users - SUCCESS" << std::endl;
        
        auto* table = catalog.getTable("test_db", "users");
        if (!table) {
            std::cout << "✓ Table users no longer exists - SUCCESS" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "✗ FAILED: " << e.what() << std::endl;
    }
}

void testDropDatabase() {
    printHeader("Test 4: DROP DATABASE");
    
    auto& catalog = Catalog::getInstance();
    
    try {
        catalog.dropDatabase("test_db");
        std::cout << "✓ DROP DATABASE test_db - SUCCESS" << std::endl;
        
        if (!catalog.databaseExists("test_db")) {
            std::cout << "✓ Database test_db no longer exists - SUCCESS" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "✗ FAILED: " << e.what() << std::endl;
    }
}

void testPersistence() {
    printHeader("Test 5: Persistence (Save/Load)");
    
    std::string testDataPath = "test_data";
    
    // Сохраняем данные
    {
        auto& catalog = Catalog::getInstance();
        catalog.initialize(testDataPath);
        
        catalog.createDatabase("persist_db");
        
        std::vector<Column> columns = {
            Column("id", DataType::INT),
            Column("name", DataType::TEXT)
        };
        catalog.createTable("persist_db", "test_table", columns);
        
        catalog.saveAll();
        std::cout << "✓ Data saved to disk" << std::endl;
    }
    
    // Перезагружаем данные
    {
        auto& catalog = Catalog::getInstance();
        catalog.initialize(testDataPath);  // Переинициализация
        
        if (catalog.databaseExists("persist_db")) {
            std::cout << "✓ Database persist_db loaded - SUCCESS" << std::endl;
        }
        
        auto* table = catalog.getTable("persist_db", "test_table");
        if (table) {
            std::cout << "✓ Table test_table loaded with " << table->getColumns().size() << " columns - SUCCESS" << std::endl;
        }
        
        // Очищаем
        catalog.dropDatabase("persist_db");
    }
}

void testInsertAndSelect() {
    printHeader("Test 6: INSERT and SELECT");
    
    auto& catalog = Catalog::getInstance();
    catalog.initialize("test_data");
    
    try {
        // Создаём БД и таблицу
        catalog.createDatabase("data_db");
        
        std::vector<Column> columns = {
            Column("id", DataType::INT),
            Column("name", DataType::VARCHAR, 50),
            Column("score", DataType::FLOAT)
        };
        catalog.createTable("data_db", "scores", columns);
        
        // Получаем таблицу
        auto* table = catalog.getTable("data_db", "scores");
        
        // Вставляем строки
        table->insertRow({"1", "Alice", "95.5"});
        table->insertRow({"2", "Bob", "87.3"});
        table->insertRow({"3", "Charlie", "92.0"});
        
        std::cout << "✓ Inserted 3 rows" << std::endl;
        std::cout << "✓ Total rows: " << table->getRowCount() << std::endl;
        
        // Выводим данные
        std::cout << "\nTable data:" << std::endl;
        std::cout << "-------------" << std::endl;
        for (const auto& row : table->getRows()) {
            for (const auto& value : row) {
                std::cout << value << " | ";
            }
            std::cout << std::endl;
        }
        
        // Сохраняем и очищаем
        catalog.saveAll();
        catalog.dropDatabase("data_db");
        
    } catch (const std::exception& e) {
        std::cerr << "✗ FAILED: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "\n╔══════════════════════════════════════════╗" << std::endl;
    std::cout << "║     CustomDB - Catalog Tests           ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════╝" << std::endl;
    
    // Инициализация
    auto& catalog = Catalog::getInstance();
    catalog.initialize("test_data");
    
    // Запуск тестов
    testCreateDatabase();
    testCreateTable();
    testDropTable();
    testDropDatabase();
    testPersistence();
    testInsertAndSelect();
    
    // Очистка тестовых данных
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Cleaning up..." << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Удаляем тестовые папки
    system("rm -rf test_data");
    
    std::cout << "\n✓ All tests completed!" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    return 0;
}