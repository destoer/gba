#pragma once
#include "forward_def.h"

class Display
{
public:
    void init(Mem *mem);
private:
    Mem *mem;
};