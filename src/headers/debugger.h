#pragma once
#include "forward_def.h"
#include "lib.h"

class Debugger
{
public:

    void init(Mem *mem, Cpu *cpu, Display *display, Disass *disass);

    void enter_debugger();
    void disable_breakpoints()
    {
        breakpoint_r.disable();
        breakpoint_w.disable();
        breakpoint_x.disable();
    }

    void enable_breakpoints()
    {
        breakpoint_r.enable();
        breakpoint_w.enable();
        breakpoint_x.enable();
    }

    /*
        write
        disass
        assemble 
    */

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

        void disable()
        {
            break_enabled = false;
        }

        void enable()
        {
            break_enabled = true;
        }

        bool is_hit(uint32_t addr, uint32_t value)
        {
            if(!break_enabled)
            {
                return false;
            }

            // value enabled and its not equal
            if(value_enabled && value != this->value)
            {
                return false;
            }

            // not breakpointed on this addr
            if(addr != this->addr)
            {
                return false;
            }

            // must be true
            return true;
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


    bool step_instr = false;

private:
    Mem *mem;
    Cpu *cpu;
    Display *disp;
    Disass *disass;

    bool debug_quit = false;


    void run(std::vector<std::string> command);
    void breakpoint(std::vector<std::string> command);
    void clear(std::vector<std::string> command);
    void step(std::vector<std::string> command);
    void info(std::vector<std::string> command);
};


using DEBUGGER_FPTR = void (Debugger::*)(std::vector<std::string>);