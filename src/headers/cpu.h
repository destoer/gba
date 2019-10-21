#pragma once
#include "forward_def.h"
#include "lib.h"
#include "arm.h"


class Cpu
{
public:
    void init(Display *disp, Mem *mem, Debugger *debug, Disass *disass);
    void step();
    void cycle_tick(int cylces); // advance the system state

    uint32_t get_pc() const {return regs[PC];}
    void set_pc(uint32_t pc) {regs[PC] = pc;}
    // print all registers for debugging
    // if we go with a graphical debugger
    // we need to make ones for each array
    // and return a std::string
    void print_regs();
    
    void execute_arm_opcode(uint32_t instr);

private:

    using ARM_OPCODE_FPTR = void (Cpu::*)(uint32_t opcode);
    std::vector<ARM_OPCODE_FPTR> opcode_table;
    void init_opcode_table();


    void exec_thumb();
    void exec_arm();

    uint32_t fetch_arm_opcode();

    void arm_fill_pipeline();

    bool cond_met(uint32_t opcode);

    //cpu instructions
    void instr_unknown(uint32_t opcode);
    void instr_branch(uint32_t opcode);
    void instr_mov(uint32_t opcode);
    void instr_ldr(uint32_t opcode);
    void instr_str(uint32_t opcode);



    void store_registers(Cpu_mode mode);
    void load_registers(Cpu_mode mode);

    //flag helpers
    void set_negative_flag(uint32_t v);
    void set_zero_flag(uint32_t v);



    Display *disp;
    Mem *mem;
    Debugger *debug;
    Disass *disass;

    // underlying registers

    // registers in the current mode
    // swapped out with backups when a mode switch occurs
    uint32_t regs[16] = {0};


    // backup stores
    uint32_t user_regs[16] = {0};
    uint32_t cpsr = 0; // status reg

    // r8 - r12 banked
    uint32_t fiq_banked[5] = {0};

    // regs 13 and 14 banked
    uint32_t hi_banked[5][2] = {0};

    // banked status regs
    uint32_t status_banked[5] = {0};

    // in arm or thumb mode?
    bool is_thumb = false;

    // what context is the arm cpu in
    Cpu_mode cpu_mode;


    // cpu pipeline
    uint32_t pipeline[2] = {0};
};

