#pragma once
#include "forward_def.h"
#include "lib.h"
#include "arm.h"

// not really happy with the impl 
// so think of a better way to model it
class Mem
{
public:
    void init(std::string filename,Debugger *debug, Cpu *cpu, Display *disp);


    // read mem
    uint32_t handle_read(std::vector<uint8_t> &buf,uint32_t addr,Access_type mode);
    uint32_t read_mem(uint32_t addr,Access_type mode);
    uint32_t read_memt(uint32_t addr,Access_type mode);

    // write mem
    void handle_write(std::vector<uint8_t> &buf,uint32_t addr,uint32_t v,Access_type mode);
    void write_mem(uint32_t addr,uint32_t v,Access_type mode);    
    void write_memt(uint32_t addr,uint32_t v,Access_type mode);

    bool get_ime() const { return ime; }

    // probablly a better way do this than to just give free reign 
    // over the array (i.e for the video stuff give display class ownership)

    // io regs
    std::vector<uint8_t> io; // 0x400

    // video ram
    std::vector<uint8_t> vram; // 0x18000

    // display memory

    // bg/obj pallette ram
    std::vector<uint8_t> pal_ram; // 0x400

    // object attribute map
    std::vector<uint8_t> oam; // 0x400 


private:
    Debugger *debug;
    Cpu *cpu;
    Display *disp;

    void tick_mem_access(int mode);

    // read_mem helpers
    uint32_t read_bios(uint32_t addr,Access_type type);
    uint32_t read_board_wram(uint32_t addr,Access_type type);
    uint32_t read_chip_wram(uint32_t addr,Access_type type);
    uint32_t read_io(uint32_t addr,Access_type type);
    uint8_t read_io_regs(uint32_t addr);
    uint32_t read_pal_ram(uint32_t addr,Access_type type);
    uint32_t read_vram(uint32_t addr,Access_type type);
    uint32_t read_oam(uint32_t addr,Access_type type);
    uint32_t read_external(uint32_t addr,Access_type type);

    // write mem helpers
    void write_board_wram(uint32_t addr,uint32_t v,Access_type mode);
    void write_chip_wram(uint32_t addr,uint32_t v,Access_type mode);
    void write_io(uint32_t addr,uint32_t v,Access_type mode);
    void write_io_regs(uint32_t addr,uint8_t v);
    void write_pal_ram(uint32_t addr,uint32_t v,Access_type mode);
    void write_vram(uint32_t addr,uint32_t v,Access_type mode);
    void write_oam(uint32_t addr,uint32_t v,Access_type mode);
    void write_external(uint32_t addr,uint32_t v,Access_type mode);

    // last accessed memory region
    enum Memory_region
    {
        BIOS = 0,WRAM_BOARD,WRAM_CHIP,
        IO,PAL,VRAM,OAM,ROM,FLASH,SRAM,
        UNDEFINED
    };

    // memory cycle timings
    // some can be set dynamically
    // b w h
    int wait_states[10][3]  = 
    {
        {1,1,1}, // bios rom
        {1,1,1}, // wram 32k
        {1,1,1}, // io
        {1,1,1}, // oam
        {3,3,6}, // wram 256k
        {1,1,2}, // pallete ram
        {1,1,2}, // vram
        {5,5,8}, // gamepak rom
        {5,5,8}, // gamepak flash
        {5,5,5} // sram
    };

    Memory_region mem_region;

    // general memory
    // bios code
    std::vector<uint8_t> bios_rom; // 0x4000

    // on board work ram
    std::vector<uint8_t> board_wram; // 0x40000

    // on chip wram
    std::vector<uint8_t> chip_wram; // 0x8000

    // cart save ram
    std::vector<uint8_t> sram; // 0xffff

    bool ime = true;


    // external memory

    // main game rom
    std::vector<uint8_t> rom; // variable

};






// memory constants
constexpr uint32_t IO_MASK = 0x3ff;

// interrupts
constexpr uint32_t IO_IE = 0x04000200 & IO_MASK;
constexpr uint32_t IO_IF = 0x04000202 & IO_MASK;
constexpr uint32_t IO_IME = 0x04000208 & IO_MASK;


constexpr uint32_t IO_WAITCNT = 0x04000204 & IO_MASK;
constexpr uint32_t IO_DISPCNT = 0x04000000 & IO_MASK;
constexpr uint32_t IO_GREENSWAP = 0x04000002 & IO_MASK;
constexpr uint32_t IO_DISPSTAT = 0x04000004 & IO_MASK;
constexpr uint32_t IO_VCOUNT = 0x04000006 & IO_MASK;
constexpr uint32_t IO_BG0CNT = 0x04000008 & IO_MASK;
constexpr uint32_t IO_BG1CNT = 0x0400000a & IO_MASK;
constexpr uint32_t IO_BG2CNT = 0x0400000c & IO_MASK;
constexpr uint32_t IO_BG3CNT = 0x0400000e & IO_MASK;
constexpr uint32_t IO_BG0HOFS = 0x04000010 & IO_MASK; // scroll x for bg0
constexpr uint32_t IO_BG0VOFS = 0x04000012 & IO_MASK; // scroll y for bg0
constexpr uint32_t IO_BG1HOFS = 0x04000014 & IO_MASK; // scroll x for bg1
constexpr uint32_t IO_BG1VOFS = 0x04000016 & IO_MASK; // scroll y for bg1
constexpr uint32_t IO_BG2HOFS = 0x04000018 & IO_MASK; // scroll x for bg2
constexpr uint32_t IO_BG2VOFS = 0x0400001a & IO_MASK; // scroll y for bg2
constexpr uint32_t IO_BG3HOFS = 0x0400001c & IO_MASK; // scroll x for bg3
constexpr uint32_t IO_BG3VOFS = 0x0400001e & IO_MASK; // scroll y for bg3
constexpr uint32_t IO_BG2PA = 0x04000020 & IO_MASK;
constexpr uint32_t IO_BG2PB = 0x04000022 & IO_MASK;
constexpr uint32_t IO_BG2PC = 0x04000024 & IO_MASK;
constexpr uint32_t IO_BG2PD = 0x04000026 & IO_MASK;
constexpr uint32_t IO_BG3PA = 0x04000030 & IO_MASK;
constexpr uint32_t IO_BG3PB = 0x04000032 & IO_MASK;
constexpr uint32_t IO_BG3PC = 0x04000034 & IO_MASK;
constexpr uint32_t IO_BG3PD = 0x04000036 & IO_MASK;
constexpr uint32_t IO_BG2X_L = 0x04000028 & IO_MASK;
constexpr uint32_t IO_BG2X_H = 0x0400002a & IO_MASK;
constexpr uint32_t IO_BG2Y_L = 0x0400002c & IO_MASK;
constexpr uint32_t IO_BG2Y_H = 0x0400002e & IO_MASK;
constexpr uint32_t IO_WIN0H = 0x04000040 & IO_MASK; // window 0 horizontal dimensions

// dma 0
constexpr uint32_t IO_DMA0SAD = 0x040000b0 & IO_MASK;
constexpr uint32_t IO_DMA0DAD = 0x040000b4 & IO_MASK;
constexpr uint32_t IO_DMA0CNT_L = 0x040000b8 & IO_MASK;
constexpr uint32_t IO_DMA0CNT_H = 0x040000Ba & IO_MASK;


// dma 1
constexpr uint32_t IO_DMA1SAD = 0x040000bc & IO_MASK;
constexpr uint32_t IO_DMA1DAD = 0x040000c0 & IO_MASK;
constexpr uint32_t IO_DMA1CNT_L = 0x040000c4 & IO_MASK;
constexpr uint32_t IO_DMA1CNT_H = 0x040000c6 & IO_MASK;


// dma 2
constexpr uint32_t IO_DMA2SAD = 0x040000c8 & IO_MASK;
constexpr uint32_t IO_DMA2DAD = 0x040000cc & IO_MASK;
constexpr uint32_t IO_DMA2CNT_L = 0x040000d0 & IO_MASK;
constexpr uint32_t IO_DMA2CNT_H = 0x040000d2 & IO_MASK;

// dma 3
constexpr uint32_t IO_DMA3SAD = 0x040000d4 & IO_MASK;
constexpr uint32_t IO_DMA3DAD = 0x040000d8 & IO_MASK;
constexpr uint32_t IO_DMA3CNT_L = 0x04000dc & IO_MASK;
constexpr uint32_t IO_DMA3CNT_H = 0x040000de & IO_MASK;



// timers
constexpr uint32_t IO_TM0CNT_L = 0x04000100 & IO_MASK;
constexpr uint32_t IO_TM0CNT_H = 0x04000102 & IO_MASK;
constexpr uint32_t IO_TM1CNT_L = 0x04000104 & IO_MASK;
constexpr uint32_t IO_TM1CNT_H = 0x04000106 & IO_MASK;
constexpr uint32_t IO_TM2CNT_L = 0x04000108 & IO_MASK;
constexpr uint32_t IO_TM2CNT_H = 0x0400010a & IO_MASK;
constexpr uint32_t IO_TM3CNT_L = 0x0400010c & IO_MASK;
constexpr uint32_t IO_TM3CNT_H = 0x0400010e & IO_MASK;


constexpr uint32_t IO_KEYINPUT = 0x04000130 & IO_MASK;
constexpr uint32_t IO_KEYCNT = 0x04000132 & IO_MASK;
constexpr uint32_t IO_POSTFLG = 0x040000300 & IO_MASK;


constexpr uint32_t IO_SOUNDBIAS = 0x040000088 & IO_MASK;