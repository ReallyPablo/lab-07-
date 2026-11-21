#include "stack.hpp"
#include <iostream>

bool push(Stack& s, Memory& mem, Byte value) {
    if (s.sp < STACK_LO) return false;
    mem_set(mem, s.sp, value);
    s.sp = (std::uint16_t)(s.sp - 1);
    return true;
}

bool pop(Stack& s, const Memory& mem, Byte& out) {
    if (s.sp >= STACK_HI) return false;
    s.sp = (std::uint16_t)(s.sp + 1);
    out = mem_get(mem, s.sp);
    return true;
}

void stack_reset(Stack& s) {
    s.sp = (std::uint16_t)STACK_HI;
}

std::uint16_t stack_sp(const Stack& s) {
    return s.sp;
}

void dump_stack(const Stack& s, const Memory& mem) {
    std::cout << "SP=" << s.sp << '\n';
    for (std::size_t a = (std::size_t)s.sp + 1; a <= STACK_HI && a < (std::size_t)s.sp + 9; ++a)
        std::cout << a << ": " << (int)mem_get(mem, a) << '\n';
}
