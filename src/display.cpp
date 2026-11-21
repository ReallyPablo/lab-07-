#include "display.hpp"
#include <iostream>

void show(const Memory& mem) {
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 64; ++x) {
            Byte cell = mem.data[VRAM_LO + y * 8 + (x / 8)];
            bool lit = (cell >> (7 - (x % 8))) & 1;
            std::cout << (lit ? '#' : '.');
        }
        std::cout << '\n';
    }
}

bool plot(Memory& mem, int x, int y) {
    if (x < 0 || x >= 64 || y < 0 || y >= 32) return false;
    mem.data[VRAM_LO + y * 8 + (x / 8)] |= (Byte)(1u << (7 - (x % 8)));
    return true;
}

void clear(Memory& mem) {
    for (std::size_t i = 0; i < VRAM_SIZE; ++i) mem.data[VRAM_LO + i] = 0;
}