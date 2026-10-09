#version 330 core

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec2 v_uvs;
layout (location = 2) in vec3 v_norm;
layout (location = 3) in ivec4 v_bone_ids; 
layout (location = 4) in vec4 v_weights;

out vec2 uv;
out vec3 frag_pos;
out vec3 normal;
out vec4 frag_pos_light_space;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 light_space_matrix;

const int MAX_BONES= 128; 
uniform mat4 u_bone_matrices[MAX_BONES];

mat4 calc_skinning_mat(){
	vec4 weights = v_weights;
    float length = weights.x + weights.y + weights.z + weights.w;
    if(length != 1){
    	weights.x /= length;
    	weights.y /= length;
    	weights.z /= length;
    	weights.w /= length;
    }
    mat4 skin_matrix = 
        u_bone_matrices[v_bone_ids.x] * weights.x +
        u_bone_matrices[v_bone_ids.y] * weights.y +
        u_bone_matrices[v_bone_ids.z] * weights.z +
        u_bone_matrices[v_bone_ids.w] * weights.w;

    return skin_matrix;
}
void main(){
	mat4 skinned_mat = calc_skinning_mat();
	vec4 skinned_pos = skinned_mat * vec4(v_pos, 1.0);
    gl_Position = projection * view * model * skinned_pos;

    normal = mat3(transpose(inverse(model * skinned_mat))) * v_norm; 
    uv = v_uvs;
    frag_pos = vec3(model * skinned_pos);
    frag_pos_light_space = light_space_matrix * vec4(frag_pos, 1.0);
}
