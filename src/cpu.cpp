#include "headers/cpu.h"

void Cpu::init(Display *disp, Mem *mem, Debugger *debug)
{
    // init components
    this->disp = disp;
    this->mem = mem;
    this->debug = debug;


    // setup main cpu state
    cpu_mode = USER; // user mode
    is_thumb = false;  // cpu in arm mode
    regs[PC] = 0x8000000; // cartrige reset vector
}


// get this booting into armwrestler
// by skipping the state forward
void Cpu::step()
{
    if(is_thumb) // step the cpu in thumb mode
    {
        exec_thumb();
    }

    else // step the cpu in arm mode
    {
        exec_arm();
    }
}


