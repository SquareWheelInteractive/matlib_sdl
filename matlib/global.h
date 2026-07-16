#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

typedef struct Input{
    bool is_event_down;
    bool is_evenet_up;
    SDL_Scancode scan_code;
} Input;
typedef struct Window_ctx{
    SDL_Window* window;
    bool has_resized;
    unsigned short screen_width;
    unsigned short screen_height;
} Window_ctx;
typedef struct Global{
    Window_ctx window_context;
    Input input;
} Global;

/* -------------- IMPORTANT: ---------------------
    There must be just one instance of Global struct delcared
   -------------------------------------------------- */
extern Global global;
