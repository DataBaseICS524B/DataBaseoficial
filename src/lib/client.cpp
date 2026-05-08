// src/lib/client.cpp
// Убеждаемся, что макрос определён перед включением заголовков
//#define CUSTOMDB_BUILD_DLL

#include "../../include/customdb/client.h"
#include "DatabaseClient.h"
#include <cstring>
#include <string>

namespace customdb {

Client::Client() : client_(std::make_unique<DatabaseClient>()) {}

Client::~Client() = default;

bool Client::connect(const std::string& host, int port) {
    try {
        bool result = client_->connect(host, port);
        if (!result) {
            lastError_ = "Connection failed";
        } else {
            lastError_.clear();
        }
        return result;
    } catch (const std::exception& e) {
        lastError_ = e.what();
        return false;
    }
}

void Client::disconnect() {
    client_->disconnect();
    lastError_.clear();
}

bool Client::isConnected() const {
    return client_->isConnected();
}

std::string Client::execute(const std::string& query) {
    try {
        std::string result = client_->execute(query);
        lastError_.clear();
        return result;
    } catch (const std::exception& e) {
        lastError_ = e.what();
        return "ERROR: " + std::string(e.what());
    }
}

std::string Client::getLastError() const {
    return lastError_;
}

} // namespace customdb

// C-совместимое API
extern "C" {

CUSTOMDB_API void* db_connect(const char* host, int port) {
    auto* client = new customdb::Client();
    if (!client->connect(host, port)) {
        delete client;
        return nullptr;
    }
    return client;
}

CUSTOMDB_API const char* db_execute(void* connection, const char* query) {
    if (!connection) return nullptr;
    
    auto* client = static_cast<customdb::Client*>(connection);
    std::string result = client->execute(query);
    
    char* resultCopy = new char[result.length() + 1];
    std::strcpy(resultCopy, result.c_str());
    return resultCopy;
}

CUSTOMDB_API void db_disconnect(void* connection) {
    if (connection) {
        auto* client = static_cast<customdb::Client*>(connection);
        delete client;
    }
}

CUSTOMDB_API void db_free_string(const char* str) {
    delete[] str;
}

} // extern "C"