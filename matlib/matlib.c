#include "matlib.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "glad/glad.h"
#include "global.h"
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"
#define FAST_OBJ_IMPLEMENTATION
#include "external/fast_obj.h"
#define CGLTF_IMPLEMENTATION
#include "external/cgltf.h"
#include "skeleton.h"

/* - - - Window related - - - */

bool init_window(const char* app_name, int width, int height){
    global.window_context.screen_width = width;
    global.window_context.screen_height = height;

    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    // Create window
    global.window_context.window = SDL_CreateWindow(app_name, width, height, SDL_WINDOW_OPENGL);
    if (!global.window_context.window) {
        SDL_Log("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    // Enable resizing
    if(!SDL_SetWindowResizable(global.window_context.window, true)){
        SDL_Log("Setting window to resizable has failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(global.window_context.window);
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("Failed to load OpenGL\n");
        return -1;
    }

    glad_glViewport(0,0, width, height);

    glad_glEnable(GL_DEPTH_TEST);
    glad_glEnable(GL_CULL_FACE);
    glad_glCullFace(GL_BACK);
    glad_glEnable(GL_BLEND);
    glad_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable vsync
    if (!SDL_GL_SetSwapInterval(1)) {
        SDL_Log("VSync failed: %s", SDL_GetError());
    }


    return 1;
}

static void resize_window(Camera* cam){
    if(global.window_context.has_resized){
        glad_glViewport(0,0, global.window_context.screen_width, global.window_context.screen_height);
        switch (cam->type) {
            case CAMERA_PERSPECTIVE:
                cam->proj_matrix = glms_perspective(cam->fovy * DEG2RAD, (float)global.window_context.screen_width / global.window_context.screen_height, 0.01f, 100.0f);
                break;
            case CAMERA_ORTHO:
                ;float halfHeight = cam->zoom;
                float halfWidth = cam->zoom * (float)global.window_context.screen_width / global.window_context.screen_height;;
                cam->proj_matrix = glms_ortho(
                    -halfWidth,
                     halfWidth,
                    -halfHeight,
                     halfHeight,
                    0.01f,
                    100.0f
                );
                break;
        }
    }
}

bool window_should_close(){
    global.input.is_event_down  = false;
    global.input.is_evenet_up   = false;
    global.window_context.has_resized = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                return true;
                break;
            case SDL_EVENT_KEY_DOWN:
                global.input.is_event_down = true;
                global.input.scan_code = event.key.scancode;
                break;
            case SDL_EVENT_KEY_UP:
                global.input.is_evenet_up = true;
                global.input.scan_code = event.key.scancode;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                global.window_context.screen_width = event.window.data1;
                global.window_context.screen_height= event.window.data2;
                global.window_context.has_resized = true;
                break;
        }
    }
    return false;
}

void close_window(){
    SDL_DestroyWindow(global.window_context.window);
    SDL_Quit();
}

/* - - - Model related - - - */

static int is_file_extension(const char* filename, const char* extension) {
    size_t file_len = strlen(filename);
    size_t ext_len = strlen(extension);

    if (ext_len > file_len)
        return 0;

    return strcmp(filename + file_len - ext_len, extension) == 0;
}


void draw_model(Model* model, Camera* cam, Color ambient){
    if(!model || !cam || !model->meshes) return;
    mat4s t = glms_translate_make(model->local_transform.translation);
    mat4s r = glms_quat_mat4(model->local_transform.rotation);
    mat4s s = glms_scale_make(model->local_transform.scale);

    mat4s trs = glms_mat4_mul(t, glms_mat4_mul(r, s));

    glad_glUseProgram(model->material.shader);

    unsigned int model_loc   = glad_glGetUniformLocation(model->material.shader, "model");
    unsigned int view_loc    = glad_glGetUniformLocation(model->material.shader, "view");
    unsigned int proj_loc    = glad_glGetUniformLocation(model->material.shader, "projection");
    unsigned int ambient_loc = glad_glGetUniformLocation(model->material.shader, "ambient");

    glad_glUniformMatrix4fv(model_loc, 1, GL_FALSE, (const float*)trs.raw);
    glad_glUniformMatrix4fv(view_loc , 1, GL_FALSE, (const float*)cam->view_matrix.raw);
    glad_glUniformMatrix4fv(proj_loc , 1, GL_FALSE, (const float*)cam->proj_matrix.raw);
    glad_glUniform4f(ambient_loc, ambient.r, ambient.g, ambient.b, ambient.a);

    if(model->material.albedo.id > 0)
        glad_glBindTexture(GL_TEXTURE_2D, model->material.albedo.id);

    for (size_t i = 0; i < model->mesh_count; i++) {
        glad_glBindVertexArray(model->meshes[i].vao);
        if(model->meshes[i].index_count > 0)
            glad_glDrawElements(GL_TRIANGLES, model->meshes[i].index_count, GL_UNSIGNED_INT, 0);
        else
            glad_glDrawArrays(GL_TRIANGLES, 0, model->meshes[i].vertex_count);

        glad_glBindVertexArray(0);
    }
    glad_glBindTexture(GL_TEXTURE_2D, 0);
    glad_glUseProgram(0);
}

void free_model(Model* model){
    if(!model)return;
    for (size_t i = 0; i< model->mesh_count; i++) {
        free_mesh(&model->meshes[i]);
    }

    if(model->meshes != NULL)
        free(model->meshes);

    skeleton_free(&model->skeleton);
}

/* - - - Mesh related - - - */
Mesh* load_meshes_gltf(const char* filename, cgltf_data* data, unsigned int* mesh_cont){
    *mesh_cont = data->meshes[0].primitives_count;
    
    Mesh* meshes_out = malloc(sizeof(Mesh) * *mesh_cont);

    for (size_t i = 0 ; i < *mesh_cont; i++) {
        meshes_out[i] = load_mesh_gltf(filename, data, i);
    }

    return meshes_out;
}

Mesh load_mesh_gltf(const char* filename, cgltf_data* data, unsigned int primitive_index) {
    Mesh mesh_out = {0};
    mesh_out.positions = NULL;
    mesh_out.normals = NULL;
    mesh_out.tex_coords = NULL;
    mesh_out.bone_ids= NULL;
    mesh_out.weights = NULL;
        
    // Safety check: Ensure we actually have at least one mesh and one primitive
    if (data->meshes_count > 0 && data->meshes[0].primitives_count > 0) {
        
        // For now, we only grab the first primitive of the first mesh
        cgltf_primitive* primitive = &data->meshes[0].primitives[primitive_index];
   
        for (cgltf_size k = 0; k < primitive->attributes_count; ++k) {
            cgltf_attribute* attr = &primitive->attributes[k];
            cgltf_accessor* accessor = attr->data;
                        
    
            cgltf_size float_count = cgltf_accessor_unpack_floats(accessor, NULL, 0);
            float* num_buffer = (float*)malloc(sizeof(float) * float_count);
            cgltf_accessor_unpack_floats(accessor, num_buffer, float_count);
    
            int num_components = cgltf_num_components(accessor->type);

            if (attr->type == cgltf_attribute_type_position) {
                mesh_out.positions = malloc(accessor->count * 3 * sizeof(float));
                mesh_out.vertex_count = accessor->count;
                
                for (cgltf_size v = 0; v < accessor->count; ++v) {
                    mesh_out.positions[v * 3 + 0] = num_buffer[v * num_components + 0];
                    mesh_out.positions[v * 3 + 1] = num_buffer[v * num_components + 1];
                    mesh_out.positions[v * 3 + 2] = num_buffer[v * num_components + 2];
                }
            } 
            else if (attr->type == cgltf_attribute_type_normal) {
                mesh_out.normals = malloc(accessor->count * 3 * sizeof(float));
                
                for (cgltf_size v = 0; v < accessor->count; ++v) {
                    mesh_out.normals[v * 3 + 0] = num_buffer[v * num_components + 0];
                    mesh_out.normals[v * 3 + 1] = num_buffer[v * num_components + 1];
                    mesh_out.normals[v * 3 + 2] = num_buffer[v * num_components + 2];
                }
            } 
            else if (attr->type == cgltf_attribute_type_texcoord) {
                mesh_out.tex_coords = malloc(accessor->count * 2 * sizeof(float));
                
                for (cgltf_size v = 0; v < accessor->count; ++v) {
                    mesh_out.tex_coords[v * 2 + 0] = num_buffer[v * num_components + 0];
                    // Note: "1.0 - ..." to flip Y coord, OpenGL expect Y on bottom left
                    mesh_out.tex_coords[v * 2 + 1] = 1.0f - num_buffer[v * num_components + 1]; 
                }
            }
            else if (attr->type == cgltf_attribute_type_joints) {
                mesh_out.bone_ids= malloc(accessor->count * 4 * sizeof(uint8_t));
                // Unpack and copy data into mesh_out.joint_ids (4 components per vertex)
                for (cgltf_size v = 0; v < accessor->count; ++v) {
                    mesh_out.bone_ids[v * 4 + 0] = (unsigned int)num_buffer[v * num_components + 0];
                    mesh_out.bone_ids[v * 4 + 1] = (unsigned int)num_buffer[v * num_components + 1];
                    mesh_out.bone_ids[v * 4 + 2] = (unsigned int)num_buffer[v * num_components + 2];
                    mesh_out.bone_ids[v * 4 + 3] = (unsigned int)num_buffer[v * num_components + 3];
                }
            }
            else if (attr->type == cgltf_attribute_type_weights) {
                mesh_out.weights = malloc(accessor->count * 4 * sizeof(float));
                // Unpack and copy data into mesh_out.weights (4 components per vertex)
                for (cgltf_size v = 0; v < accessor->count; ++v) {
                    mesh_out.weights[v * 4 + 0] = num_buffer[v * num_components + 0];
                    mesh_out.weights[v * 4 + 1] = num_buffer[v * num_components + 1];
                    mesh_out.weights[v * 4 + 2] = num_buffer[v * num_components + 2];
                    mesh_out.weights[v * 4 + 3] = num_buffer[v * num_components + 3];
                }
            }
            free(num_buffer);
        }

        if (primitive->indices) {
            mesh_out.index_count = primitive->indices->count;
            mesh_out.indices = malloc(mesh_out.index_count * sizeof(uint32_t));
            for (cgltf_size i = 0; i < mesh_out.index_count; ++i) {
                mesh_out.indices[i] = (uint32_t)cgltf_accessor_read_index(primitive->indices, i);
            }
        }
    }
    glad_glGenVertexArrays(1, &mesh_out.vao);
    glad_glBindVertexArray(mesh_out.vao);

    glad_glGenBuffers(5, mesh_out.vbo_ids);

    glad_glBindBuffer(GL_ARRAY_BUFFER, mesh_out.vbo_ids[ATTRIB_POSITION]);
    glad_glBufferData(GL_ARRAY_BUFFER, mesh_out.vertex_count * 3 * sizeof(float), mesh_out.positions, GL_STATIC_DRAW);
    glad_glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glad_glEnableVertexAttribArray(ATTRIB_POSITION);

    glad_glBindBuffer(GL_ARRAY_BUFFER, mesh_out.vbo_ids[ATTRIB_NORMAL]);
    glad_glBufferData(GL_ARRAY_BUFFER, mesh_out.vertex_count * 3 * sizeof(float), mesh_out.normals, GL_STATIC_DRAW);
    glad_glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glad_glEnableVertexAttribArray(ATTRIB_NORMAL);

    glad_glBindBuffer(GL_ARRAY_BUFFER, mesh_out.vbo_ids[ATTRIB_TEX_COORDS]);
    glad_glBufferData(GL_ARRAY_BUFFER, mesh_out.vertex_count * 2 * sizeof(float), mesh_out.tex_coords, GL_STATIC_DRAW);
    glad_glVertexAttribPointer(ATTRIB_TEX_COORDS, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glad_glEnableVertexAttribArray(ATTRIB_TEX_COORDS);

    glad_glBindBuffer(GL_ARRAY_BUFFER, mesh_out.vbo_ids[ATTRIB_BONE_IDS]);
    glad_glBufferData(GL_ARRAY_BUFFER, mesh_out.vertex_count * 4 * sizeof(int), mesh_out.bone_ids, GL_STATIC_DRAW);
    glad_glVertexAttribIPointer(ATTRIB_BONE_IDS, 4, GL_UNSIGNED_BYTE, 4 * sizeof(uint8_t), (void*)0);
    glad_glEnableVertexAttribArray(ATTRIB_BONE_IDS);

    glad_glBindBuffer(GL_ARRAY_BUFFER, mesh_out.vbo_ids[ATTRIB_WEIGHTS]);
    glad_glBufferData(GL_ARRAY_BUFFER, mesh_out.vertex_count * 4 * sizeof(float), mesh_out.weights, GL_STATIC_DRAW);
    glad_glVertexAttribPointer(ATTRIB_WEIGHTS, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glad_glEnableVertexAttribArray(ATTRIB_WEIGHTS);

    unsigned int ebo;
    glad_glGenBuffers(1, &ebo);
    glad_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glad_glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_out.index_count * sizeof(unsigned int), mesh_out.indices, GL_STATIC_DRAW);

    glad_glBindVertexArray(0);
    glad_glBindBuffer(GL_ARRAY_BUFFER, 0);
    glad_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return mesh_out;
}

Model load_model(const char* file_name){
    Model model_out = {0};
    model_out.local_transform.translation = glms_vec3_zero();
    model_out.local_transform.rotation    = glms_quat_identity();
    model_out.local_transform.scale       = glms_vec3_one();
    model_out.material.shader = 0;
    model_out.material.albedo = (Texture){0};
    model_out.skeleton = (Skeleton){0};
    model_out.meshes = NULL;
    model_out.mesh_count = 0;

    if(is_file_extension(file_name , ".obj")){
        model_out.meshes = malloc(sizeof(Model));
        model_out.mesh_count = 1;
        model_out.meshes[0] = load_mesh_obj(file_name);
    }
    if(is_file_extension(file_name, ".glb") || is_file_extension(file_name, ".gltf")){
        cgltf_options options = {0};
        cgltf_data* data = NULL;
    
        cgltf_result result = cgltf_parse_file(&options, file_name, &data);
    
        if (result == cgltf_result_success) {
            cgltf_load_buffers(&options, data, file_name);
            model_out.meshes = load_meshes_gltf(file_name, data, &model_out.mesh_count); // mesh loading
            skeleton_load(&model_out.skeleton, data, 0);

            cgltf_free(data);
        }
    }

    return model_out;
}
Mesh load_mesh_obj(const char* path){
    Mesh mesh_out = {0};
    mesh_out.positions = NULL;
    mesh_out.normals= NULL;
    mesh_out.tex_coords= NULL;
    mesh_out.bone_ids= NULL;
    mesh_out.weights= NULL;

    fastObjMesh* mesh = fast_obj_read(path);
    assert(mesh);

    unsigned int vertices_count = mesh->face_count * 3;
    mesh_out.vertex_count       = vertices_count;
    mesh_out.index_count        = mesh->index_count;

    mesh_out.positions   = malloc(vertices_count * 3 * sizeof(float));
    mesh_out.normals     = malloc(vertices_count * 3 * sizeof(float));
    mesh_out.tex_coords  = malloc(vertices_count * 2 * sizeof(float));
    mesh_out.indices     = malloc(mesh->index_count * sizeof(unsigned int));

    for (unsigned int i = 0; i < mesh->index_count; i++) {
        fastObjIndex inx = mesh->indices[i];

        mesh_out.positions[i * 3 + 0] = mesh->positions[inx.p * 3 + 0];
        mesh_out.positions[i * 3 + 1] = mesh->positions[inx.p * 3 + 1];
        mesh_out.positions[i * 3 + 2] = mesh->positions[inx.p * 3 + 2];

        if(inx.n < mesh->normal_count){
            mesh_out.normals[i * 3 + 0] = mesh->normals[inx.n * 3 + 0];
            mesh_out.normals[i * 3 + 1] = mesh->normals[inx.n * 3 + 1];
            mesh_out.normals[i * 3 + 2] = mesh->normals[inx.n * 3 + 2];
        }else{
            mesh_out.normals[i * 3 + 0] = 0.0f;
            mesh_out.normals[i * 3 + 1] = 0.0f;
            mesh_out.normals[i * 3 + 2] = 0.0f;
        }

        if(inx.t < mesh->texcoord_count){
            mesh_out.tex_coords[i * 2 + 0] = mesh->texcoords[inx.t * 2 + 0];
            mesh_out.tex_coords[i * 2 + 1] = mesh->texcoords[inx.t * 2 + 1];
        }else{
            mesh_out.tex_coords[i * 2 + 0] = 0.0f;
            mesh_out.tex_coords[i * 2 + 1] = 0.0f;
        }

        mesh_out.indices[i] = i;
    }

    fast_obj_destroy(mesh);

    glad_glGenVertexArrays(1, &mesh_out.vao);
    glad_glBindVertexArray(mesh_out.vao);

    glad_glGenBuffers(3, mesh_out.vbo_ids);

    glad_glBindBuffer(GL_ARRAY_BUFFER, mesh_out.vbo_ids[ATTRIB_POSITION]);
    glad_glBufferData(GL_ARRAY_BUFFER, mesh_out.vertex_count * 3 * sizeof(float), mesh_out.positions, GL_STATIC_DRAW);
    glad_glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glad_glEnableVertexAttribArray(ATTRIB_POSITION);

    glad_glBindBuffer(GL_ARRAY_BUFFER, mesh_out.vbo_ids[ATTRIB_NORMAL]);
    glad_glBufferData(GL_ARRAY_BUFFER, mesh_out.vertex_count * 3 * sizeof(float), mesh_out.normals, GL_STATIC_DRAW);
    glad_glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glad_glEnableVertexAttribArray(ATTRIB_NORMAL);

    glad_glBindBuffer(GL_ARRAY_BUFFER, mesh_out.vbo_ids[ATTRIB_TEX_COORDS]);
    glad_glBufferData(GL_ARRAY_BUFFER, mesh_out.vertex_count * 2 * sizeof(float), mesh_out.tex_coords, GL_STATIC_DRAW);
    glad_glVertexAttribPointer(ATTRIB_TEX_COORDS, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glad_glEnableVertexAttribArray(ATTRIB_TEX_COORDS);

    unsigned int ebo;
    glad_glGenBuffers(1, &ebo);
    glad_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glad_glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_out.index_count * sizeof(unsigned int), mesh_out.indices, GL_STATIC_DRAW);

    glad_glBindVertexArray(0);
    glad_glBindBuffer(GL_ARRAY_BUFFER, 0);
    glad_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return mesh_out;
}

void free_mesh(Mesh* mesh){
    if(mesh->positions != NULL)
        free(mesh->positions);
    if(mesh->normals != NULL)
        free(mesh->normals);
    if(mesh->tex_coords != NULL)
        free(mesh->tex_coords);
    if(mesh->indices != NULL)
        free(mesh->indices);
    if(mesh->bone_ids != NULL)
        free(mesh->bone_ids);
    if(mesh->weights != NULL)
        free(mesh->weights);
}

/* - - - Input related - - - */

static bool key_is_pressed[128];
bool is_key_pressed_once(SDL_Scancode scan_code){
    if(global.input.is_event_down && key_is_pressed[scan_code] == false){
        if(global.input.scan_code == scan_code){
            key_is_pressed[scan_code] = true;
            return true;
        }
    }
    if(global.input.is_evenet_up && key_is_pressed[scan_code] == true){
        if(global.input.scan_code == scan_code){
            key_is_pressed[scan_code] = false;
            return false;
        }
    }
   
    return false;
}

char* get_file_content(const char* fileName){
    FILE *fp;
    long size = 0;
    char* file_content;
    
    /* Read File to get size */
    fp = fopen(fileName, "rb");
    if(fp == NULL) {
        return NULL;
    }
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp)+1;
    fclose(fp);
    /* Read File for Content */
    fp = fopen(fileName, "r");
    file_content = (char*)memset(malloc(size), '\0', size);
    fread(file_content, 1, size-1, fp);
    fclose(fp);

    return file_content;
}
unsigned int create_shader_program(const char* vert_shader_path, const char* frag_shader_path){
    const char* vert = get_file_content(vert_shader_path);
    unsigned int vert_shader = glad_glCreateShader(GL_VERTEX_SHADER);
    glad_glShaderSource(vert_shader, 1, &vert, NULL);
    glad_glCompileShader(vert_shader);

    const char* frag = get_file_content(frag_shader_path);
    unsigned int frag_shader= glad_glCreateShader(GL_FRAGMENT_SHADER);
    glad_glShaderSource(frag_shader, 1, &frag, NULL);
    glad_glCompileShader(frag_shader);
    unsigned int shader_prog = glad_glCreateProgram();
    glad_glAttachShader(shader_prog, vert_shader);
    glad_glAttachShader(shader_prog, frag_shader);
    glad_glLinkProgram(shader_prog);

    glad_glDeleteShader(vert_shader);
    glad_glDeleteShader(frag_shader);
    GLint success;
    glGetProgramiv(shader_prog, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shader_prog, 512, NULL, infoLog);
        printf("Shader link error:\n%s\n", infoLog);
    }

    free((void*)vert);
    free((void*)frag);
    glad_glUseProgram(0);

    return shader_prog;
}

void clear_background(Color color){
    glad_glClearColor(color.r,color.g,color.b, color.a);
}

static void update_camera_matrix(Camera* cam){
    cam->view_matrix = glms_lookat(cam->position, cam->target, cam->up);
}

void begin_drawing(Camera* cam){
    glad_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    update_camera_matrix(cam);
    resize_window(cam);
}
void end_drawing(){
    SDL_GL_SwapWindow(global.window_context.window);
}

Camera create_camera(CameraProjectionType type){
    Camera cam = {0};

    cam.fovy = 70;
    cam.target = (vec3s){.x = 0, .y = 0, .z = 0};
    cam.position = (vec3s){.x = 0, .y = 0, .z = 2};
    cam.up = (vec3s){0,1,0};
    cam.zoom = 1;
    float aspect = (float)global.window_context.screen_width / global.window_context.screen_height;
    switch (type) {
        case CAMERA_PERSPECTIVE:
            cam.type = CAMERA_PERSPECTIVE;
            cam.proj_matrix = glms_perspective(cam.fovy * DEG2RAD, aspect, 0.05f, 100.0f);
            break;
        case CAMERA_ORTHO:
            cam.type = CAMERA_ORTHO;
            cam.zoom = 5;
            float halfHeight = cam.zoom;
            float halfWidth = cam.zoom* aspect;

            cam.proj_matrix = glms_ortho(
                -halfWidth,
                 halfWidth,
                -halfHeight,
                 halfHeight,
                0.01f,
                100.0f
            );
            break;
        default:
            printf("CAMERA CREATION ERROR: try using the correct enum values\n");
            break;
    }

    return cam;
}
Texture load_texture(const char* path){
    Texture texture = {0};
    glad_glGenTextures(1, &texture.id);
    glad_glBindTexture(GL_TEXTURE_2D, texture.id);

    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(path, &texture.width, &texture.height, &texture.channels, 0);
    if(data){
        switch (texture.channels) {
            case 3:
                glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glad_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.width, texture.height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                glad_glGenerateMipmap(GL_TEXTURE_2D);
                break;
            case 4:
                glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glad_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                glad_glGenerateMipmap(GL_TEXTURE_2D);
                break;
        }
    }
    else
        printf("FAILED TO LOAD TEXTURE");

    stbi_image_free(data);

    glad_glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

static float pitch = 0, yaw = -90;
static bool cursor_captured = false;
void update_camera(Camera* camera, float sens, float move_speed, float dt){
    SDL_SetWindowRelativeMouseMode(global.window_context.window, cursor_captured);
    if(is_key_pressed_once(SDL_SCANCODE_ESCAPE)){
        cursor_captured = !cursor_captured;
    }

    vec2s mouse_delta;
    /* -- Getting mouse delta every frame to avoid camera snapping / jump when cursor is recaptured -- */
    SDL_GetRelativeMouseState(&mouse_delta.x, &mouse_delta.y);

    if(!cursor_captured) return;

    yaw += mouse_delta.x * sens;
    pitch += -mouse_delta.y * sens;
    if(pitch > 89) pitch = 89;
    if(pitch < -89) pitch = -89;

    float ry = yaw * DEG2RAD;
    float rp = pitch * DEG2RAD;

    vec3s target = (vec3s){.x = cosf(ry) * cosf(rp),
                           .y = sinf(rp),
                           .z = sinf(ry) * cosf(rp)};

    vec3s forward = glms_vec3_normalize(glms_vec3_sub(camera->target, camera->position));
    vec3s right = glms_vec3_normalize(glms_vec3_cross(forward, camera->up));

    vec3s dir = {0};
    const bool* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_W])
        dir = glms_vec3_add(dir, forward);
    if (keys[SDL_SCANCODE_S])
        dir = glms_vec3_sub(dir, forward);
    if (keys[SDL_SCANCODE_D])
        dir = glms_vec3_add(dir, right);
    if (keys[SDL_SCANCODE_A])
        dir = glms_vec3_sub(dir, right);
    if (keys[SDL_SCANCODE_SPACE])
        dir = glms_vec3_add(dir, camera->up);

    dir = glms_vec3_normalize(dir);

    camera->position = glms_vec3_add(camera->position, glms_vec3_scale(dir, move_speed * dt));
    camera->target = glms_vec3_add(camera->position, target);
}

static float last_time = 0;
float get_frame_time(){
    float curr_time = SDL_GetTicks() * 0.001;

    float delta =  curr_time - last_time;
    last_time = curr_time;;
    return delta;
}

/* ----------------------------------------------------------------------------------------
    Animates model skeleton, calculates final bone matrices and upload them to GPU 
   ----------------------------------------------------------------------------------------*/
void update_model_animation(Model* model, int anim_index, float dt){
    model->skeleton.anim_state.clip_index = anim_index;
    float t = skeleton_advance(&model->skeleton, dt);
    skeleton_update(&model->skeleton, t);

    glad_glUseProgram(model->material.shader);

    int loc = glad_glGetUniformLocation(model->material.shader, "u_bone_matrices");

    if (loc != -1) {
        glad_glUniformMatrix4fv(
            loc,
            model->skeleton.bone_count,
            GL_FALSE,
            (float *)model->skeleton.final_matrices);
    }

    glad_glUseProgram(0);
}
