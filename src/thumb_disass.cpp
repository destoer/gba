#include "headers/disass.h"
#include "headers/arm.h"
#include "headers/cpu.h"



std::string Disass::disass_thumb(uint16_t opcode,uint32_t pc)
{
    this->pc = pc; 
    uint32_t op = get_thumb_opcode_bits(opcode);

    return std::invoke(disass_thumb_table[op],this,opcode);
}


void Disass::init_thumb_disass_table()
{
    disass_thumb_table.resize(256);

    for(int i = 0; i < 256; i++)
    {
        // THUMB.6: load PC-relative
        if(((i >> 3) & 0b01001) ==  0b01001)
        {
            disass_thumb_table[i] = disass_thumb_ldr_pc;
        }

        else if(((i >> 5) & 0x7) == 0)
        {
            disass_thumb_table[i] = disass_thumb_mov_reg_shift;
        }

        else 
        {
            disass_thumb_table[i] = disass_thumb_unknown;
        }                
    }
}


std::string Disass::disass_thumb_ldr_pc(uint16_t opcode)
{
    int rd = (opcode >> 8) & 0x7;

    // 0 - 1020 in offsets of 4
    int offset = (opcode & 0xff) * 4;

    return fmt::format("ldr {}, [pc,#0x{:x}]",user_regs_names[rd],offset);
}

std::string Disass::disass_thumb_mov_reg_shift(uint16_t opcode)
{
    int rd = opcode & 0x7;
    int rs = (opcode >> 3) & 0x7;

    int n = (opcode >> 6) & 0x1f;

    int shift_type = (opcode >> 11) & 0x3;

    return fmt::format("{} {},{},#0x{:x}",shift_names[shift_type],user_regs_names[rd],user_regs_names[rs],n);

}

std::string Disass::disass_thumb_unknown(uint16_t opcode)
{
    uint8_t op = get_thumb_opcode_bits(opcode);
    printf("[disass-thumb]%08x:unknown opcode %04x:%x\n",pc,opcode,op);
    cpu->print_regs();
    exit(1);   
}