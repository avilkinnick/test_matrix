#include "glad/glad.h"

#include <SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_filesystem.h>
#include <SDL_hints.h>
#include <SDL_keycode.h>
#include <SDL_main.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_ttf.h>
#include <SDL_video.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* absolute_bin_dir = NULL;
char* absolute_font_path = NULL;
TTF_Font* font = NULL;

SDL_Window* main_window = NULL;
SDL_GLContext gl_context = NULL;

GLuint vertex_shader = 0;
GLuint fragment_shader = 0;
GLuint shader_program = 0;

GLuint vbo = 0;
GLuint ebo = 0;
GLuint vao = 0;

SDL_Window* info_window = NULL;
SDL_Renderer* renderer = NULL;

static void cleanup(void);

static char* get_absolute_path(const char* const relative_path);

static GLuint load_shader(const char* const relative_path, const GLenum type);

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

    absolute_bin_dir = SDL_GetBasePath();
    if (absolute_bin_dir == NULL)
    {
        fputs("Failed to get absolute bin dir\n", stderr);
        fprintf(stderr, "SDL error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    absolute_font_path = get_absolute_path("resources/fonts/IosevkaNerdFont-Regular.ttf");
    if (absolute_font_path == NULL)
    {
        return EXIT_FAILURE;
    }

    font = TTF_OpenFont(absolute_font_path, 24);
    if (font == NULL)
    {
        fprintf(stderr, "Failed to open font %s\n", absolute_font_path);
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

    if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
    {
        fputs("Failed to initialize glad\n", stderr);
        return EXIT_FAILURE;
    }

    vertex_shader = load_shader("resources/shaders/shader.vert", GL_VERTEX_SHADER);
    if (vertex_shader == 0)
    {
        return EXIT_FAILURE;
    }

    fragment_shader = load_shader("resources/shaders/shader.frag", GL_FRAGMENT_SHADER);
    if (fragment_shader == 0)
    {
        return EXIT_FAILURE;
    }

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glDetachShader(shader_program, fragment_shader);
    glDetachShader(shader_program, vertex_shader);

    glDeleteShader(fragment_shader);
    fragment_shader = 0;

    glDeleteShader(vertex_shader);
    vertex_shader = 0;

    GLint success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar info_log[512];
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        fputs("Failed to link shader program\n", stderr);
        fprintf(stderr, "Info log: %s\n", info_log);
        return EXIT_FAILURE;
    }

    const float vertices[] = {
        // position         color
        -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f
    };

    const unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

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
                case SDL_QUIT:
                {
                    quit = true;
                    break;
                }
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

        SDL_GL_MakeCurrent(main_window, gl_context);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        SDL_GL_SwapWindow(main_window);

        SDL_SetRenderTarget(renderer, NULL);
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

    if (vao != 0)
    {
        glDeleteVertexArrays(1, &vao);
    }

    if (ebo != 0)
    {
        glDeleteBuffers(1, &ebo);
    }

    if (vbo != 0)
    {
        glDeleteBuffers(1, &vbo);
    }

    if (shader_program != 0)
    {
        glDeleteProgram(shader_program);
    }

    if (fragment_shader != 0)
    {
        glDeleteShader(fragment_shader);
    }

    if (vertex_shader != 0)
    {
        glDeleteShader(vertex_shader);
    }

    if (gl_context != NULL)
    {
        SDL_GL_DeleteContext(gl_context);
    }

    if (main_window != NULL)
    {
        SDL_DestroyWindow(main_window);
    }

    if (font != NULL)
    {
        TTF_CloseFont(font);
    }

    free(absolute_font_path);

    if (absolute_bin_dir != NULL)
    {
        SDL_free(absolute_bin_dir);
    }

    TTF_Quit();
    SDL_Quit();
}

static char* get_absolute_path(const char* const relative_path)
{
    const size_t absolute_path_size = strlen(absolute_bin_dir) + strlen(relative_path) + 1;

    char* const absolute_path = malloc(absolute_path_size);
    if (absolute_path == NULL)
    {
        fputs("Failed to allocate memory for absolute path\n", stderr);
        return NULL;
    }

    snprintf(absolute_path, absolute_path_size, "%s%s", absolute_bin_dir, relative_path);

    return absolute_path;
}

static GLuint load_shader(const char* const relative_path, const GLenum type)
{
    char* const absolute_shader_path = get_absolute_path(relative_path);
    if (absolute_shader_path == NULL)
    {
        return 0;
    }

    FILE* const shader_file = fopen(absolute_shader_path, "r");
    free(absolute_shader_path);
    if (shader_file == NULL)
    {
        fprintf(stderr, "Failed to open %s\n", relative_path);
        return 0;
    }

    fseek(shader_file, 0, SEEK_END);
    const long shader_code_length = ftell(shader_file);

    char* const shader_code = malloc(shader_code_length + 1);
    if (shader_code == NULL)
    {
        fputs("Failed to allocate memory for shader code\n", stderr);
        fclose(shader_file);
        return 0;
    }

    fseek(shader_file, 0, SEEK_SET);
    fread(shader_code, shader_code_length, 1, shader_file);
    shader_code[shader_code_length] = '\0';

    fclose(shader_file);

    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar* const *)&shader_code, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        fprintf(stderr, "Failed to compile %s\n", relative_path);
        fprintf(stderr, "Info log: %s\n", info_log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}
