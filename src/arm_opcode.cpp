#include "headers/cpu.h"
#include "headers/memory.h"
#include "headers/disass.h"
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
    std::invoke(arm_opcode_table[op],this,instr);    
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
    if(!cond_met(instr >> 28))
    {
       return;
    }

    execute_arm_opcode(instr);
}




void Cpu::arm_unknown(uint32_t opcode)
{
    uint32_t op = ((opcode >> 4) & 0xf) | ((opcode >> 16) & 0xff0);
    printf("[cpu-arm]unknown opcode %08x:%08x\n",opcode,op);
    print_regs();
    exit(1);
}


// need to handle instr variants and timings on these

void Cpu::arm_branch(uint32_t opcode)
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

// psr transfer
void Cpu::arm_psr(uint32_t opcode)
{
    bool is_msr = is_set(opcode,21); // 21 set msr else mrs
    bool spsr = is_set(opcode,22); // to cpsr or spsr?

    bool is_imm = is_set(opcode,25);



    // msr
    if(is_msr)
    {
        // msr mask
        int mask = 0;

        if(is_set(opcode,19)) mask |= 0xff000000;
        if(is_set(opcode,18)) mask |= 0x00ff0000;
        if(is_set(opcode,17)) mask |= 0x0000ff00;
        if(is_set(opcode,16)) mask |= 0x000000ff;


        uint32_t v;

        if(is_imm)
        {
            // dest oper is rotr imm
            v = get_arm_operand2_imm(opcode);
        }

        else 
        {
            // dest operand is reg rm
            v = regs[opcode & 0xf];
        }



        // only write specifed bits
        v &= mask;
        

        if(mask & 0xff) // writes to mode bits?
        {
            // mode switch if mode bits are overwritten
            // 0:4 (arm has different values to us for it)
            switch_mode(cpu_mode_from_bits(v & 0x1f));
        }


        // all bits in mask should be deset
        // and then the value ored
        if(!spsr) // cpsr
        {
            cpsr = (cpsr & ~mask) | v;
        }

        else // spsr
        {
            status_banked[cpu_mode] = (status_banked[cpu_mode] & ~mask) | v;
        }
    }

    // psr
    else
    {
        int rd = (opcode >> 12) & 0xf;

        // read spsr or cpsr?
        regs[rd] = spsr? status_banked[cpu_mode] : cpsr;

    }

    cycle_tick(1); // one s cycle
}

// generalize this so we can include other opcodes


/*flag handling
z and n are easy
how we handle V and C i dont know
what has higher precedence the shifter carry
or that of the operation eg add
and how we will 
a) detect the carry
b) set them appropiately...

// logical ones C is the carry out of the shifter
// if shift is zero (it is preserved)

// arithmetic its the carry out of the alu
// and V is somehow set?


// so for logical ones we need the shifter to return
// the carray out and use that to set C
// unless its zero then its preserved

// for arithmetic we need to set C in a sub function
// based on the carry

// and V  needs to be set but im not sure how




*/

void Cpu::arm_data_processing(uint32_t opcode)
{

    int cycles = 1; // 1 cycle for normal data processing

    int rd = (opcode >> 12) & 0xf;

    uint32_t op2;


    int rn = (opcode >> 16) & 0xf;

    uint32_t op1 = regs[rn];

    if(rn == PC)
    {
        // pc += 8 if used as operand
        op1 += 4;
    }

    // if s bit is set update flags
    bool update_flags = is_set(opcode,20);

    // default to preserve the carry
    // incase of a zero shift
    bool shift_carry = is_set(cpsr,C_BIT);




    // ror shifted immediate in increments of two
    if(is_set(opcode,25)) 
    {
        // how to calc the carry?
        const int imm = opcode & 0xff;
        const int shift = (opcode >> 8) & 0xf;

        if(shift != 0)
        {
            shift_carry = is_set(imm,shift-1);
            op2 = rotr(imm,shift*2);
        }

        else 
        {
            op2 = imm;
        }    
    }

    else // shifted register 
    {
        Shift_type type = static_cast<Shift_type>((opcode >>5 ) & 0x3);


        // immediate is allways register rm
        int rm = opcode & 0xf;
        uint32_t imm = regs[rm];

        if(rm == PC)
        {
            // pc +8 if used as operand
            imm += 4; 
        }

        uint32_t shift_ammount;
        // shift ammount is a register
        if(is_set(opcode,4))
        {
            // bottom byte of rs
            int rs = (opcode >> 8) & 0xf;

            shift_ammount = regs[rs] & 0xff;

            if(rs == PC)
            {
                // pc + 12 if used as shift ammount
                shift_ammount += 8;
            }      
        }

        else // shift ammount is an 5 bit int
        {
            shift_ammount = (opcode >> 7) & 0x1f;
        }


        op2 = barrel_shift(type,imm,shift_ammount,shift_carry);
    }



    if(rd == PC) // pc writen two extra cycle
    {
        // 1s + 1n if r15 loaded
        cycles += 2;
    }

   

    
    // switch on the opcode to decide what to do
    switch((opcode >> 21) & 0xf)
    {


        case 0x4: // add
        {
            regs[rd] = add(op1,op2,update_flags);
            break;
        }

        case 0xd: // mov
        {
            regs[rd] = op2;
            // V preserved
            if(update_flags) // if the set cond bit is on
            {
                set_nz_flag(op2);
                // carry is that of the shift oper
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);                 
            }


            break;
        }


        default:
        {
            puts("cpu unknown data processing instruction!");
            arm_unknown(opcode);
            break;
        }
    }


    cycle_tick(cycles);
}


// bx 
void Cpu::arm_branch_and_exchange(uint32_t opcode)
{

    int rn = opcode & 0xf;

    // if bit 0 of rn is a 1
    // subsequent instrs decoded as thumb
    is_thumb = regs[rn] & 1;

    // branch
    regs[PC] = regs[rn] & ~1;

    // 2s + 1n
    cycle_tick(3);
}

// ldr , str
void Cpu::arm_single_data_transfer(uint32_t opcode)
{

    int cycles = 0;

    bool load = is_set(opcode,20);
    bool w = is_set(opcode,21); // write back
    bool pre = is_set(opcode,24); // pre index

    if(!pre && w) // operate it in a seperate mode
    {
        puts("T present operate load/store in user mode");
        exit(1);
    }


    int rd = (opcode >> 12) & 0xf;
    int base = (opcode >> 16) & 0xf;

    uint32_t addr = regs[base];

    // due to prefetch pc is += 8; at this stage
    if(base == PC) { addr += 4;}

    uint32_t offset;

    if(is_set(opcode,25))
    {
        puts("cpu data transfer unhandled register offset!");
        exit(1);
    }

    else // immeditate
    {
        // 12 bit immediate
        offset = opcode & 0xfff;
    }


    //byte / word bit
    Access_type mode = is_set(opcode,22)? BYTE: WORD;

    // up or down bit decides wether we add
    // or subtract the offest
    bool add = is_set(opcode,23);

    // up / down bit decides wether to add or subtract
    // the offset
    if(pre)
    {
        addr += add? offset : -offset;
    }



    // perform the specifed memory access
    if(load) // ldr
    {
        regs[rd] = mem->read_memt(addr,mode);
        cycles = 3; // 1s + 1n + 1i

        if(rd == PC)
        {
            cycles += 2; // 1s + 1n if pc loaded
        }
    }

    else // str 
    {
        uint32_t v = regs[rd];

        // due to prefetch pc is now higher
        // (pc+12)
        if(rd == PC)
        {
            v += 8;
        }

        mem->write_mem(addr,v,mode);

        cycles = 2; // 2 N cycles for a sote

    }


    // handle any write backs that occur
    // arm says we cant use the same rd and base with a writeback
    if(base != rd) 
    {
        if(!pre) // post allways do a writeback
        {
            regs[base] += add? offset : -offset;
        }

        else if(w) // writeback
        {
            regs[base] = addr;
        }
    }

    cycle_tick(cycles);
}
