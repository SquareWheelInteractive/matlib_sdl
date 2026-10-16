#include "renderer/renderer.h"
#include "external/glad/glad.h"
#include "matlib/global.h"

void renderer_init(Renderer* renderer, Camera* main_camera){
    for (int i = 0; i < MAX_LIGHTS; i++) {
        renderer->lights[i].id = INVALID_LIGHT;
    }
    renderer->light_count = 0;
    renderer->fog = (Fog){0};

    renderer->main_camera = main_camera;   

    renderer->shadow_map_shader = create_shader_program("./shaders/shadow_mapping_vert.glsl", "./shaders/shadow_mapping_frag.glsl");
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

    glUseProgram(renderer->skinning_shader);
    glUniform1i(glGetUniformLocation(renderer->skinning_shader, "tex"), 0);
    glUniform1i(glGetUniformLocation(renderer->skinning_shader, "shadow_map"), 1);
}

void renderer_shadow_pass(Renderer* renderer, Model* models, unsigned int model_count){
    Light directional_light = {0};
    directional_light.id = INVALID_LIGHT;
    for (unsigned char i = 0; i < renderer->light_count; i++) {
        if(renderer->lights[i].type == LIGHT_TYPE_DIRECTIONAL){
            directional_light = renderer->lights[i];
            break;
        }
    }
    assert(directional_light.id != INVALID_LIGHT);

    glad_glCullFace(GL_FRONT);
    mat4s light_projection = glms_ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 20.0f);
    mat4s light_view = glms_lookat(directional_light.position, (vec3s){0,0,0}, (vec3s){0,1,0});
    renderer->light_space_matrix = glms_mat4_mul(light_projection, light_view);

    glad_glUseProgram(renderer->shadow_map_shader);

    glad_glViewport(0,0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
    glad_glBindFramebuffer(GL_FRAMEBUFFER, renderer->shadow_map_fbo);
    glad_glClear(GL_DEPTH_BUFFER_BIT);

    glad_glActiveTexture(GL_TEXTURE1);
    glad_glBindTexture(GL_TEXTURE_2D, renderer->shadow_map_texture);

    unsigned int light_space_matrix_loc = glad_glGetUniformLocation(renderer->shadow_map_shader, "light_space_matrix");
    glad_glUniformMatrix4fv(light_space_matrix_loc, 1, GL_FALSE, (const float*)renderer->light_space_matrix.raw);

    for (unsigned int i = 0; i < model_count; i++) {
        draw_model(&models[i], renderer->shadow_map_shader);
    }
}
void renderer_normal_pass(Renderer* renderer, Model* models, unsigned int model_count){
    glad_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // end of shadow pass

    begin_drawing(renderer->main_camera);
    clear_background(BLACK);

    // normal pass
    glad_glCullFace(GL_BACK);
    glad_glUseProgram(renderer->skinning_shader);
    glad_glViewport(0,0, global.window_context.screen_width, global.window_context.screen_height);
    unsigned int view_loc     = glad_glGetUniformLocation(renderer->skinning_shader, "view");
    unsigned int proj_loc     = glad_glGetUniformLocation(renderer->skinning_shader, "projection");
    unsigned int ambient_loc  = glad_glGetUniformLocation(renderer->skinning_shader, "ambient");
    unsigned int view_pos_loc = glad_glGetUniformLocation(renderer->skinning_shader, "view_pos");
    unsigned int light_space_matrix_loc = glad_glGetUniformLocation(renderer->skinning_shader, "light_space_matrix");
    Color ambient = (Color){0.7f, 0.65f, 0.75f, 1};

    glad_glUniformMatrix4fv(view_loc , 1, GL_FALSE, (const float*)renderer->main_camera->view_matrix.raw);
    glad_glUniformMatrix4fv(proj_loc , 1, GL_FALSE, (const float*)renderer->main_camera->proj_matrix.raw);
    glad_glUniform4f(ambient_loc, ambient.r, ambient.g, ambient.b, ambient.a);
    glad_glUniform3f(view_pos_loc, renderer->main_camera->position.x, renderer->main_camera->position.y, renderer->main_camera->position.z);
    glad_glUniformMatrix4fv(light_space_matrix_loc, 1, GL_FALSE, (const float*)renderer->light_space_matrix.raw);

    glad_glActiveTexture(GL_TEXTURE1);
    glad_glBindTexture(GL_TEXTURE_2D, renderer->shadow_map_texture);

    for (unsigned int i = 0; i < model_count; i++) {
        draw_model(&models[i], renderer->skinning_shader);
    }

    if(renderer->cube_map.cubemap_tex > 0)
        draw_cubemap(renderer->cube_map, *renderer->main_camera);

    end_drawing();
}

void renderer_load_cube_map(Renderer* renderer, char** faces_path){
    renderer->cube_map = load_cubemap(faces_path);
}

unsigned int renderer_create_light(Renderer* renderer, int type, vec3s position, Color color, float radius, vec3s direction){
    if (renderer->light_count >= MAX_LIGHTS)
        return INVALID_LIGHT;

    unsigned int id = renderer->light_count;

    renderer->lights[renderer->light_count++] = create_light(
        type,
        position,
        color,
        radius,
        direction,
        id
    );

    return id;
}
