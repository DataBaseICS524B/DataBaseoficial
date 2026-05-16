// src/core/catalog/Table.cpp
#include "Table.h"
#include "../storage/Index.h"
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <cctype>

namespace customdb {

Table::Table(const std::string& name) : name_(name) {}

void Table::addColumn(const Column& column) {
    if (columnIndexMap_.find(column.getName()) != columnIndexMap_.end())
        throw std::runtime_error("Column " + column.getName() + " already exists");
    columns_.push_back(column);
    columnIndexMap_[column.getName()] = columns_.size() - 1;
}

const std::string& Table::getName() const { return name_; }
const std::vector<Column>& Table::getColumns() const { return columns_; }
const Column* Table::getColumn(const std::string& name) const {
    auto it = columnIndexMap_.find(name);
    return it != columnIndexMap_.end() ? &columns_[it->second] : nullptr;
}
int Table::getColumnIndex(const std::string& name) const {
    auto it = columnIndexMap_.find(name);
    return it != columnIndexMap_.end() ? it->second : -1;
}

// NEW: вставка с AUTO_INCREMENT и UNIQUE
void Table::insertRow(const std::vector<std::string>& values) {
    if (values.size() != columns_.size())
        throw std::runtime_error("Expected " + std::to_string(columns_.size()) + " values, got " + std::to_string(values.size()));

    std::vector<std::string> finalValues = values;

    // AUTO_INCREMENT: если значение не задано или NULL, генерируем max+1
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (columns_[i].isAutoIncrement()) {
            if (i >= finalValues.size() || finalValues[i].empty() || finalValues[i] == "NULL") {
                int maxId = 0;
                for (const auto& row : rows_) {
                    if (i < row.size()) {
                        try { maxId = std::max(maxId, std::stoi(row[i])); } catch(...) {}
                    }
                }
                finalValues[i] = std::to_string(maxId + 1);
            }
        }
    }

    // UNIQUE проверка
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (columns_[i].isUnique() && i < finalValues.size()) {
            for (const auto& row : rows_) {
                if (i < row.size() && row[i] == finalValues[i]) {
                    throw std::runtime_error("Duplicate value for UNIQUE column " + columns_[i].getName());
                }
            }
        }
    }

    // Валидация типов (существующая)
    for (size_t i = 0; i < finalValues.size(); ++i) {
        if (!columns_[i].validateValue(finalValues[i]))
            throw std::runtime_error("Invalid value '" + finalValues[i] + "' for column " + columns_[i].getName());
    }

    rows_.push_back(finalValues);
    size_t newRowId = rows_.size() - 1;

    // Обновляем индексы
    for (auto& [colName, index] : indexes_) {
        int colIdx = getColumnIndex(colName);
        if (colIdx >= 0 && colIdx < (int)finalValues.size())
            index->addValue(finalValues[colIdx], newRowId);
    }
}

// NEW: Индексы
void Table::createIndex(const std::string& columnName) {
    if (getColumnIndex(columnName) == -1)
        throw std::runtime_error("Column " + columnName + " does not exist");
    if (indexes_.find(columnName) != indexes_.end())
        return; // already exists
    auto index = std::make_unique<Index>(columnName);
    int colIdx = getColumnIndex(columnName);
    for (size_t i = 0; i < rows_.size(); ++i) {
        if (colIdx < (int)rows_[i].size())
            index->addValue(rows_[i][colIdx], i);
    }
    indexes_[columnName] = std::move(index);
}

std::vector<size_t> Table::searchByIndex(const std::string& columnName, const std::string& value) const {
    auto it = indexes_.find(columnName);
    if (it == indexes_.end())
        return {}; // no index, можно вернуть пустой или выполнить линейный поиск
    return it->second->find(value);
}

// NEW: Полнотекстовый поиск
std::vector<Table::SearchResult> Table::fullTextSearch(const std::string& columnName, const std::string& keyword) const {
    std::vector<SearchResult> results;
    int colIdx = getColumnIndex(columnName);
    if (colIdx == -1) return results;
    for (size_t i = 0; i < rows_.size(); ++i) {
        if (colIdx >= (int)rows_[i].size()) continue;
        const std::string& cell = rows_[i][colIdx];
        int count = 0;
        size_t pos = 0;
        while ((pos = cell.find(keyword, pos)) != std::string::npos) {
            ++count;
            pos += keyword.length();
        }
        if (count > 0) results.push_back({i, count});
    }
    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) { return a.relevance > b.relevance; });
    return results;
}

// Остальные методы без изменений
std::vector<std::vector<std::string>> Table::getRows() const { return rows_; }
void Table::setRows(const std::vector<std::vector<std::string>>& rows) { rows_ = rows; }
void Table::clearRows() { rows_.clear(); }
size_t Table::getRowCount() const { return rows_.size(); }
Table::~Table() = default;
} // namespace customdb