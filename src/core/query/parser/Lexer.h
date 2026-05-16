#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <unordered_map>

enum class TokenType {
    SELECT, FROM, WHERE, INSERT, INTO, VALUES, UPDATE, SET, DELETE,
    CREATE, DROP, DATABASE, TABLE, IF, NOT, EXISTS, AND, OR,
    EQ, GT, LT, GTE, LTE, NE,
    IDENTIFIER, NUMBER, STRING,
    STAR, COMMA, SEMICOLON, LPAREN, RPAREN, DOT,
    END_OF_FILE, UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line, column;
    Token(TokenType t = TokenType::END_OF_FILE, const std::string& v = "", int l = 0, int c = 0)
        : type(t), value(v), line(l), column(c) {}
};

class Lexer {
public:
    Lexer(const std::string& source);
    Token nextToken();
    Token peekToken();
    std::vector<Token> tokenizeAll();
private:
    std::string source;
    size_t position;
    int line, column;
    char currentChar();
    void advance();
    void skipWhitespace();
    Token readIdentifier();
    Token readNumber();
    Token readString();
    TokenType checkKeyword(const std::string& word);
    static std::unordered_map<std::string, TokenType> keywords;
};

#endif
