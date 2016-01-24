#include <fstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <autograph/backend/opengl/Backend.hpp>
#include <autograph/device.hpp>
#include <autograph/draw.hpp>
#include <autograph/pipeline.hpp>
#include <autograph/pixel_format.hpp>
#include <autograph/surface.hpp>

#include "../common/sample.hpp"
#include "../common/uniforms.hpp"

using GL = ag::opengl::OpenGLBackend;

class SimpleSample : public samples::GLSample<SimpleSample> {
public:
  SimpleSample(unsigned width, unsigned height)
      : GLSample(width, height, "Simple") {
    bunnyMesh = loadMesh("common/meshes/stanford_bunny.obj");
    ag::opengl::GraphicsPipelineInfo gpinfo;
    gpinfo.depthStencilState.depthTestEnable = false;
    gpinfo.depthStencilState.depthWriteEnable = false;
    gpinfo.vertexAttribs = samples::kMeshVertexDesc;
    pipeline =
        loadGraphicsPipeline("common/glsl/model_viewer.glsl", gpinfo, nullptr);
    texRender = device->createTexture2D<ag::RGBA8>({512, 512});
  }

  void render() {
    using namespace glm;

    samples::uniforms::Object objectData;
    /*objectData.modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
    auto cbObjectData = device->pushDataToUploadBuffer(
        objectData, GL::kUniformBufferOffsetAlignment);*/

    auto out = device->getOutputSurface();
    ag::clear(*device, out, ag::ClearColor { 1.0f, 0.0f, 1.0f, 1.0f });
	ag::clear(*device, texRender, ag::ClearColor{ 0.0f, 1.0f, 0.0f, 1.0f });
    samples::drawMesh(bunnyMesh, *device, out, pipeline, cbSceneData,
                      glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)));

    copyTex(texRender, out, 1000, 800, {0, 0}, 1.0);
  }

private:
  ag::GraphicsPipeline<GL> pipeline;
  samples::Mesh<GL> bunnyMesh;
  ag::Texture2D<ag::RGBA8, GL> texRender;
};

int main() {
  SimpleSample sample(1000, 800);
  return sample.run();
}
