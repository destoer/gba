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
        //THUMB.14: push/pop registers
        if(((i >> 4) & 0b1111) == 0b1011 
            && ((i >> 1) & 0b11) == 0b10)
        {
            disass_thumb_table[i] = disass_thumb_push_pop;
        }

        // THUMB.9: load/store with immediate offset
        else if(((i>>5) & 0b111) == 0b011)
        {
            disass_thumb_table[i] = disass_thumb_ldst_imm;
        }

        // THUMB.5: Hi register operations/branch exchange
        else if(((i >> 2) & 0b111111) == 0b010001)
        {
            disass_thumb_table[i] = disass_thumb_hi_reg_ops;
        }


        //  THUMB.15: multiple load/store
        else if(((i >> 4) & 0b1111) == 0b1100)
        {
            disass_thumb_table[i] = disass_thumb_multiple_load_store;
        }

        // THUMB.2: add/subtract
        else if(((i >> 3) & 0b11111) == 0b00011)
        {
            disass_thumb_table[i] = disass_thumb_add_sub;
        }

        // THUMB.4: ALU operations
        else if(((i >> 2) & 0b111111) == 0b010000)
        {
            disass_thumb_table[i] = disass_thumb_alu;
        }

        // THUMB.19: long branch with link
        else if(((i >> 4) & 0b1111) == 0b1111)
        {
            disass_thumb_table[i] = disass_thumb_long_bl;
        }

        // THUMB.6: load PC-relative
        else if(((i >> 3) & 0b11111) ==  0b01001)
        {
            disass_thumb_table[i] = disass_thumb_ldr_pc;
        }

        // THUMB.3: move/compare/add/subtract immediate
        else if(((i >> 5) & 0b111) == 0b001)
        {
            disass_thumb_table[i] = disass_thumb_mcas_imm;
        }

        // THUMB.1: move shifted register
        // top 3 bits unset
        else if(((i >> 5) & 0b111) == 0b000)
        {
            disass_thumb_table[i] = disass_thumb_mov_reg_shift;
        }

        // THUMB.16: conditional branch
        else if(((i >> 4)  & 0b1111) == 0b1101)
        {
            disass_thumb_table[i] = disass_thumb_cond_branch;
        }

        else 
        {
            disass_thumb_table[i] = disass_thumb_unknown;
        }                
    }
}


std::string Disass::disass_thumb_ldst_imm(uint16_t opcode)
{
    int op = (opcode >> 11) & 3;

    int imm = opcode >> 6 & 31;

    int rb = (opcode >> 3) & 0x7;
    int rd = opcode & 0x7;

    switch(op)
    {
        case 0b00: // str
        {  
            return fmt::format("str {},[{},#0x{:x}]",user_regs_names[rd],
                user_regs_names[rb],imm*4);
            break;
        }

        case 0b01: // ldr
        {
            return fmt::format("ldr {},[{},#0x{:x}]",user_regs_names[rd],
                user_regs_names[rb],imm*4);
            break;            
        }

        case 0b10: // strb
        {
            return fmt::format("strb {},[{},#0x{:x}]",user_regs_names[rd],
                user_regs_names[rb],imm);
            break;
        }

        case 0b11: // ldrb
        {
            return fmt::format("ldrb {},[{},#0x{:x}]",user_regs_names[rd],
                user_regs_names[rb],imm*4);            
            break;
        }
    }
    puts("disass_thumb_ldst_imm fell through!?");
    exit(1);
}

// hi reg ops/branch exchange
std::string Disass::disass_thumb_hi_reg_ops(uint16_t opcode)
{
    int rd = opcode & 0x7;
    int rs = (opcode >> 3) & 0x7;
    int op = (opcode >> 8) & 0x3;

    // can be used as bl/blx flag (not revlant for gba?)
    bool msbd = is_set(opcode,7);

    // bit 7 and 6 act as top bits of the reg
    rd = msbd? set_bit(rd,3) : rd;
    rs = is_set(opcode,6) ? set_bit(rs,3) : rs;



    switch(op)
    {
        case 0b00: // add
        {
            return fmt::format("add {},{}",user_regs_names[rd],user_regs_names[rs]);
            break;
        }

        case 0b01: // cmp
        {
            return fmt::format("cmp {},{}",user_regs_names[rd],user_regs_names[rs]);
            break;
        }

        case 0b10: // mov
        {
            return fmt::format("mov {},{}",user_regs_names[rd],user_regs_names[rs]);
            break;
        }

        case 0b11: // bx
        {
            return fmt::format("bx {}",user_regs_names[rs]);
            break;
        }
    }
    puts("disass_thumb_hi_reg_ops fell through!?");
    exit(1);
}

std::string Disass::disass_thumb_push_pop(uint16_t opcode)
{
    bool pop = is_set(opcode,11);

    bool lr = is_set(opcode,8);

    uint8_t reg_range = opcode & 0xff;

    std::string reg_str = "{";

    // for now we are just gonna list each
    // (there is probs a cleaner way to do this)

    for(int i = 0; i < 8; i++)
    {
        if(is_set(reg_range,i))
        {
            reg_str += fmt::format("r{},",i);
        }
    }
    if(pop)
    {
        if(lr)
        {
            reg_str += "pc}";
        }

        else
        {
            reg_str[reg_str.size()-1] = '}';
        }

        return fmt::format("pop {}",reg_str);
    }

    else // push
    {
        if(lr)
        {
            reg_str += "lr}";
        }

        else
        {
            reg_str[reg_str.size()-1] = '}';
        }
        return fmt::format("push {}",reg_str);
    }

}

std::string Disass::disass_thumb_multiple_load_store(uint16_t opcode)
{
    int rb = (opcode >> 8) & 0x7;
    std::string instr = is_set(opcode,11)? "ldmia" : "stmia";

    uint8_t reg_range = opcode & 0xff;

    std::string reg_str = "{";

    // for now we are just gonna list each
    // (there is probs a cleaner way to do this)

    for(int i = 0; i < 8; i++)
    {
        if(is_set(reg_range,i))
        {
            reg_str += fmt::format("r{},",i);
        }
    }

    // set last element to a '}'
    reg_str[reg_str.length()-1] = '}';

    return fmt::format("{} {}!,{}",instr,user_regs_names[rb],reg_str);

}

std::string Disass::disass_thumb_add_sub(uint16_t opcode)
{
    int rd = opcode & 0x7;
    int rs = (opcode >> 3) & 0x7;
    int rn = (opcode  >> 6) & 0x7; // can also be 3 bit imm
    int op = (opcode >> 9) & 0x3;

    switch(op)
    {
        case 0b00: // add reg
        { 
            return fmt::format("add {},{},{}",user_regs_names[rd],
                user_regs_names[rs],user_regs_names[rn]);
        }
        case 0b01: // sub reg
        { 
            return fmt::format("sub {},{},{}",user_regs_names[rd],
                user_regs_names[rs],user_regs_names[rn]);
        }        
        case 0b10: // add imm
        { 
            return fmt::format("add {},{},#0x{:x}",user_regs_names[rd],
                user_regs_names[rs],rn);
        }        
        case 0b11: // sub imm
        { 
            return fmt::format("sub {},{},#0x{:x}",user_regs_names[rd],
                user_regs_names[rs],rn);
        }        
    }
    puts("disass_thumb_add_sub fell though!?");
    exit(1);
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

// the final addr calc on this is incorrect!
// ^ <--- start here
std::string Disass::disass_thumb_long_bl(uint16_t opcode)
{

    bool first = !is_set(opcode,11);

    // 4 byte instr made up of two "sub ops"
    int offset1 = opcode & 0x7ff;
    uint16_t opcode2 = mem->read_mem(pc,HALF);
    pc += ARM_HALF_SIZE;
    int offset2 = opcode2 & 0x7ff;

    // sign extend the first offset
    offset1 <<= 12;
    offset1 = sign_extend(offset1,23);

    uint32_t addr = (offset2 << 1) + (pc + offset1);
    return fmt::format("bl #0x{:08x} ; {}",addr, first? "first" : "second");

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