#pragma once
#include "cpu.h"
#include "memory.h"
#include "arm_disass.h"
#include "display.h"
#include "debugger.h"

class GBA
{
public:
    GBA(std::string filename);
    
    void run();


private:
    Cpu cpu;
    Mem mem;
    Disass disass;
    Display disp;
    Debugger debug;
};