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
    io[IO_KEYINPUT+1] = 0xff;




}




void Mem::tick_mem_access(int mode)
{
    if(mem_region != UNDEFINED)
    {
        cpu->cycle_tick(wait_states[mem_region][mode]);
    }
}





// gba is locked to little endian

uint32_t Mem::read_external(uint32_t addr,Access_type mode)
{

    if((addr&0x1FFFFFF) > rom.size())
    {
        printf("rom read out of range: %08x:%08x\n",addr&0x1FFFFFF,rom.size());
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
                puts("sram read");
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
    exit(1);    
}

//access handler for reads (for non io mapped mem)
// need checks for endianess here for completeness
uint32_t Mem::handle_read(std::vector<uint8_t> &buf,uint32_t addr,Access_type mode)
{
    switch(mode)
    {
        case BYTE:
        {
            return buf[addr];
        }

        case HALF:
        {
            return(*(uint16_t*)(buf.data()+addr));
        }

        case WORD:
        {
            return(*(uint32_t*)(buf.data()+addr));
        }
    }
    puts("Handle_read fell through!?");
    exit(1);
}

// this will require special handling
uint8_t Mem::read_io_regs(uint32_t addr)
{
    addr &= IO_MASK;
    switch(addr)
    {

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


        case IO_KEYINPUT:
        {
            return io[IO_KEYINPUT];
            break;
        }

      
        case IO_KEYINPUT+1: // 10-15 unused
        {
            return io[IO_KEYINPUT];
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

        //unused
        case 0x400020A: 
        case 0x400020B:
        { 
            return 0;
            break;
        }

        default:
        {    
            printf("unknown io reg read at %08x\n",addr);
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
            printf("read breakpoint hit at %08x\n",addr);
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
            printf("write breakpoint hit at %08x\n",addr);
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
                exit(1);
            }
                
            else // unused
            {
                mem_region = UNDEFINED;
                return;
            }
        }
    }
    printf("write_external fell through %08x\n",addr);
    exit(1);    
}

//access handler for reads (for non io mapped mem)
// need checks for endianess here for completeness
void Mem::handle_write(std::vector<uint8_t> &buf,uint32_t addr,uint32_t v,Access_type mode)
{
    switch(mode)
    {
        case BYTE:
        {
            buf[addr] = v;
            return;
        }

        case HALF:
        {
           (*(uint16_t*)(buf.data()+addr)) = v;
           return;
        }

        case WORD:
        {
           (*(uint32_t*)(buf.data()+addr)) = v;
           return;
        }
    }
    puts("Handle_write fell through!?");
    exit(1);
}


void Mem::write_io_regs(uint32_t addr,uint8_t v)
{
    addr &= IO_MASK;
    switch(addr)
    {


        case IO_DISPCNT:
        {
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

        case IO_BG0HOFS: // write only
        {
            io[addr] = v;
            break;
        }

        // only 1st bit used
        case IO_BG0HOFS+1:
        {
            io[addr] = v & 1;
            break;
        }


        case IO_BG0VOFS: // write only
        {
            io[addr] = v;
            break;
        }

        // only 1st bit used
        case IO_BG0VOFS+1:
        {
            io[addr] = v & 1;
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

        //unused
        case 0x400020A&IO_MASK: 
        case 0x400020B&IO_MASK:
        { 
            break;
        }




        default:
        {    
            printf("unknown io reg write at %08x\n",addr);
            exit(1);
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