#include "lights.h"
#include "matlib.h"
#include "glad/glad.h"
#include <stdarg.h>

static char* mp_format_text(const char* fmt, ...) {
    static char buffer[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return buffer;
}

static unsigned char light_num = 0;
Light create_light(int type, vec3s position, Color color, float radious, vec3s direciton, unsigned int shader){
    Light light = {0}; if(light_num < MAX_LIGHTS){
        light.enabled   = 1;
        light.type      = type;
        light.direction = direciton;
        light.position  = position;
        light.color     = color;
        light.radius    = radious;

        light.type_loc   = glad_glGetUniformLocation(shader, mp_format_text("lights[%i].type",light_num));
        light.enable_loc = glad_glGetUniformLocation(shader, mp_format_text("lights[%i].enabled",light_num));
        light.dir_loc    = glad_glGetUniformLocation(shader, mp_format_text("lights[%i].direction",light_num));
        light.pos_loc    = glad_glGetUniformLocation(shader, mp_format_text("lights[%i].position",light_num));
        light.color_loc  = glad_glGetUniformLocation(shader, mp_format_text("lights[%i].color"   ,light_num));
        light.radius_loc = glad_glGetUniformLocation(shader, mp_format_text("lights[%i].radius"  ,light_num));

        update_light_values(light, shader);
        light_num++;
    }
    return light;
}
void update_light_values(Light light, unsigned int shader){
    glad_glUseProgram(shader);

    glad_glUniform1i(light.enable_loc, light.enabled);
    glad_glUniform1i(light.type_loc, light.type);
    glad_glUniform3f(light.pos_loc, light.position.x, light.position.y, light.position.z);
    glad_glUniform3f(light.dir_loc, light.direction.x, light.direction.y, light.direction.z);
    glad_glUniform4f(light.color_loc, light.color.r, light.color.g, light.color.b, light.color.a);
    glad_glUniform1f(light.radius_loc, light.radius);

    glad_glUseProgram(0);
}
