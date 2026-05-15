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
    std::vector<std::string> parseColumnList();
    std::vector<Column> parseColumnDefinitions();
    Column parseColumnDefinition();
    DataType parseDataType();
    std::unique_ptr<Condition> parseWhere();
    std::vector<Value> parseValueList();
    Value parseValue();
};

#endif
