#pragma once
#include "forward_def.h"
#include "lib.h"

class Mem
{
public:
    void init(std::string filename,Debugger *debug, Cpu *cpu);


    // read mem unticked
    uint8_t read_byte(uint32_t addr);
    uint16_t read_half(uint32_t addr);
    uint32_t read_word(uint32_t addr);

    // write mem unticked
    void write_byte(uint32_t addr,uint8_t v);
    void write_half(uint32_t addr,uint16_t v);
    void write_word(uint32_t addr,uint32_t v);

    // read mem ticked
    uint8_t read_bytet(uint32_t addr);
    uint16_t read_halft(uint32_t addr);
    uint32_t read_wordt(uint32_t addr);

    // write mem ticked
    void write_bytet(uint32_t addr,uint8_t v);
    void write_halft(uint32_t addr,uint16_t v);
    void write_wordt(uint32_t addr,uint32_t v);


private:
    Debugger *debug;
    Cpu *cpu;
    
    // how many cpu cylces the last access took
    int mem_cycles; 

    // general memory
    // bios code
    std::vector<uint8_t> bios_rom; // 0x4000

    // on board work ram
    std::vector<uint8_t> board_wram; // 0x40000

    // on chip wram
    std::vector<uint8_t> chip_wram; // 8000

    std::vector<uint8_t> io; // 0x400


    // display memory

    // bg/obj pallette ram
    std::vector<uint8_t> bg_ram; // 0x400

    // video ram
    std::vector<uint8_t> vram; // 0x18000

    // object attribute map
    std::vector<uint8_t> oam; // 0x400 

    // external memory

    // main game rom
    std::vector<uint8_t> rom; // variable
};