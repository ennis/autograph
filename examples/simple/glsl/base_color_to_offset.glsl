#version 450
// convert base color values to shading offsets
#include "brush.glsl"
#include "canvas.glsl"
#include "utils.glsl"
#include "rgb_hsv.glsl"

layout(binding = 0) uniform U0 { Canvas canvas; };
layout(binding = 1) uniform U1 { vec3 lightPos; };


layout(binding = 0) uniform sampler1D mapShadingProfileLN;
layout(binding = 1) uniform sampler2D mapBaseColorUV;
layout(binding = 2) uniform sampler2D texShadingTermSmooth;
layout(binding = 3) uniform sampler2D texMask;

layout(binding = 0, rgba8) writeonly uniform image2D mapHSVOffsetUV;


layout(local_size_x = 1, local_size_y = 1) in;

void main() {
  ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);
  float ldotn = texelFetch(texShadingTermSmooth, texelCoords, 0).r;
  vec3 hsvcurve = texture(mapShadingProfileLN, ldotn).rgb;
  vec4 ref = texelFetch(mapBaseColorUV, texelCoords, 0);
  vec3 offset = rgb2hsv(ref.rgb) - hsvcurve;
  imageStore(mapHSVOffsetUV, texelCoords, vec4(offset / 2.0f + vec3(0.5f), ref.a)); 
}
