#version 450
/////////////// Canvas evaluator
#include "brush.glsl"
#include "canvas.glsl"
#include "utils.glsl"

/////////////// Uniforms
layout(binding = 0) uniform U0 { Canvas canvas; };
#ifdef PREVIEW_BASE_COLOR_UV
layout(binding = 1) uniform U1 { vec4 brushColor; };
#endif

/////////////// 1D parameter maps
// shading profile (LdotN => HSV color)
layout(binding = 0) uniform sampler1D mapShadingProfileLN;
// blur parameters
layout(binding = 1) uniform sampler1D mapBlurParametersLN;
// detail texture contrib
layout(binding = 2) uniform sampler1D mapDetailMaskLN;

/////////////// 2d/texcoord parameter maps
// base color (albedo?)
layout(binding = 3) uniform sampler2D mapBaseColorXY;
// HSV offset
layout(binding = 4) uniform sampler2D mapHSVOffsetXY;
// blur parameters
layout(binding = 5) uniform sampler2D mapBlurParametersXY;
// other ideas
// HSV warp map

///////////////
#ifdef PREVIEW_BASE_COLOR_UV
layout(binding = 6) uniform sampler2D texStrokeMask;
#endif

/////////////// Target image (RW because time effects?)
layout(binding = 0, rgba8) writeonly uniform image2D imgTarget;

// CS block layout (arbitrarily chosen)
layout(local_size_x = 16, local_size_y = 16) in;

// PREVIEW_BLUR_UV
// PREVIEW_BASE_COLOR_UV
// PREVIEW_DETAIL_UV

// Blur in shading space, follows L.N
// Blur in UV space
// Color in shading space (toon shading?) -> write to shading profile
// Color in UV space -> write (modify) color offset
// Color in shading space, keep local brush details -> update shading profile,
// then update offset

// SHADER OF DOOM
// ABANDON ALL HOPE
void main() {
  ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);
  texelCoords = clamp(texelCoords, ivec2(0,0), ivec2(canvas.size)-ivec2(1,1));
  vec2 uv = vec2(texelCoords) / canvas.size;

  //vec4 D = imageLoad(imgTarget, texelCoords);
  vec4 S = vec4(0.0f);
  S = blend(S, texture(mapBaseColorXY, uv));

// blend over preview of current stroke
#ifdef PREVIEW_BASE_COLOR_UV
  vec4 baseColor = brushColor;
  baseColor.a *= texture(texStrokeMask, uv).r;
  S = blend(baseColor, S);
#endif

  imageStore(imgTarget, texelCoords, S);

  // TODO other layers
  memoryBarrierImage();
}

// BLUR EVALUATOR
