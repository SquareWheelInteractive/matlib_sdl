#pragma once
#include "matlib/matlib.h"
#include "matlib/lights.h"

#define MAX_LIGHTS 16
#define INVALID_LIGHT -1
#define SHADOW_MAP_WIDTH 1024
#define SHADOW_MAP_HEIGHT 1024

typedef struct{
    Color color;
    float density;
} Fog;

typedef struct{
    Camera* main_camera;

    CubeMap cube_map;

    Light lights[MAX_LIGHTS];
    unsigned int light_count;
    mat4s light_space_matrix;

    Fog fog;
    Color ambient;

    unsigned int shadow_map_fbo;
    unsigned int shadow_map_texture;

    unsigned int skinning_shader;
    unsigned int shadow_map_shader;
} Renderer;

void renderer_init(Renderer* renderer, Camera* main_camera);
void renderer_shadow_pass(Renderer* renderer, Model* models, unsigned int model_count);
void renderer_normal_pass(Renderer* renderer, Model* models, unsigned int model_count);
unsigned int renderer_create_light(Renderer* renderer, int type, vec3s position, Color color, float range, vec3s direction);
void renderer_load_cube_map(Renderer* renderer, char** faces_path);
void renderer_update_lights(Renderer* renderer);
