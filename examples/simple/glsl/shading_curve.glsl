// Compute the average shading curves
#version 450
#include "canvas.glsl"
#include "rgb_hsv.glsl"

layout(std140, binding = 0) uniform U0 { Canvas canvas; };
layout(std140, binding = 1) uniform U1 { vec3 lightPos; };

#define CURVE_SAMPLES 256

layout(local_size_x = 16, local_size_y = 16) in;
layout(binding = 0) uniform sampler2D texNormals;
layout(binding = 1) uniform sampler2D texShadingTerm;	// same size as the canvas
layout(binding = 2) uniform sampler2D texMask;	// same size as the canvas
layout(binding = 3) uniform sampler2D texBaseColor;

layout(binding = 0, r32ui) coherent uniform uimage1D imgCurveH;
layout(binding = 1, r32ui) coherent uniform uimage1D imgCurveS;
layout(binding = 2, r32ui) coherent uniform uimage1D imgCurveV;
layout(binding = 3, r32ui) coherent uniform uimage1D imgCurveAccum;

void main()
{
  ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);
  vec4 C = texelFetch(texBaseColor, texelCoords, 0);
  float S = texelFetch(texShadingTerm, texelCoords, 0).x;
  int bin = clamp(int(floor(S * CURVE_SAMPLES)), 0, CURVE_SAMPLES);

  vec3 hsv = rgb2hsv(C.xyz);
  imageAtomicAdd(imgCurveH, bin, uint(hsv.x));
  imageAtomicAdd(imgCurveS, bin, uint(hsv.y));
  imageAtomicAdd(imgCurveV, bin, uint(hsv.z));
  imageAtomicAdd(imgCurveAccum, bin, 1);

  memoryBarrierImage();
}
