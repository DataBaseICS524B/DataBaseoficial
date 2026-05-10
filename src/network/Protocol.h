#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace customdb {
namespace network {

enum class ResponseStatus : uint32_t {
    SUCCESS = 0,
    ERROR = 1
};

class Protocol {
public:
    static std::vector<char> encodeQuery(const std::string& query);
    static std::string decodeQuery(const char* data, uint32_t length);
    static std::vector<char> encodeResponse(ResponseStatus status, const std::string& data);
    static bool decodeResponse(const char* data, uint32_t length, ResponseStatus& status, std::string& responseData);
    
    static nlohmann::json createSelectResult(const std::vector<std::string>& columns,
                                              const std::vector<std::vector<std::string>>& rows);
    static nlohmann::json createDMLResult(int affectedRows);
    static nlohmann::json createDDLResult(bool success, const std::string& message);
    static nlohmann::json createErrorResult(const std::string& errorMessage);
};

} // namespace network
} // namespace customdb

#endif