#include "Server.h"
#include "ThreadPool.h"
#include "ClientSession.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET_ERROR -1
    #define INVALID_SOCKET -1
    #define closesocket close
#endif

namespace customdb {
namespace network {

Server::Server(const std::string& host, int port)
    : host_(host), port_(port), listenSocket_(-1), running_(false), maxThreads_(4) {}

Server::~Server() { stop(); }

bool Server::start() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }
#endif

    listenSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket_ == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        return false;
    }

    int opt = 1;
    setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    serverAddr.sin_addr.s_addr = inet_addr(host_.c_str());

    if (bind(listenSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed on port " << port_ << std::endl;
        return false;
    }

    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        return false;
    }

    threadPool_ = std::make_unique<ThreadPool>(maxThreads_);
    running_ = true;
    acceptThread_ = std::thread(&Server::acceptLoop, this);

    std::cout << "Server started on " << host_ << ":" << port_ << std::endl;
    return true;
}

void Server::acceptLoop() {
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(listenSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket == INVALID_SOCKET) {
            if (running_) std::cerr << "Accept failed" << std::endl;
            continue;
        }

        threadPool_->enqueue([this, clientSocket]() {
            ClientSession session(clientSocket);
            session.handle();
        });
    }
}

void Server::stop() {
    running_ = false;
    if (listenSocket_ != -1) {
        closesocket(listenSocket_);
        listenSocket_ = -1;
    }
    if (threadPool_) {
        threadPool_->stop();
    }
    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
#ifdef _WIN32
    WSACleanup();
#endif
    std::cout << "Server stopped" << std::endl;
}

} // namespace network
} // namespace customdb