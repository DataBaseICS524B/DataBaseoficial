#include "ClientSession.h"
#include "Protocol.h"
#include "../core/catalog/Catalog.h"
#include "../core/storage/StorageEngine.h"
#include <iostream>
#include <cstring>
#include <regex>
#include <sstream>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <unistd.h>
#endif
namespace customdb {
namespace network {

ClientSession::ClientSession(int socketFd) : socket_(socketFd), active_(true) {}

ClientSession::~ClientSession() { close(); }

void ClientSession::handle() {
    while (active_) {
        uint32_t queryLen;
        int received = recv(socket_, reinterpret_cast<char*>(&queryLen), 4, 0);
        if (received <= 0) break;
        
        queryLen = ntohl(queryLen);
        if (queryLen > 1024 * 1024) {
            active_ = false;
            break;
        }
        
        std::vector<char> queryBuffer(queryLen + 1);
        recv(socket_, queryBuffer.data(), queryLen, 0);
        queryBuffer[queryLen] = '\0';
        
        std::string query(queryBuffer.data());
        std::string result = processQuery(query);
        
        auto response = Protocol::encodeResponse(ResponseStatus::SUCCESS, result);
        send(socket_, response.data(), response.size(), 0);
    }
}

std::string ClientSession::processQuery(const std::string& query) {
    auto& catalog = Catalog::getInstance();
    
    try {
        std::string upperQuery = query;
        std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
        
        // CREATE DATABASE
        std::regex createDbRegex(R"(CREATE\s+DATABASE\s+(\w+);?)", std::regex::icase);
        std::smatch match;
        if (std::regex_search(query, match, createDbRegex)) {
            std::string dbName = match[1];
            catalog.createDatabase(dbName);
            catalog.saveAll();
            return Protocol::createDDLResult(true, "Database " + dbName + " created").dump();
        }
        
        // DROP DATABASE
        std::regex dropDbRegex(R"(DROP\s+DATABASE\s+(\w+);?)", std::regex::icase);
        if (std::regex_search(query, match, dropDbRegex)) {
            std::string dbName = match[1];
            catalog.dropDatabase(dbName);
            catalog.saveAll();
            return Protocol::createDDLResult(true, "Database " + dbName + " dropped").dump();
        }
        
        // CREATE TABLE
        std::regex createTableRegex(R"(CREATE\s+TABLE\s+(\w+)\s*\((.+)\);?)", std::regex::icase);
        if (std::regex_search(query, match, createTableRegex)) {
            std::string tableName = match[1];
            std::string columnsDef = match[2];
            
            std::vector<Column> columns;
            std::regex colRegex(R"(\s*(\w+)\s+(INT|FLOAT|BOOL|TEXT|VARCHAR(?:\((\d+)\))?))", std::regex::icase);
            std::sregex_iterator it(columnsDef.begin(), columnsDef.end(), colRegex);
            std::sregex_iterator end;
            
            for (; it != end; ++it) {
                std::string colName = (*it)[1];
                std::string colType = (*it)[2];
                std::string varcharLen = (*it)[3];
                
                if (colType == "INT") {
                    columns.emplace_back(colName, DataType::INT);
                } else if (colType == "FLOAT") {
                    columns.emplace_back(colName, DataType::FLOAT);
                } else if (colType == "BOOL") {
                    columns.emplace_back(colName, DataType::BOOL);
                } else if (colType == "TEXT") {
                    columns.emplace_back(colName, DataType::TEXT);
                } else if (colType.find("VARCHAR") != std::string::npos) {
                    int len = varcharLen.empty() ? 255 : std::stoi(varcharLen);
                    columns.emplace_back(colName, DataType::VARCHAR, len);
                }
            }
            
            std::string dbName = catalog.getCurrentDatabaseName();
            if (dbName.empty()) dbName = "test_db";
            if (!catalog.databaseExists(dbName)) catalog.createDatabase(dbName);
            
            catalog.createTable(dbName, tableName, columns);
            catalog.saveAll();
            return Protocol::createDDLResult(true, "Table " + tableName + " created").dump();
        }
        
        // INSERT
        std::regex insertRegex(R"(INSERT\s+INTO\s+(\w+)\s+VALUES\s*\((.+)\);?)", std::regex::icase);
        if (std::regex_search(query, match, insertRegex)) {
            std::string tableName = match[1];
            std::string valuesStr = match[2];
            
            std::vector<std::string> values;
            std::regex valRegex(R"('([^']*)'|([^,)]+))");
            std::sregex_iterator vit(valuesStr.begin(), valuesStr.end(), valRegex);
            std::sregex_iterator end;
            for (; vit != end; ++vit) {
                std::string val = (*vit)[1].matched ? (*vit)[1].str() : (*vit)[2].str();
                val = std::regex_replace(val, std::regex(R"(^\s+|\s+$)"), "");
                values.push_back(val);
            }
            
            std::string dbName = catalog.getCurrentDatabaseName();
            if (dbName.empty()) dbName = "test_db";
            
            Table* table = catalog.getTable(dbName, tableName);
            if (!table) return Protocol::createErrorResult("Table not found").dump();
            
            table->insertRow(values);
            catalog.saveAll();
            return Protocol::createDMLResult(1).dump();
        }
        
        // SELECT
        std::regex selectRegex(R"(SELECT\s+(.*?)\s+FROM\s+(\w+)(?:\s+WHERE\s+(.+))?;?)", std::regex::icase);
        if (std::regex_search(query, match, selectRegex)) {
            std::string columnsList = match[1];
            std::string tableName = match[2];
            
            std::string dbName = catalog.getCurrentDatabaseName();
            if (dbName.empty()) dbName = "test_db";
            
            Table* table = catalog.getTable(dbName, tableName);
            if (!table) return Protocol::createErrorResult("Table not found").dump();
            
            std::vector<std::string> selectedCols;
            if (columnsList == "*") {
                for (const auto& col : table->getColumns()) {
                    selectedCols.push_back(col.getName());
                }
            } else {
                std::regex colRegex(R"(\w+)");
                std::sregex_iterator cit(columnsList.begin(), columnsList.end(), colRegex);
                std::sregex_iterator end;
                for (; cit != end; ++cit) {
                    selectedCols.push_back((*cit).str());
                }
            }
            
            std::vector<int> colIndices;
            for (const auto& colName : selectedCols) {
                colIndices.push_back(table->getColumnIndex(colName));
            }
            
            std::vector<std::vector<std::string>> resultRows;
            for (const auto& row : table->getRows()) {
                std::vector<std::string> resultRow;
                for (int idx : colIndices) {
                    if (idx >= 0 && idx < (int)row.size()) {
                        resultRow.push_back(row[idx]);
                    }
                }
                resultRows.push_back(resultRow);
            }
            
            return Protocol::createSelectResult(selectedCols, resultRows).dump();
        }
        
        // UPDATE
        std::regex updateRegex(R"(UPDATE\s+(\w+)\s+SET\s+(.+?)(?:\s+WHERE\s+(.+))?;?)", std::regex::icase);
        if (std::regex_search(query, match, updateRegex)) {
            std::string tableName = match[1];
            std::string setClause = match[2];
            
            std::string dbName = catalog.getCurrentDatabaseName();
            if (dbName.empty()) dbName = "test_db";
            
            Table* table = catalog.getTable(dbName, tableName);
            if (!table) return Protocol::createErrorResult("Table not found").dump();
            
            std::regex setRegex(R"(\s*(\w+)\s*=\s*'([^']*)'|\s*(\w+)\s*=\s*([^,]+))");
            std::vector<std::pair<int, std::string>> updates;
            std::sregex_iterator sit(setClause.begin(), setClause.end(), setRegex);
            std::sregex_iterator end;
            for (; sit != end; ++sit) {
                std::string colName = (*sit)[1].matched ? (*sit)[1].str() : (*sit)[3].str();
                std::string val = (*sit)[2].matched ? (*sit)[2].str() : (*sit)[4].str();
                val = std::regex_replace(val, std::regex(R"(^\s+|\s+$)"), "");
                int idx = table->getColumnIndex(colName);
                if (idx >= 0) updates.push_back({idx, val});
            }
            
            auto rows = table->getRows();
            for (auto& row : rows) {
                for (const auto& [idx, val] : updates) {
                    if (idx < (int)row.size()) row[idx] = val;
                }
            }
            table->setRows(rows);
            catalog.saveAll();
            return Protocol::createDMLResult(rows.size()).dump();
        }
        
        // DELETE
        std::regex deleteRegex(R"(DELETE\s+FROM\s+(\w+)(?:\s+WHERE\s+(.+))?;?)", std::regex::icase);
        if (std::regex_search(query, match, deleteRegex)) {
            std::string tableName = match[1];
            
            std::string dbName = catalog.getCurrentDatabaseName();
            if (dbName.empty()) dbName = "test_db";
            
            Table* table = catalog.getTable(dbName, tableName);
            if (!table) return Protocol::createErrorResult("Table not found").dump();
            
            int deletedCount = table->getRowCount();
            table->clearRows();
            catalog.saveAll();
            return Protocol::createDMLResult(deletedCount).dump();
        }
        
        // USE DATABASE
        std::regex useRegex(R"(USE\s+(\w+);?)", std::regex::icase);
        if (std::regex_search(query, match, useRegex)) {
            std::string dbName = match[1];
            if (catalog.databaseExists(dbName)) {
                catalog.setCurrentDatabase(dbName);
                return Protocol::createDDLResult(true, "Switched to database " + dbName).dump();
            }
            return Protocol::createErrorResult("Database not found").dump();
        }
        
        // DROP TABLE
        std::regex dropTableRegex(R"(DROP\s+TABLE\s+(\w+);?)", std::regex::icase);
        if (std::regex_search(query, match, dropTableRegex)) {
            std::string tableName = match[1];
            std::string dbName = catalog.getCurrentDatabaseName();
            if (dbName.empty()) dbName = "test_db";
            catalog.dropTable(dbName, tableName);
            catalog.saveAll();
            return Protocol::createDDLResult(true, "Table " + tableName + " dropped").dump();
        }
        
        return Protocol::createErrorResult("Unknown query type").dump();
        
    } catch (const std::exception& e) {
        return Protocol::createErrorResult(e.what()).dump();
    }
}

void ClientSession::close() {
    if (socket_ >= 0) {
#ifdef _WIN32
        closesocket(socket_);
#else
        ::close(socket_);
#endif
        socket_ = -1;
    }
    active_ = false;
}

} 
} 