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


    uint32_t get_mode() const { return cpu_mode; }

    bool is_cpu_thumb() const { return is_thumb; }

    // print all registers for debugging
    // if we go with a graphical debugger
    // we need to make ones for each array
    // and return a std::string
    void print_regs();
    
    void execute_arm_opcode(uint32_t instr);
    void execute_thumb_opcode(uint16_t instr);
private:

    using ARM_OPCODE_FPTR = void (Cpu::*)(uint32_t opcode);
    using THUMB_OPCODE_FPTR = void (Cpu::*)(uint16_t opcode);
    std::vector<ARM_OPCODE_FPTR> arm_opcode_table;
    std::vector<THUMB_OPCODE_FPTR> thumb_opcode_table;
    void init_opcode_table();
    void init_arm_opcode_table();
    void init_thumb_opcode_table();
    

    void exec_thumb();
    void exec_arm();

    uint32_t fetch_arm_opcode();
    uint16_t fetch_thumb_opcode();

    void arm_fill_pipeline();

    bool cond_met(int opcode);

    //arm cpu instructions
    void arm_unknown(uint32_t opcode);
    void arm_branch(uint32_t opcode);
    void arm_data_processing(uint32_t opcode);
    void arm_psr(uint32_t opcode);
    void arm_single_data_transfer(uint32_t opcode);
    void arm_branch_and_exchange(uint32_t opcode);
    void arm_hds_data_transfer(uint32_t opcode);
    void arm_block_data_transfer(uint32_t opcode);
    void arm_swap(uint32_t opcode);
    void arm_mul(uint32_t opcode);
    void arm_mull(uint32_t opcode);

    // thumb cpu instructions
    void thumb_unknown(uint16_t opcode);
    void thumb_ldr_pc(uint16_t opcode);
    void thumb_mov_reg_shift(uint16_t opcode);
    void thumb_cond_branch(uint16_t opcode);
    void thumb_mcas_imm(uint16_t opcode);
    void thumb_long_bl(uint16_t opcode);
    void thumb_alu(uint16_t opcode);
    void thumb_add_sub(uint16_t opcode);
    void thumb_multiple_load_store(uint16_t opcode);
    void thumb_hi_reg_ops(uint16_t opcode);
    void thumb_ldst_imm(uint16_t opcode);
    void thumb_push_pop(uint16_t opcode);

    // cpu operations eg adds
    uint32_t add(uint32_t v1, uint32_t v2, bool s);
    uint32_t adc(uint32_t v1, uint32_t v2, bool s);
    uint32_t bic(uint32_t v1, uint32_t v2, bool s);
    uint32_t sub(uint32_t v1, uint32_t v2, bool s);
    uint32_t sbc(uint32_t v1, uint32_t v2, bool s);
    uint32_t logical_and(uint32_t v1, uint32_t v2, bool s);
    uint32_t logical_or(uint32_t v1, uint32_t v2, bool s);
    uint32_t logical_eor(uint32_t v1, uint32_t v2, bool s);


    // mode switching
    void switch_mode(Cpu_mode new_mode);
    void store_registers(Cpu_mode mode);
    void load_registers(Cpu_mode mode);

    //flag helpers
    void set_negative_flag(uint32_t v);
    void set_zero_flag(uint32_t v);
    void set_nz_flag(uint32_t v);

    void set_negative_flag_long(uint64_t v);
    void set_zero_flag_long(uint64_t v);
    void set_nz_flag_long(uint64_t v);

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