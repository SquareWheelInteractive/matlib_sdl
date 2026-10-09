#include "matlib/matlib.h"
#include "glad/glad.h"
#include "matlib/lights.h"
#include "matlib/global.h"

#define WIDTH 800
#define HEIGHT 600

int main() {
    // initialize window
    if(!init_window("pingo", WIDTH, HEIGHT)){
        printf("failed to init sdl window\n");
    }
    char* sky_box_textures[6] = {
        "resources/sky_sunset/right.png",
        "resources/sky_sunset/left.png",
        "resources/sky_sunset/top.png",
        "resources/sky_sunset/bottom.png",
        "resources/sky_sunset/front.png",
        "resources/sky_sunset/back.png"
    };
    CubeMap cube_map = load_cubemap(sky_box_textures);

    unsigned int light_shader = create_shader_program("./shaders/lighting_vert.glsl", "./shaders/lighting_frag.glsl");
    unsigned int skinning_shader= create_shader_program("./shaders/skinning_vert.glsl", "./shaders/lighting_frag.glsl");

    Camera cam = create_and_init_camera(CAMERA_PERSPECTIVE);
    set_camera_fov(&cam, 65);

    Model house = load_model("./resources/medieval_house.obj");
    house.material.shader = light_shader;
    house.material.albedo = load_texture("./resources/houseTexture.png");

    Light light = create_light(LIGHT_TYPE_SPOT, (vec3s){3,1,0}, WHITE, 5, glms_vec3_zero());
    Light sun = create_light(LIGHT_TYPE_DIRECTIONAL, (vec3s){10,10,0}, (Color){0.8f,0.75f,0.65f,1}, 0, (vec3s){-1,-1,0});
    light.intensity = 4;

    Model man = load_model("./resources/CesiumMan.glb");
    man.material.shader = skinning_shader;
    man.material.albedo = load_texture("./resources/CesiumMan_img0.jpg");

    man.transform = glms_translate(glms_mat4_identity(), (vec3s){3, 2, -2});
    man.transform = glms_rotate_y(man.transform, 90*DEG2RAD);

    #define SHADOW_MAP_WIDTH 1024
    #define SHADOW_MAP_HEIGHT 1024

    unsigned int shadowmap_fbo;
    unsigned int shadowmap_tex;
    glad_glGenFramebuffers(1, &shadowmap_fbo);

    glad_glGenTextures(1, &shadowmap_tex);
    glad_glBindTexture(GL_TEXTURE_2D, shadowmap_tex);
    glad_glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 0,
                      GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glad_glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

    glad_glBindFramebuffer(GL_FRAMEBUFFER, shadowmap_fbo);
    glad_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowmap_tex, 0);
    glad_glDrawBuffer(GL_NONE);
    glad_glReadBuffer(GL_NONE);
    glad_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glad_glBindTexture(GL_TEXTURE_2D, 0);


    unsigned int shadowmap_shader = create_shader_program("./shaders/shadow_mapping_vert.glsl", "./shaders/shadow_mapping_frag.glsl");

    glUseProgram(light_shader);
    glUniform1i(glGetUniformLocation(light_shader, "tex"), 0);
    glUniform1i(glGetUniformLocation(light_shader, "shadow_map"), 1);
    glUseProgram(skinning_shader);
    glUniform1i(glGetUniformLocation(skinning_shader, "tex"), 0);
    glUniform1i(glGetUniformLocation(skinning_shader, "shadow_map"), 1);

    //main loop
    while (!window_should_close()) {
        double dt = get_frame_time();

        light.position = cam.position;
        light.direction = glms_vec3_sub(cam.target, cam.position);

        camera_move(&cam, 0.2f, 2.0f, dt);
        // 9 is f key
        if(is_key_pressed_once(9)){
            light.enabled = !light.enabled;
        }


        update_model_animation(&man, 0, dt);

        update_light_values(light, light_shader);
        update_light_values(sun, light_shader);
        update_light_values(light, skinning_shader);
        update_light_values(sun, skinning_shader);


        // shadow pass
        glad_glCullFace(GL_FRONT);
        mat4s light_projection = glms_ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 20.0f);
        mat4s light_view = glms_lookat(sun.position, (vec3s){0,0,0}, (vec3s){0,1,0});
        mat4s light_space_matrix = glms_mat4_mul(light_projection, light_view);
        glad_glUseProgram(shadowmap_shader);
        unsigned int light_space_matrix_loc = glad_glGetUniformLocation(shadowmap_shader, "light_space_matrix");
        glad_glUniformMatrix4fv(light_space_matrix_loc, 1, GL_FALSE, (const float*)light_space_matrix.raw);
        glad_glViewport(0,0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
        glad_glBindFramebuffer(GL_FRAMEBUFFER, shadowmap_fbo);
        glad_glClear(GL_DEPTH_BUFFER_BIT);
        glad_glBindTexture(GL_TEXTURE_2D, shadowmap_tex);
        house.material.shader = shadowmap_shader;
        man.material.shader = shadowmap_shader;
        draw_model(&house, &cam, (Color){0.7f, 0.65f, 0.75f, 1}, shadowmap_tex, light_space_matrix);
        draw_model(&man, &cam, (Color){0.7f, 0.65f, 0.75f, 1}, shadowmap_tex, light_space_matrix);

        glad_glBindFramebuffer(GL_FRAMEBUFFER, 0);

        begin_drawing(&cam);
        clear_background(BLACK);

        // normal pass
        glad_glCullFace(GL_BACK);
        glad_glViewport(0,0, global.window_context.screen_width, global.window_context.screen_height);
        house.material.shader = light_shader;
        man.material.shader = skinning_shader;
        draw_model(&house, &cam, (Color){0.7f, 0.65f, 0.75f, 1}, shadowmap_tex, light_space_matrix);
        draw_model(&man, &cam, (Color){0.7f, 0.65f, 0.75f, 1}, shadowmap_tex, light_space_matrix);

        draw_cubemap(cube_map, cam);

        end_drawing();
    }

    free_model(&house);
    free_model(&man);

    close_window();

    return 0;
}
