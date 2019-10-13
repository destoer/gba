#pragma once
#include "forward_def.h"
#include "lib.h"
#include "arm.h"


class Cpu
{
public:
    void init(Display *disp, Mem *mem, Debugger *debug);
    void step();

private:

    void exec_thumb();
    void exec_arm();



    Display *disp;
    Mem *mem;
    Debugger *debug;

    // underlying registers

    // registers in the current mode
    // swapped out with backups when a mode switch occurs
    uint32_t regs[16];


    // backup stores
    uint32_t user_regs[16];
    uint32_t cpsr; // status reg

    // r8 - r12 banked
    uint32_t fiq_banked[6];

    // regs 13 and 14 banked
    uint32_t hi_banked[5][2];

    // banked status regs
    uint32_t status_banked[5];

    // in arm or thumb mode?
    bool is_thumb;

    // what context is the arm cpu in
    Cpu_mode cpu_mode;
};