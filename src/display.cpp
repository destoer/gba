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
    mem->io[IO_VCOUNT] = ly; 

    uint8_t lyc = mem->io[IO_DISPSTAT+1];

    // need to fire an interrupt here if enabled
    if(ly == lyc)
    {
        // set the v counter flag
        mem->io[IO_DISPSTAT] = set_bit(mem->io[IO_DISPSTAT],2);
    }

    else
    {
        mem->io[IO_DISPSTAT] = deset_bit(mem->io[IO_DISPSTAT],2);
    }

    // exit hblank
    mem->io[IO_DISPSTAT] = deset_bit(mem->io[IO_DISPSTAT],1);
    cyc_cnt = 0; // reset cycle counter
}



uint32_t convert_color(uint16_t color)
{
    int r = color & 0x1f;
    int g = (color >> 5) & 0x1f;
    int b = (color >> 10) & 0x1f;



    return r << 19 |  g << 11 | b << 3 | 0xFF000000;
}


void Display::render()
{
    


    int render_mode = mem->io[IO_DISPCNT] & 0x7;



    switch(render_mode)
    {
        case 0x4: // mode 4
        {
            // bitmap mode 256 palette entries
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
            break;
        }
    }
}

void Display::tick(int cycles)
{


    // forced blank is active
    // does the display still run during this?
    if(is_set(mem->io[IO_DISPCNT],7))
    {
        mode = VBLANK;
        cyc_cnt = 0;
        return;
    }


    cyc_cnt += cycles;

    switch(mode)
    {
        case VISIBLE:
        {
            if(cyc_cnt >= 960)
            {
                // enter hblank
                mem->io[IO_DISPSTAT] = set_bit(mem->io[IO_DISPSTAT],1);
                mode = HBLANK;
            }
            break;
        }

        case HBLANK:
        {
            // end of line
            if(cyc_cnt >= 1232)
            {
                advance_line();

                if(ly == 160)
                {
                    mode = VBLANK;
                    mem->io[IO_DISPSTAT] = set_bit(mem->io[IO_DISPSTAT],0); // set vblank flag
                }

                else
                {
                    mode = VISIBLE;
                }
            }
            break;
        }

        case VBLANK:
        {


            // inc a line
            if(cyc_cnt >= 1232)
            {
                advance_line();
                if(ly == 228)
                {
                    // exit vblank
                    new_vblank = true;
                    mode = VISIBLE;
                    mem->io[IO_DISPSTAT] = deset_bit(mem->io[IO_DISPSTAT],0);
                    ly = 0;

                    render();
                }
            }


            else if(cyc_cnt >= 960) // hblank is still active even in vblank
            {
                // enter hblank (dont set the internal mode here)
                mem->io[IO_DISPSTAT] = set_bit(mem->io[IO_DISPSTAT],1);
            }

            break;
        }
    }
}