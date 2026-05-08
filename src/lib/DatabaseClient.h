// src/lib/DatabaseClient.h
#ifndef DATABASE_CLIENT_H
#define DATABASE_CLIENT_H

#include "exports.h"
#include <string>
#include <memory>
#include <vector>

namespace customdb {

class CUSTOMDB_API DatabaseClient {
public:
    DatabaseClient();
    ~DatabaseClient();
    
    bool connect(const std::string& host, int port);
    void disconnect();
    bool isConnected() const;
    
    std::string execute(const std::string& query);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace customdb

#endif