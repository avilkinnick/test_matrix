#include "glad/glad.h"

#include <SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_filesystem.h>
#include <SDL_hints.h>
#include <SDL_keycode.h>
#include <SDL_main.h>
#include <SDL_mouse.h>
#include <SDL_pixels.h>
#include <SDL_rect.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>
#include <SDL_ttf.h>
#include <SDL_video.h>

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Vec3f
{
    float value[3];
} Vec3f;

static float vec3f_get_length(const Vec3f* const vec);

static Vec3f vec3f_normalize(const Vec3f* const vec);

static Vec3f vec3f_cross(const Vec3f* const vec1, const Vec3f* const vec2);

typedef struct Mat4f
{
    float value[4][4];
} Mat4f;

static Mat4f mat4f_product(const Mat4f* const mat1, const Mat4f* const mat2);

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

static bool render_text(const char* const text, const SDL_Color color, const int x, const int y);

static bool render_text_float(const char* const name, const float value,
    const SDL_Color color, const int x, const int y);

static bool render_text_vec3f(const char* const name, const Vec3f* const vec,
    const SDL_Color color, const int x, const int y);

static bool render_text_mat4f(const char* const name, const Mat4f* const mat,
    const SDL_Color color, const int x, const int y);

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

    main_window = SDL_CreateWindow("Main", 20, 20, 900, 900, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
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

    info_window = SDL_CreateWindow("Info", 940, 20, 900, 900, SDL_WINDOW_RESIZABLE);
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

    const Vec3f points[] = {
        {vertices[0], vertices[1], vertices[2]},
        {vertices[6], vertices[7], vertices[8]},
        {vertices[12], vertices[13], vertices[14]},
        {vertices[18], vertices[19], vertices[20]},
    };

    const SDL_Color color_blue = {0, 128, 255, 255};
    const SDL_Color color_green = {128, 255, 0, 255};
    const SDL_Color color_orange = {255, 128, 0, 255};

    const Vec3f world_up = {0.0f, 1.0f, 0.0f};

    Vec3f camera_pos = {0.0f, 0.0f, 3.0f};
    Vec3f camera_dir = {0.0f, 0.0f, -1.0f};
    Vec3f camera_right = vec3f_cross(&camera_dir, &world_up);
    camera_right = vec3f_normalize(&camera_right);
    Vec3f camera_up = vec3f_cross(&camera_right, &camera_dir);

    float yaw_deg = 0.0f;
    float pitch_deg = 0.0f;

    Mat4f look_at_matrix;

    {
        const float* const R = camera_right.value;
        const float* const U = camera_up.value;
        const float* const D = camera_dir.value;
        const float* const P = camera_pos.value;

        const Mat4f mat1 = {.value = {
            {R[0], R[1], R[2], 0.0f},
            {U[0], U[1], U[2], 0.0f},
            {D[0], D[1], D[2], 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        }};

        const Mat4f mat2 = {.value = {
            {1.0f, 0.0f, 0.0f, -P[0]},
            {0.0f, 1.0f, 0.0f, -P[1]},
            {0.0f, 0.0f, 1.0f, -P[2]},
            {0.0f, 0.0f, 0.0f, 1.0f}
        }};

        look_at_matrix = mat4f_product(&mat1, &mat2);
    }

    SDL_GL_MakeCurrent(main_window, gl_context);
    glUseProgram(shader_program);
    const GLint view_location = glGetUniformLocation(shader_program, "view");

    bool is_lmb_pressed = false;

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
                case SDL_MOUSEBUTTONDOWN:
                {
                    switch (event.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                        {
                            is_lmb_pressed = true;
                            break;
                        }
                    }

                    break;
                }
                case SDL_MOUSEBUTTONUP:
                {
                    switch (event.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                        {
                            is_lmb_pressed = false;
                            break;
                        }
                    }

                    break;
                }
                case SDL_MOUSEMOTION:
                {
                    if (!is_lmb_pressed)
                    {
                        break;
                    }

                    yaw_deg += event.motion.xrel;

                    pitch_deg -= event.motion.yrel;
                    if (pitch_deg < -89.0f)
                    {
                        pitch_deg = -89.0f;
                    }
                    else if (pitch_deg > 89.0f)
                    {
                        pitch_deg = 89.0f;
                    }

                    const float yaw_rad = yaw_deg * M_PI / 180.0f;
                    const float pitch_rad = pitch_deg * M_PI / 180.0f;

                    camera_dir.value[0] = sinf(yaw_rad) * cosf(pitch_rad);
                    camera_dir.value[1] = sinf(pitch_rad);
                    camera_dir.value[2] = -cosf(yaw_rad) * cosf(pitch_rad);
                    camera_dir = vec3f_normalize(&camera_dir);

                    camera_right = vec3f_cross(&camera_dir, &world_up);
                    camera_right = vec3f_normalize(&camera_right);

                    camera_up = vec3f_cross(&camera_right, &camera_dir);

                    {
                        const float* const R = camera_right.value;
                        const float* const U = camera_up.value;
                        const float* const D = camera_dir.value;
                        const float* const P = camera_pos.value;

                        const Mat4f mat1 = {.value = {
                            {R[0], R[1], R[2], 0.0f},
                            {U[0], U[1], U[2], 0.0f},
                            {D[0], D[1], D[2], 0.0f},
                            {0.0f, 0.0f, 0.0f, 1.0f}
                        }};

                        const Mat4f mat2 = {.value = {
                            {1.0f, 0.0f, 0.0f, -P[0]},
                            {0.0f, 1.0f, 0.0f, -P[1]},
                            {0.0f, 0.0f, 1.0f, -P[2]},
                            {0.0f, 0.0f, 0.0f, 1.0f}
                        }};

                        look_at_matrix = mat4f_product(&mat1, &mat2);
                    }

                    break;
                }
            }
        }

        SDL_GL_MakeCurrent(main_window, gl_context);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);
        glUniformMatrix4fv(view_location, 1, GL_FALSE, &look_at_matrix.value[0][0]);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        SDL_GL_SwapWindow(main_window);

        SDL_SetRenderTarget(renderer, NULL);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render_text_vec3f("points[0]", &points[0], color_green, 10, 10);
        render_text_vec3f("points[1]", &points[1], color_green, 10, 40);
        render_text_vec3f("points[2]", &points[2], color_green, 10, 70);
        render_text_vec3f("points[3]", &points[3], color_green, 10, 100);
        render_text_vec3f("world_up ", &world_up, color_green, 10, 130);
        render_text_float("yaw_deg", yaw_deg, color_orange, 10, 160);
        render_text_float("pitch_deg", pitch_deg, color_orange, 286, 160);
        render_text_vec3f("camera_pos  ", &camera_pos, color_blue, 10, 190);
        render_text_vec3f("camera_dir  ", &camera_dir, color_blue, 10, 220);
        render_text_vec3f("camera_right", &camera_right, color_blue, 10, 250);
        render_text_vec3f("camera_up   ", &camera_up, color_blue, 10, 280);
        render_text_mat4f("look_at", &look_at_matrix, color_orange, 10, 310);

        SDL_RenderPresent(renderer);
    }

    return EXIT_SUCCESS;
}

static float vec3f_get_length(const Vec3f* const vec)
{
    return sqrtf(vec->value[0] * vec->value[0] + vec->value[1] * vec->value[1] + vec->value[2] * vec->value[2]);
}

static Vec3f vec3f_normalize(const Vec3f* const vec)
{
    const float length = vec3f_get_length(vec);

    return (Vec3f){vec->value[0] / length, vec->value[1] / length, vec->value[2] / length};
}

static Vec3f vec3f_cross(const Vec3f* const vec1, const Vec3f* const vec2)
{
    return (Vec3f){
        vec1->value[1] * vec2->value[2] - vec1->value[2] * vec2->value[1],
        vec1->value[2] * vec2->value[0] - vec1->value[0] * vec2->value[2],
        vec1->value[0] * vec2->value[1] - vec1->value[1] * vec2->value[0]
    };
}

static Mat4f mat4f_product(const Mat4f* const mat1, const Mat4f* const mat2)
{
    Mat4f result;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            result.value[i][j] = 0.0f;
            for (int k = 0; k < 4; ++k)
            {
                result.value[i][j] += mat1->value[i][k] * mat2->value[k][j];
            }
        }
    }
    return result;
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

static bool render_text(const char* const text, const SDL_Color color, const int x, const int y)
{
    SDL_Surface* const text_surface = TTF_RenderUTF8_Blended_Wrapped(font, text, color, 0);
    if (text_surface == NULL)
    {
        fputs("Failed to create text surface\n", stderr);
        fprintf(stderr, "TTF error: %s\n", TTF_GetError());
        return false;
    }

    SDL_Texture* const text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    SDL_FreeSurface(text_surface);
    if (text_texture == NULL)
    {
        fputs("Failed to create texture from text surface\n", stderr);
        fprintf(stderr, "SDL error: %s\n", SDL_GetError());
        return false;
    }

    SDL_Rect dstrect;
    dstrect.x = x;
    dstrect.y = y;
    SDL_QueryTexture(text_texture, NULL, NULL, &dstrect.w, &dstrect.h);

    SDL_RenderCopy(renderer, text_texture, NULL, &dstrect);

    SDL_DestroyTexture(text_texture);

    return true;
}

static bool render_text_float(const char* const name, const float value,
    const SDL_Color color, const int x, const int y)
{
    char text[512];
    snprintf(text, 512, "%s = %9.3f", name, value);

    return render_text(text, color, x, y);
}

static bool render_text_vec3f(const char* const name, const Vec3f* const vec,
    const SDL_Color color, const int x, const int y)
{
    char format[512];
    snprintf(format, 512, "%s = {%%9.3f, %%9.3f, %%9.3f}", name);

    char text[512];
    snprintf(text, 512, format, vec->value[0], vec->value[1], vec->value[2]);

    return render_text(text, color, x, y);
}

static bool render_text_mat4f(const char* const name, const Mat4f* const mat,
    const SDL_Color color, const int x, const int y)
{
    char format[512];
    snprintf(format, 512, "%s = {\n"
        "    %%9.3f, %%9.3f, %%9.3f, %%9.3f,\n"
        "    %%9.3f, %%9.3f, %%9.3f, %%9.3f,\n"
        "    %%9.3f, %%9.3f, %%9.3f, %%9.3f,\n"
        "    %%9.3f, %%9.3f, %%9.3f, %%9.3f\n"
    "}", name);

    char text[512];
    snprintf(text, 512, format, mat->value[0][0], mat->value[0][1], mat->value[0][2], mat->value[0][3],
        mat->value[1][0], mat->value[1][1], mat->value[1][2], mat->value[1][3],
        mat->value[2][0], mat->value[2][1], mat->value[2][2], mat->value[2][3],
        mat->value[3][0], mat->value[3][1], mat->value[3][2], mat->value[3][3]);

    return render_text(text, color, x, y);
}
