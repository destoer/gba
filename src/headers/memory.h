#pragma once
#include "forward_def.h"
#include "lib.h"

class Mem
{
public:
    void init(std::string filename,Debugger *debug);


    // read mem unticked
    uint8_t read_memb(uint32_t addr);
    uint16_t read_memh(uint32_t addr);
    uint32_t read_word(uint32_t addr);

    // write mem unticked
    void write_memb(uint32_t addr,uint8_t v);
    void write_memh(uint32_t addr,uint16_t v);
    void write_memw(uint32_t addr,uint32_t v);

    // read mem ticked
    uint8_t read_membt(uint32_t addr);
    uint16_t read_memht(uint32_t addr);
    uint32_t read_wordt(uint32_t addr);

    // write mem ticked
    void write_membt(uint32_t addr,uint8_t v);
    void write_memht(uint32_t addr,uint16_t v);
    void write_memwt(uint32_t addr,uint32_t v);


private:
    Debugger *debug;

    // underlying memory storage
    std::vector<uint8_t> rom;
};