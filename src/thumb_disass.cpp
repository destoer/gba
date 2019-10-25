#include "headers/disass.h"
#include "headers/arm.h"
#include "headers/cpu.h"
#include "headers/memory.h"



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


        // THUMB.4: ALU operations
        if((i >> 2) == 0b010000)
        {
            disass_thumb_table[i] = disass_thumb_alu;
        }

        // THUMB.19: long branch with link
        else if((i >> 4) == 0b1111)
        {
            disass_thumb_table[i] = disass_thumb_long_bl;
        }

        // THUMB.6: load PC-relative
        else if(((i >> 3) & 0b01001) ==  0b01001)
        {
            disass_thumb_table[i] = disass_thumb_ldr_pc;
        }

        // THUMB.3: move/compare/add/subtract immediate
        else if(((i >> 5) & 0b001) == 0b001)
        {
            disass_thumb_table[i] = disass_thumb_mcas_imm;
        }

        // THUMB.1: move shifted register
        // top 3 bits unset
        else if(((i >> 5) & 0x7) == 0)
        {
            disass_thumb_table[i] = disass_thumb_mov_reg_shift;
        }

        // THUMB.16: conditional branch
        else if((i >> 4) == 0b1101)
        {
            disass_thumb_table[i] = disass_thumb_cond_branch;
        }

        else 
        {
            disass_thumb_table[i] = disass_thumb_unknown;
        }                
    }
}

std::string Disass::disass_thumb_alu(uint16_t opcode)
{
    int op = (opcode >> 6) & 0xf;
    int rs = (opcode >> 3) & 0x7;
    int rd = opcode & 0x7;

    const static char *names[16] =
    {
        "and","eor","lsl","lsr",
        "asr","adc","sbc","ror",
        "tst","neg","cmp","cmn",
        "orr","mul","bic","mvn",
    };

    return fmt::format("{} {},{}",names[op],user_regs_names[rd],
        user_regs_names[rs]);
}

std::string Disass::disass_thumb_long_bl(uint16_t opcode)
{
    // this is a 4 byte opcode
    // unlike most thumb instrs
    // the 11th bit will mark if the lower immediate is the 
    // higher or lower section of it

    uint16_t opcode2 = mem->read_mem(pc,HALF);
    pc += ARM_HALF_SIZE;


    // 11 bit addrs each
    // total is 23 bit with bit 0 ignored
    opcode2 &= 0b11111111111;
    opcode &= 0b11111111111;

    bool is_high = is_set(opcode2,11);

    // 2nd instr pc is + 4 ahead
    uint32_t addr = is_high? (opcode2) << 12 : (opcode2) << 1;
    addr |= is_high? opcode << 12 : opcode << 1;

    printf("addr: %x\n",addr);

    addr = (addr + pc) & ~1;

    return fmt::format("bl #0x{:08x}",addr);

}


std::string Disass::disass_thumb_mcas_imm(uint16_t opcode)
{
    const static char *names[4] = {"mov","cmp","add","sub"};

    int op = (opcode >> 11) & 0x3;

    int rd = (opcode >> 8) & 0x7;

    uint8_t imm = opcode & 0xff;


    return fmt::format("{} {}, #0x{:02x}",names[op],user_regs_names[rd],imm);
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


std::string Disass::disass_thumb_cond_branch(uint16_t opcode)
{
    int8_t offset = opcode & 0xff;
    uint32_t addr = (pc+2) + offset*2;
    int cond = (opcode >> 8) & 0xf;

    return fmt::format("b{} #0x{:08x}",suf_array[cond],addr);
}

std::string Disass::disass_thumb_unknown(uint16_t opcode)
{
    uint8_t op = get_thumb_opcode_bits(opcode);
    printf("[disass-thumb]%08x:unknown opcode %04x:%x\n",pc,opcode,op);
    cpu->print_regs();
    exit(1);   
}