// src/core/catalog/Table.h
#ifndef TABLE_H
#define TABLE_H

#include "Column.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace customdb {

class Index; // forward declaration

class Table {
public:
    ~Table();
    Table(const std::string& name);
    
    void addColumn(const Column& column);
    const std::string& getName() const;
    const std::vector<Column>& getColumns() const;
    const Column* getColumn(const std::string& name) const;
    int getColumnIndex(const std::string& name) const;
    
    // Данные
    void insertRow(const std::vector<std::string>& values);
    std::vector<std::vector<std::string>> getRows() const;
    void setRows(const std::vector<std::vector<std::string>>& rows);
    void clearRows();
    size_t getRowCount() const;

    // NEW: Индексы
    void createIndex(const std::string& columnName);
    std::vector<size_t> searchByIndex(const std::string& columnName, const std::string& value) const;

    // NEW: Полнотекстовый поиск
    struct SearchResult {
        size_t rowId;
        int relevance;
    };
    std::vector<SearchResult> fullTextSearch(const std::string& columnName, const std::string& keyword) const;

private:
    std::string name_;
    std::vector<Column> columns_;
    std::unordered_map<std::string, int> columnIndexMap_;
    std::vector<std::vector<std::string>> rows_;
    
    // NEW: индексы
    std::unordered_map<std::string, std::unique_ptr<Index>> indexes_;
};

} // namespace customdb

#endif