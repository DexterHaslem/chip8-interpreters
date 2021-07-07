/* simple CHIP-8 interpreter just for fun
	Dexter Haslem <dmh@fastmail.com> 2021
*/

#include "c8.h"


static void init(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 0x1f, 0x1f, 0x1f, 0xff);
	SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0x00, 0xc2, 0x00, 0xff);
    SDL_RenderSetScale(renderer, (float)C8_PIXEL_SCALE, (float)C8_PIXEL_SCALE);
    srand((unsigned int)time(NULL));
    c8_init();
}

int main(int argc, char** argv)
{
    (void*)argc;
    (void*)argv;

    SDL_Window* window = NULL;
    SDL_Renderer* renderer;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "failed to create sdl window\n");
        return -1;
    }

    /* create the window at scale */
    SDL_CreateWindowAndRenderer((int)(C8_WIDTH * C8_PIXEL_SCALE), 
        (int)(C8_HEIGHT * (C8_PIXEL_SCALE)), SDL_WINDOW_SHOWN, &window, &renderer);

    if (!window || !renderer)
    {
        fprintf(stderr, "failed to create sdl window\n");
        return -1;
    }

    SDL_SetWindowTitle(window, "CHIP8 Interp - Drag a ROM onto me!");
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    SDL_Event sevt;
    bool done = false;
    while (!done)
    {
        while (SDL_PollEvent(&sevt) != 0)
        {
            switch (sevt.type)
            {
            case SDL_QUIT:
                done = true;
                break;
            case SDL_DROPFILE:
                bool loaded_ok = c8_load_rom(sevt.drop.file);
                if (loaded_ok)
                {
                    /* call full init. we want to clear anything left over */
	                init(renderer);
                }
                SDL_free(sevt.drop.file);
                break;
            case SDL_KEYUP:
                //printf("key up %u\n", sevt.key.keysym.sym);
                break;
            case SDL_KEYDOWN:
                //printf("key down %u\n", sevt.key.keysym.sym);
                break;
            }
        }

        bool running = c8_running();
        if (!running)
        {
            SDL_Delay(100);
            continue;
        }

        for (int x = 0; x < C8_CYCLES_PER_FRAME; ++x)
        {
            c8_cycle();
        }

        if (c8_gfx_dirty())
        {
            c8_draw_frame(renderer);
            SDL_RenderPresent(renderer);
        }

		SDL_Delay(C8_FRAME_DELAY_MS);
    }

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    return 0;
}