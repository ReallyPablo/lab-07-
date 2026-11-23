#include "cpu.hpp"
#include "alu.hpp"
#include "display.hpp"
#include <iomanip>
#include <iostream>

DecodedInstruction decode(Byte op) {
    DecodedInstruction inst;
    inst.group = (op >> 4) & 0x0F;
    inst.index = op & 0x0F;
    return inst;
}

void dump_regs(const CPU& cpu) {
    std::cout << "PC:" << cpu.pc
              << " H:" << cpu.h
              << " A:" << (int)cpu.a
              << " B:" << (int)cpu.b
              << " SP:" << stack_sp(cpu.stack)
              << " Z:" << cpu.f.z
              << " N:" << cpu.f.n
              << " C:" << cpu.f.c << '\n';
}

std::size_t op_size(Op op) {
    switch (op) {
    case Op::LoadIA:
    case Op::LoadIB:
        return 2;
    case Op::LoadA:
    case Op::LoadB:
    case Op::StoreA:
    case Op::StoreB:
    case Op::LoadH:
    case Op::Jmp:
    case Op::Jz:
    case Op::Jnz:
    case Op::Call:
    case Op::OutS:
        return 3;
    default:
        return 1;
    }
}

static void fault(CPU& cpu, const char* msg) {
    std::cout << "error: " << msg << " (PC=" << cpu.pc << ")\n";
    cpu.halted = true;
}

static void unknown(CPU& cpu, Byte op) {
    std::cout << "error: unknown opcode 0x" << std::hex << std::setw(2)
              << std::setfill('0') << (int)op << std::dec << std::setfill(' ')
              << " (PC=" << cpu.pc << ")\n";
    cpu.halted = true;
}

void step(CPU& cpu) {
    if (cpu.halted) {
        std::cout << "halted\n";
        return;
    }

    Byte raw = mem_get(*cpu.mem, cpu.pc);
    Op op = static_cast<Op>(raw);
    std::size_t size = op_size(op);

    if ((std::size_t)cpu.pc + size > MEM_SIZE) {
        fault(cpu, "instruction runs past end of memory");
        return;
    }

    Byte group = decode(raw).group;

    if (group == 0x0) {
        switch (op) {
        case Op::Halt: cpu.halted = true; break;
        case Op::Nop: break;
        case Op::Out: std::cout << (char)cpu.a << std::flush; break;
        case Op::OutN: std::cout << (int)cpu.a << ' ' << std::flush; break;
        case Op::OutS: {
            std::uint16_t addr = get16(*cpu.mem, cpu.pc + 1);
            for (int i = 0; i < 256 && (std::size_t)addr + i < MEM_SIZE; ++i) {
                Byte c = mem_get(*cpu.mem, addr + i);
                if (c == 0) break;
                std::cout << (char)c;
            }
            std::cout << std::flush;
            break;
        }
        default: unknown(cpu, raw); return;
        }
    } else if (group == 0x1) {
        switch (op) {
        case Op::Add: cpu.a = alu_add(cpu.a, cpu.b, cpu.f); break;
        case Op::Sub: cpu.a = alu_sub(cpu.a, cpu.b, cpu.f); break;
        case Op::And: cpu.a = alu_and(cpu.a, cpu.b, cpu.f); break;
        case Op::Or:  cpu.a = alu_or(cpu.a, cpu.b, cpu.f);  break;
        case Op::Xor: cpu.a = alu_xor(cpu.a, cpu.b, cpu.f); break;
        case Op::Not: cpu.a = alu_not(cpu.a, cpu.f);        break;
        case Op::Shl: cpu.a = alu_shl(cpu.a, cpu.f);        break;
        case Op::Shr: cpu.a = alu_shr(cpu.a, cpu.f);        break;
        case Op::Inc: cpu.a = alu_inc(cpu.a, cpu.f);        break;
        case Op::Dec: cpu.a = alu_dec(cpu.a, cpu.f);        break;
        case Op::Cmp: alu_sub(cpu.a, cpu.b, cpu.f);         break;
        case Op::Mul: cpu.a = alu_mul(cpu.a, cpu.b, cpu.f); break;
        default: unknown(cpu, raw); return;
        }
    } else if (group == 0x2) {
        std::uint16_t addr = (size == 3) ? get16(*cpu.mem, cpu.pc + 1) : 0;
        bool uses_abs = (op == Op::LoadA || op == Op::LoadB ||
                         op == Op::StoreA || op == Op::StoreB);
        bool uses_h = (op == Op::LoadAH || op == Op::StoreHA);

        if (uses_abs && addr >= MEM_SIZE) {
            fault(cpu, "address out of range");
            return;
        }
        if (uses_h && cpu.h >= MEM_SIZE) {
            fault(cpu, "H out of range");
            return;
        }

        switch (op) {
        case Op::LoadIA: cpu.a = mem_get(*cpu.mem, cpu.pc + 1); break;
        case Op::LoadIB: cpu.b = mem_get(*cpu.mem, cpu.pc + 1); break;
        case Op::LoadA: cpu.a = mem_get(*cpu.mem, addr); break;
        case Op::LoadB: cpu.b = mem_get(*cpu.mem, addr); break;
        case Op::StoreA: mem_set(*cpu.mem, addr, cpu.a); break;
        case Op::StoreB: mem_set(*cpu.mem, addr, cpu.b); break;
        case Op::MovAB: cpu.a = cpu.b; break;
        case Op::MovBA: cpu.b = cpu.a; break;
        case Op::LoadH: cpu.h = addr; break;
        case Op::LoadAH: cpu.a = mem_get(*cpu.mem, cpu.h); break;
        case Op::StoreHA: mem_set(*cpu.mem, cpu.h, cpu.a); break;
        case Op::IncH: cpu.h = (std::uint16_t)(cpu.h + 1); break;
        case Op::DecH: cpu.h = (std::uint16_t)(cpu.h - 1); break;
        case Op::HLow: cpu.a = (Byte)(cpu.h & 0xFF); break;
        default: unknown(cpu, raw); return;
        }
    } else if (group == 0x3) {
        std::uint16_t addr = get16(*cpu.mem, cpu.pc + 1);
        bool take = false;
        if (op == Op::Jmp) take = true;
        else if (op == Op::Jz) take = cpu.f.z;
        else if (op == Op::Jnz) take = !cpu.f.z;
        else { unknown(cpu, raw); return; }
        if (take) {
            cpu.pc = addr;
            return;
        }
    } else if (group == 0x4) {
        switch (op) {
        case Op::Cls:  clear(*cpu.mem); break;
        case Op::Plot: cpu.f.c = !plot(*cpu.mem, cpu.a, cpu.b); break;
        case Op::Show: show(*cpu.mem); break;
        default: unknown(cpu, raw); return;
        }
    } else if (group == 0x5) {
        switch (op) {
        case Op::PushA:
            if (!push(cpu.stack, *cpu.mem, cpu.a)) { fault(cpu, "stack overflow"); return; }
            break;
        case Op::PushB:
            if (!push(cpu.stack, *cpu.mem, cpu.b)) { fault(cpu, "stack overflow"); return; }
            break;
        case Op::PopA:
            if (!pop(cpu.stack, *cpu.mem, cpu.a)) { fault(cpu, "stack underflow"); return; }
            break;
        case Op::PopB:
            if (!pop(cpu.stack, *cpu.mem, cpu.b)) { fault(cpu, "stack underflow"); return; }
            break;
        case Op::Call: {
            std::uint16_t addr = get16(*cpu.mem, cpu.pc + 1);
            std::uint16_t ret = (std::uint16_t)(cpu.pc + 3);
            if (!push(cpu.stack, *cpu.mem, (Byte)(ret & 0xFF)) ||
                !push(cpu.stack, *cpu.mem, (Byte)(ret >> 8))) {
                fault(cpu, "stack overflow");
                return;
            }
            cpu.pc = addr;
            return;
        }
        case Op::Ret: {
            Byte hi = 0, lo = 0;
            if (!pop(cpu.stack, *cpu.mem, hi) || !pop(cpu.stack, *cpu.mem, lo)) {
                fault(cpu, "stack underflow");
                return;
            }
            cpu.pc = (std::uint16_t)((hi << 8) | lo);
            return;
        }
        default: unknown(cpu, raw); return;
        }
    } else {
        unknown(cpu, raw);
        return;
    }

    cpu.pc = (std::uint16_t)(cpu.pc + size);
}

void run(CPU& cpu) {
    const int LIMIT = 100000;
    int steps = 0;
    while (!cpu.halted && steps < LIMIT) {
        step(cpu);
        ++steps;
    }
    if (!cpu.halted) {
        std::cout << "\nstep limit reached, PC=" << cpu.pc << '\n';
    } else {
        std::cout << '\n';
    }
}