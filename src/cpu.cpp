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
    opcode_table.resize(4096);


    for(int i = 0; i < 4096; i++)
    {
        // bit pattern 101
        // at bit 27 is branch
        if((i & 0xa00) == 0xa00)
        {
            opcode_table[i] = instr_branch;
        }


        else 
        {
            opcode_table[i] = instr_unknown;
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
}


// store current active registers back into the copies
void Cpu::store_registers(Cpu_mode mode)
{
    switch(mode)
    {

        case SYSTEM:
        case USER:
        {
            memcpy(user_regs,regs,sizeof(user_regs));
            break;
        }

        default:
        {
            printf("unhandled mode switch: %d\n",mode);
            exit(1);
        }
    }
}