// src/core/catalog/Column.h
#ifndef COLUMN_H
#define COLUMN_H

#include <string>
#include <variant>
#include <memory>

namespace customdb {

enum class DataType {
    INT, FLOAT, BOOL, TEXT, VARCHAR
};

class Column {
public:
    Column(const std::string& name, DataType type, int varcharLength = 0);
    
    const std::string& getName() const;
    DataType getType() const;
    int getVarcharLength() const;
    bool validateValue(const std::string& value) const;
    std::string toString() const;

    // NEW: AUTO_INCREMENT и UNIQUE
    void setAutoIncrement(bool enabled) { autoIncrement_ = enabled; }
    bool isAutoIncrement() const { return autoIncrement_; }
    void setUnique(bool enabled) { unique_ = enabled; }
    bool isUnique() const { return unique_; }

private:
    std::string name_;
    DataType type_;
    int varcharLength_;
    bool autoIncrement_ = false;   // NEW
    bool unique_ = false;          // NEW
};

} // namespace customdb

#endif