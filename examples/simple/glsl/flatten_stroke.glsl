#version 450
/////////////// Merge stroke into UV parameter maps
#include "brush.glsl"
#include "canvas.glsl"
#include "utils.glsl"

/////////////// Uniforms
layout(binding = 0) uniform U0 { Canvas canvas; };
#ifdef TOOL_BASE_COLOR_UV
layout(binding = 1) uniform U1{vec4 brushColor; };
#endif

/////////////// Stroke mask
layout(binding = 0) uniform sampler2D texStrokeMask;

/////////////// UV parameter maps
// base color (albedo?)
#ifdef TOOL_BASE_COLOR_UV
layout(binding = 0, rgba8) coherent uniform image2D mapBaseColorXY;
#endif

// HSV offset
#ifdef TOOL_HSV_OFFSET_UV
layout(binding = 0, rgba8) coherent uniform image2D mapHSVOffsetXY;
#endif

// blur parameters
#ifdef TOOL_BLUR_UV
layout(binding = 0, rgba8) coherent uniform image2D mapBlurParametersXY;
#endif

layout(local_size_x = 16, local_size_y = 16) in;

/////////////// CODE
void main() {
  ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);
  vec2 uv = vec2(texelCoords) / canvas.size;
#ifdef TOOL_BASE_COLOR_UV
  // brush: paint base color
  // Blend into base color map
  vec4 D = imageLoad(mapBaseColorXY, texelCoords);
  vec4 baseColor = brushColor;
  baseColor.a *= texture(texStrokeMask, uv).r;
  imageStore(mapBaseColorXY, texelCoords, blend(baseColor, D));
  //imageStore(mapBaseColorXY, texelCoords, vec4(1.0f));
  memoryBarrierImage();
#endif
  // TODO other tools
}
