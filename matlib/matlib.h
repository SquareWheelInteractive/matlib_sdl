#pragma once
#include "../cglm/struct.h"// IWYU pragma: keep
#include "external/cgltf.h"
#include "skeleton.h"

#define DEG2RAD 0.0174533f
#define RAD2DEG 57.2958f

#define SKY_BLUE (Color){0.45f, 0.65f, 0.86f, 1.0f}
#define BLACK (Color){0.1f, 0.1f, 0.1f, 1.0f}
#define WHITE (Color){0.92f, 0.92f, 0.92f, 1.0f}
#define GRAY (Color){0.25f, 0.25f, 0.25f, 1.0f}

typedef enum{
    ATTRIB_POSITION   = 0x0,
    ATTRIB_TEX_COORDS = 0x1,
    ATTRIB_NORMAL     = 0x2,
    ATTRIB_BONE_IDS   = 0x3,
    ATTRIB_WEIGHTS    = 0x4
}VertexAttribLocs;

typedef enum{
    CAMERA_PERSPECTIVE = 0xa,
    CAMERA_ORTHO
} CameraProjectionType;

typedef struct{
    float r;
    float g;
    float b;
    float a;
} Color;
typedef struct{
    vec3s translation;
    versors rotation;
    vec3s scale;
} Transform;

typedef struct{
    vec3s target;
    vec3s position;
    vec3s up;
    float fovy;
    float zoom; // Only used when camera is orthographic

    mat4s view_matrix;
    mat4s proj_matrix;
} Camera;
typedef struct{
    unsigned int id;
    int width;
    int height;
    int channels;
} Texture;

typedef struct {
    unsigned int vao;
    unsigned int vbo_ids[5];

    float* positions;
    float* normals;
    float* tex_coords;
    uint8_t* bone_ids;
    float* weights;

    unsigned int vertex_count;

    unsigned int* indices;
    unsigned int index_count;
} Mesh;

typedef struct{
    mat4s transform;
    Texture texture;
    unsigned int shader;
    Mesh* meshes;
    unsigned int mesh_count;

    Skeleton skeleton;
} Model;

bool init_window(const char* app_name, int width, int height);
void close_window();
bool window_should_close();
unsigned int create_shader_program(const char* vs, const char* fs);
char* get_file_content(const char* fileName);
void draw_model(Model* mesh, Camera* cam, Color ambient);
void update_camera_matrix(Camera* cam, unsigned int shader_prog);
void clear_background(Color color);
void begin_drawing(Camera* cam, unsigned int shader);
void end_drawing();
void free_mesh(Mesh* mesh);
Camera create_camera(CameraProjectionType type);
Texture load_texture(const char* path);
void update_camera(Camera* camera, float sens, float move_speed, float dt);
float get_frame_time();
bool is_key_pressed_once(unsigned int scan_code);
Mesh load_mesh_obj(const char* path);
Mesh load_mesh_gltf(const char* filename, cgltf_data* data, unsigned int primitive_index);
Model load_model(const char* file_name);
void free_model(Model* model);
void update_model_animation(Model* model, int anim_index, float dt);
