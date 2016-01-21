#include <iostream>
#include <fstream>
#include <glm/glm.hpp>

#include <PixelType.hpp>
#include <Surface.hpp>
#include <backend/opengl/Backend.hpp>
#include <Device.hpp>
#include <ResourceScope.hpp>
#include <Draw.hpp>
#include <Pipeline.hpp>

#include <shaderpp/shaderpp.hpp>

#include "../common/uniforms.hpp"

using GL = ag::opengl::OpenGLBackend;

ag::GraphicsPipeline<GL> ppDefault;
ag::GraphicsPipeline<GL> ppCopyTex;
ag::GraphicsPipeline<GL> ppCopyTexWithMask;
ag::ComputePipeline<GL> ppCompute;

void reloadPipelines(ag::Device<GL>& device) {
  using namespace ag::opengl;
  using namespace shaderpp;

  VertexAttribute vertexAttribs[] = {
      VertexAttribute{0, gl::FLOAT, 3, 3 * sizeof(float), false}};

  {
    ShaderSource src_default("../../../examples/common/glsl/default.glsl");
    ag::opengl::GraphicsPipelineInfo info;
    auto VSSource =
        src_default.preprocess(PipelineStage::Vertex, nullptr, nullptr);
    auto PSSource =
        src_default.preprocess(PipelineStage::Pixel, nullptr, nullptr);
    info.VSSource = VSSource.c_str();
    info.PSSource = PSSource.c_str();
    info.vertexAttribs = gsl::as_span(vertexAttribs);
    ppDefault = device.createGraphicsPipeline(info);
  }

  {
    ShaderSource src_copy("../../../examples/common/glsl/copy_tex.glsl");
    ag::opengl::GraphicsPipelineInfo info;
    auto VSSource =
        src_copy.preprocess(PipelineStage::Vertex, nullptr, nullptr);
    auto PSSource = src_copy.preprocess(PipelineStage::Pixel, nullptr, nullptr);
    info.VSSource = VSSource.c_str();
    info.PSSource = PSSource.c_str();
    info.vertexAttribs = gsl::as_span(vertexAttribs);
    ppCopyTex = device.createGraphicsPipeline(info);
    const char* defines[] = {"USE_MASK"};
    VSSource = src_copy.preprocess(PipelineStage::Vertex, defines, nullptr);
    PSSource = src_copy.preprocess(PipelineStage::Pixel, defines, nullptr);
    ppCopyTexWithMask = device.createGraphicsPipeline(info);
  }
}

struct Vertex2D {
  float x;
  float y;
  float tx;
  float ty;
};

void makeCopyRect(unsigned tex_width, unsigned tex_height,
                  unsigned target_width, unsigned target_height, glm::vec2 pos,
                  float scale, gsl::span<Vertex2D, 6> out) {
  float xleft = (pos.x / target_width) * 2.0f - 1.0f;
  float ytop = (1.0f - pos.y / target_height) * 2.0f - 1.0f;
  float xright = xleft + (tex_width / target_width * scale) * 2.0f;
  float ybottom = ytop - (tex_height / target_height * scale) * 2.0f;

  out[0] = Vertex2D{xleft, ytop, 0.0, 1.0};
  out[1] = Vertex2D{xright, ytop, 1.0, 1.0};
  out[2] = Vertex2D{xleft, ybottom, 0.0, 0.0};
  out[3] = Vertex2D{xleft, ybottom, 0.0, 0.0};
  out[4] = Vertex2D{xright, ytop, 1.0, 1.0};
  out[5] = Vertex2D{xright, ybottom, 1.0, 0.0};
}

template <typename SurfaceTy, typename T>
void copyTex(ag::Device<GL>& device, ag::Texture2D<T>& tex,
             SurfaceTy&& out_surface, unsigned target_width,
             unsigned target_height, glm::vec2 pos, float scale) {
  float target_rect[6];
  makeCopyRect(tex.info.dimensions.width, tex.info.dimensions.height,
               target_width, target_height, pos, scale, target_rect);
  ag::draw(
      device, out_surface, ppCopyTex,
      ag::DrawArrays(ag::PrimitiveType::Triangles, gsl::as_span(target_rect)),
      glm::vec2(target_width, target_height), tex);
}

int main() {
  using namespace glm;
  ag::opengl::OpenGLBackend gl;
  ag::DeviceOptions opts;
  ag::Device<GL> device(gl, opts);

  Pipelines pp(device);

  auto tex = device.createTexture2D<ag::RGBA8>({512, 512});
  glm::vec3 vbo_data[] = {
      {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
  auto vbo = device.createBuffer(gsl::span<glm::vec3>(vbo_data));

  ag::SamplerInfo linearSamplerInfo;
  linearSamplerInfo.addrU = ag::TextureAddressMode::Repeat;
  linearSamplerInfo.addrV = ag::TextureAddressMode::Repeat;
  linearSamplerInfo.addrW = ag::TextureAddressMode::Repeat;
  linearSamplerInfo.minFilter = ag::TextureFilter::Nearest;
  linearSamplerInfo.magFilter = ag::TextureFilter::Nearest;
  auto sampler = device.createSampler(linearSamplerInfo);

  device.run([&]() {
    uniforms::Scene sceneData;
    sceneData.viewportSize = glm::vec2(512, 512);

    auto out = device.getOutputSurface();
    device.clear(out, vec4(1.0, 0.0, 1.0, 1.0));

    // render to texture
    ag::draw(device, tex, pp.ppDefault,
             ag::DrawArrays(ag::PrimitiveType::Triangles,
                            gsl::span<glm::vec3>(vbo_data)),
             glm::mat4(1.0f));

    // copy to screen
    ag::draw(device, out, pp.ppCopyTex, tex, sceneData, );
  });

  return 0;
}
