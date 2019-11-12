#pragma once
#include "cpu.h"
#include "memory.h"
#include "disass.h"
#include "display.h"
#include "debugger.h"


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


    enum class Button 
    {
        A = 0,B=1,SELECT=2,START=3,
        RIGHT=4,LEFT=5,UP=6,DOWN=7,
        R=8,L=9
    };


    void handle_input();
    void init_screen();
    void button_event(Button b, bool down);

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

