#version 450
#include "scene.glsl"
#include "brush.glsl"

layout(std140, binding=0) uniform U0 { Canvas canvas; };
layout(std140, binding=1) uniform U1 { BrushSplat splat; };

#ifdef TEXTURED
layout(binding=0) uniform sampler2D splatTex;
#endif

#ifdef _VERTEX_
layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;
out vec2 fTexcoord;
void main()
{
	vec2 clip_pos = toClipPos(canvas, splat.transform*position);
	gl_Position = vec4(clip_pos.x, -clip_pos.y, 0.0, 1.0);
	fTexcoord = texcoord;
}
#endif

#ifdef _PIXEL_
in vec2 fTexcoord;
out vec4 color;
void main()
{
	ivec2 pos = ivec2(gl_FragCoord.x, gl_FragCoord.y);
	float Da = imageLoad(target, pos).r;
#ifdef TEXTURED
	float Sa = 1.0-texture(splatTex, fTexcoord).r;
#else
	float Sa = brushKernel(vec2(pos.x, pos.y), splat.center, splat.width);
#endif
	// This could be done using the blending unit 
	Da = Sa + Da * (1.0 - Sa);
	imageStore(target, pos, vec4(Da));
}
#endif
