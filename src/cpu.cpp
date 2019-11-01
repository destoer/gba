#include "headers/cpu.h"
#include "headers/display.h"
#include <limits.h>

void Cpu::init(Display *disp, Mem *mem, Debugger *debug, Disass *disass)
{
    // init components
    this->disp = disp;
    this->mem = mem;
    this->debug = debug;
    this->disass = disass;

    // setup main cpu state
    cpu_mode = SYSTEM; // system mode
    is_thumb = false;  // cpu in arm mode
    regs[PC] = 0x08000000; // cartrige reset vector
    regs[LR] = 0x08000000;
    cpsr = 0x1f;
    regs[SP] = 0x03007f00;
    //arm_fill_pipeline(); // fill the intitial cpu pipeline
    init_opcode_table();
}


void Cpu::init_opcode_table()
{
    init_arm_opcode_table();
    init_thumb_opcode_table();
}

void Cpu::init_thumb_opcode_table()
{
    thumb_opcode_table.resize(256);

    for(int i = 0; i < 256; i++)
    {

        //THUMB.14: push/pop registers
        if(((i >> 4) & 0b1111) == 0b1011 
            && ((i >> 1) & 0b11) == 0b10)
        {
            thumb_opcode_table[i] = thumb_push_pop;
        }


        // THUMB.9: load/store with immediate offset
        else if(((i>>5) & 0b111) == 0b011)
        {
            thumb_opcode_table[i] = thumb_ldst_imm;
        }


        // THUMB.5: Hi register operations/branch exchange
        else if(((i >> 2) & 0b111111) == 0b010001)
        {
            thumb_opcode_table[i] = thumb_hi_reg_ops;
        }



        //  THUMB.15: multiple load/store
        else if(((i >> 4) & 0b1111) == 0b1100)
        {
            thumb_opcode_table[i] = thumb_multiple_load_store;
        }

        // THUMB.2: add/subtract
        else if(((i >> 3) & 0b11111) == 0b00011)
        {
            thumb_opcode_table[i] = thumb_add_sub;
        }

        // THUMB.4: ALU operations
        else if(((i >> 2) & 0b111111) == 0b010000)
        {
            thumb_opcode_table[i] = thumb_alu;
        }

        // THUMB.19: long branch with link
        else if(((i >> 4) & 0b1111) == 0b1111)
        {
            thumb_opcode_table[i] = thumb_long_bl;
        }

        // THUMB.6: load PC-relative
        else if(((i >> 3) & 0b11111) ==  0b01001)
        {
            thumb_opcode_table[i] = thumb_ldr_pc;
        }

        // THUMB.3: move/compare/add/subtract immediate
        else if(((i >> 5) & 0b111) == 0b001)
        {
            thumb_opcode_table[i] = thumb_mcas_imm;
        }

        // THUMB.1: move shifted register
        // top 3 bits unset
        else if(((i >> 5) & 0b111) == 0b000)
        {
            thumb_opcode_table[i] = thumb_mov_reg_shift;
        }

        // THUMB.16: conditional branch
        else if(((i >> 4)  & 0b1111) == 0b1101)
        {
            thumb_opcode_table[i] = thumb_cond_branch;
        }

        else 
        {
            thumb_opcode_table[i] = thumb_unknown;
        }                 
    }
}

void Cpu::init_arm_opcode_table()
{
    arm_opcode_table.resize(4096);


    for(int i = 0; i < 4096; i++)
    {
        switch(i >> 10) // bits 27 and 26 of opcode
        {
            case 0b00:
            {
                int op = (i >> 5) & 0xf;

                //  ARM.7: Multiply and Multiply-Accumulate (MUL,MLA)
                if(((i & 0b1111) == 0b1001) && ((i >> 7) & 0b111111) == 0b000000) 
                {
                    arm_opcode_table[i] = arm_mul;
                }

                //  multiply and accumulate long
                else if(((i & 0b1111) == 0b1001) && ((i >> 7) & 0b111111) == 0b000001) 
                {
                    arm_opcode_table[i] = arm_mull;
                }


                // Single Data Swap (SWP)  
                else if(((i & 0b1111) == 0b1001) && ((i >> 7) & 0b11111) == 0b00010)
                {
                    arm_opcode_table[i] = arm_swap;
                }



                // ARM.10: Halfword, Doubleword, and Signed Data Transfer
                // (may require more stringent checks than this)
                // think this might cause conflicts?
                else if(((i >> 9) & 0b111) == 0b000 && is_set(i,3) && is_set(i,0))
                {
                   arm_opcode_table[i] = arm_hds_data_transfer;
                }


                //ARM.3: Branch and Exchange
                // bx
                else if(i == 0b000100100001) // <--- start here
                {
                    arm_opcode_table[i] = arm_branch_and_exchange;
                }


                // msr and mrs
                // ARM.6: PSR Transfer
                // bit 24-23 must be 10 for this instr
                // bit 20 must also be zero
                
                // check it ocupies the unused space for
                //TST,TEQ,CMP,CMN with a S of zero
                else if(op >= 0b1000 && op <= 0b1011 && !is_set(i,4))
                {
                    arm_opcode_table[i] = arm_psr;
                }

                //  ARM.5: Data Processing 00 at bit 27
                else
                { 
                    arm_opcode_table[i] = arm_data_processing;
                }
                break;
            }

            case 0b01:
            {
                //ARM.9: Single Data Transfer
                if(true) // assume for now
                {
                    arm_opcode_table[i] = arm_single_data_transfer;   
                }

                else 
                {
                    arm_opcode_table[i] = arm_unknown;
                }
                break;
            }

            case 0b10:
            {

                // 101 (ARM.4: Branch and Branch with Link)
                if(is_set(i,9))
                {
                    arm_opcode_table[i] = arm_branch;
                }


                // 100
                // ARM.11: Block Data Transfer (LDM,STM)
                else if(!is_set(i,9))
                {
                    arm_opcode_table[i] = arm_block_data_transfer;
                }


                else 
                {
                    arm_opcode_table[i] = arm_unknown;
                }

                break;
            }

            case 0b11:
            {
                arm_opcode_table[i] = arm_unknown;
                break;
            }
        } 
    }

}

void Cpu::cycle_tick(int cycles)
{
    disp->tick(cycles);
}

// get this booting into armwrestler
// by skipping the state forward
void Cpu::step()
{
    if(is_thumb) // step the cpu in thumb mode
    {
        exec_thumb();
    }

    else // step the cpu in arm mode
    {
        exec_arm();
    }
}

// start here
// debug register printing
void Cpu::print_regs()
{

    printf("current mode: %s\n",mode_names[cpu_mode]);
    printf("cpu state: %s\n", is_thumb? "thumb" : "arm");

    // update current registers
    // so they can be printed
    store_registers(cpu_mode);

    puts("USER & SYSTEM REGS");

    for(int i = 0; i < 16; i++)
    {
        printf("%s: %08x ",user_regs_names[i],user_regs[i]);
        if((i % 2) == 0)
        {
            putchar('\n');
        }
    }


    puts("\n\nFIQ BANKED");


    for(int i = 0; i < 5; i++)
    {
        printf("%s: %08x ",fiq_banked_names[i],fiq_banked[i]);
        if((i % 2) == 0)
        {
            putchar('\n');
        }        
    }

    puts("\nHI BANKED");

    for(int i = 0; i < 5; i++)
    {
        printf("%s: %08x %s: %08x\n",hi_banked_names[i][0],hi_banked[i][0],
            hi_banked_names[i][1],hi_banked[i][1]);
    }

    puts("\nSTAUS BANKED");

    for(int i = 0; i < 5; i++)
    {
        printf("%s: %08x ",status_banked_names[i],status_banked[i]);
        if((i % 2) == 0)
        {
            putchar('\n');
        }       
    }


    printf("\ncpsr: %08x\n",cpsr);

    puts("FLAGS");

    printf("Z: %s\n",is_set(cpsr,Z_BIT)? "true" : "false");
    printf("C: %s\n",is_set(cpsr,C_BIT)? "true" : "false");
    printf("N: %s\n",is_set(cpsr,N_BIT)? "true" : "false");
    printf("V: %s\n",is_set(cpsr,V_BIT)? "true" : "false");

}


// set zero flag based on arg
void Cpu::set_zero_flag(uint32_t v)
{
    if(!v) // if zero set the flag else deset
    {
        cpsr = set_bit(cpsr,Z_BIT);
    }

    else 
    {
        cpsr = deset_bit(cpsr,Z_BIT);
    }    
}


void Cpu::set_negative_flag(uint32_t v)
{
    if(is_set(v,31)) // is negative when signed
    {
        cpsr = set_bit(cpsr,N_BIT);
    }

    else
    {
        cpsr = deset_bit(cpsr,N_BIT);
    }    
}


// both are set together commonly
// so add a shortcut
void Cpu::set_nz_flag(uint32_t v)
{
    set_zero_flag(v);
    set_negative_flag(v);
}




// set zero flag based on arg
void Cpu::set_zero_flag_long(uint64_t v)
{
    if(!v) // if zero set the flag else deset
    {
        cpsr = set_bit(cpsr,Z_BIT);
    }

    else 
    {
        cpsr = deset_bit(cpsr,Z_BIT);
    }    
}


void Cpu::set_negative_flag_long(uint64_t v)
{
    if(is_set(v,63)) // is negative when signed
    {
        cpsr = set_bit(cpsr,N_BIT);
    }

    else
    {
        cpsr = deset_bit(cpsr,N_BIT);
    }    
}


// both are set together commonly
// so add a shortcut
void Cpu::set_nz_flag_long(uint64_t v)
{
    set_zero_flag_long(v);
    set_negative_flag_long(v);
}




void Cpu::switch_mode(Cpu_mode new_mode)
{
    // save and load regs
    store_registers(cpu_mode);
    load_registers(new_mode);
    cpu_mode = new_mode; // finally change modes
}


void Cpu::load_registers(Cpu_mode mode)
{
    switch(mode)
    {

        case SYSTEM:
        case USER:
        {
            // load user registers back into registers
            memcpy(regs,user_regs,sizeof(regs));
            break;
        }


        case FIQ:
        {
            // load bottom 8 user regs
            memcpy(regs,user_regs,sizeof(uint32_t)*8);
            regs[PC] = user_regs[PC]; // may be overkill

            // load fiq banked 
            memcpy(&regs[5],fiq_banked,sizeof(uint32_t)*5);
            regs[SP] = hi_banked[mode][0];
            regs[LR] = hi_banked[mode][1];

            break;
        }

        // they all have the same register layout
        // bar the banked ones
        case SUPERVISOR:
        case ABORT:
        case IRQ:
        case UNDEFINED:
        {
            // load first 13 user regs back to reg
            memcpy(regs,user_regs,sizeof(uint32_t)*13);

            // load hi regs
            regs[SP] = hi_banked[mode][0];
            regs[LR] = hi_banked[mode][1];

            break;          
        }


        default:
        {
            printf("unhandled mode switch: %d\n",mode);
            exit(1);
        }
    }    
}

// store current active registers back into the copies
void Cpu::store_registers(Cpu_mode mode)
{
    switch(mode)
    {

        case SYSTEM:
        case USER:
        {
            // store user registers back into registers
            memcpy(user_regs,regs,sizeof(user_regs));
            break;
        }


        case FIQ:
        {
            // store bottom 8 user regs
            memcpy(user_regs,regs,sizeof(uint32_t)*8);
            user_regs[PC] = regs[PC]; // may be overkill

            // store fiq banked 
            memcpy(fiq_banked,&regs[5],sizeof(uint32_t)*5);
            hi_banked[mode][0] = regs[SP];
            hi_banked[mode][1] = regs[LR];

            break;
        }

        // they all have the same register layout
        // bar the banked ones
        case SUPERVISOR:
        case ABORT:
        case IRQ:
        case UNDEFINED:
        {
            // write back first 13 regs to user
            memcpy(user_regs,regs,sizeof(uint32_t)*13);

            // store hi regs
            hi_banked[mode][0] = regs[SP];
            hi_banked[mode][1] = regs[LR];

            break;          
        }


        default:
        {
            printf("unhandled mode switch: %d\n",mode);
            exit(1);
        }
    }
}




// tests if a cond field in an instr has been met
bool Cpu::cond_met(int opcode)
{

    // switch on the cond bits
    // (lower 4)
    switch(opcode & 0xf)
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

// common arithmetic and logical operations


// v flag needs impl

uint32_t Cpu::add(uint32_t v1, uint32_t v2, bool s)
{
    uint32_t ans = v1 + v2;
    if(s)
    {
        // how to set this shit?
        // happens when a change of sign occurs (so bit 31)
        /// changes to somethign it shouldunt
        //bool set_v = v1 > INT_MAX - v2 || v1 < INT_MIN - v2; 

        cpsr = ans < v1? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT); 
        //cpsr = set_v? set_bit(cpsr,V_BIT) : deset_bit(cpsr,V_BIT); 

        set_nz_flag(ans);

        return ans;
    }

    return ans;
}


// needs double checking!
uint32_t Cpu::adc(uint32_t v1, uint32_t v2, bool s)
{

    uint32_t v3 = is_set(cpsr,C_BIT);

    uint32_t ans = v1 + v2 + v3;
    if(s)
    {
        // how to set this shit?
        
        //bool set_v = 

        cpsr = ans < (v1+v3)? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT); 
        //cpsr = set_v? set_bit(cpsr,V_BIT) : deset_bit(cpsr,V_BIT); 

        set_nz_flag(ans);

        return ans;
    }

    return ans;
}


uint32_t Cpu::sub(uint32_t v1, uint32_t v2, bool s)
{
    uint32_t ans = v1 - v2;
    if(s)
    {
        //bool set_v = 
        cpsr = (v1 >= v2)? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
        //cpsr = set_v? set_bit(cpsr,V_BIT) : deset_bit(cpsr,V_BIT);


        set_nz_flag(ans);
    }
    return ans;
}

// nneds double checking
uint32_t Cpu::sbc(uint32_t v1, uint32_t v2, bool s)
{
    // subtract one from ans if carry is not set
    uint32_t v3 = is_set(cpsr,C_BIT)? 0 : 1;

    uint32_t ans = v1 - v2 - v3;
    if(s)
    {
        //bool set_v = 
        cpsr = (v1 >= (v2+v3))? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
        //cpsr = set_v? set_bit(cpsr,V_BIT) : deset_bit(cpsr,V_BIT);


        set_nz_flag(ans);
    }
    return ans;
}

uint32_t Cpu::logical_and(uint32_t v1, uint32_t v2, bool s)
{
    uint32_t ans = v1 & v2;

    if(s)
    {
        set_nz_flag(ans);
    }

    return ans;
}

uint32_t Cpu::logical_or(uint32_t v1, uint32_t v2, bool s)
{
    uint32_t ans = v1 | v2;
    if(s)
    {
        set_nz_flag(ans);
    }
    return ans;
}

uint32_t Cpu::bic(uint32_t v1, uint32_t v2, bool s)
{
    uint32_t ans = v1 & ~v2;
    if(s)
    {
        set_nz_flag(ans);
    }
    return ans;
}

uint32_t Cpu::logical_eor(uint32_t v1, uint32_t v2, bool s)
{
    uint32_t ans = v1 ^ v2;
    if(s)
    {
        set_nz_flag(ans);
    }
    return ans;
}