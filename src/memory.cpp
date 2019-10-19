#include "headers/memory.h"
#include "headers/cpu.h"

void Mem::init(std::string filename, Debugger *debug,Cpu *cpu)
{
    // init component
    this->debug = debug;
    this->cpu = cpu;

    // read out rom
    read_file(filename,rom);

    

    // alloc our underlying system memory
    bios_rom.resize(0x4000);
    board_wram.resize(0x40000);
    chip_wram.resize(8000);
    io.resize(0x400);
    bg_ram.resize(0x400);
    vram.resize(0x18000);
    oam.resize(0x400); 
    
    // read out rom info here...
    std::cout << "rom size: " << rom.size() << "\n";
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
    switch((addr >> 24) & 0xf)
    {
        case 0x8: // wait state 0
        case 0x9:
        {
            mem_region = ROM;
            //return rom[addr - 0x08000000];
            return handle_read(rom,addr-0x08000000,mode);
            break;
        }

        case 0xa: // wait state 1
        case 0xb:
        {
            mem_region = ROM;
            //return rom[addr - 0x0a000000];
            return handle_read(rom,addr-0x0a000000,mode);
            break;
        }
            
        case 0xc: // wait state 2
        case 0xd:
        {
            mem_region = ROM;
            //return rom[addr - 0x0c000000];
            return handle_read(rom,addr-0x0c000000,mode);
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
inline uint32_t Mem::handle_read(std::vector<uint8_t> &buf,uint32_t addr,Access_type mode)
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
uint32_t Mem::read_io(uint32_t addr,Access_type mode)
{
    puts("IO READ!");
    exit(1);
    mem_region = IO;
    //return io[addr & 0x3ff];
    return handle_read(io,addr&0x3ff,mode);
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

uint32_t Mem::read_obj_ram(uint32_t addr,Access_type mode)
{
    mem_region = BG;
    //return bg_ram[addr & 0x3ff];
    return handle_read(bg_ram,addr&0x3ff,mode);
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

    // 28 bit bus
    addr &= 0x0fffffff;

    uint32_t v;
    if(addr < 0x00004000) v = read_bios(addr,mode);
    else if(addr < 0x02000000) { mem_region = UNDEFINED; return 0; }
    else if(addr < 0x03000000) v = read_board_wram(addr,mode);
    else if(addr < 0x04000000) v = read_chip_wram(addr,mode);
    else if(addr < 0x05000000) v = read_io(addr,mode);
    else if(addr < 0x06000000) v = read_obj_ram(addr,mode);
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