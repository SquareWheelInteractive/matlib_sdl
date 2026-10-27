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

Light create_light(int type, vec3s position, Color color, float range, vec3s direciton, int id){
    Light light = {0};
    light.id        = id;
    light.enabled   = 1;
    light.type      = type;
    light.direction = direciton;
    light.position  = position;
    light.color     = color;
    light.intensity = 1;
    light.range    = range;
    return light;
}
void update_light_values(Light light, unsigned int shader){
    glad_glUseProgram(shader);

    int type_loc      = glad_glGetUniformLocation(shader, format_text("lights[%i].type",light.id));
    int enable_loc    = glad_glGetUniformLocation(shader, format_text("lights[%i].enabled",light.id));
    int dir_loc       = glad_glGetUniformLocation(shader, format_text("lights[%i].direction",light.id));
    int pos_loc       = glad_glGetUniformLocation(shader, format_text("lights[%i].position",light.id));
    int color_loc     = glad_glGetUniformLocation(shader, format_text("lights[%i].color"   ,light.id));
    int radius_loc    = glad_glGetUniformLocation(shader, format_text("lights[%i].range"  ,light.id));
    int intensity_loc = glad_glGetUniformLocation(shader, format_text("lights[%i].intensity"  ,light.id));

    glad_glUniform1i(enable_loc, light.enabled);
    glad_glUniform1i(type_loc, light.type);
    glad_glUniform3f(pos_loc, light.position.x, light.position.y, light.position.z);
    glad_glUniform3f(dir_loc, light.direction.x, light.direction.y, light.direction.z);
    glad_glUniform4f(color_loc, light.color.r, light.color.g, light.color.b, light.color.a);
    glad_glUniform1f(radius_loc, light.range);
    glad_glUniform1f(intensity_loc, light.intensity);

    glad_glUseProgram(0);
}
