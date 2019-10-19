#include "headers/gba.h"

// init all sup compenents
GBA::GBA(std::string filename)
{
    mem.init(filename,&debug,&cpu);
    disass.init(&mem,&cpu);
    disp.init(&mem);
    cpu.init(&disp,&mem,&debug,&disass);
    debug.init(&mem,&cpu,&disp,&disass);
}

// start the main emulation loop
void GBA::run()
{
    cpu.step();
}