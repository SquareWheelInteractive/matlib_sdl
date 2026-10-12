#pragma once
#include "matlib/matlib.h"
#include "matlib/lights.h"

#define MAX_LIGHTS 4
#define SHADOW_MAP_WIDTH 1024
#define SHADOW_MAP_HEIGHT 1024

typedef struct{
    Camera* main_camera;

    Light lights[MAX_LIGHTS];
    unsigned int light_count;

    unsigned int shadow_map_texture;
    unsigned int shadow_map_fbo;

    unsigned int lighting_shader;
    unsigned int skinning_shader;
    unsigned int shadow_map_shader;
} Renderer;

void renderer_init(Renderer* renderer, Camera* main_camera);
