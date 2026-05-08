// include/customdb/client.h
#ifndef CUSTOMDB_CLIENT_H
#define CUSTOMDB_CLIENT_H

#include <string>
#include <memory>
#include "../../src/lib/exports.h"

namespace customdb {

class DatabaseClient;

// Основной класс для работы с СУБД
class Client {
public:
    Client();
    ~Client();
    
    // Подключение к серверу
    bool connect(const std::string& host, int port);
    void disconnect();
    bool isConnected() const;
    
    // Выполнение SQL-запроса
    std::string execute(const std::string& query);
    
    // Получение последней ошибки
    std::string getLastError() const;
    
private:
    std::unique_ptr<DatabaseClient> client_;
    std::string lastError_;
};

// C-совместимое API для P/Invoke из C#
extern "C" {
    CUSTOMDB_API void* db_connect(const char* host, int port);
    CUSTOMDB_API const char* db_execute(void* connection, const char* query);
    CUSTOMDB_API void db_disconnect(void* connection);
    CUSTOMDB_API void db_free_string(const char* str);
}

} // namespace customdb

#endif