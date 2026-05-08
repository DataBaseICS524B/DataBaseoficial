// src/core/catalog/Column.h
#ifndef COLUMN_H
#define COLUMN_H

#include <string>
#include <variant>
#include <memory>

namespace customdb {

// Поддерживаемые типы данных
enum class DataType {
    INT,
    FLOAT,
    BOOL,
    TEXT,
    VARCHAR
};

class Column {
public:
    Column(const std::string& name, DataType type, int varcharLength = 0);
    
    // Getters
    const std::string& getName() const;
    DataType getType() const;
    int getVarcharLength() const;
    
    // Валидация значения
    bool validateValue(const std::string& value) const;
    
    // Преобразование строки в соответствующий тип
    std::string toString() const;
    
private:
    std::string name_;
    DataType type_;
    int varcharLength_; // только для VARCHAR
};

} // namespace customdb

#endif