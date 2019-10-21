#include "headers/debugger.h"
#include "headers/arm.h"
#include "headers/memory.h"
#include "headers/cpu.h"
#include "headers/arm_disass.h"
#include <sstream>
#include <exception>


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
    // reset stepping state
    step_instr = false;

    // need a write and exec command (then look into handling all the instruction variants)
    static const std::vector<std::string> commands{"run","break","clear","step","info","disass","write","exec"};
    static const std::vector<DEBUGGER_FPTR> command_funcs{run,breakpoint,clear,step,info,disass_addr,write,exec};
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
       addr = std::stoll(command[1],nullptr,16);
    }

    catch(std::exception &e)
    {
        std::cout << "unable to convert: " << command[1] << "to an address\n";
        std::cout << "exception: " << e.what() << "\n";
        return;
    }

    bool value_enabled = false;
    uint32_t value = 0xdeadbeef;
    // value or type break
    if(size <= 3)
    {
        // type 
        if(!isdigit(command[2][0])) 
        {
            set_break_type(command[2],value,addr,value_enabled);
        }

        // value
        else
        {
            try
            {
                value = std::stoll(command[2],nullptr,16);
                value_enabled = true;
            }

            catch(std::exception &e)
            {
                std::cout << "unable to convert: " << command[2] << "to a value\n";
                std::cout << "exception: " << e.what() << "\n";
                return;
            }
        }
    }

    // set a rwx
    else if(size >= 4)
    {
        set_break_type(command[3],value,addr,value_enabled);
    }

    else // defualt to execute
    {
        breakpoint_x.set(value,addr,value_enabled,true);
    }

    printf("breakpoint set at %08x\n",addr);
}

void Debugger::set_break_type(std::string command,uint32_t value,uint32_t addr, bool value_enabled )
{
    int len = (command.size() > 3) ? 3 : command.size();

    for(int i = 0; i < len; i++)
    {
        switch(command[i])
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
                printf("invalid breakpoint type %c\n",command[i]);
                break;
            }
        }
    }    
}

// display system state info
// info (*addr value) | regs
void Debugger::info(std::vector<std::string> command)
{

    save_breakpoints();
    disable_breakpoints();

    if(command.size() < 2)
    {
        puts("no operand for info");
        return;
    }

    // memory print
    if(command[1][0] == '*')
    {
        try 
        {
            command[1] = command[1].substr(1);
            uint32_t addr = std::stoll(command[1],nullptr,16);

            // print x number of times
            if(command.size() == 3)
            {
                int x = std::stoll(command[2],nullptr,16);

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
        
        catch(std::exception &e)
        {
            restore_breakpoints();
            puts("unable to convert args for memory printing");
            std::cout << "exception: " << e.what() << "\n";
            return;
        }
        restore_breakpoints();
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

// assume arm mode for now
// disass addr value(opt)
void Debugger::disass_addr(std::vector<std::string> command)
{
    save_breakpoints();
    disable_breakpoints();

    if(command.size() < 2)
    {
        puts("invalid args for command");
        return;
    }

    try
    {
        uint32_t addr = std::stoll(command[1],nullptr,16);

        if(command.size() == 3)
        {
            int value = std::stoi(command[2],nullptr,16);
            for(int i = 0; i < value; i++)
            {
                uint32_t address = addr+(i*ARM_WORD_SIZE);
                std::string s = disass->disass_arm(mem->read_mem(address,WORD),address+ARM_WORD_SIZE);
                std::cout << fmt::format("{:08x}: {}\n",address,s);
            }
        }
        
        else 
        {
            std::cout << disass->disass_arm(mem->read_mem(addr,WORD),addr+ARM_WORD_SIZE) << "\n";
        }
        restore_breakpoints();
    }

    catch(std::exception &e)
    {
        restore_breakpoints();
        std::cout << "failed to convert arg to value!\n";
        std::cout << "exception: " << e.what() << "\n";
        return;
    }
}


//write addr value
void Debugger::write(std::vector<std::string> command)
{
    save_breakpoints();
    disable_breakpoints();

    if(command.size() < 3)
    {
        puts("invalid args for command");
        return;
    }

    try
    {
        uint32_t addr = std::stoll(command[1],nullptr,16);
        int value = std::stoll(command[2],nullptr,16);

        // add length specifier later
        mem->write_mem(addr,value,WORD);

        restore_breakpoints();
    }

    catch(std::exception &e)
    {
        restore_breakpoints();
        std::cout << "failed to convert arg to value!\n";
        std::cout << "exception: " << e.what() << "\n";
        return;
    }    
}

// exec value
void Debugger::exec(std::vector<std::string> command)
{
    if(command.size() < 2)
    {
        puts("invalid args for command");
        return;
    }


    try
    {
        uint32_t opcode = std::stoll(command[1],nullptr,16);

        printf("executing %08x\n",opcode);

        std::cout << disass->disass_arm(opcode,cpu->get_pc()+ARM_WORD_SIZE) << "\n";

        cpu->set_pc(cpu->get_pc()+ARM_WORD_SIZE);

        cpu->execute_arm_opcode(opcode);
    }


    catch(std::exception &e)
    {
        std::cout << "failed to convert arg to opcode: " << command[1] << "\n";
        std::cout << "exception: " << e.what() << "\n";
        return;
    }

}