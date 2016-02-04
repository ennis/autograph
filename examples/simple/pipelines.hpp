#ifndef PIPELINES_HPP
#define PIPELINES_HPP

#include <filesystem/path.h>
#include "types.hpp"
#include "../common/sample.hpp"
#include "../common/uniforms.hpp"

namespace uniforms 
{
  struct Scene 
  {
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    glm::mat4 viewProjMatrix;
    glm::vec2 viewportSize;
  };

  struct CanvasData
  {
      glm::vec2 size;
  };

  struct Splat 
  {
      // splat transform: contains scale and center position
    glm::mat2x3 transform;
    glm::vec2 center;
  };
}

struct Pipelines {
  Pipelines(Device& device, const filesystem::path& samplesRoot) {
    // TODO: load
  }

  // Render the normal map and the shading of a model
  // [normal_map.glsl]
  GraphicsPipeline ppRenderNormalMap;

  // Compute the average shading curve
  // [shading_curve.glsl]
  ComputePipeline ppComputeShadingCurve;

  // Compute the lit-sphere
  ComputePipeline ppComputeLitSphere;

  // Draw stroke mask
  // [draw_stroke_mask.glsl]
  GraphicsPipeline ppDrawRoundSplatToStrokeMask;
  GraphicsPipeline ppDrawTexturedSplatToStrokeMask;

  // Compose stroke mask onto target
  ComputePipeline ppComposeStroke;

  // Evaluate final image
  GraphicsPipeline ppEvalutate;

  // Copy a texture with a mask
  GraphicsPipeline ppCopyTexWithMask;
};

#endif
