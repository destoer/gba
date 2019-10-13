#pragma once

#include "forward_def.h"

class Disass
{
public:
    void init(Mem *mem, Cpu *cpu);

private:
    Mem *mem;
    Cpu *cpu;
};