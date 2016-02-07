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

  Pipelines(Device& device, const filesystem::path& samplesRoot) {
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

	{
		GraphicsPipelineInfo g;
		g.blendState.enabled = true;
		g.blendState.modeAlpha = gl::FUNC_ADD;
		g.blendState.modeRGB = gl::FUNC_ADD;
		g.blendState.funcSrcAlpha = gl::SRC_ALPHA;
		g.blendState.funcDstAlpha = gl::ONE_MINUS_SRC_ALPHA;
		g.blendState.funcSrcRGB = gl::ONE;
		g.blendState.funcDstRGB = gl::ONE_MINUS_SRC_ALPHA;
		g.vertexAttribs = gsl::as_span(samples::vertexAttribs_2D);
		auto VSSource = draw_stroke_mask.preprocess(PipelineStage::Vertex, nullptr, nullptr);
		auto PSSource = draw_stroke_mask.preprocess(PipelineStage::Pixel, nullptr, nullptr);
		g.VSSource = VSSource.c_str();
		g.PSSource = PSSource.c_str();
		ppDrawRoundSplatToStrokeMask = device.createGraphicsPipeline(g);

		const char* defines[] = { "TEXTURED" };
		VSSource = draw_stroke_mask.preprocess(PipelineStage::Vertex, defines, nullptr);
		PSSource = draw_stroke_mask.preprocess(PipelineStage::Pixel, defines, nullptr);
		g.VSSource = VSSource.c_str();
		g.PSSource = PSSource.c_str();
		ppDrawTexturedSplatToStrokeMask = device.createGraphicsPipeline(g);
	}

	{
		ComputePipelineInfo c;
		const char* defines[] = { "TOOL_BASE_COLOR_UV" };
		auto CSSource = flatten_stroke.preprocess(PipelineStage::Compute, nullptr, nullptr);
		c.CSSource = CSSource.c_str();
		ppFlattenStroke = device.createComputePipeline(c);
	}
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
  ComputePipeline ppFlattenStroke;

  // Evaluate final image
  GraphicsPipeline ppEvalutate;

  // Copy a texture with a mask
  GraphicsPipeline ppCopyTexWithMask;

private:
  shaderpp::ShaderSource loadShaderSource(const filesystem::path& path) {
    return shaderpp::ShaderSource(path.str().c_str());
  }
};

#endif
