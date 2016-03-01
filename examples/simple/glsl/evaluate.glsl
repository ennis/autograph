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

#ifdef EVAL_DETAIL
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
  if (sigma2 < 0.3f) return x2y2 < 0.3f ? 1.0f : 0.0f;
  return 1.0f/(sigma2*TWOPI)*exp(-x2y2/(2.0f*sigma2));
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
  float sigma = texture(mapBlurParametersLN, ldotn).r*10.0f;
  int windowSize = int(ceil(3.0f*sigma));

  /////////////////////////////////////////////
  //
  vec4 S = vec4(0.0f);
  vec2 center = vec2(texelCoords);
  for (int y = -windowSize; y <= windowSize; ++y) {
    for (int x = -windowSize; x <= windowSize; ++x) {
      ivec2 p = texelCoords + ivec2(x, y);
      vec2 d = center - vec2(p);
      //int t = max(2*windowSize+1,1);
	  S += G(dot(d,d), sq(sigma)) * imageLoad(imgSource, p);
    }
  }
 // S.a = 1.0f;
  
  imageStore(imgTarget, texelCoords, S);
}
#endif


/////////////////////////////////////////////
#ifdef EVAL_DETAIL
// noise function
float nrand(vec2 n) {
  return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

void main()
{
  ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);
  vec2 uv = vec2(texelCoords) / canvas.size;

  /////////////////////////////////////////////
  // shading term
  float ldotn = texelFetch(texShadingTermSmooth, texelCoords, 0).r;

  /////////////////////////////////////////////
  // fetch detail term
  float detail = texture(mapDetailMaskLN, ldotn).r;
  vec4 S = imageLoad(imgSource, texelCoords);
  float offset = detail * nrand(uv) * 0.1f;

  /////////////////////////////////////////////
  // offset to shading curve
  // TODO: try offsetting spatially along the shading gradient
  vec3 hsvcurve = texture(mapShadingProfileLN, ldotn + offset).rgb;
  vec3 rgbcurve = hsv2rgb(hsvcurve);
  vec4 D = blend(vec4(rgbcurve, detail), S);
  imageStore(imgTarget, texelCoords, D);
}

#endif
