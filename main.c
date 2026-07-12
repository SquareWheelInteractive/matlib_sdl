#include "matlib/matlib.h"
#include "matlib/global.h"
#include "glad/glad.h"

#define WIDTH 1000
#define HEIGHT 600

Global global;

int main() {
    // initialize window
    if(!init_window("pingo", WIDTH, HEIGHT)){
        printf("failed to init sdl window\n");
    }

    unsigned int shader = create_shader_program("./matlib/vert.glsl", "./matlib/frag.glsl");
    unsigned int skinning_shader = create_shader_program("./matlib/skinning_vert.glsl", "./matlib/frag.glsl");

    Camera cam = create_camera(CAMERA_PERSPECTIVE);

    Model house = load_model("./resources/medieval_house.obj");
    house.shader = shader;
    house.texture = load_texture("./resources/houseTexture.png");

    Model man = load_model("./resources/CesiumMan.glb");
    man.shader = skinning_shader;
    man.transform = glms_translate_make((vec3s){0,0,3});

    int anim_index = 0;
    //main loop
    while (!window_should_close()) {
        double dt = get_frame_time();;
        update_camera(&cam, 0.2f, 2.0f, dt);

        if(is_key_pressed_once(SDL_SCANCODE_RIGHT))
            anim_index++;
        if(is_key_pressed_once(SDL_SCANCODE_LEFT))
            anim_index--;

        update_model_animation(&man, anim_index, dt);

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


