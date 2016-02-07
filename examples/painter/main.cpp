#include <fstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <autograph/backend/opengl/backend.hpp>
#include <autograph/device.hpp>
#include <autograph/draw.hpp>
#include <autograph/pipeline.hpp>
#include <autograph/pixel_format.hpp>
#include <autograph/surface.hpp>

#include <extra/image_io/load_image.hpp>

#include "../common/sample.hpp"
#include "../common/uniforms.hpp"

using GL = ag::opengl::OpenGLBackend;

using StrokeMask = ag::Texture2D<ag::R8>;


struct BrushPath
{
	std::vector<glm::vec2> splatCenters;
	std::vector<glm::vec2> mousePath;
};

struct BrushEngine
{
	ag::GraphicsPipeline<GL>& ppDrawToStrokeMask_TexturedBrush;
	ag::GraphicsPipeline<GL>& ppDrawToStrokeMask_RoundBrush;
	ag::ComputePipeline<GL>& ppComposeStrokeMask;
};

enum class BrushSplatMode
{
	Textured,
	Round
};

struct BrushProperties
{
	glm::vec3 color;
	float flow;
	float opacity;
};

class Painter : public samples::GLSample<Painter> {
public:
  Painter(unsigned width, unsigned height)
      : GLSample(width, height, "Painter") {

    ag::opengl::GraphicsPipelineInfo gpinfo;
    gpinfo.depthStencilState.depthTestEnable = false;
    gpinfo.depthStencilState.depthWriteEnable = false;
    gpinfo.vertexAttribs = samples::kMeshVertexDesc;
    pipeline = loadGraphicsPipeline("painter/glsl/draw_stroke_mask.glsl",
                                    gpinfo, nullptr);

    {
      ag::opengl::ComputePipelineInfo info;
      pipeline = loadComputePipeline("painter/glsl/compose_stroke.glsl", info,
                                     nullptr);
    }

    texRender = device->createTexture2D<ag::RGBA8>({512, 512});
    texDefault = loadTexture2D("common/img/tonberry.jpg");
  }

  void render() {
    using namespace glm;
    auto out = device->getOutputSurface();
    ag::clear(*device, out, ag::ClearColor{1.0f, 0.0f, 1.0f, 1.0f});
  }

private:

  ag::Texture2D<ag::RGBA8, GL> texRender;
  ag::Texture2D<ag::RGBA8, GL> texDefault;
};

int main() {
  Painter sample(1000, 800);
  return sample.run();
}
