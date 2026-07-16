#version 330

layout (location = 0) in vec3 v_pos;

out vec3 tex_coords;

uniform mat4 proj;
uniform mat4 view;

void main(){
    vec4 pos = proj * view * vec4(v_pos, 1);
    gl_Position = vec4(pos.xyww);
    tex_coords = vec3(v_pos.xy, -v_pos.z);
}
