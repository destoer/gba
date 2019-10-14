#include "headers/cpu.h"
#include "headers/memory.h"

// fetch opcodes and handle the 3 stage
// pipeline
uint32_t Cpu::fetch_arm_opcode()
{
    // read next opcode out of the pipeline
    uint32_t opcode = prefetch[0];
    prefetch[0] = prefetch[1]; // move in next instr
    regs[PC] += ARM_WORD_SIZE;
    // read the next instruction
    prefetch[1] = mem->read_wordt(regs[PC]); 
    return opcode;
}

void Cpu::exec_arm()
{
    //uint32_t opcode = fetch_arm_opcode();


    // cool now we try and decode this damb thing


}