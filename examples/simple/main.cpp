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

#include "../common/boilerplate.hpp"
#include "../common/uniforms.hpp"

using GL = ag::opengl::OpenGLBackend;

int main() {
  using namespace glm;
  ag::opengl::OpenGLBackend gl;
  ag::DeviceOptions opts;
  ag::Device<GL> device(gl, opts);

  samples::Framework framework(device);

  glm::vec3 vbo_data[] = {
      {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
  auto vbo = device.createBuffer(gsl::span<glm::vec3>(vbo_data));

  ag::opengl::GraphicsPipelineInfo gpi;
  gpi.depthStencilState.depthTestEnable = true;
  gpi.depthStencilState.depthWriteEnable = true;
  auto pipeline = framework.loadMeshShaderPipeline(
      gpi, "common/glsl/model_viewer.glsl", nullptr);
  auto mesh = framework.loadMesh("common/meshes/stanford_bunny.obj");
  auto tex = device.createTexture2D<ag::RGBA8>({512, 512});

  device.run([&]() {
    samples::uniforms::Scene u_scene;
    u_scene.viewMatrix =
        glm::lookAt(glm::vec3{0.0f, 0.0f, -4.0f}, glm::vec3{0.0f, 0.0f, 0.0f},
                    glm::vec3{0.0f, 1.0f, 0.0f});
    u_scene.projMatrix = glm::perspective(45.0f, 1.0f, 0.01f, 10.0f);
    u_scene.viewProjMatrix = u_scene.projMatrix * u_scene.viewMatrix;
    auto u_scene_buf = device.pushDataToUploadBuffer(
        u_scene, GL::kUniformBufferOffsetAlignment);

    samples::uniforms::Object u_object;
    u_object.modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    auto u_object_buf = device.pushDataToUploadBuffer(
        u_object, GL::kUniformBufferOffsetAlignment);

    auto out = device.getOutputSurface();
    device.clear(out, vec4(1.0, 0.0, 1.0, 1.0));

    // render to texture
    /*ag::draw(device, tex, pipeline,
             ag::DrawArrays(ag::PrimitiveType::Triangles,
                            gsl::span<glm::vec3>(vbo_data)),
             u_scene_buf, u_object_buf);*/
	samples::drawMesh(mesh, device, tex, pipeline, u_scene_buf, u_object_buf);

    framework.copyTex(tex, out, 640, 480, {0, 0}, 1.0);
  });

  return 0;
}
