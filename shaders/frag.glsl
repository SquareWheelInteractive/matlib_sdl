#version 330 core

out vec4 frag_color;

in vec2 uvs;

uniform sampler2D texture_01;
uniform vec4 ambient;

void main(){
	vec4 tex_color = texture(texture_01, uvs);
	if(tex_color.a < 0.1f)
		discard;
	frag_color = tex_color * ambient;
}
