#version 330

out vec4 frag_color;

in vec3 tex_coords;

uniform samplerCube skybox;

float exposure = 2;

vec3 reinhard_luminance(vec3 color) {
    float l = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float l_mapped = l / (1.0 + l);
    return color * (l_mapped / max(l, 0.0001));
}
void main(){
    vec3 texel_color = texture(skybox, tex_coords).rgb;
    texel_color *= exposure;

    vec3 mapped = reinhard_luminance(texel_color);
    frag_color = vec4(mapped, 1.0);
}
