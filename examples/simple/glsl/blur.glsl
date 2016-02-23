// Adapted from:
// http://callumhay.blogspot.com/2010/09/gaussian-blur-shader-glsl.html
#version 450
#include "utils.glsl"

layout(binding=0, rgba8) readonly uniform image2D tex0;
layout(binding=1, rgba8) writeonly uniform image2D tex1;

layout(binding=0, std140) uniform U0 { vec2 size; int blurSize; float sigma; };

layout(local_size_x = 16, local_size_y = 16) in;

void main() 
{  
  ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);
  float numBlurPixelsPerSide = float(blurSize / 2); 
  
#ifdef BLUR_H
  ivec2 blurMultiplyVec = ivec2(1, 0);
#else
  ivec2 blurMultiplyVec = ivec2(0, 1);
#endif

  // Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
  vec3 incrementalGaussian;
  incrementalGaussian.x = 1.0 / (sqrt(TWOPI) * sigma);
  incrementalGaussian.y = exp(-0.5 / (sigma * sigma));
  incrementalGaussian.z = incrementalGaussian.y * incrementalGaussian.y;

  vec4 avgValue = vec4(0.0, 0.0, 0.0, 0.0);
  float coefficientSum = 0.0;

  // Take the central sample first...
  avgValue += imageLoad(tex0, texelCoords) * incrementalGaussian.x;
  coefficientSum += incrementalGaussian.x;
  incrementalGaussian.xy *= incrementalGaussian.yz;

  // Go through the remaining 8 vertical samples (4 on each side of the center)
  for (int i = 1; i <= numBlurPixelsPerSide; i++) { 
    avgValue += imageLoad(tex0, texelCoords - i * blurMultiplyVec) * incrementalGaussian.x;         
    avgValue += imageLoad(tex0, texelCoords + i * blurMultiplyVec) * incrementalGaussian.x;         
    coefficientSum += 2.0 * incrementalGaussian.x;
    incrementalGaussian.xy *= incrementalGaussian.yz;
  }

  imageStore(tex1, texelCoords, avgValue / coefficientSum);
}
