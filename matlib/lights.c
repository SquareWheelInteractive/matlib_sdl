#include "glad/glad.h"
#include "matlib/lights.h"
#include <stdarg.h>

/* - - - - - - - - - - - - - - - - - - - - - - - - - -
    TODO: move this function to somewhere more appropriate
   - - - - - - - - - - - - - - - - - - - - - - - - - -        */
static char* format_text(const char* fmt, ...) {
    static char buffer[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return buffer;
}

/* TODO: this light counting should be the renderer's job,
   but for now it's alright, this is a remember to do that
   at some point */
static unsigned char light_num = 0;
Light create_light(int type, vec3s position, Color color, float radious, vec3s direciton){
    Light light = {0};
    if(light_num < MAX_LIGHTS){
        light.id = light_num;
        light.enabled   = 1;
        light.type      = type;
        light.direction = direciton;
        light.position  = position;
        light.color     = color;
        light.intensity = 1;
        light.radius    = radious;

        light_num++;
    }
    return light;
}
void update_light_values(Light light, unsigned int shader){
    glad_glUseProgram(shader);

    int type_loc      = glad_glGetUniformLocation(shader, format_text("lights[%i].type",light.id));
    int enable_loc    = glad_glGetUniformLocation(shader, format_text("lights[%i].enabled",light.id));
    int dir_loc       = glad_glGetUniformLocation(shader, format_text("lights[%i].direction",light.id));
    int pos_loc       = glad_glGetUniformLocation(shader, format_text("lights[%i].position",light.id));
    int color_loc     = glad_glGetUniformLocation(shader, format_text("lights[%i].color"   ,light.id));
    int radius_loc    = glad_glGetUniformLocation(shader, format_text("lights[%i].radius"  ,light.id));
    int intensity_loc = glad_glGetUniformLocation(shader, format_text("lights[%i].intensity"  ,light.id));

    glad_glUniform1i(enable_loc, light.enabled);
    glad_glUniform1i(type_loc, light.type);
    glad_glUniform3f(pos_loc, light.position.x, light.position.y, light.position.z);
    glad_glUniform3f(dir_loc, light.direction.x, light.direction.y, light.direction.z);
    glad_glUniform4f(color_loc, light.color.r, light.color.g, light.color.b, light.color.a);
    glad_glUniform1f(radius_loc, light.radius);
    glad_glUniform1f(intensity_loc, light.intensity);

    glad_glUseProgram(0);
}
