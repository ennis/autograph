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

#ifdef EVAL_BLUR
layout(binding = 1, rgba8) readonly uniform image2D imgSource;
#endif

// CS block layout (arbitrarily chosen)
layout(local_size_x = 16, local_size_y = 16) in;


#ifdef EVAL_MAIN
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
  vec3 tmp = hsvcurve /* + (hsvoffset.rgb * 2.0f - 1.0f)*/;
  S = blend(S, vec4(hsv2rgb(tmp), 1.0));

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
#endif  // EVAL_MAIN


/////////////////////////////////////////////
#ifdef EVAL_BLUR
// blur kernel
float G(float x2y2, float sigma2)
{
  // degenerate case
  if (sigma2 < 0.01f) return x2y2 < 0.01f ? 1.0f : 0.0f;
  return 1.0f/(sigma2*TWOPI)*exp(-0.5*x2y2/(2.0f*sigma2));
}

void main()
{
  // 
  ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);
  vec2 uv = vec2(texelCoords) / canvas.size;

  /////////////////////////////////////////////
  // shading term
  float ldotn = texelFetch(texShadingTermSmooth, texelCoords, 0).r;

  /////////////////////////////////////////////
  // fetch blur sigma
  float sigma = texture(mapBlurParametersLN, ldotn).r;
  int windowSize = max(3*int(ceil(sigma)),1);

  /////////////////////////////////////////////
  //
  vec4 S = vec4(0.0f);
  vec2 center = vec2(texelCoords);
  for (int y = -windowSize; y < windowSize; ++y) {
    for (int x = -windowSize; x < windowSize; ++x) {
      vec2 p = vec2(texelCoords + ivec2(x, y));
      vec2 d = center - p;
	  S += G(dot(d,d), sq(sigma)) * imageLoad(imgSource, texelCoords);
    }
  }
  
  imageStore(imgTarget, texelCoords, S);
}
#endif
