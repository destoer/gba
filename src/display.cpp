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





void Display::render()
{
    


    int render_mode = mem->io[IO_DISPCNT] & 0x7;



    switch(render_mode)
    {

        case 0x0: // text mode
        {
            
            uint16_t bg0_cnt = mem->handle_read(mem->io,IO_BG0CNT,HALF);
            uint32_t bg_tile_data_base = ((bg0_cnt >> 2) & 0x3) * 0x4000;
            uint32_t bg_map_base =  ((bg0_cnt >> 8) & 0x1f) * 0x800;


            // 256 color one pal 8bpp? or 16 color 16 pal 4bpp 
            //bool col_256 = is_set(bg0_cnt,7); // 4bpp assumed
        

            // lets just try and outright dump the first 30  tiles
            for(int i = 0; i < 30; i++)
            {
                // rip the background entry and pull the tile and palette number
                uint16_t bg_entry = mem->handle_read(mem->vram,bg_map_base + (i * 2),HALF);
                uint32_t tile_num = bg_entry & 0x3ff;
                uint32_t pal_num = (bg_entry >> 12) & 0x1f;

                // now we will rip the tile and blit it at the first space
                uint32_t tile_addr = bg_tile_data_base+(tile_num*0x20); 


                //printf("%x,%x,%x,%x:%x\n",pal_num,bg_tile_data_base,bg_map_base + (i * 2),tile_addr,tile_num);


                //printf("tile number %d\n",tile_num);
                for(int y = 0; y < 8; y++) 
                {
                    for(int x = 0; x < 8; x += 2)
                    {

                        // read out the color indexs from the tile
                        uint8_t tile = mem->handle_read(mem->vram,tile_addr+((8*y)+(x/2)),BYTE);
                        uint32_t idx1 =  tile & 0xf;
                        uint32_t idx2 = (tile >> 4) & 0xf;

                        // read out the colors
                        uint16_t color1 = mem->handle_read(mem->pal_ram,(0x20*pal_num)+idx1*2,HALF);
                        uint16_t color2 = mem->handle_read(mem->pal_ram,(0x20*pal_num)+idx2*2,HALF);

                        // convert and smash them to the screen
                        screen[y][(8*i)+x] = convert_color(color1);
                        screen[y][(8*i)+x+1] = convert_color(color2);
                    }
                }
            }
            

            break;
        }


        case 0x4: // mode 4
        {
            // bitmap mode 256 palette entries
            for(int y = 0; y < Y; y++)
            {
                for(int x = 0; x < X; x++)
                {
                    uint8_t idx = mem->vram[(y*X)+x];
                    uint16_t color = mem->handle_read(mem->pal_ram,(idx*2),HALF);
                    uint32_t c = convert_color(color);
                    screen[y][x] = c;
                }
            }
            break;
        }


        default: // mode ?
        {
            printf("unknown ppu mode %08x\n",render_mode);
            exit(1);
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