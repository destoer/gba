#include "headers/cpu.h"

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
        // THUMB.6: load PC-relative
        if(((i >> 3) & 0b01001) ==  0b01001)
        {
            thumb_opcode_table[i] = thumb_ldr_pc;
        }

        else if(((i >> 5) & 0x7) == 0)
        {
            thumb_opcode_table[i] = thumb_mov_reg_shift;
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

                //ARM.3: Branch and Exchange
                // bx
                if(i == 0b000100100001) // <--- start here
                {
                    arm_opcode_table[i] = arm_branch_and_exchange;
                }


                // msr and mrs
                // ARM.6: PSR Transfer
                // bit 24-23 must be 10 for this instr
                // bit 20 must also be zero
                
                // check it ocupies the unused space for
                //TST,TEQ,CMP,CMN with a S of zero
                else if(op >= 0b1000 && op <= 0b1011)
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
    (void)cycles;
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


// common arithmetic and logical operations

// add two ints and handle flags
// unsure how we set v
uint32_t Cpu::add(uint32_t v1, uint32_t v2, bool s)
{
    if(s)
    {
        uint64_t ans = v1 + v2;

        cpsr = is_set(ans,32)? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT); 

        set_nz_flag(ans);

        return ans;
    }

    
    else
    {
        return v1 + v2;
    }

}


