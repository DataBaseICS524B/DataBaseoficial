#ifndef PARSER_H
#define PARSER_H

#include "Lexer.h"
#include "../ast/ASTNodes.h"
#include <memory>
#include <vector>

class Parser {
public:
    Parser(const std::string& sql);
    std::unique_ptr<Query> parse();
    std::vector<std::unique_ptr<Query>> parseBatch();  // НОВЫЙ МЕТОД
    std::string getLastError() const { return lastError; }
    
private:
    Lexer lexer;
    Token currentToken;
    std::string lastError;
    
    void advance();
    bool match(TokenType type);
    bool expect(TokenType type);
    void error(const std::string& msg);
    
    std::unique_ptr<Query> parseCreate();
    std::unique_ptr<Query> parseDrop();
    std::unique_ptr<Query> parseSelect();
    std::unique_ptr<Query> parseInsert();
    std::unique_ptr<Query> parseUpdate();
    std::unique_ptr<Query> parseDelete();
    std::unique_ptr<Query> parseUse();  // НОВЫЙ МЕТОД
    
    std::vector<std::string> parseColumnList();
    std::vector<Column> parseColumnDefinitions();
    Column parseColumnDefinition();
    DataType parseDataType();
    std::unique_ptr<Condition> parseWhere();
    std::vector<Value> parseValueList();
    std::unique_ptr<Query> parseUse();
    Value parseValue();
    
    // Для массивов
    Value parseArrayLiteral();
};

#endif