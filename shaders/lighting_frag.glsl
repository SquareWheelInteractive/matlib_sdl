#version 330

out vec4 frag_color;

in vec2 uv;
in vec3 frag_pos;
in vec3 normal;

#define MAX_LIGHTS 4
#define POINT_LIGHT_TYPE 0
#define SPOT_LIGHT_TYPE 1
#define DIRECTIONAL_LIGHT_TYPE 2

struct Light {
    int enabled;
    int type;
    vec3 direction;
    vec3 position;
    vec4 color;
    float intensity;
    float radius;
};

uniform sampler2D tex;

uniform Light lights[MAX_LIGHTS];

uniform vec4 ambient;

uniform vec3 view_pos;

float fog_density = 0.08;
vec4 fog_color = vec4(0.7, 0.6, 0.66, 1.0);

float exposure = 1.6;

vec3 reinhard_luminance(vec3 color) {
    float l = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float l_mapped = l / (1.0 + l);
    return color * (l_mapped / max(l, 0.0001));
}

void main() {
    vec3 norm = normalize(normal);
    vec4 texel_color = texture(tex, uv);
    vec4 diffuse = vec4(0);

    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (lights[i].enabled == 1) {
            vec3 dir_to_light = normalize(lights[i].position - frag_pos);

            float dot_nl = max(dot(norm, dir_to_light), 0.0);

            float distance = length(lights[i].position - frag_pos);
            float attenuation = (lights[i].radius - distance) / (lights[i].radius + distance);
            attenuation = clamp(attenuation, 0, 1);

            if(lights[i].type == POINT_LIGHT_TYPE){
                diffuse += lights[i].color* dot_nl * attenuation * lights[i].intensity;
            }
            if(lights[i].type == SPOT_LIGHT_TYPE){
                float cutoff = 0.9;
                float spot_factor = dot(dir_to_light, -normalize(lights[i].direction));
                if( spot_factor > cutoff){
                    float smoothness = smoothstep(0,1, ( 1.0 - (1.0 - spot_factor) / (1.0 - cutoff)));
                    diffuse += (lights[i].color * dot_nl * attenuation) * smoothness * lights[i].intensity;
                }
            }
            if(lights[i].type == DIRECTIONAL_LIGHT_TYPE){
                vec3 sun_dir = normalize(-lights[i].direction);
                float dot_ns = dot(sun_dir, norm);
                float factor = max(dot_ns, 0.0);
                diffuse += lights[i].color * factor * lights[i].intensity;
            }
        }
    }

    vec3 phong_color = texel_color.rgb * (ambient + diffuse).rgb;
    phong_color *= exposure;

    vec3 tone_mapped = reinhard_luminance(phong_color);

    float dist = length(view_pos - frag_pos);
    float fog_factor= 1.0/exp((dist*fog_density)*(dist*fog_density));
    fog_factor = clamp(fog_factor, 0.0, 1.0);

    tone_mapped = mix(fog_color.rgb, tone_mapped, fog_factor);


    frag_color = vec4(tone_mapped, texel_color.a);
}
