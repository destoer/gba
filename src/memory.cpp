#include "headers/memory.h"
#include "headers/cpu.h"
#include "headers/debugger.h"
#include "headers/display.h"

void Mem::init(std::string filename, Debugger *debug,Cpu *cpu,Display *disp)
{
    // init component
    this->debug = debug;
    this->cpu = cpu;
    this->disp = disp;

    // read out rom
    read_file(filename,rom);

    

    // alloc our underlying system memory
    bios_rom.resize(0x4000);
    board_wram.resize(0x40000);
    chip_wram.resize(0x8000);
    io.resize(0x400);
    pal_ram.resize(0x400);
    vram.resize(0x18000);
    oam.resize(0x400); 
    
    // read out rom info here...
    std::cout << "rom size: " << rom.size() << "\n";


    // all unpressed
    io[IO_KEYINPUT] = 0xff;
    io[IO_KEYINPUT+1] = 0x3;



    // read and copy in the bios rom
    read_file("GBA.BIOS",bios_rom);

    if(bios_rom.size() != 0x4000)
    {
        puts("invalid bios size!");
        exit(1);
    }
}




void Mem::tick_mem_access(int mode)
{
    // should unmapped addresses still tick a cycle?
    if(mem_region != UNDEFINED)
    {
        cpu->cycle_tick(wait_states[mem_region][mode]);
    }
}





// gba is locked to little endian

uint32_t Mem::read_external(uint32_t addr,Access_type mode)
{

    uint32_t len =  rom.size();
    if((addr&0x1FFFFFF) > len)
    {
        printf("rom read out of range: %08x:%08x:%08x\n",addr&0x1FFFFFF,len,cpu->get_pc());
        cpu->print_regs();
        exit(1);
    }

    switch((addr >> 24) & 0xf)
    {
        case 0x8: // wait state 0
        case 0x9:
        {
            mem_region = ROM;
            //return rom[addr - 0x08000000];
            return handle_read(rom,addr&0x1FFFFFF,mode);
            break;
        }

        case 0xa: // wait state 1
        case 0xb:
        {
            mem_region = ROM;
            //return rom[addr - 0x0a000000];
            return handle_read(rom,addr&0x1FFFFFF,mode);
            break;
        }
            
        case 0xc: // wait state 2
        case 0xd:
        {
            mem_region = ROM;
            //return rom[addr - 0x0c000000];
            return handle_read(rom,addr&0x1FFFFFF,mode);
            break;
        }

        case 0xe: // sram
        {
            if(addr <= 0x0e00ffff)
            {
                mem_region = SRAM;
                printf("sram read %08x:%08x\n",cpu->get_pc(),addr);
                cpu->print_regs();
                exit(1);
            }
                
            else // unused
            {
                mem_region = UNDEFINED;
                return 0x00;
            }
        }
    }
    printf("read_external fell through %08x\n",addr);
    cpu->print_regs();
    exit(1);    
}

//access handler for reads (for non io mapped mem)
// need checks for endianess here for completeness
uint32_t Mem::handle_read(std::vector<uint8_t> &buf,uint32_t addr,Access_type mode)
{

#ifdef DEBUG // bounds check the memory access

    uint32_t sz = access_sizes[mode];


    if(buf.size() < addr + sz)
    {
        printf("out of range handle read at: %08x\n",cpu->get_pc());
        cpu->print_regs();
        exit(1);
    }
#endif



    switch(mode)
    {
        case BYTE:
        {
            return buf[addr];
        }

        case HALF:
        {
            //return(*(uint16_t*)(buf.data()+addr));
            uint16_t v;
            memcpy(&v,buf.data()+addr,ARM_HALF_SIZE);
            return v;
        }

        case WORD:
        {
            //return(*(uint32_t*)(buf.data()+addr));
            uint32_t v;
            memcpy(&v,buf.data()+addr,ARM_WORD_SIZE);  
            return v;          
        }
    }
    puts("Handle_read fell through!?");
    cpu->print_regs();
    exit(1);
}

// this will require special handling
uint8_t Mem::read_io_regs(uint32_t addr)
{
    addr &= IO_MASK;
    switch(addr)
    {

        // should only be accessible from bios..
        // handle this later
        case IO_SOUNDBIAS:
        case IO_SOUNDBIAS+1:
        {
            return io[addr];
        }

        // unused
        case IO_SOUNDBIAS+2:
        case IO_SOUNDBIAS+3:
        {
            return 0; 
        }

        case IO_DISPCNT:
        {
            return io[addr];
            break;
        }

        case IO_DISPCNT+1:
        {
            return io[addr];
            break;
        }

        // these two are just stubs atm
        case IO_DISPSTAT:
        {
            return io[addr];
            break;
        }

        case IO_DISPSTAT+1:
        {
            return io[addr];
            break;
        }


        case IO_VCOUNT:
        {
            return io[addr];
            break;
        }

        case IO_VCOUNT+1: // not used
        {
            return 0x0;
            break;
        }

        // bit one toggle green swap (ingore for now)
        case IO_GREENSWAP:
        {
            return 0; 
            break;
        }

        case IO_GREENSWAP+1:
        {
            return 0;
            break;
        }






        // dma  word count
        // read only but may be read from
        // as a word to access the higher part
        case IO_DMA0CNT_L:
        case IO_DMA0CNT_L+1:
        case IO_DMA1CNT_L:
        case IO_DMA1CNT_L+1:
        case IO_DMA2CNT_L:
        case IO_DMA2CNT_L+1:                        
        case IO_DMA3CNT_L:
        case IO_DMA3CNT_L+1:
        {
            return 0;
            break;
        }


        // dma  transfer control
        case IO_DMA0CNT_H:
        case IO_DMA0CNT_H+1:
        case IO_DMA1CNT_H:
        case IO_DMA1CNT_H+1:
        case IO_DMA2CNT_H:
        case IO_DMA2CNT_H+1:                               
        case IO_DMA3CNT_H:
        case IO_DMA3CNT_H+1:
        {
            return io[addr];
            break;
        }



        case IO_KEYINPUT:
        {
            return io[addr];
            break;
        }

      
        case IO_KEYINPUT+1: // 10-15 unused
        {
            return io[addr];
            break;
        }


        case IO_KEYCNT:
        {
            return io[addr];
            break;
        }

        case IO_KEYCNT+1: // 10-13 not used
        {
            return io[addr];
            break;
        }


        case IO_IME: // 0th bit toggles ime
        {
            return ime;
            break;
        }
        case IO_IME + 1:
        { 
            return 0; // do nothing
            break;
        }

        case IO_IE: // inteerupt enable
        case IO_IE+1:
        {
            return io[addr];
        }


        case IO_IF:
        case IO_IF+1:
        {
            return io[addr];
        }


        case IO_WAITCNT:
        case IO_WAITCNT+1:
        {
            return io[addr];
        }

        // unused
        case IO_WAITCNT+2:
        case IO_WAITCNT+3:
        {
            return 0;
        }


        //unused
        case 0x400020A: 
        case 0x400020B:
        { 
            return 0;
            break;
        }

        case IO_POSTFLG:
        {
            return io[addr] & 1;
            break;
        }

        default:
        {    
            printf("unknown io reg read at %08x:%08x\n",addr,cpu->get_pc());
            cpu->print_regs();
            exit(1);
        }
    }
}

// this will require special handling
uint32_t Mem::read_io(uint32_t addr,Access_type mode)
{
    mem_region = IO;
    //return io[addr & 0x3ff];

    switch(mode)
    {
        // read out the adjacent byte
        // write back our byte plus the existing one...
        case BYTE: 
        {
            return read_io_regs(addr);
            break;
        }

        case WORD: // write lower and higher
        {

            uint32_t v = read_io_regs(addr);
            v |= read_io_regs(addr+1)  << 8;
            v |= read_io_regs(addr+2) << 16;
            v |= read_io_regs(addr+3) << 16;
            return v;
            break;
        }

        case HALF: // in the correct format
        {
            uint16_t v = read_io_regs(addr);
            v |= read_io_regs(addr+1) << 8;
            return v;
            break;
        }
    } 

    printf("read_io fell through: %08x\n",mode);
    cpu->print_regs();
    exit(1);
}

uint32_t Mem::read_oam(uint32_t addr,Access_type mode)
{
    mem_region = OAM;
    //return oam[addr & 0x3ff];
    return handle_read(oam,addr&0x3ff,mode);   
}

uint32_t Mem::read_vram(uint32_t addr,Access_type mode)
{
    mem_region = VRAM;
    //return vram[addr-0x06000000];
    return handle_read(vram,addr-0x06000000,mode);
    

}

uint32_t Mem::read_pal_ram(uint32_t addr,Access_type mode)
{
    mem_region = PAL;
    //return pal_ram[addr & 0x3ff];
    return handle_read(pal_ram,addr&0x3ff,mode);

}

uint32_t Mem::read_board_wram(uint32_t addr,Access_type mode)
{
    mem_region = WRAM_BOARD;
    //return board_wram[addr & 0x3ffff];
    return handle_read(board_wram,addr&0x3ffff,mode);
}

uint32_t Mem::read_chip_wram(uint32_t addr,Access_type mode)
{
    mem_region = WRAM_CHIP;
    //return chip_wram[addr & 0x7fff];
    return handle_read(chip_wram,addr&0x7fff,mode);
}

uint32_t Mem::read_bios(uint32_t addr,Access_type mode)
{
    mem_region = BIOS;
    //return bios_rom[addr];
    return handle_read(bios_rom,addr,mode);
}


// unused memory is to be ignored

 // read mem unticked
uint32_t Mem::read_mem(uint32_t addr,Access_type mode)
{


#ifdef DEBUG
    if(debug->breakpoint_r.break_enabled)
    {
        debug->breakpoint_r.disable();
        uint32_t value = read_mem(addr,mode);
        debug->breakpoint_r.enable();

        if(debug->breakpoint_r.is_hit(addr,value))
        {
            printf("read breakpoint hit at %08x:%08x:%08x\n",addr,value,cpu->get_pc());
            debug->enter_debugger();
        }
    }    
#endif

    // 28 bit bus
    addr &= 0x0fffffff;

    // handle address alignment
    switch(mode)
    {
        case BYTE: break; // free access
        case HALF: addr &= ~1; break; 
        case WORD: addr &= ~3; break;
        default:
        { 
            printf("[read-mem]unknown access mode %x:%08x!\n",mode,cpu->get_pc());
            exit(1);
        }
    }


    uint32_t v;
    if(addr < 0x00004000) v = read_bios(addr,mode);
    else if(addr < 0x02000000) { mem_region = UNDEFINED; return 0; }
    else if(addr < 0x03000000) v = read_board_wram(addr,mode);
    else if(addr < 0x04000000) v = read_chip_wram(addr,mode);
    else if(addr < 0x05000000) v = read_io(addr,mode);
    else if(addr < 0x06000000) v = read_pal_ram(addr,mode);
    else if(addr < 0x06018000) v = read_vram(addr,mode);
    else if(addr < 0x07000000) { mem_region = UNDEFINED; return 0; }
    else if(addr < 0x08000000) v = read_oam(addr,mode);
    else v = read_external(addr,mode);




    return v;
}

// timed memory access
uint32_t Mem::read_memt(uint32_t addr,Access_type mode)
{
    uint32_t v = read_mem(addr,mode);
    tick_mem_access(mode);
    return v;
}



// write mem
 // write mem unticked
void Mem::write_mem(uint32_t addr,uint32_t v,Access_type mode)
{

#ifdef DEBUG
    if(debug->breakpoint_w.break_enabled)
    {
        if(debug->breakpoint_w.is_hit(addr,v))
        {
            printf("write breakpoint hit at %08x:%08x:%08x\n",addr,v,cpu->get_pc());
            debug->enter_debugger();
        }
    }    
#endif


    // 28 bit bus
    addr &= 0x0fffffff;

    switch(mode)
    {
        case BYTE: break; // free access
        case HALF: addr &= ~1; break; 
        case WORD: addr &= ~3; break;
        default:
        { 
            printf("[write-mem]unknown access mode %x:%08x!\n",mode,cpu->get_pc());
            exit(1);
        }        
    }

    if(addr < 0x00004000) { mem_region = BIOS; return; } // bios is read only
    else if(addr < 0x02000000) { mem_region = UNDEFINED; return; }
    else if(addr < 0x03000000) write_board_wram(addr,v,mode);
    else if(addr < 0x04000000) write_chip_wram(addr,v,mode);
    else if(addr < 0x05000000) write_io(addr,v,mode);
    else if(addr < 0x06000000) write_pal_ram(addr,v,mode);
    else if(addr < 0x06018000) write_vram(addr,v,mode);
    else if(addr < 0x07000000) { mem_region = UNDEFINED; return; }
    else if(addr < 0x08000000) write_oam(addr,v,mode);
    else write_external(addr,v,mode); // rom is read only but could be flash

}

// ticked access
void Mem::write_memt(uint32_t addr,uint32_t v,Access_type mode)
{
    write_mem(addr,v,mode);
    tick_mem_access(mode);
}


void Mem::write_external(uint32_t addr,uint32_t v,Access_type mode)
{
    UNUSED(v); UNUSED(mode);
    // rom is read only
    switch((addr >> 24) & 0xf)
    {
        case 0x8: // wait state 0
        case 0x9:
        {
            mem_region = ROM;
            return;
        }

        case 0xa: // wait state 1
        case 0xb:
        {
            mem_region = ROM;
            return;
        }
            
        case 0xc: // wait state 2
        case 0xd:
        {
            mem_region = ROM;
            return;
        }

        case 0xe: // sram
        {
            if(addr <= 0x0e00ffff)
            {
                mem_region = SRAM;
                printf("sram write %08x:%08x\n",cpu->get_pc(),addr);
                cpu->print_regs();
                exit(1);
            }
                
            else // unused
            {
                mem_region = UNDEFINED;
                return;
            }
        }
    }
    printf("write_external fell through %08x:%08x\n",addr,cpu->get_pc());
    cpu->print_regs();
    exit(1);    
}

//access handler for reads (for non io mapped mem)
// need checks for endianess here for completeness
void Mem::handle_write(std::vector<uint8_t> &buf,uint32_t addr,uint32_t v,Access_type mode)
{

#ifdef DEBUG // bounds check the memory access

    uint32_t sz = access_sizes[mode];


    if(buf.size() < addr + sz)
    {
        printf("out of range handle write at: %08x\n",cpu->get_pc());
        cpu->print_regs();
        exit(1);
    }
#endif


    switch(mode)
    {
        case BYTE:
        {
            buf[addr] = v;
            return;
        }

        case HALF:
        {
           //(*(uint16_t*)(buf.data()+addr)) = v;
           memcpy(buf.data()+addr,&v,ARM_HALF_SIZE);
           return;
        }

        case WORD:
        {
           //(*(uint32_t*)(buf.data()+addr)) = v;
           memcpy(buf.data()+addr,&v,ARM_WORD_SIZE);
           return;
        }
    }
    puts("Handle_write fell through!?");
    cpu->print_regs();
    exit(1);
}


// this definitely needs to be cleaned up!
void Mem::write_io_regs(uint32_t addr,uint8_t v)
{
    addr &= IO_MASK;


    // unused areas (once we have the full map)
    // we can just make this be ignored in the default case!
    if(addr > 0x208 && addr < 0x300)
    {
        return;
    }


    switch(addr)
    {

        case IO_DISPCNT:
        {
            /*
            // enter forced blank
            if(!is_set(io[addr],7) && is_set(v,7))
            {
                puts("entering forced blank!");
                disp->set_cycles(0);
                disp->set_mode(VBLANK);
            }

            // leaving forced blank
            else if(is_set(io[addr],7) && !is_set(v,7))
            {
                disp->set_mode(VISIBLE);
            }
            */

            // gba / cgb mode is reserved
            io[addr] = v & ~8;
            break;
        }

        case IO_DISPCNT+1:
        {
            io[addr] = v;
            break;
        }

        // bit one toggle green swap (ingore for now)
        case IO_GREENSWAP:
        {
                
            break;
        }

        case IO_GREENSWAP+1:
        {
            break;
        }


        case IO_DISPSTAT:
        {
            // first 3 bits read only
            // 6 and 7 are unused
            io[addr] = v & ~0xc3;
            break;
        }

        
        case IO_DISPSTAT+1: // vcount (lyc)
        {
            io[addr] = v;
            break;
        }

        // read only!
        case IO_VCOUNT: // (ly)
        case IO_VCOUNT+1:
        {
            break;
        }

        // probs should store the bgcnts
        // in a easy to access array
        case IO_BG0CNT:
        case IO_BG0CNT+1:
        {
            io[addr] = v;
            break;
        }

        case IO_BG1CNT:
        case IO_BG1CNT+1:
        {
            io[addr] = v;
            break;
        }

        case IO_BG2CNT:
        case IO_BG2CNT+1:
        {
            io[addr] = v;
            break;
        }

        case IO_BG3CNT:
        case IO_BG3CNT+1:
        {
            io[addr] = v;
            break;
        }


        // all backgrounds function the same so we will group them

        case IO_BG0HOFS: // write only
        case IO_BG1HOFS:
        case IO_BG2HOFS:
        case IO_BG3HOFS:
        {
            io[addr] = v;
            break;
        }

        // only 1st bit used
        case IO_BG0HOFS+1:
        case IO_BG1HOFS+1:
        case IO_BG2HOFS+1:
        case IO_BG3HOFS+1:
        {
            io[addr] = v & 1;
            break;
        }


        case IO_BG0VOFS: // write only
        case IO_BG1VOFS:
        case IO_BG2VOFS:
        case IO_BG3VOFS:
        {
            io[addr] = v;
            break;
        }

        // only 1st bit used
        case IO_BG0VOFS+1:
        case IO_BG1VOFS+1:
        case IO_BG2VOFS+1:
        case IO_BG3VOFS+1:
        {
            io[addr] = v & 1;
            break;
        }




        // both have full write
        // bg2 scaling param X
        case IO_BG2PA:
        case IO_BG2PA+1:
        case IO_BG2PB:
        case IO_BG2PB+1:
        case IO_BG2PC:
        case IO_BG2PC+1:
        case IO_BG2PD:
        case IO_BG2PD+1:                        
        {
            io[addr] = v;
            break;
        }




        // both have full write
        // bg3 scaling param X
        case IO_BG3PA:
        case IO_BG3PA+1:
        case IO_BG3PB:
        case IO_BG3PB+1:
        case IO_BG3PC:
        case IO_BG3PC+1:
        case IO_BG3PD:
        case IO_BG3PD+1:                        
        {
            io[addr] = v;
            break;
        }




        // background 2 reference point registers
        // on write these copy to internal regs
        case IO_BG2X_L:
        case IO_BG2X_L+1:
        case IO_BG2Y_L:
        case IO_BG2Y_L+1:
        case IO_BG2X_H:
        case IO_BG2Y_H:                        
        {
            io[addr] = v;
            disp->load_reference_point_regs();
            break;
        }

        case IO_BG2X_H+1:
        case IO_BG2Y_H+1:        
        {
            io[addr] = v & ~0xf0;
            disp->load_reference_point_regs();
            break;
        }




        //rightmost cord of window plus 1
        case IO_WIN0H:
        {
            io[addr] = v;
            break;
        }

        // leftmost window cord
        case IO_WIN0H+1:
        {
            io[addr] = v;
            break;
        }


        // DMA 0

        // dma 0 source reg
        case IO_DMA0SAD: 
        case IO_DMA0SAD+1:
        case IO_DMA0SAD+2:
        case IO_DMA0SAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 0 dest reg
        case IO_DMA0DAD: 
        case IO_DMA0DAD+1:
        case IO_DMA0DAD+2:
        case IO_DMA0DAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 0 transfer len
        case IO_DMA0CNT_L:
        case IO_DMA0CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // dma 0 transfer control
        case IO_DMA0CNT_H:
        {
            io[addr] = v;
            break;
        }

        // dma 0 transfer control
        case IO_DMA0CNT_H+1:
        {
            io[addr] = v;
            if(is_set(v,7)) // transfer enabeld
            {
                cpu->handle_dma(Dma_type::IMMEDIATE);
            }
            break;
        }







        // DMA 1

        // dma 1 source reg
        case IO_DMA1SAD: 
        case IO_DMA1SAD+1:
        case IO_DMA1SAD+2:
        case IO_DMA1SAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 1 dest reg
        case IO_DMA1DAD: 
        case IO_DMA1DAD+1:
        case IO_DMA1DAD+2:
        case IO_DMA1DAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 1 transfer len
        case IO_DMA1CNT_L:
        case IO_DMA1CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // dma 1 transfer control
        case IO_DMA1CNT_H:
        {
            io[addr] = v;
            break;
        }

        // dma 1 transfer control
        case IO_DMA1CNT_H+1:
        {
            io[addr] = v;
            if(is_set(v,7)) // transfer enabeld
            {
                cpu->handle_dma(Dma_type::IMMEDIATE);
            }
            break;
        }



        // DMA 2

        // dma 2 source reg
        case IO_DMA2SAD: 
        case IO_DMA2SAD+1:
        case IO_DMA2SAD+2:
        case IO_DMA2SAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 2 dest reg
        case IO_DMA2DAD: 
        case IO_DMA2DAD+1:
        case IO_DMA2DAD+2:
        case IO_DMA2DAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 2 transfer len
        case IO_DMA2CNT_L:
        case IO_DMA2CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // dma 2 transfer control
        case IO_DMA2CNT_H:
        {
            io[addr] = v;
            break;
        }

        // dma 2 transfer control
        case IO_DMA2CNT_H+1:
        {
            io[addr] = v;
            if(is_set(v,7)) // transfer enabeld
            {
                cpu->handle_dma(Dma_type::IMMEDIATE);
            }
            break;
        }



        // DMA 3

        // dma 3 source reg
        case IO_DMA3SAD: 
        case IO_DMA3SAD+1:
        case IO_DMA3SAD+2:
        case IO_DMA3SAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 3 dest reg
        case IO_DMA3DAD: 
        case IO_DMA3DAD+1:
        case IO_DMA3DAD+2:
        case IO_DMA3DAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 3 transfer len
        case IO_DMA3CNT_L:
        case IO_DMA3CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // dma 3 transfer control
        case IO_DMA3CNT_H:
        {
            io[addr] = v;
            break;
        }

        // dma 3 transfer control
        case IO_DMA3CNT_H+1:
        {
            io[addr] = v;
            if(is_set(v,7)) // transfer enabeld
            {
                printf("dma started %08x\n",cpu->get_pc());
                cpu->handle_dma(Dma_type::IMMEDIATE);
            }
            break;
        }



 
        // timer 0  reload value
        case IO_TM0CNT_L:
        case IO_TM0CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // timer 0 control
        case IO_TM0CNT_H:
        {
            io[addr] = v;
            if(is_set(v,7))
            {
                // reload the timer
                cpu->set_timer(0,handle_read(io,IO_TM0CNT_L,HALF));
                break;
            }
            break;
        } 

        // unused
        case IO_TM0CNT_H+1:
        {
            break;
        }


        // timer 1  reload value
        case IO_TM1CNT_L:
        case IO_TM1CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // timer 1 control
        case IO_TM1CNT_H:
        {
            io[addr] = v;
            if(is_set(v,7))
            {
                cpu->set_timer(1,handle_read(io,IO_TM1CNT_L,HALF));
            }
            break;
        } 

        // unused
        case IO_TM1CNT_H+1:
        {
            break;
        }




        // timer 2  reload value
        case IO_TM2CNT_L:
        case IO_TM2CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // timer 2 control
        case IO_TM2CNT_H:
        {
            io[addr] = v;
            if(is_set(v,7))
            {
                cpu->set_timer(2,handle_read(io,IO_TM2CNT_L,HALF));
            }
            break;
        } 

        // unused
        case IO_TM2CNT_H+1:
        {
            break;
        }


        // timer 3  reload value
        case IO_TM3CNT_L:
        case IO_TM3CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // timer 3 control
        case IO_TM3CNT_H:
        {
            io[addr] = v;
            if(is_set(v,7))
            {
                cpu->set_timer(3,handle_read(io,IO_TM3CNT_L,HALF));
            }
            break;
        } 

        // unused
        case IO_TM3CNT_H+1:
        {
            break;
        }



        case IO_IME: // 0th bit toggles ime
        {
            ime = is_set(v,0);
            break;
        }
        case IO_IME + 1:
        { 
            break; // do nothing
        }


        case IO_IE: // interrupt enable
        {
            io[addr] = v;
            break;
        }

        case IO_IE+1:
        {
            io[addr] = v & ~0xc0;
            break;
        }

        case IO_IF: // interrupt flag writing a 1 desets the bit
        {
            io[addr] &= ~v;
            break;
        }

        case IO_IF+1:
        {
            io[addr] &= ~(v & ~0xc0);
            break;
        }

        case IO_WAITCNT: // configures game pak access times
        {
            io[addr] = v;
            break;
        }

        case IO_WAITCNT+1: 
        {
            io[addr] = v & ~0x20;
            break;
        }        

        
        case IO_WAITCNT+2: // top half not used
        case IO_WAITCNT+3: 
        {
            break;
        }



        //unused
        case 0x400020A&IO_MASK: 
        case 0x400020B&IO_MASK:
        { 
            break;
        }

        // gba bios inits this to one to know its not in initial boot 
        case IO_POSTFLG:
        {
            io[addr] = v & 1;
            break;
        }



        default:
        {    
            //printf("unknown io reg write at %08x:%08x\n",addr,cpu->get_pc());
            io[addr] = v; // get this done fast <--- fix later
            //exit(1);
        }
    }
}

// this will require special handling
void Mem::write_io(uint32_t addr,uint32_t v,Access_type mode)
{
    mem_region = IO;
    //io[addr & 0x3ff] = v;


    switch(mode)
    {
        // read out the adjacent byte
        // write back our byte plus the existing one...
        case BYTE: 
        {
            write_io_regs(addr,v&0xff);
            break;
        }

        case WORD: // write lower and higher
        {
            write_io_regs(addr,v&0xff);
            write_io_regs(addr+1,(v&0xff00) >> 8); 
            write_io_regs(addr+2,(v&0xff0000) >> 16);
            write_io_regs(addr+3,(v&0xff000000) >> 24);   
            break;
        }

        case HALF: // in the correct format
        {
            write_io_regs(addr,v&0xff);
            write_io_regs(addr+1,(v&0xff00) >> 8);
            break;
        }
    } 
}

void Mem::write_oam(uint32_t addr,uint32_t v,Access_type mode)
{
    mem_region = OAM;
    //oam[addr & 0x3ff] = v;
    handle_write(oam,addr&0x3ff,v,mode);
}

void Mem::write_vram(uint32_t addr,uint32_t v,Access_type mode)
{
    mem_region = VRAM;
    //vram[addr-0x06000000] = v;
    handle_write(vram,addr-0x06000000,v,mode); 
}

void Mem::write_pal_ram(uint32_t addr,uint32_t v,Access_type mode)
{
    mem_region = PAL;
    //pal_ram[addr & 0x3ff] = v;
    handle_write(pal_ram,addr&0x3ff,v,mode);
}

void Mem::write_board_wram(uint32_t addr,uint32_t v,Access_type mode)
{
    mem_region = WRAM_BOARD;
    //return board_wram[addr & 0x3ffff] = v;
    handle_write(board_wram,addr&0x3ffff,v,mode);
}

void Mem::write_chip_wram(uint32_t addr,uint32_t v,Access_type mode)
{
    mem_region = WRAM_CHIP;
    //chip_wram[addr & 0x7fff] = v;
    handle_write(chip_wram,addr&0x7fff,v,mode);
}