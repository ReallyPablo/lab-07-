#pragma once

#include <string>

struct Token {
    enum class Kind { Ident, Number, Comma, Colon, LBracket, RBracket, Newline, Eof };
    Kind kind;
    std::string text;      // або зріз (вказівник + довжина) у вихідний текст
    int line;
    Token* next;           // nullptr у кінці
};

void destroy(Token* head);

const char* kind_name(Token::Kind kind);