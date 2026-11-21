#pragma once
#include <cstdint>
#include "memory.hpp"
#include "stack.hpp"

enum class Op : Byte {
    Halt = 0x00, Nop = 0x01, Out = 0x02, OutN = 0x03, OutS = 0x04,
    Add = 0x10, Sub = 0x11, And = 0x12, Or = 0x13, Xor = 0x14, Not = 0x15,
    Shl = 0x16, Shr = 0x17, Inc = 0x18, Dec = 0x19, Cmp = 0x1A, Mul = 0x1B,
    LoadIA = 0x20, LoadIB = 0x21, LoadA = 0x22, LoadB = 0x23,
    StoreA = 0x24, StoreB = 0x25, MovAB = 0x26, MovBA = 0x27,
    LoadH = 0x28, LoadAH = 0x29, StoreHA = 0x2A, IncH = 0x2B, DecH = 0x2C, HLow = 0x2D,
    Jmp = 0x30, Jz = 0x31, Jnz = 0x32,
    PushA = 0x50, PopA = 0x51, PushB = 0x52, PopB = 0x53, Call = 0x54, Ret = 0x55,
    Cls = 0x40, Plot = 0x41, Show = 0x43
};

struct Flags {
    bool z = false;
    bool n = false;
    bool c = false;
};

struct CPU {
    Memory* mem = nullptr;
    std::uint16_t pc = 0;
    std::uint16_t h = 0;
    Byte a = 0;
    Byte b = 0;
    Flags f;
    Stack stack;
    bool halted = false;
};

struct Sprite {
    std::uint8_t x, y;
    std::int8_t vx, vy;
    bool alive;
};

void step(CPU& cpu);
void run(CPU& cpu);
void dump_regs(const CPU& cpu);

struct DecodedInstruction {
    Byte group;
    Byte index;
};

DecodedInstruction decode(Byte op);