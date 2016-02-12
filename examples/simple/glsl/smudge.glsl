#version 450
#include "canvas.glsl"
#include "utils.glsl"

layout(std140, binding = 0) uniform U0 { Canvas canvas; };
layout(std140, binding = 1) uniform U1 { uvec2 origin; uvec2 size; float opacity; };

layout(binding=0, rgba8) coherent uniform image2D imgBaseColorUV;
layout(binding=1, rgba8) coherent uniform image2D imgSmudgeFootprint;
layout(binding=0) uniform sampler2D texBrushTip; 

layout(local_size_x = 16, local_size_y = 16) in;

void main()
{
	ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);
	ivec2 canvasCoords = ivec2(origin)+texelCoords;
	vec4 S = imageLoad(imgSmudgeFootprint, texelCoords);
	vec4 D = imageLoad(imgBaseColorUV, canvasCoords);
	S.a *= opacity;
	vec4 Out = blend(S,D);
	imageStore(imgBaseColorUV, canvasCoords, Out);
	float tipOpacity = texture(texBrushTip, vec2(texelCoords)/vec2(size)).r;
	Out.a *= tipOpacity;
	imageStore(imgSmudgeFootprint, texelCoords, Out);
}
