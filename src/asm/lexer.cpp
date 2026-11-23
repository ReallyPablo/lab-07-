// lexer.cpp — ДАНО, крім number(). Прочитайте цикл у lex() згори донизу:
// це псевдокод з теорії §1, записаний на C++.
#include "lexer.hpp"

#include <cctype>

// Стан лексера: текст, курсор, номер рядка і список, який ми будуємо.
struct Lexer {
    const std::string& src;
    std::size_t i = 0;
    int line = 1;
    Token* head = nullptr;
    Token* tail = nullptr;
    std::string error;  // непорожній, щойно лексер щось відхилив
};

// Додати вузол у кінець списку (Notes 08 §3).
static void emit(Lexer& lx, Token::Kind kind, const std::string& text) {
    Token* t = new Token{kind, text, lx.line, nullptr};
    if (!lx.head) {
        lx.head = lx.tail = t;
    } else {
        lx.tail->next = t;
        lx.tail = t;
    }
}

// Записати помилку з номером рядка. Повертає false, щоб зручно писати
// `return fail(lx, "...");`
static bool fail(Lexer& lx, const std::string& msg) {
    lx.error = "line " + std::to_string(lx.line) + ": " + msg;
    return false;
}

void destroy(Token* head) {
    while (head) {
        Token* next = head->next;
        delete head;
        head = next;
    }
}

const char* kind_name(Token::Kind kind) {
    switch (kind) {
    case Token::Kind::Ident:    return "Ident";
    case Token::Kind::Number:   return "Number";
    case Token::Kind::Comma:    return "Comma";
    case Token::Kind::Colon:    return "Colon";
    case Token::Kind::LBracket: return "LBracket";
    case Token::Kind::RBracket: return "RBracket";
    case Token::Kind::Newline:  return "Newline";
    case Token::Kind::Eof:      return "Eof";
    }
    return "?";
}

// ВАШЕ (Lab 8, M1). Курсор lx.i стоїть на цифрі.
// З'їсти число — `0x` і шістнадцяткові цифри, `0b` і двійкові, або десяткові —
// і видати його: emit(lx, Token::Kind::Number, текст_числа).
// Зіпсоване число — `0xGG`, `0x` без жодної цифри, `12ab` — це
// `return fail(lx, "bad number");`, а не тихий нуль.
static bool number(Lexer& lx) {
    const std::string& s = lx.src;
    std::size_t start = lx.i;
    std::size_t j = start;
    int base = 10;
    if (s[j] == '0' && j + 1 < s.size() && s[j + 1] == 'x') { base = 16; j += 2; }
    else if (s[j] == '0' && j + 1 < s.size() && s[j + 1] == 'b') { base = 2; j += 2; }

    std::size_t digits = j;
    while (j < s.size()) {
        char c = s[j];
        bool ok = (base == 16) ? std::isxdigit((unsigned char)c) != 0
                : (base == 2)  ? (c == '0' || c == '1')
                               : std::isdigit((unsigned char)c) != 0;
        if (!ok) break;
        ++j;
    }

    bool glued = j < s.size() && (std::isalnum((unsigned char)s[j]) || s[j] == '_');
    if (j == digits || glued) {
        while (j < s.size() && (std::isalnum((unsigned char)s[j]) || s[j] == '_')) ++j;
        return fail(lx, "bad number '" + s.substr(start, j - start) + "'");
    }

    emit(lx, Token::Kind::Number, s.substr(start, j - start));
    lx.i = j;
    return true;
}

Token* lex(const std::string& src, std::string& error) {
    Lexer lx{src, 0, 1, nullptr, nullptr, ""};
    while (lx.i < src.size()) {
        char c = src[lx.i];
        if (c == ' ' || c == '\t' || c == '\r') {
            ++lx.i;                                   // пробіли не токени
        } else if (c == ';') {
            while (lx.i < src.size() && src[lx.i] != '\n') ++lx.i;  // коментар
        } else if (c == '\n') {
            emit(lx, Token::Kind::Newline, "\\n");
            ++lx.line;
            ++lx.i;
        } else if (std::isalpha((unsigned char)c) || c == '_') {
            std::size_t start = lx.i;
            while (lx.i < src.size() &&
                   (std::isalnum((unsigned char)src[lx.i]) || src[lx.i] == '_')) {
                ++lx.i;
            }
            emit(lx, Token::Kind::Ident, src.substr(start, lx.i - start));
        } else if (std::isdigit((unsigned char)c)) {
            if (!number(lx)) break;
        } else if (c == ',') { emit(lx, Token::Kind::Comma, ",");    ++lx.i; }
        else if (c == ':')   { emit(lx, Token::Kind::Colon, ":");    ++lx.i; }
        else if (c == '[')   { emit(lx, Token::Kind::LBracket, "["); ++lx.i; }
        else if (c == ']')   { emit(lx, Token::Kind::RBracket, "]"); ++lx.i; }
        else {
            fail(lx, std::string("unexpected character '") + c + "'");
            break;
        }
    }
    if (!lx.error.empty()) {
        destroy(lx.head);
        error = lx.error;
        return nullptr;
    }
    emit(lx, Token::Kind::Eof, "");
    return lx.head;
}