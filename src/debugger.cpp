#include "headers/debugger.h"
#include <sstream>
#include <functional>


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
    static const std::vector<DEBUGGER_FPTR> command_funcs{run,breakpoint};
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
            std::cout << "Command: " << command[0] << "not recognised!\n";
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
{ UNUSED(command);
/*
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
       addr = std::stoi(command[1]);
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
            value = std::stoi(command[2]);
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
        int len = command[3].size() > 3 ? 3 : command[3].size();
    }

    else // defualt to execute
    {

    }
*/

}

void Debugger::run(std::vector<std::string> command)
{
    UNUSED(command);
    puts("Resuming emulation!");
    debug_quit = true;
}