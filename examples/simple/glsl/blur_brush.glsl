#version 450
// convert base color values to shading offsets
#include "brush.glsl"
#include "canvas.glsl"
#include "utils.glsl"
#include "rgb_hsv.glsl"

layout(binding = 0) uniform U0 {
  vec2 center;
  float width;
};

layout(binding = 0) uniform sampler2D texShadingTermSmooth;
layout(binding = 0, rgba8) writeonly uniform image1D mapBlurParametersLN;

layout(local_size_x = 16) in;

void main() {
  ivec2 texelCoords = ivec2(center);
  int texelCoords1D = int(gl_GlobalInvocationID.x);
  float ldotn = texelFetch(texShadingTermSmooth, texelCoords, 0).r;
  int bin = int(ldotn * 255.0f);
  imageStore(mapBlurParametersLN, texelCoords1D,
                 (1.0f - smoothstep(0, width / 2.0f,
                                    abs(float(bin - texelCoords1D)))).xxxx);
}
