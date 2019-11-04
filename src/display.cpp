#include "headers/display.h"
#include "headers/lib.h"
#include "headers/memory.h"

void Display::init(Mem *mem)
{
    this->mem = mem;
}


void Display::advance_line()
{
    ly++;
    mem->io[IO_VCOUNT&IO_MASK] = ly; 

    uint8_t lyc = mem->io[(IO_DISPSTAT+1)&IO_MASK];

    // need to fire an interrupt here if enabled
    if(ly == lyc)
    {
        // set the v counter flag
        set_bit(mem->io[IO_DISPSTAT&IO_MASK],2);
    }

    else
    {
        deset_bit(mem->io[IO_DISPSTAT&IO_MASK],2);
    }

}


uint32_t convert_color(uint16_t color)
{
    int r = color & 0x1f;
    int g = (color >> 5) & 0x1f;
    int b = (color >> 10) & 0x1f;



    return r << 19 |  g << 11 | b << 3 | 0xFF000000;
}

void Display::tick(int cycles)
{

    cyc_cnt += cycles;

    switch(mode)
    {
        case VISIBLE:
        {
            if(cyc_cnt > 960)
            {
                uint8_t &dispstat = mem->io[IO_DISPSTAT&IO_MASK];
                dispstat = set_bit(dispstat,1); // enter hblank
                mode = HBLANK;
            }
            break;
        }

        case HBLANK:
        {
            // end of line
            if(cyc_cnt > 1232)
            {
                uint8_t &dispstat = mem->io[IO_DISPSTAT&IO_MASK];
                advance_line();

                if(ly > 160)
                {
                    mode = VBLANK;
                    dispstat = set_bit(dispstat,0); // set vblank flag
                }

                else
                {
                    mode = VISIBLE;
                    dispstat = deset_bit(dispstat,1); // exit hblank
                }
                cyc_cnt = 0;
            }
            break;
        }

        case VBLANK:
        {
            if(cyc_cnt > 960) // hblank is still active even in vblank
            {
                uint8_t &dispstat = mem->io[IO_DISPSTAT&IO_MASK];
                dispstat = set_bit(dispstat,1); // enter hblank
            }

            // inc a line
            if(cyc_cnt > 1232)
            {
                uint8_t &dispstat = mem->io[IO_DISPSTAT&IO_MASK];

                advance_line();
                if(ly > 228)
                {
                    // exit vblank
                    new_vblank = true;
                    mode = VISIBLE;
                    dispstat = deset_bit(dispstat,0);

                    
                    // how do i properly smash this to the screen?
                    for(int y = 0; y < Y; y++)
                    {
                        for(int x = 0; x < X; x++)
                        {
                            uint8_t idx = mem->vram[(y*X)+x];
                            uint16_t color = mem->bg_ram[idx*2];
                            color |= mem->bg_ram[idx*2+1] << 8;
                            uint32_t c = convert_color(color);
                            screen[y][x] = c;
                        }
                    }
                }
                // exit hblank
                dispstat = deset_bit(dispstat,1);
                cyc_cnt = 0;
            }
            break;
        }
    }
}