#pragma once
#include "forward_def.h"

class Debugger
{
public:

    void init(Mem *mem, Cpu *cpu, Display *display, Disass *disass);

private:
    Mem *mem;
    Cpu *cpu;
    Display *disp;
    Disass *disass;
};