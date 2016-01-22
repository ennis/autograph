#version 440
#include "scene.glsl"

layout(binding=0) uniform sampler2D tex;
#ifdef USE_MASK
layout(binding=1) uniform sampler2D mask;
#endif
layout(binding=0,std140) uniform U0 { vec2 viewportSize; };

#ifdef _VERTEX_
layout(location=0) in vec2 pos;
layout(location=1) in vec2 texcoord;
out vec2 fTexcoord;
void main()
{
	gl_Position = vec4(pos, 0.0, 1.0);	
	fTexcoord = texcoord;
}
#endif

#ifdef _PIXEL_
in vec2 fTexcoord;
out vec4 color;
void main()
{
#ifdef USE_MASK
	vec2 screen_tex = vec2(gl_FragCoord.x, gl_FragCoord.y)/viewportSize;
	float m = 1.0-texture(mask, screen_tex).a;
#else 
	float m = 1.0f;
#endif
	color = texture(tex, fTexcoord) * m;
}
#endif
