#include "../parser/Parser.h"
#include <iostream>

void test(const std::string& sql) {
    std::cout << "\n=== Testing: " << sql << " ===" << std::endl;
    Parser parser(sql);
    auto query = parser.parse();
    if (query) {
        std::cout << "✓ Parsing successful! Type: " << (int)query->type << std::endl;
    } else {
        std::cout << "✗ Failed: " << parser.getLastError() << std::endl;
    }
}

int main() {
    std::cout << "=== Parser Tests ===" << std::endl;

    test("CREATE DATABASE mydb;");
    test("CREATE TABLE users (id INT PRIMARY KEY, name STRING, age INT);");
    test("SELECT * FROM users;");
    test("SELECT name, age FROM users WHERE age > 18;");
    test("INSERT INTO users (name, age) VALUES ('Alice', 25);");
    test("UPDATE users SET age = 26 WHERE name = 'Alice';");
    test("DELETE FROM users WHERE age < 18;");
    test("DROP DATABASE mydb;");

    return 0;
}
