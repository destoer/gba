#include "headers/gba.h"
#include <stdio.h>

int main(int argc, char *argv[])
{

    if(argc != 2)
    {
        printf("Usage %s <rom name>",argv[0]);
        return 0;
    }

    GBA gba(argv[1]);

    gba.enter_debugger();

    // start the emulation
    gba.run();

    return 0;   
}