#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <memory>
#include <atomic>
#include <thread>

namespace customdb {
namespace network {

class ThreadPool;

class Server {
public:
    Server(const std::string& host, int port);
    ~Server();
    
    bool start();
    void stop();
    bool isRunning() const { return running_; }
    void setMaxThreads(int threads) { maxThreads_ = threads; }
    
private:
    void acceptLoop();
    
    std::string host_;
    int port_;
    int listenSocket_;
    std::atomic<bool> running_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::thread acceptThread_;
    int maxThreads_;
};

} // namespace network
} // namespace customdb

#endif