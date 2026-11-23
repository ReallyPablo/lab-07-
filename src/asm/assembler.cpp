#include "assembler.hpp"

#include <cctype>
#include <map>

#include "../cpu.hpp"

namespace {

enum class OperandKind { RegA, RegB, RegH, Value, MemValue, MemH };

struct Operand {
    OperandKind kind = OperandKind::Value;
    bool is_symbol = false;
    long value = 0;
    std::string symbol;
};

char sig_of(const Operand& o) {
    switch (o.kind) {
    case OperandKind::RegA:     return 'A';
    case OperandKind::RegB:     return 'B';
    case OperandKind::RegH:     return 'H';
    case OperandKind::Value:    return 'n';
    case OperandKind::MemValue: return 'N';
    case OperandKind::MemH:     return 'M';
    }
    return '?';
}

struct Form {
    const char* mnemonic;
    const char* sig;
    Op op;
};

const Form FORMS[] = {
    {"HALT", "", Op::Halt}, {"NOP", "", Op::Nop},
    {"OUT", "", Op::Out},   {"OUTN", "", Op::OutN}, {"OUTS", "n", Op::OutS},

    {"ADD", "A,B", Op::Add}, {"SUB", "A,B", Op::Sub}, {"AND", "A,B", Op::And},
    {"OR", "A,B", Op::Or},   {"XOR", "A,B", Op::Xor}, {"MUL", "A,B", Op::Mul},
    {"CMP", "A,B", Op::Cmp},
    {"NOT", "A", Op::Not}, {"SHL", "A", Op::Shl}, {"SHR", "A", Op::Shr},
    {"INC", "A", Op::Inc}, {"INC", "H", Op::IncH},
    {"DEC", "A", Op::Dec}, {"DEC", "H", Op::DecH},

    {"LOADI", "A,n", Op::LoadIA}, {"LOADI", "B,n", Op::LoadIB}, {"LOADI", "H,n", Op::LoadH},
    {"LOAD", "A,N", Op::LoadA},   {"LOAD", "B,N", Op::LoadB},   {"LOAD", "A,M", Op::LoadAH},
    {"STORE", "N,A", Op::StoreA}, {"STORE", "N,B", Op::StoreB}, {"STORE", "M,A", Op::StoreHA},
    {"MOV", "A,B", Op::MovAB},    {"MOV", "B,A", Op::MovBA},
    {"HLOW", "", Op::HLow},

    {"JMP", "n", Op::Jmp}, {"JZ", "n", Op::Jz}, {"JNZ", "n", Op::Jnz},

    {"CLS", "", Op::Cls}, {"PLOT", "", Op::Plot}, {"SHOW", "", Op::Show},

    {"PUSH", "A", Op::PushA}, {"PUSH", "B", Op::PushB},
    {"POP", "A", Op::PopA},   {"POP", "B", Op::PopB},
    {"CALL", "n", Op::Call},  {"RET", "", Op::Ret},
};

struct Line {
    int line = 0;
    std::string label;
    std::string mnemonic;
    std::vector<Operand> ops;
    const Form* form = nullptr;
    std::size_t size = 0;
};

struct Failure {
    std::string message;
};

[[noreturn]] void fail(int line, const std::string& msg) {
    throw Failure{"line " + std::to_string(line) + ": " + msg};
}

std::string upper(std::string s) {
    for (char& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

std::string describe(const Token* t) {
    switch (t->kind) {
    case Token::Kind::Newline: return "end of line";
    case Token::Kind::Eof:     return "end of file";
    default:                   return "'" + t->text + "'";
    }
}

bool at_end_of_line(const Token* t) {
    return t->kind == Token::Kind::Newline || t->kind == Token::Kind::Eof;
}

long number_value(const Token* t) {
    const std::string& s = t->text;
    int base = 10;
    std::size_t i = 0;
    if (s.size() > 1 && s[0] == '0' && s[1] == 'x') { base = 16; i = 2; }
    else if (s.size() > 1 && s[0] == '0' && s[1] == 'b') { base = 2; i = 2; }
    long v = 0;
    for (; i < s.size(); ++i) {
        char c = s[i];
        int d = std::isdigit((unsigned char)c) ? c - '0' : std::tolower((unsigned char)c) - 'a' + 10;
        v = v * base + d;
        if (v > 0xFFFF) fail(t->line, "number '" + s + "' is too large (max 65535)");
    }
    return v;
}

Operand parse_operand(const Token*& t) {
    Operand o;
    if (t->kind == Token::Kind::Ident) {
        if (t->text == "A") o.kind = OperandKind::RegA;
        else if (t->text == "B") o.kind = OperandKind::RegB;
        else if (t->text == "H") o.kind = OperandKind::RegH;
        else { o.is_symbol = true; o.symbol = t->text; }
        t = t->next;
    } else if (t->kind == Token::Kind::Number) {
        o.value = number_value(t);
        t = t->next;
    } else if (t->kind == Token::Kind::LBracket) {
        int line = t->line;
        t = t->next;
        if (t->kind == Token::Kind::Ident && t->text == "H") {
            o.kind = OperandKind::MemH;
        } else if (t->kind == Token::Kind::Number) {
            o.kind = OperandKind::MemValue;
            o.value = number_value(t);
        } else {
            fail(line, "expected a number or H after '[', got " + describe(t));
        }
        t = t->next;
        if (t->kind != Token::Kind::RBracket) fail(line, "expected ']', got " + describe(t));
        t = t->next;
    } else {
        fail(t->line, "expected an operand, got " + describe(t));
    }
    return o;
}

std::vector<Line> parse(const Token* t) {
    std::vector<Line> lines;
    while (t->kind != Token::Kind::Eof) {
        if (t->kind == Token::Kind::Newline) { t = t->next; continue; }

        Line ln;
        ln.line = t->line;

        if (t->kind == Token::Kind::Ident && t->next->kind == Token::Kind::Colon) {
            if (t->text == "A" || t->text == "B" || t->text == "H")
                fail(t->line, "'" + t->text + "' is a register, not a label name");
            ln.label = t->text;
            t = t->next->next;
        }

        if (!at_end_of_line(t)) {
            if (t->kind != Token::Kind::Ident) fail(t->line, "expected an instruction, got " + describe(t));
            ln.mnemonic = upper(t->text);
            t = t->next;
            if (!at_end_of_line(t)) {
                while (true) {
                    ln.ops.push_back(parse_operand(t));
                    if (t->kind == Token::Kind::Comma) {
                        t = t->next;
                        if (at_end_of_line(t)) fail(ln.line, "operand expected after ','");
                    } else if (at_end_of_line(t)) {
                        break;
                    } else {
                        fail(t->line, "expected ',' or end of line, got " + describe(t));
                    }
                }
            }
        }
        lines.push_back(std::move(ln));
    }
    return lines;
}

std::string sig_text(const std::vector<Operand>& ops) {
    std::string s;
    for (std::size_t i = 0; i < ops.size(); ++i) {
        if (i) s += ',';
        s += sig_of(ops[i]);
    }
    return s;
}

const Form* match_form(const Line& ln) {
    std::string want = sig_text(ln.ops);
    bool known = false;
    for (const Form& f : FORMS) {
        if (ln.mnemonic != f.mnemonic) continue;
        known = true;
        if (want == f.sig) return &f;
    }
    if (!known) fail(ln.line, "unknown instruction '" + ln.mnemonic + "'");
    fail(ln.line, "wrong operands for " + ln.mnemonic);
}

long resolve(const Operand& o, int line, const std::map<std::string, std::uint16_t>& labels) {
    if (!o.is_symbol) return o.value;
    auto it = labels.find(o.symbol);
    if (it == labels.end()) fail(line, "unknown label '" + o.symbol + "'");
    return it->second;
}

}  // namespace

bool assemble(const Token* head, std::vector<Byte>& out, std::string& error) {
    try {
        std::vector<Line> lines = parse(head);

        std::map<std::string, std::uint16_t> labels;
        std::map<std::string, int> label_line;
        std::size_t addr = 0;
        for (Line& ln : lines) {
            if (!ln.label.empty()) {
                auto dup = label_line.find(ln.label);
                if (dup != label_line.end())
                    fail(ln.line, "label '" + ln.label + "' already defined on line " +
                                      std::to_string(dup->second));
                labels[ln.label] = (std::uint16_t)addr;
                label_line[ln.label] = ln.line;
            }
            if (ln.mnemonic.empty()) continue;

            ln.form = match_form(ln);
            ln.size = op_size(ln.form->op);
            addr += ln.size;
            if (addr > MEM_SIZE) fail(ln.line, "program is too large");
        }

        out.clear();
        for (const Line& ln : lines) {
            if (ln.mnemonic.empty()) continue;

            out.push_back((Byte)ln.form->op);
            std::size_t operand_bytes = ln.size - 1;
            for (const Operand& o : ln.ops) {
                char s = sig_of(o);
                if (s != 'n' && s != 'N') continue;
                long v = resolve(o, ln.line, labels);
                if (operand_bytes == 1) {
                    if (v > 255)
                        fail(ln.line, "value " + std::to_string(v) + " does not fit in a byte (0..255)");
                    out.push_back((Byte)v);
                } else {
                    out.push_back((Byte)(v & 0xFF));
                    out.push_back((Byte)(v >> 8));
                }
            }
        }
        return true;
    } catch (const Failure& f) {
        error = f.message;
        out.clear();
        return false;
    }
}