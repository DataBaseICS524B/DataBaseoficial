// src/core/storage/Index.h
#ifndef INDEX_H
#define INDEX_H

#include <string>
#include <unordered_map>
#include <vector>

namespace customdb {

class Index {
public:
    Index(const std::string& columnName);
    void addValue(const std::string& value, size_t rowId);
    void removeValue(const std::string& value, size_t rowId);
    std::vector<size_t> find(const std::string& value) const;
    void clear();
private:
    std::string columnName_;
    std::unordered_map<std::string, std::vector<size_t>> map_;
};

} // namespace customdb

#endif