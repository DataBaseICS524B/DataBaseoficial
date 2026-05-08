// src/core/catalog/Table.cpp
#include "Table.h"
#include <stdexcept>
#include <algorithm>
#include <sstream>

namespace customdb {

Table::Table(const std::string& name) : name_(name) {}

void Table::addColumn(const Column& column) {
    // Проверяем, нет ли уже колонки с таким именем
    if (columnIndexMap_.find(column.getName()) != columnIndexMap_.end()) {
        throw std::runtime_error("Column " + column.getName() + " already exists");
    }
    
    columns_.push_back(column);
    columnIndexMap_[column.getName()] = columns_.size() - 1;
}

const std::string& Table::getName() const {
    return name_;
}

const std::vector<Column>& Table::getColumns() const {
    return columns_;
}

const Column* Table::getColumn(const std::string& name) const {
    auto it = columnIndexMap_.find(name);
    if (it != columnIndexMap_.end()) {
        return &columns_[it->second];
    }
    return nullptr;
}

int Table::getColumnIndex(const std::string& name) const {
    auto it = columnIndexMap_.find(name);
    if (it != columnIndexMap_.end()) {
        return it->second;
    }
    return -1;
}

void Table::insertRow(const std::vector<std::string>& values) {
    // Проверяем количество значений
    if (values.size() != columns_.size()) {
        throw std::runtime_error("Expected " + std::to_string(columns_.size()) + 
                               " values, got " + std::to_string(values.size()));
    }
    
    // Валидируем каждое значение
    for (size_t i = 0; i < values.size(); ++i) {
        if (!columns_[i].validateValue(values[i])) {
            throw std::runtime_error("Invalid value '" + values[i] + 
                                   "' for column " + columns_[i].getName());
        }
    }
    
    rows_.push_back(values);
}

std::vector<std::vector<std::string>> Table::getRows() const {
    return rows_;
}

void Table::setRows(const std::vector<std::vector<std::string>>& rows) {
    rows_ = rows;
}

void Table::clearRows() {
    rows_.clear();
}

size_t Table::getRowCount() const {
    return rows_.size();
}

} // namespace customdb