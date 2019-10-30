#pragma once
#include "cpu.h"
#include "memory.h"
#include "disass.h"
#include "display.h"
#include "debugger.h"
#include <SDL2/SDL.h>

class GBA
{
public:


     GBA(std::string filename);
    ~GBA();
    void run();
    
    

    void enter_debugger()
    {
        debug.enter_debugger();
    }

private:


    void handle_input();
    void init_screen();

    Cpu cpu;
    Mem mem;
    Disass disass;
    Display disp;
    Debugger debug;



    // screen stuff
	SDL_Window * window;
	SDL_Renderer * renderer;
	SDL_Texture * texture;
    uint32_t next_time;
};