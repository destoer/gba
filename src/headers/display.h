#pragma once
#include "forward_def.h"
#include "lib.h"


enum Display_mode
{
    VISIBLE,HBLANK,VBLANK
};

class Display
{
public:
    void init(Mem *mem);
    void tick(int cycles);

    Display_mode get_mode() const { return mode; }
    void set_mode(Display_mode mode) { this->mode = mode; }
    void set_cycles(int cycles) { cyc_cnt = cycles; }

    static constexpr int X = 240;
    static constexpr int Y = 160;    
    uint32_t screen[Y][X];
    bool new_vblank = false;
private:


    void render();
    void advance_line();



    // renderer helper functions
    uint16_t read_palette(uint32_t pal_num,uint32_t idx);
    void read_tile(uint32_t tile[],bool col_256,uint32_t base,uint32_t pal_num,uint32_t tile_num, 
        uint32_t y,bool x_flip, bool y_flip);


    int cyc_cnt = 0; // current number of elapsed cycles
    int ly = 0; // current number of cycles
    Mem *mem;
    Display_mode mode = VISIBLE;
};


inline uint32_t convert_color(uint16_t color)
{
    int r = color & 0x1f;
    int g = (color >> 5) & 0x1f;
    int b = (color >> 10) & 0x1f;



    return r << 19 |  g << 11 | b << 3 | 0xFF000000;
}
