#version 330 core

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec3 v_norm;

out vec2 uv;
out vec3 frag_pos;
out vec3 normal;
out vec4 frag_pos_light_space;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 light_space_matrix;

void main(){
    uv = v_uv;
    frag_pos = vec3(model * vec4(v_pos, 1));
    normal = mat3(transpose(inverse(model))) * v_norm;
    frag_pos_light_space = light_space_matrix * vec4(frag_pos, 1.0);

    mat4 mvp = projection * view * model;
    gl_Position = mvp * vec4(v_pos, 1.0f);
}
