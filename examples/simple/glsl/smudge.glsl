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
	float tipOpacity = 1.0f-texture(texBrushTip, vec2(texelCoords)/vec2(size)).r;
	vec4 S2 = S; S2.a *= tipOpacity*opacity;
	vec4 Out = blend(S2,D);
	Out.a = mix(Out.a, S.a, tipOpacity*opacity);
	imageStore(imgBaseColorUV, canvasCoords, Out);
	imageStore(imgSmudgeFootprint, texelCoords, Out);
}
