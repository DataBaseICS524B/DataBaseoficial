#include "Parser.h"
#include <algorithm>
#include <iostream>
#include <variant>

Parser::Parser(const std::string& sql) : lexer(sql) {
    advance();
}

void Parser::advance() {
    currentToken = lexer.nextToken();
}

bool Parser::match(TokenType type) {
    if (currentToken.type == type) {
        advance();
        return true;
    }
    return false;
}

bool Parser::expect(TokenType type) {
    if (currentToken.type == type) {
        advance();
        return true;
    }
    error("Expected token");
    return false;
}

void Parser::error(const std::string& msg) {
    lastError = msg + " at line " + std::to_string(currentToken.line) +
                ", column " + std::to_string(currentToken.column);
}

// НОВЫЙ МЕТОД: парсинг нескольких запросов, разделённых ;
std::vector<std::unique_ptr<Query>> Parser::parseBatch() {
    std::vector<std::unique_ptr<Query>> queries;
    
    while (currentToken.type != TokenType::END_OF_FILE) {
        // Пропускаем пустые точки с запятой
        if (currentToken.type == TokenType::SEMICOLON) {
            advance();
            continue;
        }
        
        // Парсим один запрос
        auto query = parse();
        if (query) {
            queries.push_back(std::move(query));
        } else {
            break; // Ошибка парсинга
        }
        
        // После запроса ожидаем ; или конец
        if (currentToken.type == TokenType::SEMICOLON) {
            advance();
        } else if (currentToken.type != TokenType::END_OF_FILE) {
            error("Expected ';' after query");
            break;
        }
    }
    
    return queries;
}

std::unique_ptr<Query> Parser::parse() {
    if (currentToken.type == TokenType::CREATE) return parseCreate();
    if (currentToken.type == TokenType::DROP) return parseDrop();
    if (currentToken.type == TokenType::SELECT) return parseSelect();
    if (currentToken.type == TokenType::INSERT) return parseInsert();
    if (currentToken.type == TokenType::UPDATE) return parseUpdate();
    if (currentToken.type == TokenType::DELETE) return parseDelete();
    if (currentToken.type == TokenType::USE) return parseUse();  // НОВОЕ

    error("Unknown query type");
    return nullptr;
}

std::unique_ptr<Query> Parser::parseCreate() {
    advance();
    auto query = std::make_unique<Query>();

    if (currentToken.type == TokenType::DATABASE) {
        advance();
        CreateDatabaseQuery dbQuery;
        if (match(TokenType::IF)) {
            expect(TokenType::NOT);
            expect(TokenType::EXISTS);
            dbQuery.ifNotExists = true;
        }
        if (currentToken.type == TokenType::IDENTIFIER) {
            dbQuery.databaseName = currentToken.value;
            advance();
        }
        query->type = Query::CREATE_DB;
        query->data = dbQuery;
    }
    else if (currentToken.type == TokenType::TABLE) {
        advance();
        CreateTableQuery tableQuery;
        if (match(TokenType::IF)) {
            expect(TokenType::NOT);
            expect(TokenType::EXISTS);
            tableQuery.ifNotExists = true;
        }
        if (currentToken.type == TokenType::IDENTIFIER) {
            tableQuery.tableName = currentToken.value;
            advance();
            if (match(TokenType::DOT)) {
                tableQuery.databaseName = tableQuery.tableName;
                if (currentToken.type == TokenType::IDENTIFIER) {
                    tableQuery.tableName = currentToken.value;
                    advance();
                }
            }
        }
        expect(TokenType::LPAREN);
        tableQuery.columns = parseColumnDefinitions();
        expect(TokenType::RPAREN);
        query->type = Query::CREATE_TABLE;
        query->data = tableQuery;
    }
    return query;
}

std::unique_ptr<Query> Parser::parseDrop() {
    advance();
    auto query = std::make_unique<Query>();

    if (currentToken.type == TokenType::DATABASE) {
        advance();
        DropDatabaseQuery dbQuery;
        if (match(TokenType::IF)) {
            expect(TokenType::EXISTS);
            dbQuery.ifExists = true;
        }
        if (currentToken.type == TokenType::IDENTIFIER) {
            dbQuery.databaseName = currentToken.value;
            advance();
        }
        query->type = Query::DROP_DB;
        query->data = dbQuery;
    }
    else if (currentToken.type == TokenType::TABLE) {
        advance();
        DropTableQuery tableQuery;
        if (match(TokenType::IF)) {
            expect(TokenType::EXISTS);
            tableQuery.ifExists = true;
        }
        if (currentToken.type == TokenType::IDENTIFIER) {
            tableQuery.tableName = currentToken.value;
            advance();
            if (match(TokenType::DOT)) {
                tableQuery.databaseName = tableQuery.tableName;
                if (currentToken.type == TokenType::IDENTIFIER) {
                    tableQuery.tableName = currentToken.value;
                    advance();
                }
            }
        }
        query->type = Query::DROP_TABLE;
        query->data = tableQuery;
    }
    return query;
}

std::unique_ptr<Query> Parser::parseSelect() {
    advance();
    SelectQuery select;

    if (match(TokenType::STAR)) {
        // SELECT *
    } else {
        select.columns = parseColumnList();
    }

    expect(TokenType::FROM);

    if (currentToken.type == TokenType::IDENTIFIER) {
        select.tableName = currentToken.value;
        advance();
        if (match(TokenType::DOT)) {
            select.databaseName = select.tableName;
            if (currentToken.type == TokenType::IDENTIFIER) {
                select.tableName = currentToken.value;
                advance();
            }
        }
    }

    select.where = parseWhere();

    auto query = std::make_unique<Query>();
    query->type = Query::SELECT;
    query->data = std::move(select);
    return query;
}

std::unique_ptr<Query> Parser::parseInsert() {
    advance();
    expect(TokenType::INTO);

    InsertQuery insert;

    if (currentToken.type == TokenType::IDENTIFIER) {
        insert.tableName = currentToken.value;
        advance();
        if (match(TokenType::DOT)) {
            insert.databaseName = insert.tableName;
            if (currentToken.type == TokenType::IDENTIFIER) {
                insert.tableName = currentToken.value;
                advance();
            }
        }
    }

    if (match(TokenType::LPAREN)) {
        insert.columns = parseColumnList();
        expect(TokenType::RPAREN);
    }

    expect(TokenType::VALUES);
    expect(TokenType::LPAREN);

    do {
        insert.values.push_back(parseValueList());
    } while (match(TokenType::COMMA));

    expect(TokenType::RPAREN);

    auto query = std::make_unique<Query>();
    query->type = Query::INSERT;
    query->data = std::move(insert);
    return query;
}

std::unique_ptr<Query> Parser::parseUpdate() {
    advance();
    UpdateQuery update;

    if (currentToken.type == TokenType::IDENTIFIER) {
        update.tableName = currentToken.value;
        advance();
        if (match(TokenType::DOT)) {
            update.databaseName = update.tableName;
            if (currentToken.type == TokenType::IDENTIFIER) {
                update.tableName = currentToken.value;
                advance();
            }
        }
    }

    expect(TokenType::SET);

    do {
        if (currentToken.type == TokenType::IDENTIFIER) {
            std::string col = currentToken.value;
            advance();
            expect(TokenType::EQ);
            Value val = parseValue();
            update.setValues[col] = val;
        }
    } while (match(TokenType::COMMA));

    update.where = parseWhere();

    auto query = std::make_unique<Query>();
    query->type = Query::UPDATE;
    query->data = std::move(update);
    return query;
}

std::unique_ptr<Query> Parser::parseDelete() {
    advance();
    expect(TokenType::FROM);

    DeleteQuery del;

    if (currentToken.type == TokenType::IDENTIFIER) {
        del.tableName = currentToken.value;
        advance();
        if (match(TokenType::DOT)) {
            del.databaseName = del.tableName;
            if (currentToken.type == TokenType::IDENTIFIER) {
                del.tableName = currentToken.value;
                advance();
            }
        }
    }

    del.where = parseWhere();

    auto query = std::make_unique<Query>();
    query->type = Query::DELETE;
    query->data = std::move(del);
    return query;
}

// НОВЫЙ МЕТОД: парсинг USE database;
std::unique_ptr<Query> Parser::parseUse() {
    advance(); // пропустить USE
    UseDatabaseQuery useQuery;
    if (currentToken.type == TokenType::IDENTIFIER) {
        useQuery.databaseName = currentToken.value;
        advance();
    }
    auto query = std::make_unique<Query>();
    query->type = Query::USE_DB;
    query->data = useQuery;
    return query;
}

std::vector<std::string> Parser::parseColumnList() {
    std::vector<std::string> columns;
    do {
        if (currentToken.type == TokenType::IDENTIFIER) {
            columns.push_back(currentToken.value);
            advance();
        }
    } while (match(TokenType::COMMA));
    return columns;
}

std::vector<Column> Parser::parseColumnDefinitions() {
    std::vector<Column> columns;
    do {
        columns.push_back(parseColumnDefinition());
    } while (match(TokenType::COMMA));
    return columns;
}

Column Parser::parseColumnDefinition() {
    Column col;
    if (currentToken.type == TokenType::IDENTIFIER) {
        col.name = currentToken.value;
        advance();
    }
    col.type = parseDataType();

    while (currentToken.type == TokenType::IDENTIFIER) {
        std::string kw = currentToken.value;
        std::transform(kw.begin(), kw.end(), kw.begin(), ::toupper);
        if (kw == "NOT") {
            advance();
            if (currentToken.type == TokenType::IDENTIFIER) {
                std::string nk = currentToken.value;
                std::transform(nk.begin(), nk.end(), nk.begin(), ::toupper);
                if (nk == "NULL") {
                    col.nullable = false;
                    advance();
                }
            }
        } else if (kw == "PRIMARY") {
            advance();
            if (currentToken.type == TokenType::IDENTIFIER) {
                std::string pk = currentToken.value;
                std::transform(pk.begin(), pk.end(), pk.begin(), ::toupper);
                if (pk == "KEY") {
                    col.primaryKey = true;
                    advance();
                }
            }
        } else {
            break;
        }
    }
    return col;
}

DataType Parser::parseDataType() {
    if (currentToken.type != TokenType::IDENTIFIER) return DataType::UNKNOWN;
    std::string typeStr = currentToken.value;
    std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::toupper);
    advance();

    if (typeStr == "INT" || typeStr == "INTEGER") return DataType::INT;
    if (typeStr == "DOUBLE" || typeStr == "FLOAT") return DataType::DOUBLE;
    if (typeStr == "STRING" || typeStr == "VARCHAR" || typeStr == "TEXT") {
        if (match(TokenType::LPAREN)) {
            while (currentToken.type != TokenType::RPAREN && currentToken.type != TokenType::END_OF_FILE) {
                advance();
            }
            match(TokenType::RPAREN);
        }
        return DataType::STRING;
    }
    if (typeStr == "BOOL" || typeStr == "BOOLEAN") return DataType::BOOL;
    if (typeStr == "ARRAY") return DataType::ARRAY;
    return DataType::UNKNOWN;
}

std::unique_ptr<Condition> Parser::parseWhere() {
    if (!match(TokenType::WHERE)) return nullptr;

    auto cond = std::make_unique<Condition>();
    if (currentToken.type == TokenType::IDENTIFIER) {
        cond->column = currentToken.value;
        advance();
    }

    if (currentToken.type == TokenType::EQ) { cond->op = "="; advance(); }
    else if (currentToken.type == TokenType::GT) { cond->op = ">"; advance(); }
    else if (currentToken.type == TokenType::LT) { cond->op = "<"; advance(); }
    else if (currentToken.type == TokenType::GTE) { cond->op = ">="; advance(); }
    else if (currentToken.type == TokenType::LTE) { cond->op = "<="; advance(); }
    else if (currentToken.type == TokenType::NE) { cond->op = "<>"; advance(); }

    cond->value = parseValue();
    return cond;
}

std::vector<Value> Parser::parseValueList() {
    std::vector<Value> values;
    do {
        values.push_back(parseValue());
    } while (match(TokenType::COMMA));
    return values;
}

Value Parser::parseArrayLiteral() {
    std::vector<Value> elements;
    
    bool isBracketStyle = match(TokenType::LBRACKET);
    
    if (!isBracketStyle) {
        expect(TokenType::ARRAY);
        expect(TokenType::LBRACKET);
    }
    
    if (currentToken.type != TokenType::RBRACKET) {
        do {
            elements.push_back(parseValue());
        } while (match(TokenType::COMMA));
    }
    
    expect(TokenType::RBRACKET);
    
    return Value(elements);
}

Value Parser::parseValue() {
    if (currentToken.type == TokenType::LBRACKET || 
        (currentToken.type == TokenType::ARRAY && lexer.peekToken().type == TokenType::LBRACKET)) {
        return parseArrayLiteral();
    }
    
    if (currentToken.type == TokenType::NUMBER) {
        std::string num = currentToken.value;
        advance();
        if (num.find('.') != std::string::npos) {
            return Value(std::stod(num));
        }
        return Value(std::stoi(num));
    }
    if (currentToken.type == TokenType::STRING) {
        std::string str = currentToken.value;
        advance();
        return Value(str);
    }
    if (currentToken.type == TokenType::IDENTIFIER) {
        std::string kw = currentToken.value;
        std::transform(kw.begin(), kw.end(), kw.begin(), ::toupper);
        advance();
        if (kw == "NULL") return Value();
        if (kw == "TRUE") return Value(true);
        if (kw == "FALSE") return Value(false);
    }
    return Value();
}