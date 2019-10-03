#version 450

layout(binding = 1) uniform sampler2D teximage;
layout(location = 0) in vec2 frag_texcoord;

layout(location = 0) out vec4 output_color;

void main()
{
	output_color = texture(teximage, frag_texcoord);
}
