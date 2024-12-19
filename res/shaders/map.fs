#version 430 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D map_texture;

void main()
{
	FragColor = texture(map_texture, TexCoord);
}
