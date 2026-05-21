#include <gtest/gtest.h>
#include <filesystem>
#include "core/catalog/Catalog.h"
#include "core/storage/StorageEngine.h"

using namespace customdb;

class CatalogStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = "test_integration_data";
        std::filesystem::remove_all(test_dir_);
        Catalog::getInstance().initialize(test_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
        // Если в Catalog есть метод resetInstance() – раскомментируйте:
        // Catalog::resetInstance();
    }

    std::string test_dir_;
};

TEST_F(CatalogStorageTest, CreateDatabaseAndTablePersists) {
    auto& cat = Catalog::getInstance();
    cat.createDatabase("db1");
    cat.setCurrentDatabase("db1");
    std::vector<Column> cols = {Column("id", DataType::INT), Column("name", DataType::TEXT)};
    cat.createTable("db1", "users", cols);
    cat.saveAll();

    // Перезагружаем каталог (симулируем перезапуск)
    Catalog::getInstance().initialize(test_dir_);
    auto& cat2 = Catalog::getInstance();
    EXPECT_TRUE(cat2.databaseExists("db1"));
    Table* table = cat2.getTable("db1", "users");
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(table->getColumns().size(), 2);
}

TEST_F(CatalogStorageTest, InsertedRowsSurviveReload) {
    auto& cat = Catalog::getInstance();
    cat.createDatabase("db2");
    cat.setCurrentDatabase("db2");
    std::vector<Column> cols = {Column("id", DataType::INT), Column("val", DataType::TEXT)};
    cat.createTable("db2", "data", cols);
    Table* table = cat.getTable("db2", "data");
    table->insertRow({"1", "hello"});
    table->insertRow({"2", "world"});
    cat.saveAll();

    // Перезагрузка
    Catalog::getInstance().initialize(test_dir_);
    auto& cat2 = Catalog::getInstance();
    Table* table2 = cat2.getTable("db2", "data");
    ASSERT_NE(table2, nullptr);
    EXPECT_EQ(table2->getRowCount(), 2);
    auto rows = table2->getRows();
    EXPECT_EQ(rows[0][1], "hello");
    EXPECT_EQ(rows[1][1], "world");
}

TEST_F(CatalogStorageTest, DropTableAndDatabase) {
    auto& cat = Catalog::getInstance();
    cat.createDatabase("db3");
    cat.setCurrentDatabase("db3");
    std::vector<Column> cols = {Column("id", DataType::INT)};
    cat.createTable("db3", "temp", cols);
    cat.dropTable("db3", "temp");
    EXPECT_EQ(cat.getTable("db3", "temp"), nullptr);
    cat.dropDatabase("db3");
    EXPECT_FALSE(cat.databaseExists("db3"));
}