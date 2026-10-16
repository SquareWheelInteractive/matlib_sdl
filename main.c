#include "matlib/matlib.h"
#include "glad/glad.h"
#include "matlib/lights.h"

#include "renderer/renderer.h"

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

    Camera cam = create_and_init_camera(CAMERA_PERSPECTIVE);
    set_camera_fov(&cam, 65);

    Renderer renderer;
    renderer_init(&renderer, &cam);

    renderer_load_cube_map(&renderer, sky_box_textures);

    Model house = load_model("./resources/medieval_house.obj");
    house.material.albedo = load_texture("./resources/houseTexture.png");

    int light_id  = renderer_create_light(&renderer, LIGHT_TYPE_SPOT, (vec3s){0,1,1}, YELLOW, 4, GLMS_VEC3_ONE);
    int sun_id = renderer_create_light(&renderer, LIGHT_TYPE_DIRECTIONAL, (vec3s){10,10,0}, (Color){0.8f,0.75f,0.65f,1}, 1, (vec3s){-1,-1,0});

    Model man = load_model("./resources/CesiumMan.glb");
    man.material.albedo = load_texture("./resources/CesiumMan_img0.jpg");

    man.transform = glms_translate(glms_mat4_identity(), (vec3s){3, 2, -2});
    man.transform = glms_rotate_y(man.transform, 90*DEG2RAD);

    Model models[] = {house, man};
    
    //main loop
    while (!window_should_close()) {
        double dt = get_frame_time();

        Light* light = &renderer.lights[light_id];
        light->position = cam.position;
        light->direction = glms_vec3_sub(cam.target, cam.position);

        camera_move(&cam, 0.2f, 2.0f, dt);
        // 9 is f key
        if(is_key_pressed_once(9)){
            light->enabled = !light->enabled;
        }

        update_model_animation(&man, 0, dt, renderer.skinning_shader);

        update_light_values(renderer.lights[light_id], renderer.skinning_shader);
        update_light_values(renderer.lights[sun_id], renderer.skinning_shader);

        renderer_shadow_pass(&renderer, models, 2);
        renderer_normal_pass(&renderer, models, 2);
    }

    free_model(&house);
    free_model(&man);

    close_window();

    return 0;
}
