#include <iostream>
#include <stdio.h>
#include <stdlib.h>

// https://www.princeton.edu/~mae412/HANDOUTS/Datasheets/6502.pdf

using Byte = unsigned char;
using Word = unsigned short;
using u32 = unsigned int;

struct Memory {
    static constexpr u32 MAX_MEM = 1024 * 64; // 64 KB memory
    Byte Data[MAX_MEM];

    void initialize() {
        for (u32 i = 0; i < MAX_MEM; i++) {
            Data[i] = 0;
        }
    }

    // read one byte
    Byte operator[](u32 address) const {
        return Data[address];
    }

    Byte& operator[](u32 address) {
        return Data[address];
    }

    void writeWord(u32& cycles, Word value, u32 address) {
        Data[address] = value & 0xFF;
        Data[address + 1] = (value >> 8);
        cycles -= 2;
    }
};

struct CPU {
    // opcodes
    static constexpr Byte
        INS_LDA_IM = 0xA9,
        INS_LDA_ZP = 0xA5,
        INS_LDA_ZPX = 0xB5,
        INS_JSR = 0x20;

    Word programCounter;
    Word stackPointer;
    Byte accumulator, indexRegisterX, indexRegisterY;

    Byte C : 1; // carry status flag
    Byte Z : 1; // zero status flag
    Byte I : 1; // interrupt (IRQ) status flag
    Byte D : 1; // decimal mode status flag
    Byte B : 1; // break command status flag
    Byte V : 1; // overflow status flag
    Byte N : 1; // negative status flag

    void reset(Memory& memory) {
        programCounter = 0xFFFC;
        stackPointer = 0x0100;
        C = Z = I = D = B = V = N = 0;
        accumulator = indexRegisterX = indexRegisterY = 0;
        memory.initialize();
    }

    Byte fetchByte(u32& cycles, Memory& memory) {
        Byte data =  memory[programCounter];
        programCounter++;
        cycles--;

        return data;
    }

    Word fetchWord(u32& cycles, Memory& memory) {
        // 6502 is little endian
        Word data = memory[programCounter];
        programCounter++;

        data |= (memory[programCounter] << 8);
        programCounter++;
        cycles -= 2;

        return data;
    }

    void LDASetStatusFlag() {
        Z = (accumulator == 0);
        N = (accumulator & 0b10000000) > 0;
    }

    Byte readByte(u32& cycles, Byte address, Memory& memory) {
        Byte data =  memory[address];
        cycles--;

        return data;
    }

    void execute(u32 cycles, Memory& memory) {
        while (cycles > 0) {
            Byte instruction = fetchByte(cycles, memory);

            switch(instruction) {
                case INS_LDA_IM: {
                    Byte value = fetchByte(cycles, memory);
                    accumulator = value;
                    LDASetStatusFlag();
                } break;
                case INS_LDA_ZP: {
                    Byte zeroPageAddress = fetchByte(cycles, memory);
                    Byte value = readByte(cycles, zeroPageAddress, memory);
                    accumulator = value;
                    LDASetStatusFlag();
                } break;
                case INS_LDA_ZPX: {
                    Byte zeroPageAddress = fetchByte(cycles, memory);
                    zeroPageAddress += indexRegisterX;
                    cycles--;
                    Byte value = readByte(cycles, zeroPageAddress, memory);
                    accumulator = value;
                    LDASetStatusFlag();

                } break;
                case INS_JSR: {
                    Word subRoutineAddress = fetchWord(cycles, memory);
                    memory.writeWord(cycles, programCounter - 1, stackPointer);
                    programCounter = subRoutineAddress;
                    cycles--;
                } break;
                default: {
                    printf("Instruction not handled %d", instruction);
                }
            }
        }
    }

};
int main() {
//    std::cout << "Hello, World!" << std::endl;
    Memory memory;
    CPU cpu;
    cpu.reset(memory);
    memory[0xFFFC] = CPU::INS_JSR;
    memory[0xFFFD] = 0x42;
    memory[0xFFFE] = 0x42;
    memory[0x4242] = CPU::INS_LDA_IM;
    memory[0x4243] = 0x84;
    cpu.execute(9,memory);
    return 0;
}
