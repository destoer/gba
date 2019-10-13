#include "headers/cpu.h"

void Cpu::init(Display *disp, Mem *mem, Debugger *debug)
{
    // init components
    this->disp = disp;
    this->mem = mem;
    this->debug = debug;
}

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


