#pragma once

#include "forward_def.h"
#include "lib.h"

class Disass
{
public:
    void init(Mem *mem, Cpu *cpu);

    std::string disass_arm(uint32_t opcode);

    

private:
    Mem *mem;
    Cpu *cpu;

    using ARM_DISASS_FPTR = std::string (Disass::*)(uint32_t opcode);
    std::vector<ARM_DISASS_FPTR> disass_opcode_table;
    void init_disass_table();

    std::string disass_branch(uint32_t opcode);
    std::string disass_unknown(uint32_t opcode);
};