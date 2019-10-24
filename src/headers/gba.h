#pragma once
#include "cpu.h"
#include "memory.h"
#include "disass.h"
#include "display.h"
#include "debugger.h"

class GBA
{
public:
    GBA(std::string filename);
    
    void run();
    
    void enter_debugger()
    {
        debug.enter_debugger();
    }

private:
    Cpu cpu;
    Mem mem;
    Disass disass;
    Display disp;
    Debugger debug;
};