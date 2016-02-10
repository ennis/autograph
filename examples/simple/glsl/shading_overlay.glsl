#version 450 
#include "canvas.glsl"
#include "fullscreen_vs.glsl"
#include "utils.glsl"

layout (binding = 0, std140) uniform U0 { Canvas canvas; };
layout (binding = 1, std140) uniform U1 { vec3 lightPos; };

layout(binding=0) uniform sampler2D texNormals;

#ifdef _PIXEL_
in vec2 tex;
layout(location = 0) out vec4 color;
void main() {
	ivec2 texelCoords = ivec2(gl_FragCoord.xy);
	texelCoords.y = int(canvas.size.y) - texelCoords.y;
  color = vec4(vec3(shadingTerm(texNormals, texelCoords, lightPos)), 0.5f); 
}

#endif
