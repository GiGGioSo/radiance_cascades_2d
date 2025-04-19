#version 430 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D map_texture;

void main()
{
    vec3 tex = texture(map_texture, TexCoord).rgb;
    vec3 correction = vec3(1.0, 1.0, 1.0) * 1.0;
	FragColor = vec4(tex * correction, 1.f);
}
