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


// gba is locked to little endian

 // read mem unticked
uint8_t Mem::read_byte(uint32_t addr)
{
    return 0xff; // stub
}
uint16_t Mem::read_half(uint32_t addr)
{
    uint16_t v = read_byte(addr);
    v |= (read_byte(addr+1) << 8);
    return v;
}

uint32_t Mem::read_word(uint32_t addr)
{
    uint32_t v = read_byte(addr);
    v |= (read_byte(addr+1) << 8);
    v |= (read_byte(addr+2) << 16);
    v |= (read_byte(addr+3) << 24);
    return v;
}


// write mem unticked
void Mem::write_byte(uint32_t addr,uint8_t v)
{

}

void Mem::write_half(uint32_t addr,uint16_t v)
{
    write_byte(addr,v & 0xff);
    write_byte(addr+1,(v & 0xff00) >> 8);
}

void Mem::write_word(uint32_t addr,uint32_t v)
{
    write_byte(addr,v & 0xff);
    write_byte(addr+1,(v & 0xff00) >> 8);
    write_byte(addr+2,(v & 0xff0000) >> 16);
    write_byte(addr+3,(v & 0xff000000) >> 24);    
}

// read mem ticked
uint8_t Mem::read_bytet(uint32_t addr)
{
    uint8_t v = read_byte(addr);
    cpu->cycle_tick(mem_cycles);
    return v;
}

uint16_t Mem::read_halft(uint32_t addr)
{
    uint16_t v = read_bytet(addr);
    v |= (read_bytet(addr+1) << 8);
    return v;    
}

uint32_t Mem::read_wordt(uint32_t addr)
{
    uint32_t v = read_bytet(addr);
    v |= (read_bytet(addr+1) << 8);
    v |= (read_bytet(addr+2) << 16);
    v |= (read_bytet(addr+3) << 24);
    return v;    
}

// write mem ticked
void Mem::write_bytet(uint32_t addr,uint8_t v)
{
    write_byte(addr,v);
    cpu->cycle_tick(mem_cycles);
}

void Mem::write_halft(uint32_t addr,uint16_t v)
{
    write_bytet(addr,v & 0xff);
    write_bytet(addr+1,(v & 0xff00) >> 8);
}

void Mem::write_wordt(uint32_t addr,uint32_t v)
{
    write_bytet(addr,v & 0xff);
    write_bytet(addr+1,(v & 0xff00) >> 8);
    write_bytet(addr+2,(v & 0xff0000) >> 16);
    write_bytet(addr+3,(v & 0xff000000) >> 24);   
}