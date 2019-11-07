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


    // if in vdraw render the line
    if(ly < 160)
    {
        render();
    }

    // exit hblank
    mem->io[IO_DISPSTAT] = deset_bit(mem->io[IO_DISPSTAT],1);
    cyc_cnt = 0; // reset cycle counter
}


// renderer helper functions
uint16_t Display::read_palette(uint32_t pal_num,uint32_t idx)
{
    return mem->handle_read(mem->pal_ram,(0x20*pal_num)+idx*2,HALF);        
}



void Display::read_tile(uint32_t tile[],bool col_256,uint32_t base,uint32_t pal_num,uint32_t tile_num, uint32_t y,bool x_flip, bool y_flip)
{
    uint32_t tile_y = y % 8;
    tile_y = y_flip? 7-tile_y : tile_y;

    // each tile accounts for 8 vertical pixels but is 2 bytes long
    uint32_t addr = base+(tile_num*0x20) + (tile_y * 4); 


    if(col_256)
    {
        puts("256 color unimpl!");
        exit(1);
    }

    else
    {

        int x_pix = x_flip? 8 : 0;
        int x_step = x_flip? -2 : +2;
        for(int x = 0; x < 8; x += 2, x_pix += x_step)
        {
            // read out the color indexs from the tile
            uint32_t tile_offset = (x_pix / 2);
            uint8_t tile_data = mem->handle_read(mem->vram,addr+tile_offset,BYTE);
            uint32_t idx1 =  tile_data & 0xf;
            uint32_t idx2 = (tile_data >> 4) & 0xf;

            // read out the colors
            uint16_t color1 = read_palette(pal_num,idx1);
            uint16_t color2 = read_palette(pal_num,idx2);

            // convert and smash them to the screen
            tile[x] = convert_color(color1);
            tile[x+1] = convert_color(color2);
        }
    }
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
            uint32_t size = (bg0_cnt >> 14) & 0x3;  // <-- need to take this more into account!


            // 256 color one pal 8bpp? or 16 color 16 pal 4bpp 
            bool col_256 = is_set(bg0_cnt,7); // 4bpp assumed
        


            
            uint32_t scroll_x = mem->handle_read(mem->io,IO_BG0HOFS,HALF) & 511;
            uint32_t scroll_y = mem->handle_read(mem->io,IO_BG0VOFS,HALF) & 511;

            uint32_t line = (ly + scroll_y) % 512;

            // what is the start tiles
            uint32_t map_x = scroll_x / 8; 
            uint32_t map_y = line / 8;


            // add the current y offset to the base for this line
            // 32 by 32 map so it wraps around again at 32 
            bg_map_base += (map_y % 0x20) * 64; // (2 * 32);
            

            uint32_t tile_data[8];

            uint32_t x_drawn = 0; // how many pixels did we draw this time?
            for(int x = 0; x < X; x += x_drawn)
            {

                // 8 for each map but each map takes 2 bytes
                // its 32 by 32 so we want it to wrap back around
                // at that point
                uint32_t bg_map_offset = (map_x++ % 0x20) * 2; 


                uint32_t x_pos = (x + scroll_x) % 512;

                // if we are at greater than 256 x or y
                // we will be in a higher map than the initial
                // and thus must add an offset to the correct 
                // screen (may be a better way to do this)
                switch(size)
                {
                    case 1: // 512 by 256
                    {
                        bg_map_offset += x_pos> 255 ? 0x800 : 0;
                        break;
                    }

                    case 2: // 256 by 512
                    {
                        bg_map_offset += line > 255 ? 0x800 : 0;
                        break;                        
                    }

                    case 3: // 512 by 512
                    {
                        bg_map_offset += line > 255? 0x1000 : 0;
                        bg_map_offset += x_pos > 255? 0x800 : 0;
                        break;                        
                    }
                }

                // read out the bg entry and rip all the information we need about the tile
                uint16_t bg_map_entry = mem->handle_read(mem->vram,bg_map_base+bg_map_offset,HALF);
                    

                bool x_flip = is_set(bg_map_entry,10);
                bool y_flip = is_set(bg_map_entry,11);

                uint32_t tile_num = bg_map_entry & 0x1ff; 
                uint32_t pal_num = (bg_map_entry >> 12) & 0xf;


                read_tile(tile_data,col_256,bg_tile_data_base,pal_num,tile_num,line,x_flip,y_flip);

                



                
                // finally smash it to the screen probably a nicer way to do the last part :)

                uint32_t tile_offset = x_pos % 8;



                uint32_t *buf = &tile_data[tile_offset];
                int pixels_to_draw = 8 - tile_offset;

                for(int i = 0; i < pixels_to_draw; i++)
                {
                    if(x + i >= X)
                    { 
                        break;
                    }
                    screen[ly][x+i] = buf[i];
                }
                x_drawn = pixels_to_draw;           
            }
            break;
        }


        case 0x4: // mode 4 (does not handle scrolling)
        {
            for(int x = 0; x < X; x++)
            {
                uint8_t idx = mem->vram[(ly*X)+x];
                uint16_t color = mem->handle_read(mem->pal_ram,(idx*2),HALF);
                uint32_t c = convert_color(color);
                screen[ly][x] = c;
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