#include <SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_keycode.h>
#include <SDL_main.h>
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <SDL_video.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

SDL_Window* main_window = NULL;
SDL_GLContext gl_context = NULL;

SDL_Window* info_window = NULL;
SDL_Renderer* renderer = NULL;

static void cleanup(void);

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    atexit(cleanup);

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    {
        fputs("Failed to initialize SDL\n", stderr);
        fprintf(stderr, "SDL error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (TTF_Init() < 0)
    {
        fputs("Failed to initialize SDL_ttf\n", stderr);
        fprintf(stderr, "TTF error: %s\n", TTF_GetError());
        return EXIT_FAILURE;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    main_window = SDL_CreateWindow("Main", 100, 100, 800, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (main_window == NULL)
    {
        fputs("Failed to create main window\n", stderr);
        fprintf(stderr, "SDL error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    gl_context = SDL_GL_CreateContext(main_window);
    if (gl_context == NULL)
    {
        fputs("Failed to create OpenGL context\n", stderr);
        fprintf(stderr, "SDL error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    info_window = SDL_CreateWindow("Info", 1000, 100, 800, 800, SDL_WINDOW_RESIZABLE);
    if (info_window == NULL)
    {
        fputs("Failed to create info window\n", stderr);
        fprintf(stderr, "SDL error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    renderer = SDL_CreateRenderer(info_window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        fputs("Failed to create renderer\n", stderr);
        fprintf(stderr, "SDL error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    bool quit = false;
    while (!quit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_KEYDOWN:
                {
                    switch (event.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                        {
                            quit = true;
                            break;
                        }
                    }

                    break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_RenderPresent(renderer);
    }

    return EXIT_SUCCESS;
}

static void cleanup(void)
{
    if (renderer != NULL)
    {
        SDL_DestroyRenderer(renderer);
    }

    if (info_window != NULL)
    {
        SDL_DestroyWindow(info_window);
    }

    if (gl_context != NULL)
    {
        SDL_GL_DeleteContext(gl_context);
    }

    if (main_window != NULL)
    {
        SDL_DestroyWindow(main_window);
    }

    TTF_Quit();
    SDL_Quit();
}
