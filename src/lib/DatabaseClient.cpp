#include "DatabaseClient.h"
#include "Connection.h"
#include <stdexcept>

namespace customdb {

class DatabaseClient::Impl {
public:
    Impl() : connection_(nullptr) {}
    
    bool connect(const std::string& host, int port) {
        connection_ = std::make_unique<Connection>(host, port);
        return connection_->connect();
    }
    
    void disconnect() {
        if (connection_) {
            connection_->disconnect();
            connection_.reset();
        }
    }
    
    bool isConnected() const {
        return connection_ && connection_->isConnected();
    }
    
    std::string execute(const std::string& query) {
        if (!connection_) {
            throw std::runtime_error("Not connected");
        }
        return connection_->execute(query);
    }
    
private:
    std::unique_ptr<Connection> connection_;
};

DatabaseClient::DatabaseClient() : pImpl_(std::make_unique<Impl>()) {}
DatabaseClient::~DatabaseClient() = default;

bool DatabaseClient::connect(const std::string& host, int port) {
    return pImpl_->connect(host, port);
}

void DatabaseClient::disconnect() {
    pImpl_->disconnect();
}

bool DatabaseClient::isConnected() const {
    return pImpl_->isConnected();
}

std::string DatabaseClient::execute(const std::string& query) {
    return pImpl_->execute(query);
}

}