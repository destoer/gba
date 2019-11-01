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

enum Shift_type
{
    LSL=0,LSR,ASR,ROR
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
constexpr int ARM_HALF_SIZE = 2;

// register names
extern const char *user_regs_names[16];

extern const char *fiq_banked_names[6];

extern const char *hi_banked_names[5][2];

extern const char *status_banked_names[5];

extern const char *mode_names[7];






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

inline uint8_t get_thumb_opcode_bits(uint16_t instr)
{
    return ((instr >> 8) & 0xff);    
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

inline Cpu_mode cpu_mode_from_bits(int v)
{
    switch(v)
    {
        case 0b10000: return USER;
        case 0b10001: return FIQ;
        case 0b10010: return IRQ;
        case 0b10011: return SUPERVISOR;
        case 0b10111: return ABORT;
        case 0b11011: return UNDEFINED;
        case 0b11111: return SYSTEM;
        default:
        { 
            printf("unknown mode from bits: %08x\n",v);
            exit(1);
        }
    }
}



// TODO these aint handling edge cases
// with large shifts
// http://www.keil.com/support/man/docs/armasm/armasm_dom1361289852998.htm


inline uint32_t lsl(uint32_t v, uint32_t n, bool &carry)
{
    if(!n) return v;

    if(n >= 32)
    {

        if(n >= 33)
        {
            carry = false;
        }

        else
        {
            carry = is_set(v, 0);
        }


        return 0;
    }


    carry = is_set(v, 32-n);

    return v << n;
}

inline uint32_t lsr(uint32_t v, uint32_t n, bool &carry)
{
    if(!n) return v;

    if(n >= 32)
    {

        if(n >= 33)
        {
            carry = false;
        }

        else
        {
            carry = is_set(v, 0);
        }

        return 0;
    }


    carry = is_set(v,n-1);

    return v >> n;
}

inline uint32_t asr(uint32_t v, uint32_t n, bool &carry)
{
    if(!n) return v;


    if(n >= 32)
    {
        if(is_set(v,31))
        {
            return 0xffffffff;
            carry = true;
        }

        else
        {
            return 0;
            carry = false;
        }
    }

    // save the sign
    bool s = is_set(v,31);

    carry = is_set(v,n-1);

    v >>= n;

    if(s)
    {
        // set all the zeros on left hand side to a one
        v |=  0xffffffff * (32-n);
    }


    return v;
}

// how the heck are carrys defined on this
inline uint32_t ror(uint32_t v, uint32_t n, bool &carry)
{

    n %= 32;

    if(n == 0) // ror #0 = rrx  <-- need to check this
    {
        bool c = is_set(v,0);
        v >>= 1;
        v = carry? set_bit(v,31) : deset_bit(v,31);
        carry = c;
        return v;
    }


    carry = is_set(v,n-1);

    return rotr(v,n);
}

// barrel shifter
inline uint32_t barrel_shift(Shift_type type,uint32_t v, uint32_t n, bool &carry)
{
    switch(type)
    {
        case LSL: return lsl(v,n,carry); break;
        case LSR: return lsr(v,n,carry); break;
        case ASR: return asr(v,n,carry); break;
        case ROR: return ror(v,n,carry); break;
    }
    puts("barrel shifter fell though!?");
    exit(1);
}

