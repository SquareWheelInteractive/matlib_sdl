#pragma once
#include "matlib.h"

typedef enum{
    LIGHT_TYPE_POINT        = 0,
    LIGHT_TYPE_SPOT         = 1,
    LIGHT_TYPE_DIRECTIONAL  = 2,
} Light_type;

typedef struct{
    int type;
    unsigned int id;
    vec3s position;
    vec3s direction;
    Color color;
    float intensity;
    bool enabled;
    float radius;
} Light;

Light create_light(int type, vec3s position, Color color, float radious, vec3s direction, unsigned int id);
void update_light_values(Light light, unsigned int shader);
