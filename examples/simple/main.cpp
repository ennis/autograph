#include <fstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Device.hpp>
#include <Draw.hpp>
#include <Pipeline.hpp>
#include <PixelType.hpp>
#include <ResourceScope.hpp>
#include <Surface.hpp>
#include <backend/opengl/Backend.hpp>

#include "../common/sample.hpp"
#include "../common/uniforms.hpp"

using GL = ag::opengl::OpenGLBackend;

class SimpleSample : public samples::GLSample<SimpleSample> {
public:
  SimpleSample(unsigned width, unsigned height)
      : GLSample(width, height, "Simple") {
    bunnyMesh = loadMesh("common/meshes/stanford_bunny.obj");
    ag::opengl::GraphicsPipelineInfo gpi;
    gpi.depthStencilState.depthTestEnable = true;
    gpi.depthStencilState.depthWriteEnable = true;
    pipeline =
        loadMeshShaderPipeline(gpi, "common/glsl/model_viewer.glsl", nullptr);
    texRender = device->createTexture2D<ag::RGBA8>({512, 512});
  }

  void render() {
    using namespace glm;

    samples::uniforms::Object objectData;
    objectData.modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    auto cbObjectData = device->pushDataToUploadBuffer(
        objectData, GL::kUniformBufferOffsetAlignment);

    auto out = device->getOutputSurface();
    device->clear(out, vec4(1.0, 0.0, 1.0, 1.0));

    samples::drawMesh(bunnyMesh, *device, texRender, pipeline, cbSceneData,
                      cbObjectData);

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
