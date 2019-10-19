#include "headers/cpu.h"
#include "headers/memory.h"
#include "headers/arm_disass.h"

// if there is a pipeline stall (whenever pc changes besides a fetch)
void Cpu::arm_fill_pipeline() // need to verify this...
{
    pipeline[0] = mem->read_memt(regs[PC],WORD);
    regs[PC] += ARM_WORD_SIZE;
    pipeline[1] = mem->read_memt(regs[PC],WORD);
    regs[PC] += ARM_WORD_SIZE;
}

// fetch opcodes and handle the 3 stage
// pipeline
uint32_t Cpu::fetch_arm_opcode()
{
    /*
    // read next opcode out of the pipeline
    uint32_t opcode = prefetch[0];
    prefetch[0] = prefetch[1]; // move in next instr
    regs[PC] += ARM_WORD_SIZE;
    // read the next instruction
    prefetch[1] = mem->read_memt(regs[PC],WORD); 
    return opcode;
    */

    // ignore the pipeline for now
    uint32_t opcode = mem->read_memt(regs[PC],WORD);
    regs[PC] += ARM_WORD_SIZE;
    return opcode;
}

void Cpu::exec_arm()
{
    uint32_t instr = fetch_arm_opcode();

    std::cout << fmt::format("{:08x}: {}\n",regs[PC]-4,disass->disass_arm(instr));

    // get the bits that determine the kind of instr it is
    uint32_t op = get_arm_opcode_bits(instr);

    // call the function from our opcode table
    std::invoke(opcode_table[op],this,instr);
}

void Cpu::instr_unknown(uint32_t opcode)
{
    uint32_t op = ((opcode >> 4) & 0xf) | ((opcode >> 16) & 0xff0);
    printf("[cpu-arm]unknown opcode %08x:%08x\n",opcode,op);
    print_regs();
    exit(1);
}

void Cpu::instr_branch(uint32_t opcode)
{
    // account for prefetch operation
    uint32_t pc = regs[PC] + 4;

    // 24 bit offset is shifted left 2
    // and extended to a 32 bit int
    int32_t offset = (opcode & 0xffffff) << 2;


    // if the link bit is set this acts as a call instr
    if(is_set(opcode,LINK_BIT))
    {
        // bits 0:1  are allways cleared
        regs[LR] = (pc & ~3);
    }


    regs[PC] = pc + offset;
}