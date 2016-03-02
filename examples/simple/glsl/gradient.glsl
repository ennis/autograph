// Compute the gradient of the input texture
#version 450
#include "canvas.glsl"
#include "rgb_hsv.glsl"
#include "utils.glsl"

layout(std140, binding = 0) uniform U0 { vec2 size; };

layout(local_size_x = 16, local_size_y = 16) in;
layout(binding = 0) uniform sampler2D texIn;
layout(binding = 0, rgba32f) writeonly uniform image2D imgOut; 

void main()
{
  ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);
  vec2 uv = vec2(texelCoords)/size;

  float x = 1.0f/size.x;
  float y = 1.0f/size.y;

  float gx = 0.0;
  gx -= texture(texIn, vec2( uv.x - x, uv.y - y ) ).r * 1.0;
  gx -= texture(texIn, vec2( uv.x - x, uv.y     ) ).r * 2.0;
  gx -= texture(texIn, vec2( uv.x - x, uv.y + y ) ).r * 1.0;
  gx += texture(texIn, vec2( uv.x + x, uv.y - y ) ).r * 1.0;
  gx += texture(texIn, vec2( uv.x + x, uv.y     ) ).r * 2.0;
  gx += texture(texIn, vec2( uv.x + x, uv.y + y ) ).r * 1.0;

  float gy = 0.0;
  gy -= texture(texIn, vec2( uv.x - x, uv.y - y ) ).r * 1.0;
  gy -= texture(texIn, vec2( uv.x    , uv.y - y ) ).r * 2.0;
  gy -= texture(texIn, vec2( uv.x + x, uv.y - y ) ).r * 1.0;
  gy += texture(texIn, vec2( uv.x - x, uv.y + y ) ).r * 1.0;
  gy += texture(texIn, vec2( uv.x    , uv.y + y ) ).r * 2.0;
  gy += texture(texIn, vec2( uv.x + x, uv.y + y ) ).r * 1.0;

  imageStore(imgOut, texelCoords, vec4(gx, gy, 0.0f, 1.0f));
}
