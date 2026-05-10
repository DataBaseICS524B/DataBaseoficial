#include "Protocol.h"
#include <cstring>
#include <arpa/inet.h>

namespace customdb {
namespace network {

std::vector<char> Protocol::encodeQuery(const std::string& query) {
    uint32_t length = htonl(static_cast<uint32_t>(query.size()));
    std::vector<char> buffer(4 + query.size());
    memcpy(buffer.data(), &length, 4);
    memcpy(buffer.data() + 4, query.c_str(), query.size());
    return buffer;
}

std::string Protocol::decodeQuery(const char* data, uint32_t length) {
    return std::string(data, length);
}

std::vector<char> Protocol::encodeResponse(ResponseStatus status, const std::string& data) {
    uint32_t statusNet = htonl(static_cast<uint32_t>(status));
    uint32_t dataLenNet = htonl(static_cast<uint32_t>(data.size()));
    std::vector<char> buffer(8 + data.size());
    memcpy(buffer.data(), &statusNet, 4);
    memcpy(buffer.data() + 4, &dataLenNet, 4);
    memcpy(buffer.data() + 8, data.c_str(), data.size());
    return buffer;
}

bool Protocol::decodeResponse(const char* data, uint32_t length, ResponseStatus& status, std::string& responseData) {
    if (length < 8) return false;
    uint32_t statusNet, dataLenNet;
    memcpy(&statusNet, data, 4);
    memcpy(&dataLenNet, data + 4, 4);
    status = static_cast<ResponseStatus>(ntohl(statusNet));
    uint32_t dataLen = ntohl(dataLenNet);
    if (length < 8 + dataLen) return false;
    responseData = std::string(data + 8, dataLen);
    return true;
}

nlohmann::json Protocol::createSelectResult(const std::vector<std::string>& columns,
                                             const std::vector<std::vector<std::string>>& rows) {
    nlohmann::json result;
    result["type"] = "select";
    result["columns"] = columns;
    result["rows"] = rows;
    result["affected_rows"] = rows.size();
    return result;
}

nlohmann::json Protocol::createDMLResult(int affectedRows) {
    nlohmann::json result;
    result["type"] = "dml";
    result["affected_rows"] = affectedRows;
    result["message"] = std::to_string(affectedRows) + " rows affected";
    return result;
}

nlohmann::json Protocol::createDDLResult(bool success, const std::string& message) {
    nlohmann::json result;
    result["type"] = "ddl";
    result["success"] = success;
    result["message"] = message;
    return result;
}

nlohmann::json Protocol::createErrorResult(const std::string& errorMessage) {
    nlohmann::json result;
    result["type"] = "error";
    result["message"] = errorMessage;
    return result;
}

} // namespace network
} // namespace customdb