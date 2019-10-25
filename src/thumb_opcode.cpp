#include "headers/cpu.h"
#include "headers/memory.h"
#include "headers/debugger.h"
#include "headers/disass.h"


uint16_t Cpu::fetch_thumb_opcode()
{
    // ignore the pipeline for now
    uint16_t opcode = mem->read_memt(regs[PC],HALF);
    regs[PC] += ARM_HALF_SIZE;
    return opcode;
}

void Cpu::exec_thumb()
{
#ifdef DEBUG
    if(debug->breakpoint_x.is_hit(regs[PC],mem->read_mem(regs[PC],HALF)) || debug->step_instr)
    {
        std::cout << fmt::format("{:08x}: {}\n",regs[PC],disass->disass_thumb(mem->read_mem(regs[PC],HALF),regs[PC]+ARM_HALF_SIZE));
        debug->enter_debugger();
    }
#endif


    uint16_t op = fetch_thumb_opcode();

    execute_thumb_opcode(op);
}

void Cpu::thumb_unknown(uint16_t opcode)
{
    uint8_t op = get_thumb_opcode_bits(opcode);
    printf("[cpu-thumb]unknown opcode %04x:%x\n",opcode,op);
    print_regs();
    exit(1);
}

void Cpu::execute_thumb_opcode(uint16_t instr)
{
    // get the bits that determine the kind of instr it is
    uint8_t op = (instr >> 8) & 0xff;

    // call the function from our opcode table
    std::invoke(thumb_opcode_table[op],this,instr);    
}

void Cpu::thumb_long_bl(uint16_t opcode)
{
    uint16_t opcode2 = mem->read_mem(regs[PC],HALF);
    regs[PC] += ARM_HALF_SIZE;


    // 11 bit addrs each
    // total is 23 bit with bit 0 ignored
    opcode2 &= 0b11111111111;
    opcode &= 0b11111111111;

    bool is_high = is_set(opcode2,11);

    // 2nd instr pc is + 4 ahead
    uint32_t addr = is_high? (opcode2) << 12 : (opcode2) << 1;
    addr |= is_high? opcode << 12 : opcode << 1; 


    regs[LR] = regs[PC]; // not sure if this sets lr properly

    regs[PC] = addr = (addr + regs[PC]) & ~1;

    //3S+1N cycles total
    cycle_tick(4); 

}

void Cpu::thumb_mov_reg_shift(uint16_t opcode)
{
    int rd = opcode & 0x7;
    int rs = (opcode >> 3) & 0x7;

    int n = (opcode >> 6) & 0x1f;

    Shift_type type = static_cast<Shift_type>((opcode >> 11) & 0x3);

    bool did_carry = is_set(cpsr,C_BIT);

    regs[rd] = barrel_shift(type,regs[rs],n,did_carry);

    set_nz_flag(regs[rd]);


    cpsr = did_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT); 


    // 1 s cycle
    cycle_tick(1);
}

void Cpu::thumb_mcas_imm(uint16_t opcode)
{
    int op = (opcode >> 11) & 0x3;

    int rd = (opcode >> 8) & 0x7;

    uint8_t imm = opcode & 0xff;


    switch(op)
    {
        case 0x00: // mov
        {
            set_nz_flag(imm);
            regs[rd] = imm;
            break;
        }

        default:
        {
            puts("unknown mcas imm");
            thumb_unknown(opcode);
            break;
        }
    }

    // 1 s cycle
    cycle_tick(1);
}


void Cpu::thumb_cond_branch(uint16_t opcode)
{
    int8_t offset = opcode & 0xff;
    uint32_t addr = (regs[PC]+2) + offset*2;
    int cond = (opcode >> 8) & 0xf;


    // if branch taken 2s +1n cycles
    if(cond_met(cond))
    {
        regs[PC] = addr;
        cycle_tick(3);
    }

    // else 1s
    else 
    {
        cycle_tick(1);
    }

}

void Cpu::thumb_ldr_pc(uint16_t opcode)
{
    int rd = (opcode >> 8) & 0x7;

    // 0 - 1020 in offsets of 4
    int offset = (opcode & 0xff) * 4;

    // pc will have bit two deset to ensure word alignment
    // pc is + 4 ahead of current instr
    uint32_t addr = ((regs[PC] + 2) & ~2) + offset;

    regs[rd] = mem->read_memt(addr,WORD);


    // takes 2s + 1n cycles
    cycle_tick(3);

}