#include "headers/debugger.h"
#include "headers/arm.h"
#include "headers/memory.h"
#include "headers/cpu.h"
#include <sstream>


void Debugger::init(Mem *mem, Cpu *cpu, Display *disp, Disass *disass)
{
    this->mem = mem;
    this->cpu = cpu;
    this->disp = disp;
    this->disass = disass;
}




// main debugger input
void Debugger::enter_debugger()
{

    // std::set to store commands
    static const std::vector<std::string> commands{"run","break","clear","step","info","disass"};
    static const std::vector<DEBUGGER_FPTR> command_funcs{run,breakpoint,clear,step,info};
    std::string input;

    while(!debug_quit)
    {
        std::cout << "? ";
        std::getline(std::cin,input);

        // so at first for the input we 
        // are simply going to try and match
        // the command and pass the work off to a 
        // handler

        // first split the string by space
        std::istringstream iss(input);
        std::vector<std::string> command(std::istream_iterator<std::string>{iss},
            std::istream_iterator<std::string>());

        if(command.size() == 0)
        {
            std::cout << "Error parsing input!";
            continue;
        }

        // first word will have the command
        auto it = std::find(commands.begin(),commands.end(),command[0]);

        // not found
        if(it == commands.end())
        {
            std::cout << "Command: " << command[0] << " not recognised!\n";
            continue;
        }


        // get the idx
        auto idx = std::distance(commands.begin(),it);



        // now we just invoke our function from our array
        // of function ptrs
        std::invoke(command_funcs[idx],this,command);
    }
    debug_quit = false; // reset for next time
}

// command address    value(opt) modes(opt)
// break   0xdeadbeef 0xcafebabe rwx
void Debugger::breakpoint(std::vector<std::string> command)
{ 

    const size_t size = command.size();

    // set breakpoints
    if(size < 2)
    {
        puts("Insufficent args for breakpoint minimum of one required");
        return;
    }

    // pull the address
    uint32_t addr;
    try
    {
       addr = std::stoi(command[1],nullptr,16);
    }

    catch(...)
    {
        std::cout << "unable to convert: " << command[1] << "to an address\n";
        return;
    }

    bool value_enabled = false;
    uint32_t value = 0xdeadbeef;
    // value break
    if(size >= 3)
    {
        try
        {
            value = std::stoi(command[2],nullptr,16);
            value_enabled = true;
        }

        catch(...)
        {
            std::cout << "unable to convert: " << command[2] << "to a value\n";
            return;
        }

    }

    // set a rwx
    if(size >= 4)
    {
        int len = (command[3].size() > 3) ? 3 : command[3].size();

        for(int i = 0; i < len; i++)
        {
            switch(command[3][i])
            {
                case 'r':
                {
                    breakpoint_r.set(value,addr,value_enabled,true);
                    break;
                }
                case 'w':
                {
                    breakpoint_w.set(value,addr,value_enabled,true);
                    break;
                }

                case 'x':
                {
                    breakpoint_x.set(value,addr,value_enabled,true);
                    break;
                }

                default:
                {
                    printf("invalid breakpoint type %c\n",command[3][i]);
                    break;
                }
            }
        }

    }

    else // defualt to execute
    {
        breakpoint_x.set(value,addr,value_enabled,true);
    }

    printf("breakpoint set at %08x\n",addr);
}


void Debugger::info(std::vector<std::string> command)
{
    if(command.size() < 2)
    {
        puts("no operand for info");
        return;
    }

    // memory print
    if(command[1][0] == '*')
    {
        disable_breakpoints();
        try 
        {
            command[1] = command[1].substr(1);
            uint32_t addr = std::stoi(command[1],nullptr,16);

            // print x number of times
            if(command.size() == 3)
            {
                int x = std::stoi(command[2],nullptr,16);

                printf("       ");
                for(int i = 0; i < 16; i++)
                {
                    printf("  %02x",i);
                }
                
                printf("\n\n%04x: %02x ,",addr,mem->read_mem(addr,BYTE));
                for(int i = 1; i < x; i++)
                {	
                    // makes it "slow" to format but oh well
                    if(i % 16 == 0)
                    {
                        printf("\n%08x: ",addr+i);
                    }
                    
                    
                    printf("%02x ,",mem->read_mem(addr+i,BYTE));
                    
                }
            }

            else // print a single one
            {
                printf("%x: %08x\n",addr,mem->read_mem(addr,WORD));
            }
        }  
        
        catch(...)
        {
            enable_breakpoints();
            puts("unable to convert args for memory printing");
            return;
        }
        enable_breakpoints();
    }

    else // command lookup 
    {
        // do a register print
        if(command[1] == "regs")
        {
            cpu->print_regs();
        }
    }
    putchar('\n');
}

// clear breakpoints
void Debugger::clear(std::vector<std::string> command)
{
    UNUSED(command);
    breakpoint_r.set(0xdeadbeef,0xdeadbeef,false,false);
    breakpoint_w.set(0xdeadbeef,0xdeadbeef,false,false);
    breakpoint_x.set(0xdeadbeef,0xdeadbeef,false,false);

    puts("breakpoints cleared!");
}

void Debugger::step(std::vector<std::string> command)
{
    UNUSED(command);
    debug_quit = true;
    step_instr = true;
}

// resume emuatlion
void Debugger::run(std::vector<std::string> command)
{
    UNUSED(command);
    puts("Resuming emulation!");
    debug_quit = true;
}