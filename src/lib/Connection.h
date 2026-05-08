// src/lib/Connection.h
#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <memory>

namespace customdb {

class ConnectionImpl;

class Connection {
public:
    Connection(const std::string& host, int port);
    ~Connection();
    
    bool connect();
    void disconnect();
    bool isConnected() const;
    std::string execute(const std::string& query);
    
private:
    std::unique_ptr<ConnectionImpl> pImpl_;
};

} // namespace customdb

#endif