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

// add timings
void Cpu::arm_mull(uint32_t opcode)
{
    UNUSED(opcode);
    int rm = opcode & 0xf;
    int rs = (opcode >> 8) & 0xf;
    int rdhi = (opcode >> 16) & 0xf;
    int rdlo = (opcode >> 12) & 0xf;
    bool s = is_set(opcode,20);
    bool a = is_set(opcode,21);
    bool u = is_set(opcode,22);
    

    if(u) // unsigned
    {
        uint64_t ans;
        if(a)
        {
            uint64_t oper = (static_cast<uint64_t>(regs[rdhi]) << 32) | regs[rdlo];
            ans = regs[rm] * regs[rs] + oper; 
        }

        else
        {
            ans = regs[rm] * regs[rs];
        }

        if(s)
        {
            set_zero_flag_long(ans);
        } 
        // write the ans
        regs[rdhi] = (ans >> 32) & 0xffffffff;
        regs[rdlo] = ans & 0xffffffff;              
    }

    else // signed
    {
        int64_t ans;
        if(a)
        {
            int64_t oper = (static_cast<uint64_t>(regs[rdhi]) << 32) | regs[rdlo];
            ans = static_cast<int32_t>(regs[rm]) * static_cast<int32_t>(regs[rs]) + oper;             
        }

        else
        {
            ans = regs[rm] * regs[rs];
        }

        if(s)
        {
            set_zero_flag_long(ans);
        }
        // write the ans
        regs[rdhi] = (ans >> 32) & 0xffffffff;
        regs[rdlo] = ans & 0xffffffff;

    }

    // c destroyed
    if(s)
    {
        cpsr = deset_bit(cpsr,C_BIT);
    }
}

// neeeds a more accurate timings fix
void Cpu::arm_mul(uint32_t opcode)
{
    int rn = (opcode >> 12) & 0xf;
    int rd = (opcode >> 16) & 0xf;
    int rs = (opcode >> 8) & 0xf;
    int rm = opcode & 0xf;
    bool s = is_set(opcode,20);
    bool a = is_set(opcode,21);

    if(a) // mla
    {
        regs[rd] = regs[rm] * regs[rs] + regs[rn];
    }   

    else // mul
    {
        regs[rd] = regs[rm] * regs[rs];
    }

    if(s)
    {
        set_nz_flag(regs[rd]);

        // c destroyed
        cpsr = deset_bit(cpsr,C_BIT);
    }

    cycle_tick(1);
}

void Cpu::arm_swap(uint32_t opcode)
{
    int rm = opcode & 0xf;
    int rd = (opcode >> 12) & 0xf;
    int rn = (opcode >> 16) & 0xf;

    Access_type type = is_set(opcode,22) ? BYTE : WORD;

    // rd = [rn], [rn] = rm
    regs[rd] = mem->read_memt(regs[rn],type);
    mem->write_memt(regs[rn],regs[rm],type);

    // 1s +2n +1i
    cycle_tick(4);

}

// <--- double check this code as its the most likely error source
void Cpu::arm_block_data_transfer(uint32_t opcode)
{
    bool p = is_set(opcode,24);
    bool u = is_set(opcode,23);
    bool s = is_set(opcode,22); // psr or force user mode
    bool w = is_set(opcode,21);
    bool l = is_set(opcode,20);
    int rn = (opcode >> 16) & 0xf;
    int rlist = opcode & 0xffff;


    if(s)
    {
        printf("arm block data transfer unhandled s flag: %08x\n",regs[PC]);
        exit(1);
    }

    uint32_t addr = regs[rn];
    int n = 0;

    for(int i = 0; i < 16; i++)
    {
        if(is_set(rlist,i))
        {
            n++;
        }
    }

    // allways adding on address so if  we are in "down mode"
    // we need to precalc the buttom1
    if(!u) 
    {
        addr -= n * ARM_WORD_SIZE;
        if(w)
        {
            regs[rn] = addr;
        }
        // invert the pre/post
        // as predoing the addr has messed with the meaning
        p = !p;  
    }





    for(int i = 0; i < 16; i++)
    {
        if(!is_set(rlist,i))
        {
            continue;
        }

        if(p) 
        {
            addr += ARM_WORD_SIZE;
        }


        if(l) // load
        {
            regs[i] = mem->read_memt(addr,WORD);
        }

        else // store
        {
            mem->write_memt(addr,regs[i],WORD);
        }

        if(!p)
        {
            addr += ARM_WORD_SIZE;
        }
    }

    //writeback higher address if it went up
    if(w && u)
    {
        regs[rn] = addr;
    }


    if(l)
    {
        if(rn != PC)
        {
            // ns+1n+1i
            cycle_tick(n+2);
        }

        else
        {
            // (n+1)S +2n + 1i
            cycle_tick(n+4);
        }

    }

    else
    {
        //n-1S +2n
        cycle_tick(n+1);
    }
}


// need to handle instr variants and timings on these

void Cpu::arm_branch(uint32_t opcode)
{



    // account for prefetch operation
    uint32_t pc = regs[PC] + 4;

    // 24 bit offset is shifted left 2
    // and extended to a 32 bit int
    int32_t offset = (opcode & 0xffffff) << 2;
    offset = sign_extend(offset,26);

    // if the link bit is set this acts as a call instr
    if(is_set(opcode,24))
    {
        // bits 0:1  are allways cleared
        regs[LR] = (regs[PC] & ~3);
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
        Shift_type type = static_cast<Shift_type>((opcode >> 5 ) & 0x3);



        // immediate is allways register rm
        int rm = opcode & 0xf;
        uint32_t imm = regs[rm];

        if(rm == PC)
        {
            // pc + 12 if used as operand
            imm += 8; 
        }

        // its ticked some more
        if(rn == PC)
        {
            op1 += 4;
        }

        uint32_t shift_ammount = 0;
        // shift ammount is a register
        if(is_set(opcode,4))
        {
            // bottom byte of rs (no r15)
            int rs = (opcode >> 8) & 0xf;

            shift_ammount = regs[rs] & 0xff; 
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
        case 0x0: //and
        {
            regs[rd] = logical_and(op1,op2,update_flags);
            if(update_flags)
            {
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
            }
            break;
        }

        case 0x1: // eor
        {
            regs[rd] = logical_eor(op1,op2,update_flags);
            if(update_flags)
            {
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
            }
            break;            
        }

        case 0x2: // sub
        {
            regs[rd] = sub(op1,op2,update_flags);
            break;
        }

        case 0x4: // add
        {
            regs[rd] = add(op1,op2,update_flags);
            break;
        }


        case 0x5: // adc
        {
            regs[rd] = adc(op1,op2,update_flags);
            break;           
        }

        case 0x6: // sbc
        {
            regs[rd] = sbc(op1,op2,update_flags);
            break;
        }

        case 0x7: // rsc
        {
            regs[rd] = sbc(op2,op1,update_flags);
            break;
        }

        case 0x8: // tst (and without writeback)
        {
            logical_and(op1,op2,update_flags);
            break;
        }

        case 0xa: // cmp
        {
            sub(op1,op2,update_flags);
            break;
        }

        case 0xb: // cmn
        {
            add(op1,op2,update_flags);
            break;            
        }

        case 0xc: //orr
        {
            regs[rd] = logical_or(op1,op2,update_flags);
            if(update_flags)
            {
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
            }            
            break;
        }

        case 0xd: // mov
        {
            regs[rd] = op2;
            // V preserved
            if(update_flags) // if the set cond bit is on
            {
                set_nz_flag(regs[rd]);
                // carry is that of the shift oper
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);                 
            }
            break;
        }


        case 0xe: // bic
        {
            regs[rd] = bic(op1,op2,update_flags);
            if(update_flags)
            {
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
            }            
            break;
        }

        case 0xf: // mvn
        {
            regs[rd] = ~op2;
            // V preserved
            if(update_flags) // if the set cond bit is on
            {
                set_nz_flag(regs[rd]);
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


// halfword doubleword signed data transfer
// <-- handle instr timings
void Cpu::arm_hds_data_transfer(uint32_t opcode)
{
    bool p = is_set(opcode,24);
    bool u = is_set(opcode,23);
    bool i = is_set(opcode,22);
    bool l = is_set(opcode,20);
    int rn = (opcode >> 16) & 0xf;
    int rd = (opcode >> 12) & 0xf;
    int op = (opcode >> 5) & 0x3;
    bool w = is_set(opcode,21);



    uint32_t addr = regs[rn];

    // pc + 8
    if(rn == PC) 
    { 
        addr += 4;
    }


    uint32_t offset;

    if(!i) // just this?
    {
        int rm = opcode & 0xf;
        offset = regs[rm];
    }

    else
    {
        uint8_t imm = opcode & 0xf;
        imm |= (opcode >> 8) & 0xf;
        offset = imm;
    }



    if(p) // pre
    {
        addr += u? offset : -offset;
    }

    int cycles = 0;

    uint32_t value = regs[rd];
    if(rd == PC)
    {
        // pc + 12
        value += 8;
        cycles += 2; // extra 1s + 1n for pc
    }


    // now we choose between load and store
    // and just switch on the opcode
    if(l) // load
    {
        switch(op)
        {
            case 0:
            {
                printf("hds illegal load op: %08x\n",regs[PC]);
                break;
            }

            case 1: // ldrh
            {
                regs[rd] = mem->read_memt(addr,HALF);
                cycle_tick(cycles+3); // 1s + 1n + 1i
                break;
            }

            case 2: // ldrsb
            {
                regs[rd] = sign_extend(mem->read_memt(addr,BYTE),8);
                cycle_tick(cycles+3); // 1s + 1n + 1i
                break;
            }

            case 3: // ldrsh
            {
                regs[rd] = sign_extend(mem->read_memt(addr,HALF),16);
                cycle_tick(cycles+3); // 1s + 1n + 1i
                break;
            }
        }
    }

    else // store
    {
        switch(op)
        {
            case 0:
            {
                printf("hds illegal store op: %08x\n",regs[PC]);
                exit(1);
            }

            case 1: // strh
            {
                mem->write_memt(addr,value,HALF);
                cycle_tick(2); // 2n cycles
                break;
            }

            case 2: // ldrd
            {
                regs[rd] = mem->read_memt(addr,WORD);
                regs[rd+1] = mem->read_memt(addr+ARM_WORD_SIZE,WORD);
                cycle_tick(cycles+3); // 1s + 1n + 1i
                break;
            }

            case 3: // strd
            {
                mem->write_memt(addr,regs[rd],WORD);
                mem->write_memt(addr+ARM_WORD_SIZE,value,WORD);
                cycle_tick(2); // 2n cycles
                break;
            }

        }
    }


    // handle any write backs that occur
    // arm says we cant use the same rd and base with a writeback
    if(rn != rd) 
    {
        if(!p) // post allways do a writeback
        {
            regs[rn] += u? offset : -offset;
        }

        else if(w) // writeback
        {
            regs[rn] = addr;
        }
    }
}

// ldr , str
void Cpu::arm_single_data_transfer(uint32_t opcode)
{

    int cycles = 0;

    bool load = is_set(opcode,20);
    bool w = is_set(opcode,21); // write back
    bool p = is_set(opcode,24); // pre index

    if(!p && w) // operate it in a seperate mode
    {
        printf("T present operate load/store in user mode: : %08x!\n",regs[PC]);
        exit(1);
    }


    int rd = (opcode >> 12) & 0xf;
    int rn = (opcode >> 16) & 0xf;

    uint32_t addr = regs[rn];

    // due to prefetch pc is += 8; at this stage
    if(rn == PC) 
    { 
        addr += 4;
    }

    uint32_t offset;

    if(is_set(opcode,25))
    {
        Shift_type type = static_cast<Shift_type>((opcode >> 5 ) & 0x3);


        // immediate is allways register rm
        int rm = opcode & 0xf;
        uint32_t imm = regs[rm];


        // register specified shift ammounts are not allowed
        int shift_ammount = (opcode >> 7) & 0x1f;
        
        bool carry = is_set(cpsr,C_BIT);
        offset = barrel_shift(type,imm,shift_ammount,carry);
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
    bool u = is_set(opcode,23);

    // up / down bit decides wether to add or subtract
    // the offset
    if(p)
    {
        addr += u? offset : -offset;
    }



    // perform the specifed memory access
    if(load) // ldr
    {
        if(mode == BYTE)
        {
            regs[rd] = mem->read_memt(addr,mode);
        }

        else // ldr and swp use rotated reads
        {
            regs[rd] = mem->read_memt(addr,mode);
            regs[rd] = rotr(regs[rd],(addr&3)*8);
        }
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

        cycles = 2; // 2 N cycles for a store 

    }


    // handle any write backs that occur
    // arm says we cant use the same rd and base with a writeback
    if(rn != rd) 
    {
        if(!p) // post allways do a writeback
        {
            regs[rn] += u? offset : -offset;
        }

        else if(w) // writeback
        {
            regs[rn] = addr;
        }
    }

    cycle_tick(cycles);
}
