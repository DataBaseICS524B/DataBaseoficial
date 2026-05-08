// src/core/catalog/Table.h
#ifndef TABLE_H
#define TABLE_H

#include "Column.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace customdb {

class Table {
public:
    Table(const std::string& name);
    
    // Добавление колонки
    void addColumn(const Column& column);
    
    // Getters
    const std::string& getName() const;
    const std::vector<Column>& getColumns() const;
    const Column* getColumn(const std::string& name) const;
    int getColumnIndex(const std::string& name) const;
    
    // Работа с данными
    void insertRow(const std::vector<std::string>& values);
    std::vector<std::vector<std::string>> getRows() const;
    void setRows(const std::vector<std::vector<std::string>>& rows);
    void clearRows();
    
    // Получение количества строк
    size_t getRowCount() const;
    
private:
    std::string name_;
    std::vector<Column> columns_;
    std::unordered_map<std::string, int> columnIndexMap_;
    std::vector<std::vector<std::string>> rows_;
};

} // namespace customdb

#endif