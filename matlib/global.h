#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

typedef struct Input{
    bool event_down;
    bool evenet_up;
    SDL_Scancode scan_code;
} Input;
typedef struct Window_ctx{
    SDL_Window* window;
    unsigned short screen_width;
    unsigned short screen_height;
} Window_ctx;
typedef struct Global{
    Window_ctx window_context;
    SDL_Event event;
    Input input;
} Global;

extern Global global;
