#pragma once
#include "lib.h"

// user and system share the same registers
enum Cpu_mode 
{
    FIQ = 0,SUPERVISOR,ABORT,
    IRQ, UNDEFINED, USER, SYSTEM
};

enum Access_type
{
    BYTE = 0,HALF,WORD
};

constexpr int R0 = 0;
constexpr int R1 = 1;
constexpr int R2 = 2;
constexpr int R3 = 3;
constexpr int R4 = 4;
constexpr int R5 = 5;
constexpr int R6 = 6;
constexpr int R7 = 7;
constexpr int R8 = 8;
constexpr int R9 = 9;
constexpr int R10 = 10;
constexpr int R11 = 11;
constexpr int R12 = 12;
constexpr int SP = 13; // stack pointer
constexpr int LR = 14; // return address (link register)
constexpr int PC = 15; // program counter

constexpr int ARM_WORD_SIZE = 4; 


// register names
extern const char *user_regs_names[16];

extern const char *fiq_banked_names[6];

extern const char *hi_banked_names[5][2];

extern const char *status_banked_names[5];


// instr decoding
constexpr int L_BIT =  20;





// flags
constexpr int N_BIT = 31; // negative
constexpr int Z_BIT = 30; // zero
constexpr int C_BIT = 29; // carry
constexpr int V_BIT = 28; // overflow


enum Arm_cond
{
    EQ=0,NE=1,CS=2,CC=3,MI=4,PL=5,VS=6,
    VC=7,HI=8,LS=9,GE=10,LT=11,GT=12,LE=13,AL=14
};



inline uint32_t get_arm_opcode_bits(uint32_t instr)
{
    return ((instr >> 4) & 0xf) | ((instr >> 16) & 0xff0);    
}

// operand two immediate is produced
// with a 8 bit imm rotated right
// by a shift value * 2
inline uint32_t get_arm_operand2_imm(uint32_t opcode)
{
    int imm = opcode & 0xff;
    const int shift = (opcode >> 8) & 0xf;
    imm = rotr(imm,shift*2);
    return imm;    
}
