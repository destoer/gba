#pragma once
#include "forward_def.h"
#include "lib.h"
#include "arm.h"


class Cpu
{
public:
    void init(Display *disp, Mem *mem, Debugger *debug);
    void step();
    void cycle_tick(int cylces); // advance the system state
private:

    void exec_thumb();
    void exec_arm();

    uint32_t fetch_arm_opcode();

    void arm_fill_pipeline();

    Display *disp;
    Mem *mem;
    Debugger *debug;

    // underlying registers

    // registers in the current mode
    // swapped out with backups when a mode switch occurs
    uint32_t regs[16] = {0};


    // backup stores
    uint32_t user_regs[16] = {0};
    uint32_t cpsr; // status reg

    // r8 - r12 banked
    uint32_t fiq_banked[6] = {0};

    // regs 13 and 14 banked
    uint32_t hi_banked[5][2] = {0};

    // banked status regs
    uint32_t status_banked[5] = {0};

    // in arm or thumb mode?
    bool is_thumb;

    // what context is the arm cpu in
    Cpu_mode cpu_mode;


    // opcode prefetch
    uint32_t prefetch[2] = {0};
};