#include "headers/memory.h"

void Mem::init(std::string filename, Debugger *debug)
{
    // init component
    this->debug = debug;


    // read out rom
    rom = read_file(filename);

    // read out rom info here...

}