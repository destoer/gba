#include "headers/debugger.h"
#include "headers/arm.h"
#include "headers/memory.h"
#include "headers/cpu.h"
#include "headers/disass.h"
#include "headers/display.h"
#include "headers/lib.h"
#include <sstream>
#include <exception>


void Debugger::init(Mem *mem, Cpu *cpu, Display *disp, Disass *disass)
{
    this->mem = mem;
    this->cpu = cpu;
    this->disp = disp;
    this->disass = disass;


    
    tile_screen.resize(TILE_Y*TILE_X);
}


void Debugger::palette_viewer(std::vector<std::string> command)
{

    UNUSED(command);

	// sdl setup 
	
	// initialize our window
	pal_window = SDL_CreateWindow("Palette viewer",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,PAL_X*30,PAL_Y*20,SDL_WINDOW_RESIZABLE);
	// set a render for our window
	pal_renderer = SDL_CreateRenderer(pal_window, -1, SDL_RENDERER_ACCELERATED);

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"); // crashes without this on windows?

	pal_texture = SDL_CreateTexture(pal_renderer,SDL_PIXELFORMAT_RGB888, 
        SDL_TEXTUREACCESS_STATIC, PAL_X, PAL_Y);	





    for(size_t i = 0; i < pal_screen.size(); i++)
    {
        pal_screen[i] = convert_color(mem->handle_read(mem->pal_ram,i*2,HALF));
    }


    // do our screen blit
	SDL_UpdateTexture(pal_texture, NULL, pal_screen.data(),  4 * PAL_X);
	SDL_RenderCopy(pal_renderer, pal_texture, NULL, NULL);
	SDL_RenderPresent(pal_renderer);


    SDL_Event event;

    bool quit = false;

    while(!quit)
    {
        // handle input
        while(SDL_PollEvent(&event))
        {	
            switch(event.type) 
            {
                case SDL_WINDOWEVENT:
                {
                    if(event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        SDL_SetWindowSize(pal_window,event.window.data1, event.window.data2);
                    }

                    else if(event.window.event == SDL_WINDOWEVENT_CLOSE)
                    {
                        SDL_DestroyRenderer(pal_renderer);
                        SDL_DestroyWindow(pal_window); 
                        return;                       
                    }
                    break;
                }
            }
        }
        SDL_Delay(100);
    }

}


// add options to specify the background and pallette we will ignore these details for now however
void Debugger::tile_viewer(std::vector<std::string> command)
{


    if(command.size() != 3)
    {
        puts("invalid number of args!");
        return;
    }

    int bg_num;

    try
    {
        bg_num = std::stoi(command[1]);
    }

    catch(std::exception &e)
    {
        std::cout << "unable to convert: " << command[1] << "to an int\n";
        std::cout << "exception: " << e.what() << "\n";
        return;
    }

    if(bg_num > 3)
    {
        printf("bg out of range %x\n",bg_num);
        return;
    }



    int pal_num;

    try
    {
        pal_num = std::stoi(command[2]);
    }

    catch(std::exception &e)
    {
        std::cout << "unable to convert: " << command[2] << "to an int\n";
        std::cout << "exception: " << e.what() << "\n";
        return;
    }


    if(pal_num > 15)
    {
        printf("bg pal out of range: %d\n",pal_num);
        return;
    }


	// sdl setup 
	
	// initialize our window
	tile_window = SDL_CreateWindow("tile viewer",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,TILE_X*2,TILE_Y*2,SDL_WINDOW_RESIZABLE);
	// set a render for our window
	tile_renderer = SDL_CreateRenderer(tile_window, -1, SDL_RENDERER_ACCELERATED);

	tile_texture = SDL_CreateTexture(tile_renderer,SDL_PIXELFORMAT_RGB888, 
        SDL_TEXTUREACCESS_STATIC, TILE_X, TILE_Y);	





    uint16_t bg0_cnt = mem->handle_read(mem->io,IO_BG0CNT+bg_num*ARM_WORD_SIZE,HALF);
    uint32_t bg_tile_data_base = ((bg0_cnt >> 2) & 0x3) * 0x4000;

    //256 color one pal 8bpp? or 16 color 16 pal 4bpp 
    //bool col_256 = is_set(bg0_cnt,7); // 4bpp assumed
        
    // dump all the tiles to the screen
    for(int i = 0; i < 1024; i++)
    {

        // now we will rip the tile and blit it at the first space
        uint32_t tile_addr = bg_tile_data_base+(i*0x20); 

        for(int y = 0; y < 8; y++) 
        {
            for(int x = 0; x < 8; x += 2)
            {
                // read out the color indexs from the tile
                uint32_t tile_offset = ((8 * y) / 2) + (x / 2);
                uint8_t tile = mem->handle_read(mem->vram,tile_addr+tile_offset,BYTE);
                uint32_t idx1 =  tile & 0xf;
                uint32_t idx2 = (tile >> 4) & 0xf;

                // read out the colors
                uint16_t color1 = mem->handle_read(mem->pal_ram,(0x20*pal_num)+idx1*2,HALF);
                uint16_t color2 = mem->handle_read(mem->pal_ram,(0x20*pal_num)+idx2*2,HALF);


                uint32_t pos = ((y + (i/32)*8)) *TILE_X; // calc y offset
                pos += (8 * (i % 32))+x; // and then the x

                // convert and smash them to the screen
                tile_screen[pos] = convert_color(color1);
                tile_screen[pos+1] = convert_color(color2);
            }
        }
    }

    // smash the screen
    SDL_Event event;

    bool quit = false;


    // do our screen blit
	SDL_UpdateTexture(tile_texture, NULL, tile_screen.data(),  4 * TILE_X);
	SDL_RenderCopy(tile_renderer, tile_texture, NULL, NULL);
	SDL_RenderPresent(tile_renderer);



    while(!quit)
    {
        // handle input
        while(SDL_PollEvent(&event))
        {	
            switch(event.type) 
            {
                case SDL_WINDOWEVENT:
                {
                    if(event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        SDL_SetWindowSize(tile_window,event.window.data1, event.window.data2);
                    }

                    else if(event.window.event == SDL_WINDOWEVENT_CLOSE)
                    {
                        SDL_DestroyRenderer(tile_renderer);
                        SDL_DestroyWindow(tile_window); 
                        return;                       
                    }
                    break;
                }
            }
        }
        SDL_Delay(100);
    }
}


// main debugger input
void Debugger::enter_debugger()
{
    // reset stepping state
    step_instr = false;

    // need a write and exec command (then look into handling all the instruction variants)
    static const std::vector<std::string> commands{"run","break","clear","step","info","disass","write","exec","pal_viewer","tile_viewer"};
    static const std::vector<DEBUGGER_FPTR> command_funcs{run,breakpoint,clear,step,info,disass_addr,write,exec,palette_viewer,tile_viewer};
    std::string input;

    while(!debug_quit)
    {
        std::cout << "? ";
        std::getline(std::cin,input);

        // so at first for the input we 
        // are simply going to try and match
        // the command and pass the work off to a 
        // handler

        // first split the string by space
        std::istringstream iss(input);
        std::vector<std::string> command(std::istream_iterator<std::string>{iss},
            std::istream_iterator<std::string>());

        if(command.size() == 0)
        {
            std::cout << "Error parsing input!";
            continue;
        }

        // first word will have the command
        auto it = std::find(commands.begin(),commands.end(),command[0]);

        // not found
        if(it == commands.end())
        {
            std::cout << "Command: " << command[0] << " not recognised!\n";
            continue;
        }


        // get the idx
        auto idx = std::distance(commands.begin(),it);



        // now we just invoke our function from our array
        // of function ptrs
        std::invoke(command_funcs[idx],this,command);
    }
    debug_quit = false; // reset for next time
}

// command address    value(opt) modes(opt)
// break   0xdeadbeef 0xcafebabe rwx
void Debugger::breakpoint(std::vector<std::string> command)
{ 

    const size_t size = command.size();

    // set breakpoints
    if(size < 2)
    {
        puts("Insufficent args for breakpoint minimum of one required");
        return;
    }

    // pull the address
    uint32_t addr;
    try
    {
       addr = std::stoll(command[1],nullptr,16);
    }

    catch(std::exception &e)
    {
        std::cout << "unable to convert: " << command[1] << "to an address\n";
        std::cout << "exception: " << e.what() << "\n";
        return;
    }

    bool value_enabled = false;
    uint32_t value = 0xdeadbeef;
    // value or type break
    if(size == 3)
    {
        // type 
        if(!isdigit(command[2][0])) 
        {
            set_break_type(command[2],value,addr,value_enabled);
        }

        // value
        else
        {
            try
            {
                value = std::stoll(command[2],nullptr,16);
                value_enabled = true;
            }

            catch(std::exception &e)
            {
                std::cout << "unable to convert: " << command[2] << "to a value\n";
                std::cout << "exception: " << e.what() << "\n";
                return;
            }
        }
    }

    // set a rwx with value
    else if(size == 4)
    {
        try
        {
            value = std::stoll(command[2],nullptr,16);
            value_enabled = true;
        }

        catch(std::exception &e)
        {
            std::cout << "unable to convert: " << command[2] << "to a value\n";
            std::cout << "exception: " << e.what() << "\n";
            return;
        }        
        set_break_type(command[3],value,addr,value_enabled);
    }

    else // defualt to execute
    {
        breakpoint_x.set(value,addr,value_enabled,true);
    }

    printf("breakpoint set at %08x:%08x\n",addr,value);
}

void Debugger::set_break_type(std::string command,uint32_t value,uint32_t addr, bool value_enabled )
{
    int len = (command.size() > 3) ? 3 : command.size();

    for(int i = 0; i < len; i++)
    {
        switch(command[i])
        {
            case 'r':
            {
                breakpoint_r.set(value,addr,value_enabled,true);
                break;
            }

            case 'w':
            {
                breakpoint_w.set(value,addr,value_enabled,true);
                break;
            }

            case 'x':
            {
                breakpoint_x.set(value,addr,value_enabled,true);
                break;
            }

            default:
            {
                printf("invalid breakpoint type %c\n",command[i]);
                break;
            }
        }
    }    
}

// display system state info
// info (*addr value) | regs
void Debugger::info(std::vector<std::string> command)
{

    save_breakpoints();
    disable_breakpoints();

    if(command.size() < 2)
    {
        puts("no operand for info");
        return;
    }

    // memory print
    if(command[1][0] == '*')
    {
        try 
        {
            command[1] = command[1].substr(1);
            uint32_t addr = std::stoll(command[1],nullptr,16);

            // print x number of times
            if(command.size() == 3)
            {
                int x = std::stoll(command[2],nullptr,16);

                printf("       ");
                for(int i = 0; i < 16; i++)
                {
                    printf("  %02x",i);
                }
                
                printf("\n\n%04x: %02x ,",addr,mem->read_mem(addr,BYTE));
                for(int i = 1; i < x; i++)
                {	
                    // makes it "slow" to format but oh well
                    if(i % 16 == 0)
                    {
                        printf("\n%08x: ",addr+i);
                    }
                    
                    
                    printf("%02x ,",mem->read_mem(addr+i,BYTE));
                    
                }
            }

            else // print a single one
            {
                printf("%x: %08x\n",addr,mem->read_mem(addr,WORD));
            }
        }  
        
        catch(std::exception &e)
        {
            restore_breakpoints();
            puts("unable to convert args for memory printing");
            std::cout << "exception: " << e.what() << "\n";
            return;
        }
    }

    else // command lookup 
    {
        // do a register print
        if(command[1] == "regs")
        {
            cpu->print_regs();
        }
    }

    restore_breakpoints();
    putchar('\n');
}

// clear breakpoints
void Debugger::clear(std::vector<std::string> command)
{
    UNUSED(command);
    breakpoint_r.set(0xdeadbeef,0xdeadbeef,false,false);
    breakpoint_w.set(0xdeadbeef,0xdeadbeef,false,false);
    breakpoint_x.set(0xdeadbeef,0xdeadbeef,false,false);

    puts("breakpoints cleared!");
}

void Debugger::step(std::vector<std::string> command)
{
    UNUSED(command);
    debug_quit = true;
    step_instr = true;
}

// resume emuatlion
void Debugger::run(std::vector<std::string> command)
{
    UNUSED(command);
    puts("Resuming emulation!");
    debug_quit = true;
}

// assume arm mode for now
// disass addr value(opt)
void Debugger::disass_addr(std::vector<std::string> command)
{
    save_breakpoints();
    disable_breakpoints();

    if(command.size() < 2)
    {
        puts("invalid args for command");
        return;
    }

    try
    {
        uint32_t addr = std::stoll(command[1],nullptr,16);

        disass->set_pc(addr);

        if(command.size() == 3)
        {
            int value = std::stoi(command[2],nullptr,16);
            for(int i = 0; i < value; i++)
            {
                uint32_t address = disass->get_pc();
                std::string s;
                if(!cpu->is_cpu_thumb())
                {
                    s = disass->disass_arm(mem->read_mem(address,WORD),address+ARM_WORD_SIZE);
                }

                else
                {
                    s = disass->disass_thumb(mem->read_mem(address,HALF),address+ARM_HALF_SIZE);
                }

                std::cout << fmt::format("{:08x}: {}\n",address,s);
            }
        }
        
        else 
        {
            std::string s;
            if(!cpu->is_cpu_thumb())
            {
                s = disass->disass_arm(mem->read_mem(addr,WORD),addr+ARM_WORD_SIZE);
            }

            else
            {
                s = disass->disass_thumb(mem->read_mem(addr,HALF),addr+ARM_HALF_SIZE);
            }

            std::cout << fmt::format("{:08x}: {}\n",addr,s);
        }
        restore_breakpoints();
    }

    catch(std::exception &e)
    {
        restore_breakpoints();
        std::cout << "failed to convert arg to value!\n";
        std::cout << "exception: " << e.what() << "\n";
        return;
    }
}


//write addr value
void Debugger::write(std::vector<std::string> command)
{
    save_breakpoints();
    disable_breakpoints();

    if(command.size() < 3)
    {
        puts("invalid args for command");
        return;
    }

    try
    {
        uint32_t addr = std::stoll(command[1],nullptr,16);
        int value = std::stoll(command[2],nullptr,16);

        // add length specifier later
        mem->write_mem(addr,value,WORD);

        restore_breakpoints();
    }

    catch(std::exception &e)
    {
        restore_breakpoints();
        std::cout << "failed to convert arg to value!\n";
        std::cout << "exception: " << e.what() << "\n";
        return;
    }    
}

// exec value
void Debugger::exec(std::vector<std::string> command)
{
    if(command.size() < 2)
    {
        puts("invalid args for command");
        return;
    }


    try
    {
        uint32_t opcode = std::stoll(command[1],nullptr,16);

        printf("executing %08x\n",opcode);

        std::cout << disass->disass_arm(opcode,cpu->get_pc()+ARM_WORD_SIZE) << "\n";

        cpu->set_pc(cpu->get_pc()+ARM_WORD_SIZE);

        cpu->execute_arm_opcode(opcode);
    }


    catch(std::exception &e)
    {
        std::cout << "failed to convert arg to opcode: " << command[1] << "\n";
        std::cout << "exception: " << e.what() << "\n";
        return;
    }

}