#version 450
#include "brush.glsl"
#include "canvas.glsl"

layout(std140, binding = 1) uniform U1 { BrushSplat splat; };
layout(std140, binding = 0) uniform U0 { Canvas canvas; };

#ifdef TEXTURED
layout(binding = 0) uniform sampler2D texBrushTip;
#endif

/////////////// VS
#ifdef _VERTEX_
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texcoord;
out vec2 fTexcoord;
void main() {
  vec2 clip_pos = toClipPos(canvas, (splat.transform * vec3(position, 1.0f)).xy);
  // flip y since we are rendering to a texture
  gl_Position = vec4(clip_pos.x, -clip_pos.y, 0.0f, 1.0f);
  fTexcoord = texcoord;
}
#endif

/////////////// PS
#ifdef _PIXEL_
in vec2 fTexcoord;
layout(location = 0) out vec4 color;
in vec4 gl_FragCoord;
void main() {
  vec2 pos = gl_FragCoord.xy;
#ifdef TEXTURED
  float Sa = 1.0 - texture(texBrushTip, fTexcoord).r;
#else
  float Sa = roundBrushKernel(pos, splat.center, splat.width);
#endif
  color = vec4(Sa);
}
#endif
