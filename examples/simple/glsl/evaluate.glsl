#version 450
/////////////// Canvas evaluator
#include "brush.glsl"
#include "canvas.glsl"
#include "utils.glsl"
#include "rgb_hsv.glsl"

/////////////// Uniforms
layout(binding = 0) uniform U0 { Canvas canvas; };
layout(binding = 1) uniform U1 { vec3 lightPos; };
#ifdef PREVIEW_BASE_COLOR_UV
layout(binding = 2) uniform U2 { vec4 brushColor; };
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

/////////////// Canvas normal map & stencil
layout(binding = 6) uniform sampler2D texShadingTermSmooth;
layout(binding = 7) uniform sampler2D texMask;

///////////////
#ifdef PREVIEW_BASE_COLOR_UV
layout(binding = 8) uniform sampler2D texStrokeMask;
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

/////////////////////////////////////////////
void main() {
  ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);
  texelCoords = clamp(texelCoords, ivec2(0,0), ivec2(canvas.size)-ivec2(1,1));
  vec2 uv = vec2(texelCoords) / canvas.size;

  //vec4 D = imageLoad(imgTarget, texelCoords);
  vec4 S = vec4(0.0f);
  //S = blend(S, texture(mapBaseColorXY, uv));

  /////////////////////////////////////////////
  // shading term
  float ldotn = texelFetch(texShadingTermSmooth, texelCoords, 0).r;

  /////////////////////////////////////////////
  // shading curve contrib
  vec3 hsvcurve = texture(mapShadingProfileLN, ldotn).rgb;
  vec3 rgbcurve = hsv2rgb(hsvcurve);

  /////////////////////////////////////////////
  // shading offset
  vec4 hsvoffset = texelFetch(mapHSVOffsetXY, texelCoords, 0);
  vec3 tmp = hsvcurve + (hsvoffset.rgb * 2.0f - 1.0f);
  S = blend(S, vec4(hsv2rgb(tmp), hsvoffset.a));

  /////////////////////////////////////////////
  // blend over preview of current stroke
#ifdef PREVIEW_BASE_COLOR_UV
  vec4 baseColor = brushColor;
  baseColor.a *= texture(texStrokeMask, uv).r;
  S = blend(baseColor, S);
#endif

  /////////////////////////////////////////////
  // done
  imageStore(imgTarget, texelCoords, S);

  // TODO other layers
  memoryBarrierImage();
}

// BLUR EVALUATOR
