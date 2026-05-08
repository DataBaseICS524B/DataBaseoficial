// src/core/catalog/Column.cpp
#include "Column.h"
#include <stdexcept>
#include <regex>
#include <cctype>

namespace customdb {

Column::Column(const std::string& name, DataType type, int varcharLength)
    : name_(name), type_(type), varcharLength_(varcharLength) {
    
    if (type_ == DataType::VARCHAR && varcharLength_ <= 0) {
        throw std::invalid_argument("VARCHAR length must be positive");
    }
}

const std::string& Column::getName() const {
    return name_;
}

DataType Column::getType() const {
    return type_;
}

int Column::getVarcharLength() const {
    return varcharLength_;
}

bool Column::validateValue(const std::string& value) const {
    if (value == "NULL") {
        return true;
    }
    
    switch (type_) {
        case DataType::INT: {
            // Проверка на целое число (возможно отрицательное)
            std::regex intPattern(R"(^-?\d+$)");
            return std::regex_match(value, intPattern);
        }
        case DataType::FLOAT: {
            // Проверка на число с плавающей точкой
            std::regex floatPattern(R"(^-?\d+(?:\.\d+)?$)");
            return std::regex_match(value, floatPattern);
        }
        case DataType::BOOL: {
            // Проверка на TRUE/FALSE
            return value == "TRUE" || value == "FALSE";
        }
        case DataType::TEXT: {
            // TEXT может быть любым (кроме NULL)
            return true;
        }
        case DataType::VARCHAR: {
            // Проверка длины
            return value.length() <= static_cast<size_t>(varcharLength_);
        }
        default:
            return false;
    }
}

std::string Column::toString() const {
    std::string result = name_ + " ";
    switch (type_) {
        case DataType::INT:    result += "INT"; break;
        case DataType::FLOAT:  result += "FLOAT"; break;
        case DataType::BOOL:   result += "BOOL"; break;
        case DataType::TEXT:   result += "TEXT"; break;
        case DataType::VARCHAR: 
            result += "VARCHAR(" + std::to_string(varcharLength_) + ")"; 
            break;
    }
    return result;
}

} // namespace customdb