#ifndef PIPELINES_HPP
#define PIPELINES_HPP

#include "../common/sample.hpp"
#include "../common/uniforms.hpp"
#include "types.hpp"
#include <filesystem/path.h>

namespace uniforms {
struct Scene {
  glm::mat4 viewMatrix;
  glm::mat4 projMatrix;
  glm::mat4 viewProjMatrix;
  glm::vec2 viewportSize;
};

struct CanvasData {
  glm::vec2 size;
};

struct Splat {
  // splat transform: contains scale and center position
  glm::mat3x4 transform;
  glm::vec2 center;
  float width;
};
}

struct Pipelines {

  Pipelines(Device &device, const filesystem::path &samplesRoot) {
    using namespace ag::opengl;
    using namespace shaderpp;

    ShaderSource normal_map =
        loadShaderSource(samplesRoot / "simple/glsl/normal_map.glsl");
    ShaderSource draw_stroke_mask =
        loadShaderSource(samplesRoot / "simple/glsl/draw_stroke_mask.glsl");
    ShaderSource flatten_stroke =
        loadShaderSource(samplesRoot / "simple/glsl/flatten_stroke.glsl");
    ShaderSource evaluate =
        loadShaderSource(samplesRoot / "simple/glsl/evaluate.glsl");
    ShaderSource curves =
        loadShaderSource(samplesRoot / "simple/glsl/shading_curve.glsl");
    ShaderSource shading_overlay =
        loadShaderSource(samplesRoot / "simple/glsl/shading_overlay.glsl");
    ShaderSource smudge =
        loadShaderSource(samplesRoot / "simple/glsl/smudge.glsl");
    ShaderSource base_color_to_offset =
        loadShaderSource(samplesRoot / "simple/glsl/base_color_to_offset.glsl");
    ShaderSource blur = loadShaderSource(samplesRoot / "simple/glsl/blur.glsl");
    ShaderSource blur_brush = loadShaderSource(samplesRoot / "simple/glsl/blur_brush.glsl");

    {
      GraphicsPipelineInfo g;
      g.blendState.enabled = true;
      g.blendState.modeAlpha = gl::FUNC_ADD;
      g.blendState.modeRGB = gl::FUNC_ADD;
      g.blendState.funcSrcAlpha = gl::ONE;
      g.blendState.funcDstAlpha = gl::ONE_MINUS_SRC_ALPHA;
      g.blendState.funcSrcRGB = gl::ONE;
      g.blendState.funcDstRGB = gl::ONE_MINUS_SRC_ALPHA;
      g.vertexAttribs = gsl::as_span(samples::vertexAttribs_2D);
      auto VSSource =
          draw_stroke_mask.preprocess(PipelineStage::Vertex, nullptr, nullptr);
      auto PSSource =
          draw_stroke_mask.preprocess(PipelineStage::Pixel, nullptr, nullptr);
      g.VSSource = VSSource.c_str();
      g.PSSource = PSSource.c_str();
      ppDrawRoundSplatToStrokeMask = device.createGraphicsPipeline(g);

      const char *defines[] = {"TEXTURED"};
      VSSource =
          draw_stroke_mask.preprocess(PipelineStage::Vertex, defines, nullptr);
      PSSource =
          draw_stroke_mask.preprocess(PipelineStage::Pixel, defines, nullptr);
      g.VSSource = VSSource.c_str();
      g.PSSource = PSSource.c_str();
      ppDrawTexturedSplatToStrokeMask = device.createGraphicsPipeline(g);
    }

    {
      GraphicsPipelineInfo g;
      g.depthStencilState.depthTestEnable = true;
      g.blendState.enabled = false;
      g.vertexAttribs = gsl::as_span(samples::kMeshVertexDesc);
      auto VSSource =
          normal_map.preprocess(PipelineStage::Vertex, nullptr, nullptr);
      auto PSSource =
          normal_map.preprocess(PipelineStage::Pixel, nullptr, nullptr);
      g.VSSource = VSSource.c_str();
      g.PSSource = PSSource.c_str();
      ppRenderGbuffers = device.createGraphicsPipeline(g);
    }

    {
      GraphicsPipelineInfo g;
      g.depthStencilState.depthTestEnable = false;
      g.blendState.enabled = true;
      g.blendState.modeAlpha = gl::FUNC_ADD;
      g.blendState.modeRGB = gl::FUNC_ADD;
      g.blendState.funcSrcAlpha = gl::SRC_ALPHA;
      g.blendState.funcDstAlpha = gl::ONE_MINUS_SRC_ALPHA;
      g.blendState.funcSrcRGB = gl::SRC_ALPHA;
      g.blendState.funcDstRGB = gl::ONE_MINUS_SRC_ALPHA;
      auto VSSource =
          shading_overlay.preprocess(PipelineStage::Vertex, nullptr, nullptr);
      auto PSSource =
          shading_overlay.preprocess(PipelineStage::Pixel, nullptr, nullptr);
      g.VSSource = VSSource.c_str();
      g.PSSource = PSSource.c_str();
      ppShadingOverlay = device.createGraphicsPipeline(g);
    }

    {
      ComputePipelineInfo c;
      const char *defines[] = {"TOOL_BASE_COLOR_UV"};
      auto CSSource =
          flatten_stroke.preprocess(PipelineStage::Compute, defines, nullptr);
      c.CSSource = CSSource.c_str();
      ppFlattenStroke = device.createComputePipeline(c);
    }

    {
      ComputePipelineInfo c;
      const char *defines[] = {"TOOL_BASE_COLOR_UV"};
      auto CSSource =
          smudge.preprocess(PipelineStage::Compute, defines, nullptr);
      c.CSSource = CSSource.c_str();
      ppSmudge = device.createComputePipeline(c);
    }

    {
      ComputePipelineInfo c;
      const char *defines_main[] = {"EVAL_MAIN"};
      auto CSSource =
          evaluate.preprocess(PipelineStage::Compute, defines_main, nullptr);
      c.CSSource = CSSource.c_str();
      ppEvaluate = device.createComputePipeline(c);

      const char *defines_main_preview_base_color[] = {"PREVIEW_BASE_COLOR_UV",
                                                       "EVAL_MAIN"};
      CSSource = evaluate.preprocess(PipelineStage::Compute,
                                     defines_main_preview_base_color, nullptr);
      c.CSSource = CSSource.c_str();
      ppEvaluatePreviewBaseColorUV = device.createComputePipeline(c);

      const char *defines_blur[] = {"EVAL_BLUR"};
      CSSource =
          evaluate.preprocess(PipelineStage::Compute, defines_blur, nullptr);
      c.CSSource = CSSource.c_str();
      ppEvaluateBlurPass = device.createComputePipeline(c);
    }

    {
      ComputePipelineInfo c;
      auto CSSource =
          curves.preprocess(PipelineStage::Compute, nullptr, nullptr);
      c.CSSource = CSSource.c_str();
      ppComputeShadingCurveHSV = device.createComputePipeline(c);
    }

    {
      ComputePipelineInfo c;
      auto CSSource = base_color_to_offset.preprocess(PipelineStage::Compute,
                                                      nullptr, nullptr);
      c.CSSource = CSSource.c_str();
      ppBaseColorToOffset = device.createComputePipeline(c);
    }

    {
      ComputePipelineInfo c;
      auto CSSource = blur_brush.preprocess(PipelineStage::Compute,
                                                      nullptr, nullptr);
      c.CSSource = CSSource.c_str();
      ppBlurBrush = device.createComputePipeline(c);
    }

    {
      ComputePipelineInfo c;

      const char *defines_h[] = {"BLUR_H"};
      auto CSSource =
          blur.preprocess(PipelineStage::Compute, defines_h, nullptr);
      c.CSSource = CSSource.c_str();
      ppBlurH = device.createComputePipeline(c);

      const char *defines_v[] = {"BLUR_V"};
      CSSource = blur.preprocess(PipelineStage::Compute, defines_v, nullptr);
      c.CSSource = CSSource.c_str();
      ppBlurV = device.createComputePipeline(c);
    }
  }

  // Render the normal map
  // [normal_map.glsl]
  GraphicsPipeline ppRenderGbuffers;

  // Compute the average shading curve
  // [shading_curve.glsl]
  ComputePipeline ppComputeShadingCurveHSV;

  // Compute the lit-sphere
  // ComputePipeline ppComputeLitSphere;

  // Draw stroke mask
  // [draw_stroke_mask.glsl]
  GraphicsPipeline ppDrawRoundSplatToStrokeMask;
  GraphicsPipeline ppDrawTexturedSplatToStrokeMask;

  // Shading overlay
  GraphicsPipeline ppShadingOverlay;

  // Compose stroke mask onto target
  ComputePipeline ppFlattenStroke;
  ComputePipeline ppSmudge;

  // [evaluate.glsl]
  // Evaluate final image (main pass)
  ComputePipeline ppEvaluate;
  // Evaluate preview: brush stroke mask to base color
  ComputePipeline ppEvaluatePreviewBaseColorUV;
  // Evaluate (blur pass)
  ComputePipeline ppEvaluateBlurPass;

  // [base_color_to_offset.glsl]
  ComputePipeline ppBaseColorToOffset;

  // [blur_brush.glsl]
  ComputePipeline ppBlurBrush;

  // Process passes
  // [process_dynamic_color.glsl]
  ComputePipeline ppProcessDynamicColor;
  ComputePipeline
      ppProcessDynamicColorPreview; // (preview that takes a stroke mask)
  // [process_blur.glsl]
  ComputePipeline ppProcessBlur; // (no preview version)
  // [process_detail.glsl]
  // TODO

  // [blur.glsl]
  ComputePipeline ppBlurH;
  ComputePipeline ppBlurV;

  // Copy a texture with a mask
  GraphicsPipeline ppCopyTexWithMask;

private:
  shaderpp::ShaderSource loadShaderSource(const filesystem::path &path) {
    return shaderpp::ShaderSource(path.str().c_str());
  }
};

#endif
