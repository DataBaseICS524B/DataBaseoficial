// src/lib/Connection.cpp
#include "Connection.h"
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

namespace customdb {

class ConnectionImpl {
public:
    ConnectionImpl(const std::string& host, int port)
        : host_(host), port_(port), sock_(-1) {}
    
    ~ConnectionImpl() {
        disconnect();
    }
    
    bool connect() {
        #ifdef _WIN32
            // Инициализация Winsock для Windows
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                return false;
            }
        #endif
        
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_ < 0) {
            return false;
        }
        
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port_);
        serverAddr.sin_addr.s_addr = inet_addr(host_.c_str());
        
        if (::connect(sock_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            return false;
        }
        
        return true;
    }
    
    void disconnect() {
        if (sock_ >= 0) {
            #ifdef _WIN32
                closesocket(sock_);
                WSACleanup();
            #else
                close(sock_);
            #endif
            sock_ = -1;
        }
    }
    
    bool isConnected() const {
        return sock_ >= 0;
    }
    
    std::string execute(const std::string& query) {
        if (!isConnected()) {
            throw std::runtime_error("Not connected to server");
        }
        
        // Отправляем длину запроса (4 байта) + сам запрос
        uint32_t queryLen = query.length();
        send(sock_, reinterpret_cast<char*>(&queryLen), 4, 0);
        send(sock_, query.c_str(), queryLen, 0);
        
        // Получаем статус ответа (4 байта)
        uint32_t status;
        recv(sock_, reinterpret_cast<char*>(&status), 4, 0);
        
        // Получаем длину данных
        uint32_t dataLen;
        recv(sock_, reinterpret_cast<char*>(&dataLen), 4, 0);
        
        // Получаем данные
        std::vector<char> buffer(dataLen + 1);
        recv(sock_, buffer.data(), dataLen, 0);
        buffer[dataLen] = '\0';
        
        return std::string(buffer.data());
    }
    
private:
    std::string host_;
    int port_;
    int sock_;
};

// Реализация Connection
Connection::Connection(const std::string& host, int port)
    : pImpl_(std::make_unique<ConnectionImpl>(host, port)) {}

Connection::~Connection() = default;

bool Connection::connect() {
    return pImpl_->connect();
}

void Connection::disconnect() {
    pImpl_->disconnect();
}

bool Connection::isConnected() const {
    return pImpl_->isConnected();
}

std::string Connection::execute(const std::string& query) {
    return pImpl_->execute(query);
}

} // namespace customdb