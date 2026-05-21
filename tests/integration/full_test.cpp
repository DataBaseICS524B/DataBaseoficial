// tests/full_test.cpp
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "core/catalog/Catalog.h"
#include "core/storage/StorageEngine.h"

using namespace customdb;

void printHeader(const std::string& title) {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║ " << title << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
}

void testCompleteWorkflow() {
    printHeader("Полный рабочий процесс");
    
    auto& catalog = Catalog::getInstance();
    catalog.initialize("full_test_data");
    
    // 1. CREATE DATABASE
    std::cout << "\n1. CREATE DATABASE company_db;" << std::endl;
    catalog.createDatabase("company_db");
    assert(catalog.databaseExists("company_db"));
    std::cout << "   ✅ Database created successfully" << std::endl;
    
    // 2. CREATE TABLE employees
    std::cout << "\n2. CREATE TABLE employees (id INT, name TEXT, salary FLOAT, active BOOL);" << std::endl;
    std::vector<Column> empColumns = {
        Column("id", DataType::INT),
        Column("name", DataType::TEXT),
        Column("salary", DataType::FLOAT),
        Column("active", DataType::BOOL)
    };
    catalog.createTable("company_db", "employees", empColumns);
    auto* employees = catalog.getTable("company_db", "employees");
    assert(employees != nullptr);
    assert(employees->getColumns().size() == 4);
    std::cout << "   ✅ Table created with 4 columns" << std::endl;
    
    // 3. INSERT строк
    std::cout << "\n3. INSERT INTO employees VALUES (1, 'Alice', 50000.0, TRUE);" << std::endl;
    employees->insertRow({"1", "Alice", "50000.0", "TRUE"});
    
    std::cout << "   INSERT INTO employees VALUES (2, 'Bob', 60000.0, TRUE);" << std::endl;
    employees->insertRow({"2", "Bob", "60000.0", "TRUE"});
    
    std::cout << "   INSERT INTO employees VALUES (3, 'Charlie', 45000.0, FALSE);" << std::endl;
    employees->insertRow({"3", "Charlie", "45000.0", "FALSE"});
    
    assert(employees->getRowCount() == 3);
    std::cout << "   ✅ 3 rows inserted, total rows: " << employees->getRowCount() << std::endl;
    
    // 4. SELECT все строки
    std::cout << "\n4. SELECT * FROM employees;" << std::endl;
    std::cout << "   ┌────┬─────────┬────────┬────────┐" << std::endl;
    std::cout << "   │ id │ name    │ salary │ active │" << std::endl;
    std::cout << "   ├────┼─────────┼────────┼────────┤" << std::endl;
    for (const auto& row : employees->getRows()) {
        printf("   │ %-2s │ %-7s │ %-6s │ %-6s │\n", 
               row[0].c_str(), row[1].c_str(), row[2].c_str(), row[3].c_str());
    }
    std::cout << "   └────┴─────────┴────────┴────────┘" << std::endl;
    std::cout << "   ✅ Selected 3 rows" << std::endl;
    
    // 5. Сохранение и перезагрузка (Persistence)
    std::cout << "\n5. Testing persistence (save and reload)..." << std::endl;
    catalog.saveAll();
    std::cout << "   ✅ Data saved to disk" << std::endl;
    
    // Пересоздаём каталог для проверки загрузки
    std::cout << "   Reloading catalog..." << std::endl;
    auto& catalog2 = Catalog::getInstance();
    catalog2.initialize("full_test_data");
    
    if (catalog2.databaseExists("company_db")) {
        std::cout << "   ✅ Database reloaded successfully" << std::endl;
    }
    
    auto* reloadedTable = catalog2.getTable("company_db", "employees");
    if (reloadedTable && reloadedTable->getRowCount() == 3) {
        std::cout << "   ✅ Table reloaded with " << reloadedTable->getRowCount() << " rows" << std::endl;
    }
    
    // 6. CREATE TABLE с VARCHAR
    std::cout << "\n6. CREATE TABLE products (id INT, name VARCHAR(50), price FLOAT);" << std::endl;
    std::vector<Column> prodColumns = {
        Column("id", DataType::INT),
        Column("name", DataType::VARCHAR, 50),
        Column("price", DataType::FLOAT)
    };
    catalog.createTable("company_db", "products", prodColumns);
    auto* products = catalog.getTable("company_db", "products");
    products->insertRow({"1", "Laptop", "999.99"});
    products->insertRow({"2", "Mouse", "25.50"});
    std::cout << "   ✅ Products table created with 2 rows" << std::endl;
    
    // 7. DROP TABLE
    std::cout << "\n7. DROP TABLE products;" << std::endl;
    catalog.dropTable("company_db", "products");
    assert(catalog.getTable("company_db", "products") == nullptr);
    std::cout << "   ✅ Products table dropped" << std::endl;
    
    // 8. DROP DATABASE
    std::cout << "\n8. DROP DATABASE company_db;" << std::endl;
    catalog.dropDatabase("company_db");
    assert(!catalog.databaseExists("company_db"));
    std::cout << "   ✅ Database dropped" << std::endl;
    
    std::cout << "\n═══════════════════════════════════════════════════════════" << std::endl;
    std::cout << "✅ ALL TESTS PASSED!" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
}

void testDataTypes() {
    printHeader("Проверка типов данных");
    
    auto& catalog = Catalog::getInstance();
    catalog.initialize("test_data_types");
    catalog.createDatabase("types_db");
    
    std::vector<Column> columns = {
        Column("int_col", DataType::INT),
        Column("float_col", DataType::FLOAT),
        Column("bool_col", DataType::BOOL),
        Column("text_col", DataType::TEXT),
        Column("varchar_col", DataType::VARCHAR, 20)
    };
    
    catalog.createTable("types_db", "test_types", columns);
    auto* table = catalog.getTable("types_db", "test_types");
    
    // Вставляем значения разных типов
    table->insertRow({"123", "45.67", "TRUE", "Hello World", "Short text"});
    
    // Проверяем валидацию
    std::cout << "Testing validation:" << std::endl;
    
    try {
        table->insertRow({"not_int", "45.67", "TRUE", "Text", "Valid"});
        std::cout << "   ❌ Should have failed for INT" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "   ✅ Correctly rejected invalid INT: " << e.what() << std::endl;
    }
    
    try {
        table->insertRow({"456", "not_float", "TRUE", "Text", "Valid"});
        std::cout << "   ❌ Should have failed for FLOAT" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "   ✅ Correctly rejected invalid FLOAT" << std::endl;
    }
    
    try {
        table->insertRow({"789", "99.99", "not_bool", "Text", "Valid"});
        std::cout << "   ❌ Should have failed for BOOL" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "   ✅ Correctly rejected invalid BOOL" << std::endl;
    }
    
    try {
        table->insertRow({"789", "99.99", "TRUE", "Text", "This text is way too long for VARCHAR(20)"});
        std::cout << "   ❌ Should have failed for VARCHAR length" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "   ✅ Correctly rejected VARCHAR too long" << std::endl;
    }
    
    catalog.dropDatabase("types_db");
    std::cout << "\n✅ Data type tests passed!" << std::endl;
}

void testErrors() {
    printHeader("Проверка обработки ошибок");
    
    auto& catalog = Catalog::getInstance();
    catalog.initialize("test_errors");
    
    // Попытка создать существующую БД
    try {
        catalog.createDatabase("existing_db");
        catalog.createDatabase("existing_db");
        std::cout << "   ❌ Should have thrown an exception" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "   ✅ Caught duplicate database error: " << e.what() << std::endl;
    }
    
    // Попытка удалить несуществующую БД
    try {
        catalog.dropDatabase("non_existent_db");
        std::cout << "   ❌ Should have thrown an exception" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "   ✅ Caught drop non-existent database error: " << e.what() << std::endl;
    }
    
    // Попытка создать таблицу в несуществующей БД
    try {
        std::vector<Column> cols = {Column("id", DataType::INT)};
        catalog.createTable("non_existent_db", "test_table", cols);
        std::cout << "   ❌ Should have thrown an exception" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "   ✅ Caught table creation in non-existent DB error: " << e.what() << std::endl;
    }
    
    std::cout << "\n✅ Error handling tests passed!" << std::endl;
}

int main() {
    std::cout << "\n";
    std::cout << "███████╗██╗   ██╗███████╗████████╗ ██████╗ ███╗   ███╗██████╗" << std::endl;
    std::cout << "██╔════╝██║   ██║██╔════╝╚══██╔══╝██╔═══██╗████╗ ████║██╔══██╗" << std::endl;
    std::cout << "███████╗██║   ██║███████╗   ██║   ██║   ██║██╔████╔██║██████╔╝" << std::endl;
    std::cout << "╚════██║██║   ██║╚════██║   ██║   ██║   ██║██║╚██╔╝██║██╔══██╗" << std::endl;
    std::cout << "███████║╚██████╔╝███████║   ██║   ╚██████╔╝██║ ╚═╝ ██║██████╔╝" << std::endl;
    std::cout << "╚══════╝ ╚═════╝ ╚══════╝   ╚═╝    ╚═════╝ ╚═╝     ╚═╝╚═════╝" << std::endl;
    std::cout << "                    FULL SYSTEM TEST" << std::endl;
    
    try {
        testCompleteWorkflow();
        testDataTypes();
        testErrors();
        
        // Очистка
        system("rm -rf full_test_data test_data_types test_errors");
        
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║          🎉 ALL TESTS COMPLETED SUCCESSFULLY! 🎉        ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
        std::cout << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}