#include "headers/arm_disass.h"

void Disass::init(Mem *mem, Cpu *cpu)
{
    this->mem = mem;
    this->cpu = cpu;
}