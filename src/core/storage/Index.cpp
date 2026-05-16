// src/core/storage/Index.cpp
#include "Index.h"
#include <algorithm>

namespace customdb {

Index::Index(const std::string& columnName) : columnName_(columnName) {}

void Index::addValue(const std::string& value, size_t rowId) {
    map_[value].push_back(rowId);
}

void Index::removeValue(const std::string& value, size_t rowId) {
    auto it = map_.find(value);
    if (it != map_.end()) {
        auto& vec = it->second;
        vec.erase(std::remove(vec.begin(), vec.end(), rowId), vec.end());
        if (vec.empty()) map_.erase(it);
    }
}

std::vector<size_t> Index::find(const std::string& value) const {
    auto it = map_.find(value);
    if (it != map_.end()) return it->second;
    return {};
}

void Index::clear() { map_.clear(); }

} // namespace customdb