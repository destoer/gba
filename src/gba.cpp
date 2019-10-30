#include "headers/gba.h"

// init all sup compenents
GBA::GBA(std::string filename)
{
    mem.init(filename,&debug,&cpu);
    disass.init(&mem,&cpu);
    disp.init(&mem);
    cpu.init(&disp,&mem,&debug,&disass);
    debug.init(&mem,&cpu,&disp,&disass);


    // init sdl
    init_screen();
}

GBA::~GBA()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_QuitSubSystem(SDL_INIT_EVERYTHING);
	SDL_Quit();    
}




uint32_t time_left(const uint32_t &next_time)
{	
	uint32_t now = SDL_GetTicks();
	if(next_time <= now)
	{
		return 0;
	}
	
	else
	{
		return next_time - now;
	}
}


// start the main emulation loop
void GBA::run()
{

	const int fps = 60;
	const int screen_ticks_per_frame = 1000 / fps; 
    uint32_t next_time;
    for(;;)
    {

        next_time = SDL_GetTicks() + screen_ticks_per_frame;

        handle_input();

        
		while(!disp.new_vblank) // exec until a vblank hits
        {
            cpu.step();
		}

        disp.new_vblank = false;

        // do our screen blit
		SDL_UpdateTexture(texture, NULL, disp.screen,  4 * disp.X);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);


        // sleep off the rest of the frame
		// probably a bad way to do it		
        //SDL_Delay(time_left(next_time));


        next_time += screen_ticks_per_frame;
    }
}

void GBA::init_screen()
{
	/* sdl setup */
	
	// initialize our window
	window = SDL_CreateWindow("gba",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,disp.X*2,disp.Y*2,SDL_WINDOW_RESIZABLE);
	
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"); // crashes without this on windows?
	
	// set a render for our window
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB888, 
        SDL_TEXTUREACCESS_STREAMING, disp.X, disp.Y);

	memset(disp.screen ,0x00,disp.Y * disp.X *  4 * sizeof(uint8_t));	
}

void GBA::handle_input()
{
	SDL_Event event;
	
	// handle input
	while(SDL_PollEvent(&event))
	{	
		switch(event.type) 
		{
			case SDL_WINDOWEVENT:
			{
				if(event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					SDL_SetWindowSize(window,event.window.data1, event.window.data2);
				}
				break;
			}
		
	
			case SDL_QUIT:
			{
                puts("quitting...");
                exit(1);
			}	
			
			case SDL_KEYDOWN:
			{
				switch(event.key.keysym.sym)
				{

#ifdef DEBUG
					case SDLK_p:
					{
						debug.step_instr = true;
						break;
					}
#endif

                    default:
                    {
                        break;
                    }
                }
                break;
			}
			
			case SDL_KEYUP:
			{
				switch(event.key.keysym.sym)
				{
                    default:
                    {
                        break;
                    }    
                }
                break;
			}

            default:
            {
                break;
            }           
		}
	}    
}