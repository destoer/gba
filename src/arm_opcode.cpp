#include "headers/cpu.h"
#include "headers/memory.h"

// if there is a pipeline stall (whenever pc changes besides a fetch)
void Cpu::arm_fill_pipeline() // need to verify this...
{
    prefetch[0] = mem->read_memt(regs[PC],WORD);
    regs[PC] += ARM_WORD_SIZE;
    prefetch[1] = mem->read_memt(regs[PC],WORD);
    regs[PC] += ARM_WORD_SIZE;
}

// fetch opcodes and handle the 3 stage
// pipeline
uint32_t Cpu::fetch_arm_opcode()
{
    // read next opcode out of the pipeline
    uint32_t opcode = prefetch[0];
    prefetch[0] = prefetch[1]; // move in next instr
    regs[PC] += ARM_WORD_SIZE;
    // read the next instruction
    prefetch[1] = mem->read_memt(regs[PC],WORD); 
    return opcode;
}

void Cpu::exec_arm()
{
    uint32_t opcode = fetch_arm_opcode();


    // cool now we try and decode this damb thing
    printf("%08x\n",opcode);
    exit(1);
}