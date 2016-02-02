#ifndef PIPELINES_HPP
#define PIPELINES_HPP

#include <filesystem/path.h>
#include "types.hpp"

struct Pipelines {
  Pipelines(const filesystem::path& samplesRoot) {

  }

  // Render the normal map of a model
  // [normal_map.glsl]
  GraphicsPipeline ppRenderNormalMap;

  // Compute the average shading curve
  // [shading_curve.glsl]
  ComputePipeline ppComputeShadingCurve;

  // Compute the lit-sphere
  ComputePipeline ppComputeLitSphere;

  // Draw stroke mask
  GraphicsPipeline ppDrawStrokeMask;

  // Compose stroke mask onto target
  ComputePipeline ppComposeStroke;

  // Evaluate final image
  GraphicsPipeline ppEvalutate;

  // Copy a texture with a mask
  GraphicsPipeline ppCopyTexWithMask;
};

#endif