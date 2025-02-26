#version 450
#extension GL_ARB_separate_shader_objects : enable

vec2 position[6] = vec2[]
(
	vec2(-1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, -1.0),

	vec2(1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, 1.0)
);

vec2 texcoord[6] = vec2[]
(
	vec2(0.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 0.0),

	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

layout(location = 0) out vec2 frag_texcoord;

void main()
{
	gl_Position = vec4(position[gl_VertexIndex], 0.0, 1.0);
	frag_texcoord = texcoord[gl_VertexIndex];
}