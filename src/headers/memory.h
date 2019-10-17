#pragma once
#include "forward_def.h"
#include "lib.h"
#include "arm.h"

// not really happy with the impl 
// so think of a better way to model it
class Mem
{
public:
    void init(std::string filename,Debugger *debug, Cpu *cpu);


    // read mem unticked
    uint32_t read_mem(uint32_t addr,Access_type mode);


private:
    Debugger *debug;
    Cpu *cpu;
    

    void tick_mem_access(int mode);
    inline uint32_t handle_read(std::vector<uint8_t> &buf,uint32_t addr,Access_type mode);


    uint32_t read_bios(uint32_t addr,Access_type type);
    uint32_t read_board_wram(uint32_t addr,Access_type type);
    uint32_t read_chip_wram(uint32_t addr,Access_type type);
    uint32_t read_io(uint32_t addr,Access_type type);
    uint32_t read_obj_ram(uint32_t addr,Access_type type);
    uint32_t read_vram(uint32_t addr,Access_type type);
    uint32_t read_oam(uint32_t addr,Access_type type);
    uint32_t read_external(uint32_t addr,Access_type type);



    // last accessed memory region
    enum Memory_region
    {
        BIOS = 0,WRAM_BOARD,WRAM_CHIP,
        IO,BG,VRAM,OAM,ROM,FLASH,SRAM,
        UNDEFINED
    };

    // memory cycle timings
    // some can be set dynamically
    int wait_states[10][3];

    Memory_region mem_region;

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