#version 460

layout (location = 0) in vec2 inUV;

layout(location = 0) out vec4 outFragColor;


layout(set = 1, binding = 0) uniform sampler2D samplerTexture;

layout(push_constant) uniform Color
{
	layout(offset = 80) vec4 color;
} color;


void main()
{
	outFragColor = texture(samplerTexture, inUV) * color.color;
}
