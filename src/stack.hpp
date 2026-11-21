#pragma once
#include <cstdint>
#include "memory.hpp"

struct Stack {
    std::uint16_t sp = 0xFFF;
};

bool push(Stack& s, Memory& mem, Byte value);
bool pop(Stack& s, const Memory& mem, Byte& out);
void dump_stack(const Stack& s, const Memory& mem);
void stack_reset(Stack& s);
std::uint16_t stack_sp(const Stack& s);
