#pragma once
#include "forward_def.h"
#include "lib.h"
#include "arm.h"

class Debugger
{
public:

    void init(Mem *mem, Cpu *cpu, Display *display, Disass *disass);


    void enter_debugger();
    void disable_breakpoints()
    {
        breakpoint_r.disable();
        breakpoint_w.disable();
        breakpoint_x.disable();
    }

    void enable_breakpoints()
    {
        breakpoint_r.enable();
        breakpoint_w.enable();
        breakpoint_x.enable();
    }

    void save_breakpoints()
    {
        bk_breakpoint_r = breakpoint_r;
        bk_breakpoint_w = breakpoint_w;
        bk_breakpoint_x = breakpoint_x;
    }


    void restore_breakpoints()
    {
        breakpoint_r = bk_breakpoint_r;
        breakpoint_w = bk_breakpoint_w;
        breakpoint_x = bk_breakpoint_x;
    }

    /*
        write
        disass
        assemble 
    */

    // struct for hold program breakpoints
    struct Breakpoint
    {


        Breakpoint()
        {
            this->value = 0xdeadbeef;
            this->addr = 0xdeadbeef;
            this->value_enabled = false;
            this->break_enabled = false;            
        }

        void set(uint32_t value,uint32_t addr, 
            bool value_enabled,bool break_enabled)
        {
            this->value = value;
            this->addr = addr;
            this->value_enabled = value_enabled;
            this->break_enabled = break_enabled;
        }

        void disable()
        {
            break_enabled = false;
        }

        void enable()
        {
            break_enabled = true;
        }

        template<typename access_type>
        bool is_hit(uint32_t addr, access_type value)
        {
            if(!break_enabled)
            {
                return false;
            }

            // value enabled and its not equal
            else if(value_enabled && value != this->value)
            {
                return false;
            }

            // not breakpointed on this addr
            else if(addr != this->addr)
            {
                return false;
            }

            // must be true
            return true;
        }

        uint32_t value;
        bool value_enabled;
        bool break_enabled;
        uint32_t addr;
    };


    // rwx breakpoints
    Breakpoint breakpoint_r;
    Breakpoint breakpoint_w;
    Breakpoint breakpoint_x;


    bool step_instr = false;

private:
    Mem *mem;
    Cpu *cpu;
    Display *disp;
    Disass *disass;

    bool debug_quit = false;


    // breakpoint backups
    Breakpoint bk_breakpoint_r;
    Breakpoint bk_breakpoint_w;
    Breakpoint bk_breakpoint_x;    


    void set_break_type(std::string command,uint32_t value,uint32_t addr, bool value_enabled);

    void run(std::vector<std::string> command);
    void breakpoint(std::vector<std::string> command);
    void clear(std::vector<std::string> command);
    void step(std::vector<std::string> command);
    void info(std::vector<std::string> command);
    void disass_addr(std::vector<std::string> command);
    void exec(std::vector<std::string> command);
    void write(std::vector<std::string> command);


    static constexpr int PAL_X = 16;
    static constexpr int PAL_Y = 16;

    std::array<uint32_t,PAL_X*PAL_Y> pal_screen{0};
	SDL_Window * pal_window;
	SDL_Renderer * pal_renderer;
	SDL_Texture * pal_texture;

    void palette_viewer(std::vector<std::string> command);



    void tile_viewer(std::vector<std::string> command);

    static constexpr int TILE_X = 32 * 8;
    static constexpr int TILE_Y = 32 * 8;

    std::vector<uint32_t> tile_screen;
	SDL_Window * tile_window;
	SDL_Renderer * tile_renderer;
	SDL_Texture * tile_texture;


    using DEBUGGER_FPTR = void (Debugger::*)(std::vector<std::string>);
    std::vector<std::string> commands{"run","break","clear","step","info","disass","write","exec","pal_viewer","tile_viewer"};
    std::vector<DEBUGGER_FPTR> command_funcs{&Debugger::run,&Debugger::breakpoint,&Debugger::clear,&Debugger::step,&Debugger::info,&Debugger::disass_addr,
        &Debugger::write,&Debugger::exec,&Debugger::palette_viewer,&Debugger::tile_viewer};

};