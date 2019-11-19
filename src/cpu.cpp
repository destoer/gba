#include "headers/cpu.h"
#include "headers/memory.h"
#include "headers/display.h"
#include "headers/debugger.h"
#include "headers/disass.h"
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
    hi_banked[SUPERVISOR][0] = 0x03007FE0;
    hi_banked[IRQ][0] = 0x03007FA0;
    //arm_fill_pipeline(); // fill the intitial cpu pipeline
    //regs[PC] = 0;
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

         // THUMB.11: load/store SP-relative
        if(((i >> 4) & 0b1111) == 0b1001)
        {
            thumb_opcode_table[i] = &Cpu::thumb_load_store_sp;
        }

        // THUMB.13: add offset to stack pointer
        else if(i == 0b10110000)
        {
            thumb_opcode_table[i] = &Cpu::thumb_sp_add;
        }

        // THUMB.17: software interrupt and breakpoint
        else if(i  == 0b11011111)
        {
            thumb_opcode_table[i] = &Cpu::thumb_swi;
        }

        // THUMB.8: load/store sign-extended byte/halfword
        else if(((i >> 4) & 0b1111) == 0b0101 && is_set(i,1))
        {
            thumb_opcode_table[i] = &Cpu::thumb_load_store_sbh;
        }

        // THUMB.7: load/store with register offset
        else if(((i >> 4) & 0b1111) == 0b0101 && !is_set(i,1))
        {
           thumb_opcode_table[i] = &Cpu::thumb_load_store_reg;
        }

        // THUMB.12: get relative address
        else if(((i >> 4) & 0b1111) == 0b1010)
        {
            thumb_opcode_table[i] = &Cpu::thumb_get_rel_addr;
        }
        
        // THUMB.18: unconditional branch
        else if(((i >> 3) & 0b11111) == 0b11100)
        {
            thumb_opcode_table[i] = &Cpu::thumb_branch;
        }

        //THUMB.10: load/store halfword
        else if(((i >> 4) & 0b1111) == 0b1000)
        {
            thumb_opcode_table[i] = &Cpu::thumb_load_store_half;
        }

        //THUMB.14: push/pop registers
        else if(((i >> 4) & 0b1111) == 0b1011 
            && ((i >> 1) & 0b11) == 0b10)
        {
            thumb_opcode_table[i] = &Cpu::thumb_push_pop;
        }

        // THUMB.9: load/store with immediate offset
        else if(((i>>5) & 0b111) == 0b011)
        {
            thumb_opcode_table[i] = &Cpu::thumb_ldst_imm;
        }


        // THUMB.5: Hi register operations/branch exchange
        else if(((i >> 2) & 0b111111) == 0b010001)
        {
            thumb_opcode_table[i] = &Cpu::thumb_hi_reg_ops;
        }

        //  THUMB.15: multiple load/store
        else if(((i >> 4) & 0b1111) == 0b1100)
        {
            thumb_opcode_table[i] = &Cpu::thumb_multiple_load_store;
        }

        // THUMB.2: add/subtract
        else if(((i >> 3) & 0b11111) == 0b00011)
        {
            thumb_opcode_table[i] = &Cpu::thumb_add_sub;
        }

        // THUMB.4: ALU operations
        else if(((i >> 2) & 0b111111) == 0b010000)
        {
            thumb_opcode_table[i] = &Cpu::thumb_alu;
        }

        // THUMB.19: long branch with link
        else if(((i >> 4) & 0b1111) == 0b1111)
        {
            thumb_opcode_table[i] = &Cpu::thumb_long_bl;
        }

        // THUMB.6: load PC-relative
        else if(((i >> 3) & 0b11111) ==  0b01001)
        {
            thumb_opcode_table[i] = &Cpu::thumb_ldr_pc;
        }

        // THUMB.3: move/compare/add/subtract immediate
        else if(((i >> 5) & 0b111) == 0b001)
        {
            thumb_opcode_table[i] = &Cpu::thumb_mcas_imm;
        }

        // THUMB.1: move shifted register
        // top 3 bits unset
        else if(((i >> 5) & 0b111) == 0b000)
        {
            thumb_opcode_table[i] = &Cpu::thumb_mov_reg_shift;
        }

        // THUMB.16: conditional branch
        else if(((i >> 4)  & 0b1111) == 0b1101)
        {
            thumb_opcode_table[i] = &Cpu::thumb_cond_branch;
        }

        else 
        {
            thumb_opcode_table[i] = &Cpu::thumb_unknown;
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
                    arm_opcode_table[i] = &Cpu::arm_mul;
                }

                //  multiply and accumulate long
                else if(((i & 0b1111) == 0b1001) && ((i >> 7) & 0b111111) == 0b000001) 
                {
                    arm_opcode_table[i] = &Cpu::arm_mull;
                }


                // Single Data Swap (SWP)  
                else if(((i & 0b1111) == 0b1001) && ((i >> 7) & 0b11111) == 0b00010 && ((i >> 4) & 0b11) == 0b00)
                {
                    arm_opcode_table[i] = &Cpu::arm_swap;
                }



                // ARM.10: Halfword, Doubleword, and Signed Data Transfer
                // (may require more stringent checks than this)
                // think this might cause conflicts?
                else if(((i >> 9) & 0b111) == 0b000 && is_set(i,3) && is_set(i,0))
                {
                   arm_opcode_table[i] = &Cpu::arm_hds_data_transfer;
                }


                //ARM.3: Branch and Exchange
                // bx
                else if(i == 0b000100100001) // <--- start here
                {
                    arm_opcode_table[i] = &Cpu::arm_branch_and_exchange;
                }


                // msr and mrs
                // ARM.6: PSR Transfer
                // bit 24-23 must be 10 for this instr
                // bit 20 must also be zero
                
                // check it ocupies the unused space for
                //TST,TEQ,CMP,CMN with a S of zero
                else if(op >= 0x8 && op <= 0xb && !is_set(i,4))
                {
                    arm_opcode_table[i] = &Cpu::arm_psr;
                }

                //  ARM.5: Data Processing 00 at bit 27
                else
                { 
                    arm_opcode_table[i] = &Cpu::arm_data_processing;
                }
                break;
            }

            case 0b01:
            {
                //ARM.9: Single Data Transfer
                if(true) // assume for now
                {
                    arm_opcode_table[i] = &Cpu::arm_single_data_transfer;   
                }

                else 
                {
                    arm_opcode_table[i] = &Cpu::arm_unknown;
                }
                break;
            }

            case 0b10:
            {

                // 101 (ARM.4: Branch and Branch with Link)
                if(is_set(i,9))
                {
                    arm_opcode_table[i] = &Cpu::arm_branch;
                }


                // 100
                // ARM.11: Block Data Transfer (LDM,STM)
                else if(!is_set(i,9))
                {
                    arm_opcode_table[i] = &Cpu::arm_block_data_transfer;
                }


                else 
                {
                    arm_opcode_table[i] = &Cpu::arm_unknown;
                }

                break;
            }

            case 0b11:
            {
                arm_opcode_table[i] = &Cpu::arm_unknown;
                break;
            }
        } 
    }

}

void Cpu::cycle_tick(int cycles)
{
    disp->tick(cycles);
    tick_timers(cycles);
}

void Cpu::tick_timers(int cycles)
{

    static constexpr uint32_t timer_lim[4] = {1,64,256,1024};
    static constexpr Interrupt interrupt_table[4] = {Interrupt::TIMER0,Interrupt::TIMER1,Interrupt::TIMER2,Interrupt::TIMER3};
 
    // ignore count up timing for now
    for(int i = 0; i < 4; i++)
    {
        int offset = i*ARM_WORD_SIZE;
        uint16_t cnt = mem->handle_read<uint16_t>(mem->io,IO_TM0CNT_H+offset);

        if(!is_set(cnt,7)) // timer is not enabled
        {
            continue;
        }

        if(is_set(cnt,2)) // count up timer
        {
            puts("count up timer!");
            exit(1);
        }

        uint32_t lim = timer_lim[cnt & 0x3];

       // printf("%08x\n",lim);

        timer_scale[i] += cycles;

        if(timer_scale[i] >= lim)
        {
            timers[i] += timer_scale[i] / lim;
            uint32_t max_cyc = std::numeric_limits<uint16_t>::max();
            if(timers[i] >= max_cyc) // overflowed
            {
                // add the reload values
                timers[i] = mem->handle_read<uint16_t>(mem->io,IO_TM0CNT_L+offset) + timers[i] % max_cyc;
                if(is_set(cnt,6))
                {
                    request_interrupt(interrupt_table[i]);
                }
            }
            timer_scale[i] %= lim;
        }
    }    
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

    // handle interrupts
    do_interrupts();
}

// start here
// debug register printing
void Cpu::print_regs()
{

    // update current registers
    // so they can be printed
    store_registers(cpu_mode);


    printf("current mode: %s\n",mode_names[cpu_mode]);
    printf("cpu state: %s\n", is_thumb? "thumb" : "arm");

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
    cpsr = v == 0? set_bit(cpsr,Z_BIT) : deset_bit(cpsr,Z_BIT); 
}


void Cpu::set_negative_flag(uint32_t v)
{
    cpsr = is_set(v,31)? set_bit(cpsr,N_BIT) : deset_bit(cpsr,N_BIT);
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
    cpsr = v == 0? set_bit(cpsr,Z_BIT) : deset_bit(cpsr,Z_BIT); 
}


void Cpu::set_negative_flag_long(uint64_t v)
{
    cpsr = is_set(v,63)? set_bit(cpsr,N_BIT) : deset_bit(cpsr,N_BIT);   
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
    
    // set mode bits in cpsr
    cpsr &= ~0b11111; // mask bottom 5 bits
    cpsr |= get_cpsr_mode_bits(cpu_mode);
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


            regs[PC] = user_regs[PC]; // may be overkill

            // load hi regs
            regs[SP] = hi_banked[mode][0];
            regs[LR] = hi_banked[mode][1];


            break;          
        }


        default:
        {
            printf("[load-regs %08x]unhandled mode switch: %x\n",regs[PC],mode);
            exit(1);
        }
    }    
}


void Cpu::set_cpsr(uint32_t v)
{
    cpsr = v;

    // confirm this?
    is_thumb = is_set(cpsr,5);
    Cpu_mode new_mode = cpu_mode_from_bits(cpsr & 0b11111);
    switch_mode(new_mode);    
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
            user_regs[PC] = regs[PC]; // may be overkill

            // store hi regs
            hi_banked[mode][0] = regs[SP];
            hi_banked[mode][1] = regs[LR];

            break;          
        }


        default:
        {
            printf("[store-regs %08x]unhandled mode switch: %x\n",regs[PC],mode);
            exit(1);
        }
    }
}


Cpu_mode Cpu::cpu_mode_from_bits(uint32_t v)
{
    switch(v)
    {
        case 0b10000: return USER;
        case 0b10001: return FIQ;
        case 0b10010: return IRQ;
        case 0b10011: return SUPERVISOR;
        case 0b10111: return ABORT;
        case 0b11011: return UNDEFINED;
        case 0b11111: return SYSTEM;
    }

    // clearly no program should attempt this 
    // but is their a defined behavior for it?
    printf("unknown mode from bits: %08x:%08x\n",v,regs[PC]);
    print_regs();
    exit(1);
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
    printf("cond_met fell through %08x:%08x!?\n",opcode,regs[PC]);
    exit(1);
}

// common arithmetic and logical operations


/*
// tests for overflow this happens
// when the sign bit changes when it shouldunt

// causes second test to not show when calc is done
// 2nd operand must be inverted for sub :P
bool did_overflow(uint32_t v1, uint32_t v2, uint32_t ans)
{
    return  is_set((v1 ^ ans) & (v2 ^ ans),31); 
}
^ old code now replaced with compilier builtins
*/

uint32_t Cpu::add(uint32_t v1, uint32_t v2, bool s)
{
    int32_t ans;
    if(s)
    {
        // how to set this shit?
        // happens when a change of sign occurs (so bit 31)
        /// changes to somethign it shouldunt
        bool set_v = __builtin_add_overflow((int32_t)v1,(int32_t)v2,&ans);
        cpsr = ((uint32_t)ans < v1)? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT); 
        cpsr = set_v? set_bit(cpsr,V_BIT) : deset_bit(cpsr,V_BIT); 

        set_nz_flag(ans);
    }

    else
    {
        ans = v1 + v2;
    }

    return (uint32_t)ans;
}


// needs double checking!
uint32_t Cpu::adc(uint32_t v1, uint32_t v2, bool s)
{

    uint32_t v3 = is_set(cpsr,C_BIT);

    int32_t ans;
    if(s)
    {
        bool set_v = __builtin_add_overflow((int32_t)v1,(int32_t)v2,&ans);
        set_v ^= __builtin_add_overflow((int32_t)ans,(int32_t)v3,&ans);
        cpsr = (uint32_t)ans < (v1+v3)? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT); 
        cpsr = set_v? set_bit(cpsr,V_BIT) : deset_bit(cpsr,V_BIT); 

        set_nz_flag(ans);
    }

    else
    {
        ans = v1 + v2 + v3;
    }

    return (uint32_t)ans;
}


uint32_t Cpu::sub(uint32_t v1, uint32_t v2, bool s)
{
    int32_t ans;
    if(s)
    {
        bool set_v = __builtin_sub_overflow((int32_t)v1,(int32_t)v2,&ans);
        cpsr = (v1 >= v2)? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
        cpsr = set_v? set_bit(cpsr,V_BIT) : deset_bit(cpsr,V_BIT);


        set_nz_flag(ans);
    }

    else
    {
        ans = v1 - v2;
    }
    return (uint32_t)ans;
}

// nneds double checking
uint32_t Cpu::sbc(uint32_t v1, uint32_t v2, bool s)
{
    // subtract one from ans if carry is not set
    uint32_t v3 = is_set(cpsr,C_BIT)? 0 : 1;

    int32_t ans;
    if(s)
    {
        bool set_v = __builtin_sub_overflow((int32_t)v1,(int32_t)v2,&ans);
        set_v ^= __builtin_sub_overflow((int32_t)ans,(int32_t)v3,&ans);
        cpsr = (v1 >= (v2+v3))? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
        cpsr = set_v? set_bit(cpsr,V_BIT) : deset_bit(cpsr,V_BIT);


        set_nz_flag(ans);
    }

    else
    {
        return v1 - v2 - v3;
    }

    return (uint32_t)ans;
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



// write the interrupt req bit
void Cpu::request_interrupt(Interrupt interrupt)
{
    uint16_t io_if = mem->handle_read<uint16_t>(mem->io,IO_IF);
    io_if = set_bit(io_if,static_cast<uint32_t>(interrupt));
    mem->handle_write<uint16_t>(mem->io,IO_IF,io_if);
}


void Cpu::do_interrupts()
{
    if(is_set(cpsr,7)) // irqs maksed
    {
        return;
    }


    uint16_t interrupt_enable = mem->handle_read<uint16_t>(mem->io,IO_IE);
    uint16_t interrupt_flag = mem->handle_read<uint16_t>(mem->io,IO_IF);

    // the handler will find out what fired for us!
    if((mem->get_ime() & interrupt_enable & interrupt_flag) != 0)
    {
        //printf("interrupt fired!");
        service_interrupt();
    }
}

// do we need to indicate the interrupt somewhere?
// or does the handler check if?
void Cpu::service_interrupt()
{
    // spsr for irq = cpsr
    status_banked[IRQ] = cpsr;

    // lr is next instr + 4 for an irq 
    hi_banked[IRQ][1] = regs[PC] + 4;

    // irq mode switch
    switch_mode(IRQ);

    
    // switch to arm mode
    is_thumb = false; // switch to arm mode
    cpsr = deset_bit(cpsr,5); // toggle thumb in cpsr
    cpsr = set_bit(cpsr,7); //set the irq bit to mask interrupts

    regs[PC] = 0x18; // irq handler    
}


// check if for each dma if any of the start timing conds have been met
// should store all the dma information in struct so its nice to access
// also find out when dmas are actually processed?
void Cpu::handle_dma(Dma_type req_type)
{


    if(dma_in_progress) 
    { 
        return; 
    }

    uint16_t dma_cnt[4];

    dma_cnt[0] = mem->handle_read<uint16_t>(mem->io,IO_DMA0CNT_H);
    dma_cnt[1] = mem->handle_read<uint16_t>(mem->io,IO_DMA1CNT_H);
    dma_cnt[2] = mem->handle_read<uint16_t>(mem->io,IO_DMA2CNT_H);
    dma_cnt[3] = mem->handle_read<uint16_t>(mem->io,IO_DMA3CNT_H);

    static constexpr uint32_t zero_table[4] = {0x4000,0x4000,0x4000,0x10000};
    if(req_type == Dma_type::VBLANK || req_type == Dma_type::HBLANK || req_type == Dma_type::IMMEDIATE)
    {
        for(int i = 0; i < 4; i++)
        {
            if(is_set(dma_cnt[i],15)) // dma is enabled
            {
                Dma_type dma_type = static_cast<Dma_type>((dma_cnt[i] >> 12) & 0x3);

                if(req_type == dma_type)
                {
                    uint32_t base_addr = IO_DMA0SAD + i * 12;


                    if(is_set(dma_cnt[i],9)) // repeat bit so reload word count
                    {
                        dma_regs[i].nn = mem->handle_read<uint16_t>(mem->io,base_addr+8);
                    }

                    // if a zero len transfer it uses the max len for that dma
                    if(dma_regs[i].nn == 0)
                    {
                        dma_regs[i].nn = zero_table[i];
                    }



                    do_dma(dma_cnt[i],req_type,i);
                    uint32_t cnt_addr = base_addr + 10;
                    mem->handle_write<uint16_t>(mem->io,cnt_addr,dma_cnt[i]); // write back the control reg!
                }
            }
        }
    }
    
    // will have to check the special bit seperately here...
    // we will only check a specific register here!
    else
    {
        printf("unimplemented dma special type!");
        exit(1);
    }
}


// what kind of side affects can we have here!?
// need to handle an n of 0 above in the calle
// also need to handle repeats!

// this needs to know the dma number aswell as the type of dma
// <-- dma is halting my emulator haha
void Cpu::do_dma(uint16_t &dma_cnt,Dma_type req_type, int dma_number)
{

    if(is_set(dma_cnt,11))
    {
        puts("gamepak dma!");
        exit(1);
    }


    dma_in_progress = true;

    Dma_reg &dma_reg = dma_regs[dma_number];

    uint32_t source = dma_reg.src;
    uint32_t dest = dma_reg.dst;


    bool is_half = !is_set(dma_cnt,10);
    uint32_t size = is_half? ARM_HALF_SIZE : ARM_WORD_SIZE;

    source &= 0x0fffffff;
    dest &= 0x0fffffff;

    for(size_t i = 0; i < dma_reg.nn; i++)
    {
        uint32_t offset = i * size;

        if(is_half)
        {
            uint16_t v = mem->read_memt<uint16_t>(source+offset);
            mem->write_memt<uint16_t>(dest+offset,v);
        }

        else
        {
            uint32_t v = mem->read_memt<uint32_t>(source+offset);
            mem->write_memt<uint32_t>(dest+offset,v);
        }
    }

    static constexpr Interrupt dma_interrupt[4] = {Interrupt::DMA0,Interrupt::DMA1,Interrupt::DMA2,Interrupt::DMA3}; 
    if(is_set(dma_cnt,14)) // do irq on finish
    {
        request_interrupt(dma_interrupt[dma_number]);
    }


    if(!is_set(dma_cnt,9) || req_type == Dma_type::IMMEDIATE || is_set(dma_cnt,11) ) // dma does not repeat
    {
        dma_cnt = deset_bit(dma_cnt,15); // disable it
    }

    int sad_mode = (dma_cnt >> 8) & 3;
    int dad_mode = (dma_cnt >> 6) & 3;


    switch(sad_mode)
    {
        case 0: // increment
        {
            dma_reg.src += dma_reg.nn * size;
            break;
        }

        case 1: // decrement
        {
            dma_reg.src -= dma_reg.nn * size;
            break;
        }

        case 2: // fixed (do nothing)
        {
            break;
        }

        case 3: // invalid
        {
            puts("sad mode of 3!");
            exit(1);
        }
    }


    switch(dad_mode)
    {
        case 0: // increment
        {
            dma_reg.dst += dma_reg.nn * size;
            break;
        }

        case 1: // decrement
        {
            dma_reg.dst -= dma_reg.nn * size;
            break;
        }

        case 2: // fixed (do nothing)
        {
            break;
        }

        case 3: // incremnt + reload
        {
            uint32_t base_addr = IO_DMA0SAD + dma_number * 12;
            dma_reg.dst = mem->handle_read<uint32_t>(mem->io,base_addr+4);
            dma_reg.dst += dma_reg.nn * size;
            break;
        }
    }


    dma_in_progress = false;
}