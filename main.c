#include "matlib/matlib.h"
#include "matlib/global.h"

#define WIDTH 1000
#define HEIGHT 600

Global global;

int main() {
    // initialize window
    if(!init_window("pingo", WIDTH, HEIGHT)){
        printf("failed to init sdl window\n");
    }

    unsigned int shader = create_shader_program("./matlib/vert.glsl", "./matlib/frag.glsl");

    Camera cam = create_camera(CAMERA_PERSPECTIVE);

    Model house = load_model("./resources/medieval_house.obj");
    house.shader = shader;
    house.texture = load_texture("./resources/houseTexture.png");

    Model man = load_model("./resources/man.glb");
    man.shader = shader;
    man.transform = glms_translate_make((vec3s){0,0,3});

    //main loop
    while (!window_should_close()) {
        double dt = get_frame_time();;
        update_camera(&cam, 0.2f, 2.0f, dt);

        begin_drawing(&cam, shader);
        clear_background(GRAY);

        draw_model(&house, &cam, GRAY);
        draw_model(&man, &cam, WHITE);

        end_drawing();
    }

    free_model(&house);
    free_model(&man);

    close_window();

    return 0;
}
