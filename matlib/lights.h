#pragma once
#include "matlib.h"

#define MAX_LIGHTS 4

typedef enum{
    LIGHT_TYPE_POINT = 0,
    LIGHT_TYPE_SPOT  = 1,
} Light_type;

typedef struct{
    int type;
    vec3s position;
    vec3s direction;
    Color color;
    bool enabled;
    float radius;

    //shader locations
    int type_loc;
    int dir_loc;
    int pos_loc;
    int color_loc;
    int enable_loc;
    int radius_loc;
} Light;

Light create_light(int type, vec3s position, Color color, float radious, vec3s direction, unsigned int shader);
void update_light_values(Light light, unsigned int shader);
