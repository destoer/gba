#pragma once
#include "forward_def.h"
#include "lib.h"

class Debugger
{
public:

    void init(Mem *mem, Cpu *cpu, Display *display, Disass *disass);

    /*
        enter_debugger
        step
        break
        print
        write
        disass
        assemble 
    */

private:
    Mem *mem;
    Cpu *cpu;
    Display *disp;
    Disass *disass;

    // struct for hold program breakpoints
    struct Breakpoint
    {

        void set(uint32_t value,uint32_t addr, 
            bool value_enabled,bool break_enabled)
        {
            this->value = value;
            this->addr = addr;
            this->value_enabled = value_enabled;
            this->break_enabled = break_enabled;
        }

        uint32_t value;
        bool value_enabled;
        bool break_enabled;
        uint32_t addr;
    };

    // rwx breakpoints
    Breakpoint breakpoint_r;
    Breakpoint breakpoint_w;
    Breakpoint breakpoint_x;
};