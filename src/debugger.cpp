#include "headers/debugger.h"

void Debugger::init(Mem *mem, Cpu *cpu, Display *disp, Disass *disass)
{
    this->mem = mem;
    this->cpu = cpu;
    this->disp = disp;
    this->disass = disass;
}