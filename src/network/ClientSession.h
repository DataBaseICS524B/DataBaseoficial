#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include <string>
#include <memory>

namespace customdb {
namespace network {

class ClientSession {
public:
    ClientSession(int socketFd);
    ~ClientSession();
    
    void handle();
    bool isActive() const { return active_; }
    void close();
    
private:
    std::string processQuery(const std::string& query);
    
    int socket_;
    bool active_;
};

} // namespace network
} // namespace customdb

#endif