#include "headers/cpu.h"
#include "headers/memory.h"
#include "headers/arm_disass.h"
#include "headers/debugger.h"

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

void Cpu::execute_arm_opcode(uint32_t instr)
{
    // get the bits that determine the kind of instr it is
    uint32_t op = get_arm_opcode_bits(instr);

    // call the function from our opcode table
    std::invoke(opcode_table[op],this,instr);    
}

void Cpu::exec_arm()
{

#ifdef DEBUG
    if(debug->breakpoint_x.is_hit(regs[PC],mem->read_mem(regs[PC],WORD)) || debug->step_instr)
    {
        std::cout << fmt::format("{:08x}: {}\n",regs[PC],disass->disass_arm(mem->read_mem(regs[PC],WORD),regs[PC]+ARM_WORD_SIZE));
        debug->enter_debugger();
    }
#endif

    uint32_t instr = fetch_arm_opcode();




    // if the condition is not met just
    // advance past the instr
    // since impl this we get an error lol
    // page 47
    if(!cond_met(instr))
    {
       return;
    }

    execute_arm_opcode(instr);
}

// tests if a cond field in an instr has been met
bool Cpu::cond_met(uint32_t opcode)
{
    // switch on the cond bits
    // (lower 4)
    switch((opcode >> 28) & 0xf)
    {
        // z set
        case EQ: return is_set(cpsr,Z_BIT);
        
        // z clear
        case NE: return !is_set(cpsr,Z_BIT);

        // c set
        case CS: return is_set(cpsr,C_BIT);

        // c clear

        case CC: return !is_set(cpsr,C_BIT);

        // n set
        case MI: return is_set(cpsr,N_BIT);

        // n clear
        case PL: return !is_set(cpsr,N_BIT);

        // v set
        case VS: return is_set(cpsr,V_BIT);

        // v clear
        case VC: return !is_set(cpsr,V_BIT);

        // c set and z clear
        case HI: return is_set(cpsr, C_BIT) && !is_set(cpsr,Z_BIT);

        // c clear or z set
        case LS: return !is_set(cpsr,C_BIT) || is_set(cpsr,Z_BIT);

        // n equals v
        case GE: return is_set(cpsr,N_BIT) == is_set(cpsr,V_BIT);

        // n not equal to v
        case LT: return is_set(cpsr,N_BIT) != is_set(cpsr,V_BIT);

        // z clear and N equals v
        case GT: return !is_set(cpsr,Z_BIT) && is_set(cpsr,N_BIT) == is_set(cpsr,V_BIT);

        // z set or n not equal to v
        case LE: return is_set(cpsr,Z_BIT) || is_set(cpsr,N_BIT) != is_set(cpsr,V_BIT);

        // allways
        case AL: return true;

    }
    printf("cond_met fell through %08x!?\n",opcode);
    exit(1);
}


void Cpu::instr_unknown(uint32_t opcode)
{
    uint32_t op = ((opcode >> 4) & 0xf) | ((opcode >> 16) & 0xff0);
    printf("[cpu-arm]unknown opcode %08x:%08x\n",opcode,op);
    print_regs();
    exit(1);
}


// need to handle instr variants and timings on these

void Cpu::instr_branch(uint32_t opcode)
{



    // account for prefetch operation
    uint32_t pc = regs[PC] + 4;

    // 24 bit offset is shifted left 2
    // and extended to a 32 bit int
    int32_t offset = (opcode & 0xffffff) << 2;


    // if the link bit is set this acts as a call instr
    if(is_set(opcode,L_BIT))
    {
        // bits 0:1  are allways cleared
        regs[LR] = (pc & ~3);
    }


    regs[PC] = pc + offset;
    cycle_tick(3); //2s + 1n cycles
}

void Cpu::instr_mov(uint32_t opcode)
{

    int cycles = 1; // 1 cycle for normal data processing

    int rd = (opcode >> 12) & 0xf;

    if(rd == PC) // pc writen one extra cycle
    {
        cycles += 1;
    }


    if(is_set(opcode,25)) // ror shifted immediate in increments of two
    {
        uint32_t imm = get_arm_operand2_imm(opcode);
        regs[rd] = imm;

        // V preserved, unsure on C ?
        if(is_set(opcode,20)) // if the set cond bit is on
        {
            set_zero_flag(imm);
            set_negative_flag(imm);
        }
    }

    else // shifted register 
    {
        printf("instr_mov unimplemented I type: %08x:%08x\n!",opcode,regs[PC]);
        print_regs();
        exit(1);
    }

    cycle_tick(cycles);

}



void Cpu::instr_str(uint32_t opcode)
{
    if(is_set(opcode,25))
    {
        puts("cpu unhandled register offset!");
        exit(1);
    }

    else // immeditate
    {
        int imm = opcode & 0xfff;
        int rd = (opcode >> 12) & 0xf;
        int base = (opcode >> 16) & 0xf;
        Access_type mode = is_set(opcode,22) ? BYTE : WORD; 
        mem->write_mem(regs[base] + imm,regs[rd],mode);
    }
}

void Cpu::instr_ldr(uint32_t opcode)
{
    UNUSED(opcode);
    puts("cpu ldr unimplemented!");
    exit(1);
}