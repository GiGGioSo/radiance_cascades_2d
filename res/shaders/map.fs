#version 430 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D map_texture;

void main()
{
	FragColor = vec4(texture(map_texture, TexCoord).rgb, 1.f);
}
