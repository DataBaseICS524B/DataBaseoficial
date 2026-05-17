#include "../network/Server.h"
#include "../core/catalog/Catalog.h"
#include <iostream>
#include <signal.h>
#include <thread>
#include <chrono>
#include <cstring>

using namespace customdb;

static network::Server* g_server = nullptr;

void signalHandler(int signum) {
    std::cout << "\nShutting down server..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    std::string host = "0.0.0.0";
    int port = 5432;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            host = argv[++i];
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0) {
            std::cout << "Usage: " << argv[0] << " [--host HOST] [--port PORT]" << std::endl;
            return 0;
        }
    }
    
    std::cout << "CustomDB Server v1.0.0" << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << "Starting on " << host << ":" << port << std::endl;
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    Catalog::getInstance().initialize("data");
    
    network::Server server(host, port);
    g_server = &server;
    server.setMaxThreads(8);
    
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    while (server.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return 0;
}