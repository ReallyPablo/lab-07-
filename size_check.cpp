#include <cstddef>
#include <iostream>
#include "src/cpu.hpp"

int main() {
    std::cout << "CPU " << sizeof(CPU) << "\nFlags " << sizeof(Flags)
              << "\nSprite " << sizeof(Sprite) << '\n';
    std::cout << "mem " << offsetof(CPU, mem) << "\npc " << offsetof(CPU, pc)
              << "\nh " << offsetof(CPU, h) << "\na " << offsetof(CPU, a)
              << "\nb " << offsetof(CPU, b) << "\nf " << offsetof(CPU, f)
              << "\nhalted " << offsetof(CPU, halted) << '\n';
}