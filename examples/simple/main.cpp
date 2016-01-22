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

#include "../common/uniforms.hpp"
#include "../common/boilerplate.hpp"

using GL = ag::opengl::OpenGLBackend;


int main() {
  using namespace glm;
  ag::opengl::OpenGLBackend gl;
  ag::DeviceOptions opts;
  ag::Device<GL> device(gl, opts);

  samples::Framework framework(device);

  glm::vec3 vbo_data[] = {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
  auto vbo = device.createBuffer(gsl::span<glm::vec3>(vbo_data));

  auto mesh = framework.loadMesh("meshes/stanford_bunny.obj");

  device.run([&]() {
    samples::uniforms::Scene sceneData;
    sceneData.viewportSize = glm::vec2(512, 512);

    auto out = device.getOutputSurface();
    device.clear(out, vec4(1.0, 0.0, 1.0, 1.0));

    // render to texture
    ag::draw(device, tex, ppDefault,
             ag::DrawArrays(ag::PrimitiveType::Triangles,
                            gsl::span<glm::vec3>(vbo_data)),
             glm::mat4(1.0f));
  });

  return 0;
}
