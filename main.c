#include "glad/glad.h"
#include "matlib/matlib.h"
#include "matlib/lights.h"

#include "renderer/renderer.h"

#include "renderer_ui.h"
#include "matlib/global.h"

#define WIDTH 800
#define HEIGHT 500

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

    Camera cam = create_and_init_camera(CAMERA_PERSPECTIVE);
    set_camera_fov(&cam, 65);

    Renderer renderer;
    renderer_init(&renderer, &cam);

    renderer_load_cube_map(&renderer, sky_box_textures);

    Model house = load_model("./resources/medieval_house.obj");
    house.material.albedo = load_texture("./resources/houseTexture.png");

    Model ground = load_model("./resources/ground.obj");
    ground.material.albedo = load_texture("./resources/grass.png");

    int sun_id = renderer_create_light(&renderer, LIGHT_TYPE_DIRECTIONAL, (vec3s){10,10,0}, (Color){0.8f,0.75f,0.65f,1}, 1, (vec3s){-1,-1,0});
    Light* sun = &renderer.lights[sun_id];
    int light_id  = renderer_create_light(&renderer, LIGHT_TYPE_SPOT, (vec3s){0,1,1}, RED, 4, GLMS_VEC3_ONE);

    Model man = load_model("./resources/CesiumMan.glb");
    man.material.albedo = load_texture("./resources/CesiumMan_img0.jpg");

    man.transform = glms_translate(glms_mat4_identity(), (vec3s){3, 2, -2});
    man.transform = glms_rotate_y(man.transform, 90*DEG2RAD);

    Model models[] = {ground,house, man};
    
    renderer.ambient = (Color){0.7f, 0.69f, 0.63f, 1.0f};
    renderer.fog.color = (Color){0.7f, 0.6f, 0.66f, 1.0f};
    float time = 0;

    bool enable_ui = false;
    RendererUIState ui;
    init_ui_renderer(&ui);
    struct nk_context* ctx = nk_sdl_init(global.window_context.window);

    {struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();}

    while (!window_should_close(ctx)) {
        double dt = get_frame_time();
        time += dt;

        sun->direction.x = cosf(time * 0.5f);
        sun->direction.y = cosf(time * 0.5f) - 1;
        sun->direction.z = sinf(time * 0.5f);
        sun->position = glms_vec3_scale(glms_vec3_normalize(sun->direction), -10.0f);

        Light* light = &renderer.lights[light_id];
        light->position = cam.position;
        light->direction = glms_vec3_sub(cam.target, cam.position);

        camera_move(&cam, 0.2f, 2.0f, dt);
        // 9 is f key
        if(is_key_pressed_once(9)){
            light->enabled = !light->enabled;
        }

        update_model_animation(&man, 0, dt, renderer.skinning_shader);

        if(enable_ui)
            draw_ui_renderer(ctx, &renderer, &ui);
        // 20 is q key
        if(is_key_pressed_once(20))
            enable_ui = !enable_ui;

        renderer_update_lights(&renderer);

        renderer_shadow_pass(&renderer, models, 3);
        renderer_normal_pass(&renderer, models, 3);

        nk_sdl_render(NK_ANTI_ALIASING_OFF, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
        set_default_gl_state();

        end_drawing();
    }

    free_model(&house);
    free_model(&man);

    nk_sdl_shutdown();
    close_window();

    return 0;
}
