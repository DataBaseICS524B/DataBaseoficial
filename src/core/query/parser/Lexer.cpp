#include "Lexer.h"
#include <cctype>
#include <algorithm>

std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"SELECT", TokenType::SELECT},
    {"FROM", TokenType::FROM},
    {"WHERE", TokenType::WHERE},
    {"INSERT", TokenType::INSERT},
    {"INTO", TokenType::INTO},
    {"VALUES", TokenType::VALUES},
    {"UPDATE", TokenType::UPDATE},
    {"SET", TokenType::SET},
    {"DELETE", TokenType::DELETE},
    {"CREATE", TokenType::CREATE},
    {"DROP", TokenType::DROP},
    {"DATABASE", TokenType::DATABASE},
    {"TABLE", TokenType::TABLE},
    {"IF", TokenType::IF},
    {"NOT", TokenType::NOT},
    {"EXISTS", TokenType::EXISTS},
    {"AND", TokenType::AND},
    {"OR", TokenType::OR},
    {"ARRAY", TokenType::ARRAY}  // НОВОЕ КЛЮЧЕВОЕ СЛОВО
};

Lexer::Lexer(const std::string& source)
    : source(source), position(0), line(1), column(1) {}

char Lexer::currentChar() {
    if (position >= source.length()) return '\0';
    return source[position];
}

void Lexer::advance() {
    if (currentChar() == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    position++;
}

void Lexer::skipWhitespace() {
    while (std::isspace(currentChar())) {
        advance();
    }
}

TokenType Lexer::checkKeyword(const std::string& word) {
    std::string upper = word;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    auto it = keywords.find(upper);
    if (it != keywords.end()) return it->second;
    return TokenType::IDENTIFIER;
}

Token Lexer::readIdentifier() {
    std::string result;
    int startLine = line, startCol = column;
    while (std::isalnum(currentChar()) || currentChar() == '_') {
        result += currentChar();
        advance();
    }
    return Token(checkKeyword(result), result, startLine, startCol);
}

Token Lexer::readNumber() {
    std::string result;
    int startLine = line, startCol = column;
    bool hasDecimal = false;
    while (std::isdigit(currentChar()) || currentChar() == '.') {
        if (currentChar() == '.') {
            if (hasDecimal) break;
            hasDecimal = true;
        }
        result += currentChar();
        advance();
    }
    return Token(TokenType::NUMBER, result, startLine, startCol);
}

Token Lexer::readString() {
    std::string result;
    int startLine = line, startCol = column;
    advance();
    while (currentChar() != '\'' && currentChar() != '\0') {
        result += currentChar();
        advance();
    }
    if (currentChar() == '\'') advance();
    return Token(TokenType::STRING, result, startLine, startCol);
}

Token Lexer::nextToken() {
    skipWhitespace();
    int startLine = line, startCol = column;
    char c = currentChar();

    if (c == '\0') return Token(TokenType::END_OF_FILE, "", startLine, startCol);

    // Операторы
    if (c == '=') { advance(); return Token(TokenType::EQ, "=", startLine, startCol); }
    if (c == '>') {
        advance();
        if (currentChar() == '=') { advance(); return Token(TokenType::GTE, ">=", startLine, startCol); }
        return Token(TokenType::GT, ">", startLine, startCol);
    }
    if (c == '<') {
        advance();
        if (currentChar() == '=') { advance(); return Token(TokenType::LTE, "<=", startLine, startCol); }
        if (currentChar() == '>') { advance(); return Token(TokenType::NE, "<>", startLine, startCol); }
        return Token(TokenType::LT, "<", startLine, startCol);
    }

    // Спецсимволы
    if (c == '*') { advance(); return Token(TokenType::STAR, "*", startLine, startCol); }
    if (c == ',') { advance(); return Token(TokenType::COMMA, ",", startLine, startCol); }
    if (c == ';') { advance(); return Token(TokenType::SEMICOLON, ";", startLine, startCol); }
    if (c == '(') { advance(); return Token(TokenType::LPAREN, "(", startLine, startCol); }
    if (c == ')') { advance(); return Token(TokenType::RPAREN, ")", startLine, startCol); }
    if (c == '.') { advance(); return Token(TokenType::DOT, ".", startLine, startCol); }
    if (c == '[') { advance(); return Token(TokenType::LBRACKET, "[", startLine, startCol); }
    if (c == ']') { advance(); return Token(TokenType::RBRACKET, "]", startLine, startCol); }   // НОВОЕ

    // Идентификаторы и числа
    if (std::isalpha(c) || c == '_') return readIdentifier();
    if (std::isdigit(c)) return readNumber();
    if (c == '\'') return readString();

    advance();
    return Token(TokenType::UNKNOWN, std::string(1, c), startLine, startCol);
}

Token Lexer::peekToken() {
    size_t savedPos = position;
    int savedLine = line, savedCol = column;
    Token token = nextToken();
    position = savedPos;
    line = savedLine;
    column = savedCol;
    return token;
}

std::vector<Token> Lexer::tokenizeAll() {
    std::vector<Token> tokens;
    Token token = nextToken();
    while (token.type != TokenType::END_OF_FILE) {
        tokens.push_back(token);
        token = nextToken();
    }
    tokens.push_back(token);
    return tokens;
}