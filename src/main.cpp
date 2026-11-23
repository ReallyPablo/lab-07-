#include <iostream>
#include <sstream>
#include <string>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <vector>
#include "asm/assembler.hpp"
#include "asm/lexer.hpp"
#include "cpu.hpp"
#include "display.hpp"
#include "dump.hpp"
#include "memory.hpp"
#include "stack.hpp"

static bool parse_number(const std::string& word, long& out) {
    try {
        std::size_t used = 0;
        out = std::stol(word, &used, 0);
        return used == word.size();
    } catch (...) {
        return false;
    }
}

static void print_help() {
    std::cout << "commands:\n"
              << "  dump              print all " << MEM_SIZE << " bytes\n"
              << "  get <addr>        show one byte four ways\n"
              << "  set <addr> <val>  write one byte (dec or 0x hex)\n"
              << "  inc <addr>        add 1 to the byte at addr (wraps)\n"
              << "  show              draw the screen\n"
              << "  frame             draw a frame on the screen\n"
              << "  stack             show SP and the top of the stack\n"
              << "  help              this list\n"
              << "  quit              leave\n";
}

static bool read_file(const std::string& path, std::string& text) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream buf;
    buf << in.rdbuf();
    text = buf.str();
    return true;
}

static bool build(const std::string& path, std::vector<Byte>& bytes) {
    std::string text, error;
    if (!read_file(path, text)) {
        std::cerr << "error: cannot read " << path << '\n';
        return false;
    }
    Token* tokens = lex(text, error);
    if (!tokens) {
        std::cerr << path << ": " << error << '\n';
        return false;
    }
    bool ok = assemble(tokens, bytes, error);
    destroy(tokens);
    if (!ok) {
        std::cerr << path << ": " << error << '\n';
        return false;
    }
    return true;
}

static int cmd_lex(const std::string& path) {
    std::string text, error;
    if (!read_file(path, text)) {
        std::cerr << "error: cannot read " << path << '\n';
        return 1;
    }
    Token* tokens = lex(text, error);
    if (!tokens) {
        std::cerr << path << ": " << error << '\n';
        return 1;
    }
    for (const Token* t = tokens; t; t = t->next) {
        std::cout << std::setw(3) << t->line << "  " << kind_name(t->kind);
        if (t->kind == Token::Kind::Ident || t->kind == Token::Kind::Number)
            std::cout << '(' << t->text << ')';
        std::cout << '\n';
    }
    destroy(tokens);
    return 0;
}

static int cmd_asm(const std::string& path) {
    std::vector<Byte> bytes;
    if (!build(path, bytes)) return 1;
    std::cout << "load 0x000";
    for (Byte b : bytes)
        std::cout << " 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    std::cout << std::dec << std::setfill(' ') << '\n' << bytes.size() << " bytes\n";
    return 0;
}

static int cmd_run(const std::string& path) {
    std::vector<Byte> bytes;
    if (!build(path, bytes)) return 1;
    Memory mem;
    CPU cpu;
    cpu.mem = &mem;
    for (std::size_t i = 0; i < bytes.size(); ++i) mem_set(mem, i, bytes[i]);
    run(cpu);
    return 0;
}

static int usage() {
    std::cerr << "usage: ember\n"
                 "       ember file.asm\n"
                 "       ember lex file.asm\n"
                 "       ember asm file.asm\n";
    return 2;
}
static Sprite sprites[8]{};

static void tick(Memory& mem) {
    for (auto& s : sprites) {
        if (!s.alive) continue;
        int nx = s.x + s.vx, ny = s.y + s.vy;
        if (nx < 0 || nx > 63) { s.vx = -s.vx; nx = s.x + s.vx; }
        if (ny < 0 || ny > 31) { s.vy = -s.vy; ny = s.y + s.vy; }
        s.x = (std::uint8_t)std::clamp(nx, 0, 63);
        s.y = (std::uint8_t)std::clamp(ny, 0, 31);
    }
    clear(mem);
    for (auto& s : sprites) {
        if (s.alive) plot(mem, s.x, s.y);
    }
    show(mem);
}

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "lex") return cmd_lex(argv[2]);
    if (argc == 3 && std::string(argv[1]) == "asm") return cmd_asm(argv[2]);
    if (argc == 2) return cmd_run(argv[1]);
    if (argc != 1) return usage();

    Memory mem;
    CPU cpu;
    cpu.mem = &mem;

    std::cout << "ember 0.1 - 4096 bytes of memory you can see. Type `help`.\n";

    std::string line;
    while (true) {
        std::cout << "ember> ";

        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;
        }

        std::istringstream words(line);
        std::string cmd;
        words >> cmd;

        if (cmd.empty()) {
            continue;
        } else if (cmd == "quit" || cmd == "exit") {
            break;
        } else if (cmd == "help") {
            print_help();
        } else if (cmd == "dump") {
            dump(mem);
        } else if (cmd == "get") {
            std::string a;
            long addr = 0;
            if (!(words >> a) || !parse_number(a, addr)) {
                std::cout << "usage: get <addr>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else {
                show_byte(mem_get(mem, static_cast<std::size_t>(addr)));
            }
        } else if (cmd == "set") {
            std::string a, v;
            long addr = 0, value = 0;
            if (!(words >> a) || !(words >> v) || !parse_number(a, addr) ||
                !parse_number(v, value)) {
                std::cout << "usage: set <addr> <value>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else if (value < 0 || value > 255) {
                std::cout << "a byte is 0..255, got " << value << '\n';
            } else if (!mem_set(mem, static_cast<std::size_t>(addr),
                                static_cast<Byte>(value))) {
                std::cout << "address " << addr << " is outside 0.." << MEM_SIZE - 1
                          << '\n';
            }
        } else if (cmd == "inc") {
            std::string a;
            long addr = 0;
            if (!(words >> a) || !parse_number(a, addr)) {
                std::cout << "usage: inc <addr>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else {
                std::size_t address = static_cast<std::size_t>(addr);
                Byte value = mem_get(mem, address);
                mem_set(mem, address, static_cast<Byte>(value + 1));
            }
        } else if (cmd == "reg") {
            std::string r, v;
            long value = 0;
            if (!(words >> r) || !(words >> v) || !parse_number(v, value) || value < 0 || value > 255) {
                std::cout << "usage: reg <a|b> <0..255>\n";
            } else if (r == "a") {
                cpu.a = (Byte)value;
            } else if (r == "b") {
                cpu.b = (Byte)value;
            } else {
                std::cout << "unknown register: " << r << '\n';
            }
        } else if (cmd == "stack") {
            dump_stack(cpu.stack, mem);
        } else if (cmd == "regs") {
            dump_regs(cpu);
        } else if (cmd == "step") {
            step(cpu);
        } else if (cmd == "get16" || cmd == "set16") {
            std::string a, v;
            long addr = 0, value = 0;
            bool ok = (bool)(words >> a) && parse_number(a, addr) && addr >= 0;
            if (ok && cmd == "set16") {
                ok = (bool)(words >> v) && parse_number(v, value) && value >= 0 && value <= 0xFFFF;
            }
            if (!ok) {
                std::cout << "usage: get16 <addr> | set16 <addr> <0..65535>\n";
            } else if (cmd == "get16") {
                std::cout << get16(mem, (std::size_t)addr) << '\n';
            } else if (!set16(mem, (std::size_t)addr, (std::uint16_t)value)) {
                std::cout << "address out of range\n";
            }
        } else if (cmd == "load") {
            std::string a, w;
            long addr = 0, value = 0;
            if (!(words >> a) || !parse_number(a, addr) || addr < 0) {
                std::cout << "usage: load <addr> <byte> <byte> ...\n";
            } else {
                while (words >> w) {
                    if (!parse_number(w, value) || value < 0 || value > 255 ||
                        !mem_set(mem, (std::size_t)addr, (Byte)value)) {
                        std::cout << "bad byte or address at " << addr << '\n';
                        break;
                    }
                    ++addr;
                }
            }
        } else if (cmd == "find") {
            std::string l, h, n;
            long lo = 0, hi = 0, needle = 0;
            if (!(words >> l >> h >> n) || !parse_number(l, lo) ||
                !parse_number(h, hi) || !parse_number(n, needle) ||
                lo < 0 || hi < lo || hi > (long)MEM_SIZE ||
                needle < 0 || needle > 255) {
                std::cout << "usage: find <lo> <hi> <byte>\n";
            } else {
                std::uint16_t r = find(mem, (std::uint16_t)lo, (std::uint16_t)hi, (Byte)needle);
                if (r == hi) {
                    std::cout << "not found\n";
                } else {
                    std::cout << "found at " << r << '\n';
                }
            }
        } else if (cmd == "show") {
            show(mem);
        } else if (cmd == "frame") {
            for (int x = 0; x < 64; ++x) { plot(mem, x, 0); plot(mem, x, 31); }
            for (int y = 0; y < 32; ++y) { plot(mem, 0, y); plot(mem, 63, y); }
        } else if (cmd == "reset") {
            cpu.pc = 0;
            cpu.h = 0;
            cpu.halted = false;
            stack_reset(cpu.stack);
        } else if (cmd == "run") {
            run(cpu);
                    } else if (cmd == "sprite") {
            std::string w[5];
            long v[5];
            bool ok = true;
            for (int k = 0; k < 5; ++k) {
                if (!(words >> w[k]) || !parse_number(w[k], v[k])) { ok = false; break; }
            }
            if (!ok || v[0] < 0 || v[0] > 7 || v[1] < 0 || v[1] > 63 ||
                v[2] < 0 || v[2] > 31 || v[3] < -127 || v[3] > 127 ||
                v[4] < -127 || v[4] > 127) {
                std::cout << "usage: sprite <0..7> <x 0..63> <y 0..31> <vx> <vy>\n";
            } else {
                sprites[v[0]] = {(std::uint8_t)v[1], (std::uint8_t)v[2],
                                 (std::int8_t)v[3], (std::int8_t)v[4], true};
            }
        } else if (cmd == "tick") {
            tick(mem);
        } else {
            std::cout << "unknown command: " << cmd << " (try `help`)\n";
        }
    }

    return 0;
}