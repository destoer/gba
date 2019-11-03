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


    // read mem
    uint32_t handle_read(std::vector<uint8_t> &buf,uint32_t addr,Access_type mode);
    uint32_t read_mem(uint32_t addr,Access_type mode);
    uint32_t read_memt(uint32_t addr,Access_type mode);

    // write mem
    void handle_write(std::vector<uint8_t> &buf,uint32_t addr,uint32_t v,Access_type mode);
    void write_mem(uint32_t addr,uint32_t v,Access_type mode);    
    void write_memt(uint32_t addr,uint32_t v,Access_type mode);


    // probablly a better way do this than to just give free reign 
    // over the array (i.e for the video stuff give display class ownership)

    // io regs
    std::vector<uint8_t> io; // 0x400

    // video ram
    std::vector<uint8_t> vram; // 0x18000

    // display memory

    // bg/obj pallette ram
    std::vector<uint8_t> bg_ram; // 0x400

    // object attribute map
    std::vector<uint8_t> oam; // 0x400 


private:
    Debugger *debug;
    Cpu *cpu;
    

    void tick_mem_access(int mode);

    // read_mem helpers
    uint32_t read_bios(uint32_t addr,Access_type type);
    uint32_t read_board_wram(uint32_t addr,Access_type type);
    uint32_t read_chip_wram(uint32_t addr,Access_type type);
    uint32_t read_io(uint32_t addr,Access_type type);
    uint8_t read_io_regs(uint32_t addr);
    uint32_t read_obj_ram(uint32_t addr,Access_type type);
    uint32_t read_vram(uint32_t addr,Access_type type);
    uint32_t read_oam(uint32_t addr,Access_type type);
    uint32_t read_external(uint32_t addr,Access_type type);

    // write mem helpers
    void write_board_wram(uint32_t addr,uint32_t v,Access_type mode);
    void write_chip_wram(uint32_t addr,uint32_t v,Access_type mode);
    void write_io(uint32_t addr,uint32_t v,Access_type mode);
    void write_io_regs(uint32_t addr,uint8_t v);
    void write_obj_ram(uint32_t addr,uint32_t v,Access_type mode);
    void write_vram(uint32_t addr,uint32_t v,Access_type mode);
    void write_oam(uint32_t addr,uint32_t v,Access_type mode);
    void write_external(uint32_t addr,uint32_t v,Access_type mode);

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
    std::vector<uint8_t> chip_wram; // 0x8000


    bool ime = true;


    // external memory

    // main game rom
    std::vector<uint8_t> rom; // variable

};






// memory constants
constexpr int IO_MASK = 0x3ff;


constexpr int IO_IME = 0x04000208;
constexpr int IO_DISPCNT = 0x04000000;
constexpr int IO_GREENSWAP = 0x04000002;
constexpr int IO_DISPSTAT = 0x04000004;
constexpr int IO_VCOUNT = 0x04000006;
constexpr int IO_BG0CNT = 0x04000008;
constexpr int IO_BG1CNT = 0x0400000a;
constexpr int IO_BG2CNT = 0x0400000c;
constexpr int IO_BG3CNT = 0x0400000e;
constexpr int IO_KEYINPUT = 0x04000130;