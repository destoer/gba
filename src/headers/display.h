#pragma once
#include "forward_def.h"
#include "lib.h"

class Display
{
public:
    void init(Mem *mem);
    void tick(int cycles);


    static constexpr int X = 240;
    static constexpr int Y = 160;    
    uint32_t screen[Y][X];
    bool new_vblank = false;
private:

    enum Display_mode
    {
        VISIBLE,HBLANK,VBLANK
    };

    void advance_line();

    int cyc_cnt = 0; // current number of elapsed cycles
    int ly = 0; // current number of cycles
    Mem *mem;
    Display_mode mode = VISIBLE;
};