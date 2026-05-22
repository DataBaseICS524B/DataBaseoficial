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
namespace network {

// Преобразует строку вида 'admin','user' в ["admin","user"]
std::string arrayToJson(const std::string& inner) {
    std::string result = "[";
    std::regex elementRegex(R"('([^']*)'|([^,]+))");
    std::sregex_iterator it(inner.begin(), inner.end(), elementRegex);
    std::sregex_iterator end;
    bool first = true;
    for (; it != end; ++it) {
        if (!first) result += ",";
        std::string elem = (*it)[1].matched ? (*it)[1].str() : (*it)[2].str();
        elem = std::regex_replace(elem, std::regex(R"(^\s+|\s+$)"), "");
        result += "\"" + elem + "\"";
        first = false;
    }
    result += "]";
    return result;
}

// Разделяет строку на несколько запросов по символу ;
std::vector<std::string> splitQueries(const std::string& query) {
    std::vector<std::string> queries;
    std::string current;
    bool inString = false;
    
    for (char c : query) {
        if (c == '\'') {
            inString = !inString;
            current += c;
        } else if (c == ';' && !inString) {
            size_t start = current.find_first_not_of(" \t\n\r");
            size_t end = current.find_last_not_of(" \t\n\r");
            if (start != std::string::npos) {
                queries.push_back(current.substr(start, end - start + 1));
            }
            current.clear();
        } else {
            current += c;
        }
    }
    
    size_t start = current.find_first_not_of(" \t\n\r");
    if (start != std::string::npos && !current.empty()) {
        queries.push_back(current.substr(start));
    }
    
    return queries;
}

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
        auto queries = splitQueries(query);
        
        if (queries.empty()) {
            return Protocol::createErrorResult("Empty query").dump();
        }
        
        std::string lastResult;
        
        for (const auto& singleQuery : queries) {
            std::string upperQuery = singleQuery;
            std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
            
            std::smatch match;
            
            // CREATE DATABASE
            std::regex createDbRegex(R"(CREATE\s+DATABASE\s+(\w+);?)", std::regex::icase);
            if (std::regex_search(singleQuery, match, createDbRegex)) {
                std::string dbName = match[1];
                catalog.createDatabase(dbName);
                catalog.saveAll();
                lastResult = Protocol::createDDLResult(true, "Database " + dbName + " created").dump();
                continue;
            }
            
            // DROP DATABASE
            std::regex dropDbRegex(R"(DROP\s+DATABASE\s+(\w+);?)", std::regex::icase);
            if (std::regex_search(singleQuery, match, dropDbRegex)) {
                std::string dbName = match[1];
                catalog.dropDatabase(dbName);
                catalog.saveAll();
                lastResult = Protocol::createDDLResult(true, "Database " + dbName + " dropped").dump();
                continue;
            }
            
            // CREATE TABLE
            std::regex createTableRegex(R"(CREATE\s+TABLE\s+(\w+)\s*\((.+)\);?)", std::regex::icase);
            if (std::regex_search(singleQuery, match, createTableRegex)) {
                std::string tableName = match[1];
                std::string columnsDef = match[2];
                
                std::vector<Column> columns;
                std::regex colRegex(R"(\s*(\w+)\s+(INT|FLOAT|BOOL|TEXT|VARCHAR(?:\((\d+)\))?)\s*(?:AUTO_INCREMENT)?\s*(?:UNIQUE)?)", std::regex::icase);
                std::sregex_iterator it(columnsDef.begin(), columnsDef.end(), colRegex);
                std::sregex_iterator end;
                
                for (; it != end; ++it) {
                    std::string colName = (*it)[1];
                    std::string colType = (*it)[2];
                    std::string varcharLen = (*it)[3];
                    std::string modifiers = (*it)[0];
                    
                    Column col(colName, 
                        (colType == "INT") ? DataType::INT :
                        (colType == "FLOAT") ? DataType::FLOAT :
                        (colType == "BOOL") ? DataType::BOOL :
                        (colType == "TEXT") ? DataType::TEXT :
                        DataType::VARCHAR, 
                        varcharLen.empty() ? 255 : std::stoi(varcharLen));
                    
                    // Проверяем модификаторы
                    std::string upperMod = modifiers;
                    std::transform(upperMod.begin(), upperMod.end(), upperMod.begin(), ::toupper);
                    if (upperMod.find("AUTO_INCREMENT") != std::string::npos) {
                        col.setAutoIncrement(true);
                    }
                    if (upperMod.find("UNIQUE") != std::string::npos) {
                        col.setUnique(true);
                    }
                    
                    columns.push_back(col);
                }
                
                std::string dbName = catalog.getCurrentDatabaseName();
                if (dbName.empty()) dbName = "test_db";
                if (!catalog.databaseExists(dbName)) catalog.createDatabase(dbName);
                
                catalog.createTable(dbName, tableName, columns);
                catalog.saveAll();
                lastResult = Protocol::createDDLResult(true, "Table " + tableName + " created").dump();
                continue;
            }
            
            // INSERT
            std::regex insertBasicRegex(R"(INSERT\s+INTO\s+(\w+)\s+VALUES\s*\((.+)\);?)", std::regex::icase);
            std::regex insertColumnsRegex(R"(INSERT\s+INTO\s+(\w+)\s*\(([^)]+)\)\s+VALUES\s*\((.+)\);?)", std::regex::icase);
            
            std::string tableName;
            std::vector<std::string> columnNames;
            std::string valuesStr;
            
            if (std::regex_search(singleQuery, match, insertColumnsRegex)) {
                tableName = match[1];
                std::string columnsStr = match[2];
                valuesStr = match[3];
                
                std::regex colRegex(R"(\s*(\w+)\s*)");
                std::sregex_iterator colIt(columnsStr.begin(), columnsStr.end(), colRegex);
                std::sregex_iterator colEnd;
                for (; colIt != colEnd; ++colIt) {
                    columnNames.push_back((*colIt)[1].str());
                }
            } else if (std::regex_search(singleQuery, match, insertBasicRegex)) {
                tableName = match[1];
                valuesStr = match[2];
                columnNames.clear();
            }
            
            if (!tableName.empty()) {
                std::vector<std::string> values;
                std::string current;
                int parenLevel = 0;
                bool inString = false;
                
                for (char c : valuesStr) {
                    if (c == '\'' && (current.empty() || current.back() != '\\')) {
                        inString = !inString;
                        current += c;
                    } else if (!inString && c == '(') {
                        parenLevel++;
                        current += c;
                    } else if (!inString && c == ')') {
                        parenLevel--;
                        current += c;
                    } else if (!inString && c == ',' && parenLevel == 0) {
                        size_t start = current.find_first_not_of(" \t\n\r");
                        size_t end = current.find_last_not_of(" \t\n\r");
                        if (start != std::string::npos) {
                            values.push_back(current.substr(start, end - start + 1));
                        }
                        current.clear();
                    } else {
                        current += c;
                    }
                }
                
                size_t start = current.find_first_not_of(" \t\n\r");
                size_t end = current.find_last_not_of(" \t\n\r");
                if (start != std::string::npos) {
                    values.push_back(current.substr(start, end - start + 1));
                }
                
                for (auto& val : values) {
                    if (val.size() >= 2 && val.front() == '\'' && val.back() == '\'') {
                        val = val.substr(1, val.size() - 2);
                    }
                    
                    std::regex arrayRegex(R"(ARRAY\[(.+)\])", std::regex::icase);
                    std::smatch arrayMatch;
                    if (std::regex_match(val, arrayMatch, arrayRegex)) {
                        val = arrayToJson(arrayMatch[1].str());
                    }
                    
                    std::regex bracketRegex(R"(^\[(.+)\]$)");
                    std::smatch bracketMatch;
                    if (std::regex_match(val, bracketMatch, bracketRegex)) {
                        val = arrayToJson(bracketMatch[1].str());
                    }
                }
                
                std::string dbName = catalog.getCurrentDatabaseName();
                if (dbName.empty()) dbName = "test_db";
                
                Table* table = catalog.getTable(dbName, tableName);
                if (!table) {
                    lastResult = Protocol::createErrorResult("Table not found").dump();
                    continue;
                }
                
                std::vector<std::string> finalValues;
                if (!columnNames.empty()) {
                    const auto& cols = table->getColumns();
                    for (const auto& col : cols) {
                        auto it = std::find(columnNames.begin(), columnNames.end(), col.getName());
                        if (it != columnNames.end()) {
                            int idx = std::distance(columnNames.begin(), it);
                            if (idx < (int)values.size()) {
                                finalValues.push_back(values[idx]);
                            } else {
                                finalValues.push_back("NULL");
                            }
                        } else {
                            finalValues.push_back("NULL");
                        }
                    }
                } else {
                    finalValues = values;
                }
                
                const auto& cols = table->getColumns();
                for (size_t i = 0; i < cols.size() && i < finalValues.size(); ++i) {
                    if (cols[i].isAutoIncrement() && (finalValues[i] == "NULL" || finalValues[i].empty())) {
                        int maxId = 0;
                        for (const auto& row : table->getRows()) {
                            if (i < row.size()) {
                                try { maxId = std::max(maxId, std::stoi(row[i])); } catch(...) {}
                            }
                        }
                        finalValues[i] = std::to_string(maxId + 1);
                    }
                }
                
                table->insertRow(finalValues);
                catalog.saveAll();
                lastResult = Protocol::createDMLResult(1).dump();
                continue;
            }
            
            // SELECT
            std::regex selectRegex(R"(SELECT\s+(.*?)\s+FROM\s+(\w+)(?:\s+WHERE\s+(.+))?;?)", std::regex::icase);
            if (std::regex_search(singleQuery, match, selectRegex)) {
                std::string columnsList = match[1];
                std::string tableNameSel = match[2];
                
                std::string dbName = catalog.getCurrentDatabaseName();
                if (dbName.empty()) dbName = "test_db";
                
                Table* table = catalog.getTable(dbName, tableNameSel);
                if (!table) {
                    lastResult = Protocol::createErrorResult("Table not found").dump();
                    continue;
                }
                
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
                
                lastResult = Protocol::createSelectResult(selectedCols, resultRows).dump();
                continue;
            }
            
            // UPDATE
            std::regex updateRegex(R"(UPDATE\s+(\w+)\s+SET\s+(.+?)(?:\s+WHERE\s+(.+))?;?)", std::regex::icase);
            if (std::regex_search(singleQuery, match, updateRegex)) {
                std::string tableNameUp = match[1];
                std::string setClause = match[2];
                
                std::string dbName = catalog.getCurrentDatabaseName();
                if (dbName.empty()) dbName = "test_db";
                
                Table* table = catalog.getTable(dbName, tableNameUp);
                if (!table) {
                    lastResult = Protocol::createErrorResult("Table not found").dump();
                    continue;
                }
                
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
                lastResult = Protocol::createDMLResult(rows.size()).dump();
                continue;
            }
            
            // DELETE
            std::regex deleteRegex(R"(DELETE\s+FROM\s+(\w+)(?:\s+WHERE\s+(.+))?;?)", std::regex::icase);
            if (std::regex_search(singleQuery, match, deleteRegex)) {
                std::string tableNameDel = match[1];
                
                std::string dbName = catalog.getCurrentDatabaseName();
                if (dbName.empty()) dbName = "test_db";
                
                Table* table = catalog.getTable(dbName, tableNameDel);
                if (!table) {
                    lastResult = Protocol::createErrorResult("Table not found").dump();
                    continue;
                }
                
                int deletedCount = table->getRowCount();
                table->clearRows();
                catalog.saveAll();
                lastResult = Protocol::createDMLResult(deletedCount).dump();
                continue;
            }
            
            // USE DATABASE
            std::regex useRegex(R"(USE\s+(\w+);?)", std::regex::icase);
            if (std::regex_search(singleQuery, match, useRegex)) {
                std::string dbName = match[1];
                if (catalog.databaseExists(dbName)) {
                    catalog.setCurrentDatabase(dbName);
                    lastResult = Protocol::createDDLResult(true, "Switched to database " + dbName).dump();
                } else {
                    lastResult = Protocol::createErrorResult("Database not found").dump();
                }
                continue;
            }
            
            // DROP TABLE
            std::regex dropTableRegex(R"(DROP\s+TABLE\s+(\w+);?)", std::regex::icase);
            if (std::regex_search(singleQuery, match, dropTableRegex)) {
                std::string tableNameDrop = match[1];
                std::string dbName = catalog.getCurrentDatabaseName();
                if (dbName.empty()) dbName = "test_db";
                catalog.dropTable(dbName, tableNameDrop);
                catalog.saveAll();
                lastResult = Protocol::createDDLResult(true, "Table " + tableNameDrop + " dropped").dump();
                continue;
            }
            
            lastResult = Protocol::createErrorResult("Unknown query: " + singleQuery).dump();
        }
        
        return lastResult;
        
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