// Compute the average shading curve
#version 440
#include "scene.glsl"
#include "rgb_hsv.glsl"

layout(std140, binding = 0) uniform U0 { SceneData sceneData; };
layout(std140, binding = 1) uniform U1 { vec3 lightPos; };

#define CURVE_SAMPLES 256
layout(local_size_x = 16, local_size_y = 16) in;
layout(binding = 0) uniform sampler2D texNormals;
layout(binding = 1) uniform sampler2D texShadingTerm;	// same size as the canvas
layout(binding = 0, rgba8) readonly uniform image2D imgColor;
layout(binding = 1, r32ui) coherent uniform image2D imgCurveH;
layout(binding = 2, r32ui) coherent uniform image2D imgCurveS;
layout(binding = 3, r32ui) coherent uniform image2D imgCurveV;
layout(binding = 4, r32ui) coherent uniform image2D imgCurveAccum;

void main()
{
  ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);
  vec4 C = imageLoad(imgColor, texelCoords);
  float S = texelFetch(texShadingTerm, texelCoords).x;
  int bin = clamp(floor(S * CURVE_SAMPLES), 0.0f, CURVE_SAMPLES);

  vec3 hsv = rgb2hsv(C.xyz);
  //imageAtomicAdd(imgCurveH, )
  // TODO
}
