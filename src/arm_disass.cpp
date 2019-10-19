#include "headers/arm_disass.h"
#include "headers/arm.h"
#include "headers/cpu.h"

void Disass::init(Mem *mem, Cpu *cpu)
{
    this->mem = mem;
    this->cpu = cpu;
    init_disass_table();
}

void Disass::init_disass_table()
{
    disass_opcode_table.resize(4096);

    for(int i = 0; i < 4096; i++)
    {
        // bit pattern 101
        // at bit 27 is branch
        if((i & 0xa00) == 0xa00)
        {
            disass_opcode_table[i] = disass_branch;
        }


        else 
        {
            disass_opcode_table[i] = disass_unknown;
        }        
    }
}

std::string Disass::disass_arm(uint32_t opcode)
{
    uint32_t op = get_arm_opcode_bits(opcode);

    return std::invoke(disass_opcode_table[op],this,opcode);
}


std::string Disass::disass_branch(uint32_t opcode)
{
    uint32_t pc = cpu->get_pc() + 4;

    std::string output;




    // 24 bit offset is shifted left 2
    // and extended to a 32 bit int
    int32_t offset = (opcode & 0xffffff) << 2;

    pc += offset;

    // if the link bit is set this acts as a call instr
    if(is_set(opcode,LINK_BIT))
    {
        output = fmt::format("bal #0x{:08x}",pc);
    }


    else 
    {
        output = fmt::format("b #0x{:08x}",pc);
    }

    return output;
}

std::string Disass::disass_unknown(uint32_t opcode)
{
    uint32_t op = ((opcode >> 4) & 0xf) | ((opcode >> 16) & 0xff0);
    printf("[disass-arm]unknown opcode %08x:%08x\n",opcode,op);
    cpu->print_regs();
    exit(1);   
}