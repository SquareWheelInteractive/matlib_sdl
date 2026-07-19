#include "matlib/matlib.h"
#include "glad/glad.h"
#include "matlib/node_hierarchy.h"
#include "matlib/lights.h"

#define WIDTH 1000
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

    Camera cam = create_camera(CAMERA_PERSPECTIVE);

    Model house = load_model("./resources/medieval_house.obj");
    house.material.shader = light_shader;
    house.material.albedo = load_texture("./resources/houseTexture.png");

    Node root = init_node();
    Node house_node = init_node();

    house_node.model = house;

    set_child(&root, &house_node);

    Light light = create_light(LIGHT_TYPE_POINT, (vec3s){3,1,0}, (Color){1,0,0,1}, 3, glms_vec3_zero(), light_shader);

    //main loop
    while (!window_should_close()) {
        light.position = cam.target;
        double dt = get_frame_time();;
        update_camera(&cam, 0.2f, 2.0f, dt);

        update_node_transform_hierarchy(&root);
        update_light_values(light, light_shader);

        begin_drawing(&cam);
        clear_background(BLACK);

        draw_node(&root, &cam, (Color){0.5f, 0.45f, 0.55f, 1});

        draw_cubemap(cube_map, cam);

        end_drawing();
    }

    free_model(&house);

    close_window();

    return 0;
}
