#include "matlib/matlib.h"
#include "glad/glad.h"
#include "matlib/node_hierarchy.h"

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

    unsigned int shader = create_shader_program("./shaders/vert.glsl", "./shaders/frag.glsl");
    unsigned int skinning_shader = create_shader_program("./shaders/skinning_vert.glsl", "./shaders/frag.glsl");

    Camera cam = create_camera(CAMERA_PERSPECTIVE);

    Model house = load_model("./resources/medieval_house.obj");
    house.material.shader = shader;
    house.material.albedo = load_texture("./resources/houseTexture.png");

    Model man = load_model("./resources/CesiumMan.glb");
    man.material.shader = skinning_shader;
    man.transform = glms_translate_make((vec3s){0,0,1});
    man.material.albedo = load_texture("./resources/CesiumMan_img0.jpg");

    Node root = init_node();
    Node man_node = init_node();
    Node house_node = init_node();

    man_node.model = man;
    house_node.model = house;

    set_child(&root, &man_node);
    set_child(&root, &house_node);
    //main loop
    while (!window_should_close()) {
        double dt = get_frame_time();;
        update_camera(&cam, 0.2f, 2.0f, dt);

        update_model_animation(&man, 0, dt);
        update_node_transform_hierarchy(&root);

        begin_drawing(&cam);
        clear_background(BLACK);

        draw_node(&root, &cam, (Color){0.5f, 0.45f, 0.55f, 1});

        draw_cubemap(cube_map, cam);

        end_drawing();
    }

    free_model(&house);
    free_model(&man);

    close_window();

    return 0;
}
