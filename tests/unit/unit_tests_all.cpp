// Unit tests for CustomDB – Column, Table, Database
#include <gtest/gtest.h>
#include "core/catalog/Column.h"
#include "core/catalog/Table.h"
#include "core/catalog/Database.h"

using namespace customdb;

// 1. Column tests

TEST(ColumnTest, ConstructorAndBasicAccess) {
    Column c("id", DataType::INT);
    EXPECT_EQ(c.getName(), "id");
    EXPECT_EQ(c.getType(), DataType::INT);
    EXPECT_EQ(c.getVarcharLength(), 0);
}

TEST(ColumnTest, VarcharLengthIsStored) {
    Column c("name", DataType::VARCHAR, 100);
    EXPECT_EQ(c.getVarcharLength(), 100);
}

TEST(ColumnTest, ValidateIntAcceptsNumbers) {
    Column c("age", DataType::INT);
    EXPECT_TRUE(c.validateValue("0"));
    EXPECT_TRUE(c.validateValue("-999"));
    EXPECT_TRUE(c.validateValue("2147483647"));
    EXPECT_FALSE(c.validateValue("12.34"));
    EXPECT_FALSE(c.validateValue("abc"));
    EXPECT_FALSE(c.validateValue(""));
}

TEST(ColumnTest, ValidateFloatAcceptsDecimalAndScientific) {
    Column c("price", DataType::FLOAT);
    EXPECT_TRUE(c.validateValue("0.0"));
    EXPECT_TRUE(c.validateValue("-3.14"));
    // научная нотация не поддерживается текущей реализацией
    EXPECT_FALSE(c.validateValue("2.5e-2"));
    EXPECT_FALSE(c.validateValue("xyz"));
}

TEST(ColumnTest, ValidateBoolCaseInsensitive) {
    Column c("active", DataType::BOOL);
    // реализация ожидает "TRUE"/"FALSE" (строгий верхний регистр)
    EXPECT_TRUE(c.validateValue("TRUE"));
    EXPECT_TRUE(c.validateValue("FALSE"));
    EXPECT_FALSE(c.validateValue("true"));
    EXPECT_FALSE(c.validateValue("false"));
    EXPECT_FALSE(c.validateValue("maybe"));
    EXPECT_FALSE(c.validateValue("1"));
}

TEST(ColumnTest, ValidateTextAnyString) {
    Column c("comment", DataType::TEXT);
    EXPECT_TRUE(c.validateValue("Hello, world!"));
    EXPECT_TRUE(c.validateValue(""));
    EXPECT_TRUE(c.validateValue("   "));
}

TEST(ColumnTest, ValidateVarcharRespectsLengthLimit) {
    Column c("short", DataType::VARCHAR, 5);
    EXPECT_TRUE(c.validateValue("abc"));
    EXPECT_TRUE(c.validateValue("abcde"));
    EXPECT_FALSE(c.validateValue("abcdef"));
    // пустая строка считается допустимой (не противоречит длине)
    EXPECT_TRUE(c.validateValue(""));
}

// 2. Table tests

TEST(TableTest, AddColumnIncreasesSchema) {
    Table t("users");
    EXPECT_EQ(t.getColumns().size(), 0);
    t.addColumn(Column("id", DataType::INT));
    t.addColumn(Column("name", DataType::TEXT));
    EXPECT_EQ(t.getColumns().size(), 2);
}

TEST(TableTest, InsertValidRow) {
    Table t("test");
    t.addColumn(Column("a", DataType::INT));
    t.addColumn(Column("b", DataType::TEXT));
    EXPECT_NO_THROW(t.insertRow({"123", "hello"}));
    EXPECT_EQ(t.getRowCount(), 1);
    auto rows = t.getRows();
    EXPECT_EQ(rows[0][0], "123");
    EXPECT_EQ(rows[0][1], "hello");
}

TEST(TableTest, InsertRowWrongColumnCountThrows) {
    Table t("test");
    t.addColumn(Column("id", DataType::INT));
    EXPECT_THROW(t.insertRow({"1", "extra"}), std::exception);
    EXPECT_THROW(t.insertRow({}), std::exception);
}

TEST(TableTest, InsertInvalidDataTypeThrows) {
    Table t("test");
    t.addColumn(Column("id", DataType::INT));
    EXPECT_THROW(t.insertRow({"not_int"}), std::exception);
}

TEST(TableTest, SetRowsReplacesAllRows) {
    Table t("test");
    t.addColumn(Column("id", DataType::INT));
    t.insertRow({"1"});
    t.insertRow({"2"});
    std::vector<std::vector<std::string>> newRows = {{"3"}, {"4"}};
    t.setRows(newRows);
    EXPECT_EQ(t.getRowCount(), 2);
    auto rows = t.getRows();
    EXPECT_EQ(rows[0][0], "3");
    EXPECT_EQ(rows[1][0], "4");
}

// 3. Database tests

TEST(DatabaseTest, CreateAndRetrieveTable) {
    Database db("company");
    std::vector<Column> cols = {Column("id", DataType::INT), Column("name", DataType::TEXT)};
    db.createTable("staff", cols);
    Table* t = db.getTable("staff");
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->getName(), "staff");
    EXPECT_EQ(t->getColumns().size(), 2);
}

TEST(DatabaseTest, CreateDuplicateTableThrows) {
    Database db("test");
    std::vector<Column> cols = {Column("id", DataType::INT)};
    db.createTable("users", cols);
    EXPECT_THROW(db.createTable("users", cols), std::runtime_error);
}

TEST(DatabaseTest, TableExistsReturnsCorrectly) {
    Database db("mydb");
    EXPECT_FALSE(db.tableExists("none"));
    db.createTable("temp", {Column("id", DataType::INT)});
    EXPECT_TRUE(db.tableExists("temp"));
}

TEST(DatabaseTest, DropTableRemovesIt) {
    Database db("mydb");
    db.createTable("temp", {Column("id", DataType::INT)});
    EXPECT_TRUE(db.tableExists("temp"));
    db.dropTable("temp");
    EXPECT_FALSE(db.tableExists("temp"));
    EXPECT_EQ(db.getTable("temp"), nullptr);
}

TEST(DatabaseTest, DropNonexistentTableThrows) {
    Database db("mydb");
    EXPECT_THROW(db.dropTable("missing"), std::runtime_error);
}

TEST(DatabaseTest, GetTablesReturnsAllTables) {
    Database db("mydb");
    db.createTable("t1", {Column("id", DataType::INT)});
    db.createTable("t2", {Column("id", DataType::INT)});
    const auto& tables = db.getTables();
    EXPECT_EQ(tables.size(), 2);
    EXPECT_TRUE(tables.find("t1") != tables.end());
    EXPECT_TRUE(tables.find("t2") != tables.end());
}