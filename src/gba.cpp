#include "headers/gba.h"

// init all sup compenents
GBA::GBA(std::string filename)
{
    mem.init(filename,&debug,&cpu,&disp);
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




uint32_t time_left(uint32_t &next_time)
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
	std::vector<int>fps_table (10);
	int fps_table_idx = 0;

    for(;;)
    {

		uint32_t curr_time = SDL_GetTicks();

        next_time = curr_time + screen_ticks_per_frame;

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
		// need to count the ammount of cycles in a frame instead
		// but cut the screen copy off at the vblank point	
        SDL_Delay(time_left(next_time));


		// fps calc
		int curr_fps = 1000 / (SDL_GetTicks() - curr_time);
		fps_table[fps_table_idx++] = curr_fps;
		if(fps_table_idx == 10)
		{
			fps_table_idx = 0;

			int avg_fps = std::accumulate(fps_table.begin(),
				fps_table.end(),0) / fps_table.size();
			
			std::string title = fmt::format("destoer-gba fps: {}",avg_fps);

			SDL_SetWindowTitle(window,title.data());
		}	
    }
}

void GBA::init_screen()
{
	/* sdl setup */
	
	// initialize our window
	window = SDL_CreateWindow("",
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
					case SDLK_RETURN:
					{
						button_event(Button::START,true);
						break;						
					}

					case SDLK_SPACE:
					{
						button_event(Button::SELECT,true);
						break;
					}

					case SDLK_DOWN:
					{
						button_event(Button::DOWN,true);
						break;
					}

					case SDLK_UP:
					{
						button_event(Button::UP,true);
						break;
					}

					case SDLK_LEFT:
					{
						button_event(Button::LEFT,true);
						break;
					}

					case SDLK_RIGHT:
					{
						button_event(Button::RIGHT,true);
						break;
					}


					case SDLK_a:
					{
						button_event(Button::A,true);
						break;
					}

					case SDLK_s:
					{
						button_event(Button::B,true);
						break;
					}



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

					case SDLK_RETURN:
					{
						button_event(Button::START,false);
						break;						
					}

					case SDLK_SPACE:
					{
						button_event(Button::SELECT,false);
						break;
					}

					case SDLK_DOWN:
					{
						button_event(Button::DOWN,false);
						break;
					}

					case SDLK_UP:
					{
						button_event(Button::UP,false);
						break;
					}

					case SDLK_LEFT:
					{
						button_event(Button::LEFT,false);
						break;
					}

					case SDLK_RIGHT:
					{
						button_event(Button::RIGHT,false);
						break;
					}


					case SDLK_a:
					{
						button_event(Button::A,false);
						break;
					}

					case SDLK_s:
					{
						button_event(Button::B,false);
						break;
					}


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

// we will decide if we are going to switch our underlying memory
// for io after the test 
void GBA::button_event(Button b, bool down)
{
	uint16_t keyinput = mem.handle_read(mem.io,IO_KEYINPUT&IO_MASK,HALF);

	int button = static_cast<int>(b);

	// 0 = pressed

	if(down)
	{
		keyinput = deset_bit(keyinput,button); 
	}

	else
	{
		keyinput = set_bit(keyinput,button);
	}

	mem.handle_write(mem.io,IO_KEYINPUT&IO_MASK,keyinput,HALF);
}