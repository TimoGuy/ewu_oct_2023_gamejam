#version 460

layout (location = 0) in vec2 inUV;

layout(location = 0) out vec4 outFragColor;


layout(set = 1, binding = 0) uniform sampler2D samplerTexture;

layout(push_constant) uniform UIQuadSettings
{
	layout(offset = 80) vec4 tint;
	layout(offset = 96) float useNineSlicing;
	layout(offset = 100) float nineSlicingBoundX1;
	layout(offset = 104) float nineSlicingBoundX2;
	layout(offset = 108) float nineSlicingBoundY1;
	layout(offset = 112) float nineSlicingBoundY2;
} uqs;

#define ONE_THIRD 0.333333333333333

void main()
{
	if (uqs.useNineSlicing > 0.0)
	{
		vec2 transUV;
		// X
		if (inUV.x < uqs.nineSlicingBoundX1)
			transUV.x = inUV.x / uqs.nineSlicingBoundX1 * ONE_THIRD;
		else if (inUV.x < uqs.nineSlicingBoundX2)
			transUV.x = mod(inUV.x, uqs.nineSlicingBoundX1) / uqs.nineSlicingBoundX1 * ONE_THIRD + ONE_THIRD;
		else
			transUV.x = (inUV.x - uqs.nineSlicingBoundX2) / uqs.nineSlicingBoundX1 * ONE_THIRD + ONE_THIRD * 2.0;

		// Y
		if (inUV.y < uqs.nineSlicingBoundY1)
			transUV.y = inUV.y / uqs.nineSlicingBoundY1 * ONE_THIRD;
		else if (inUV.y < uqs.nineSlicingBoundY2)
			transUV.y = mod(inUV.y, uqs.nineSlicingBoundY1) / uqs.nineSlicingBoundY1 * ONE_THIRD + ONE_THIRD;
		else
			transUV.y = (inUV.y - uqs.nineSlicingBoundY2) / uqs.nineSlicingBoundY1 * ONE_THIRD + ONE_THIRD * 2.0;

		// @DEBUG: look at uv coords.
		// outFragColor = vec4(transUV.x, transUV.y, 0.0, 1.0);
		outFragColor = texture(samplerTexture, transUV) * uqs.tint;
	}
	else
		outFragColor = texture(samplerTexture, inUV) * uqs.tint;
}
