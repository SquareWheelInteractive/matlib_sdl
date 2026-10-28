#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

typedef struct {
    float delta_x;
    float delta_y;
} Wheel;
typedef struct {
    Wheel wheel;
    bool button_is_pressed;
    unsigned int button_idx;
    float delta_x;
    float delta_y;
} Mouse;
typedef struct{
    const bool* keys;
} Keyboard;
typedef struct Input{
    SDL_Scancode scan_code;
    bool is_event_down;
    bool is_event_up;
    Mouse mouse;
    Keyboard keyboard;
} Input;
typedef struct WindowCtx{
    SDL_Window* window;
    bool has_resized;
    unsigned short screen_width;
    unsigned short screen_height;
} WindowCtx;
typedef struct Global{
    WindowCtx window_context;
    Input input;
} Global;
/* -------------- IMPORTANT: ---------------------
    There must be just one instance of Global struct delcared
   -------------------------------------------------- */
extern Global global;
