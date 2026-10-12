#include "renderer/renderer.h"
#include "external/glad/glad.h"

void renderer_init(Renderer* renderer, Camera* main_camera){
    renderer->light_count = 0;

    renderer->main_camera = main_camera;   

    renderer->shadow_map_shader = create_shader_program("./shaders/shadow_mapping_vert.glsl", "./shaders/shadow_mapping_frag.glsl");
    renderer->lighting_shader   = create_shader_program("./shaders/lighting_vert.glsl", "./shaders/lighting_frag.glsl");
    renderer->skinning_shader   = create_shader_program("./shaders/skinning_vert.glsl", "./shaders/lighting_frag.glsl");

    glad_glGenFramebuffers(1, &renderer->shadow_map_fbo);

    glad_glGenTextures(1, &renderer->shadow_map_texture);
    glad_glBindTexture(GL_TEXTURE_2D, renderer->shadow_map_texture);
    glad_glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 0,
                      GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glad_glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

    glad_glBindFramebuffer(GL_FRAMEBUFFER, renderer->shadow_map_fbo);
    glad_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, renderer->shadow_map_texture, 0);
    glad_glDrawBuffer(GL_NONE);
    glad_glReadBuffer(GL_NONE);
    glad_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glad_glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(renderer->lighting_shader);
    glUniform1i(glGetUniformLocation(renderer->lighting_shader, "tex"), 0);
    glUniform1i(glGetUniformLocation(renderer->lighting_shader, "shadow_map"), 1);
    glUseProgram(renderer->skinning_shader);
    glUniform1i(glGetUniformLocation(renderer->skinning_shader, "tex"), 0);
    glUniform1i(glGetUniformLocation(renderer->skinning_shader, "shadow_map"), 1);
}
