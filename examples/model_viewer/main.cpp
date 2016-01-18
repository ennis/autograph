#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <docopt.h>
#include <optional.hpp>

#include <shaderpp/shaderpp.hpp>
#include <backend/opengl/Backend.hpp>
#include <Device.hpp>
#include <Draw.hpp>
#include <Error.hpp>

#include "../common/uniforms.hpp"

using GL = ag::opengl::OpenGLBackend;
namespace xp = std::experimental;

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
};

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

ag::GraphicsPipeline<GL> loadPipeline(ag::Device<GL>& device) {
  using namespace ag::opengl;
  using namespace shaderpp;

  VertexAttribute vertexAttribs[] = {
      VertexAttribute{0, gl::FLOAT, 3, 3 * sizeof(float), false}, // positions
      VertexAttribute{0, gl::FLOAT, 3, 3 * sizeof(float), false}  // normals
  };

  ShaderSource src("../examples/common/glsl/model_viewer.glsl");
  ag::opengl::GraphicsPipelineInfo info;
  auto VSSource = src.preprocess(PipelineStage::Vertex, nullptr, nullptr);
  auto PSSource = src.preprocess(PipelineStage::Pixel, nullptr, nullptr);
  info.VSSource = VSSource.c_str();
  info.PSSource = PSSource.c_str();
  info.vertexAttribs = gsl::as_span(vertexAttribs);
  return device.createGraphicsPipeline(info);
}

Mesh loadModel(const char* path) {
  Assimp::Importer importer;

  const aiScene* scene = importer.ReadFile(
      path, aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph |
                aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                aiProcess_SortByPType);

  if (!scene)
    ag::failWith("Could not load scene");

  // load the first mesh
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  if (scene->mNumMeshes > 0) {
    auto mesh = scene->mMeshes[0];
    vertices.resize(mesh->mNumVertices);
    indices.resize(mesh->mNumFaces * 3);
    for (unsigned i = 0; i < mesh->mNumVertices; ++i)
      vertices[i].position = glm::vec3(
          mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
    if (mesh->mNormals)
      for (int i = 0; i < mesh->mNumVertices; ++i)
        vertices[i].normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y,
                                       mesh->mNormals[i].z);
    for (unsigned i = 0; i < mesh->mNumFaces; ++i) {
      indices[i * 3 + 0] = mesh->mFaces[i].mIndices[0];
      indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
      indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
    }
  }

  return Mesh{std::move(vertices), std::move(indices)};
}

static const char kUsage[] = R"(Model viewer
Usage:
    model_viewer <file>
)";

int main(int argc, const char** argv) {
  auto args = docopt::docopt(kUsage, {argv + 1, argv + argc},
                             true,            // show help if requested
                             "Model viewer"); // version string

  auto model = loadModel(args["<file>"].asString().c_str());

  using namespace glm;
  ag::opengl::OpenGLBackend gl;
  ag::DeviceOptions opts;
  ag::Device<GL> device(gl, opts);
  auto pipeline = loadPipeline(device);

  auto vbo = device.createBuffer(gsl::as_span(model.vertices));
  xp::optional<ag::Buffer<GL, unsigned[]>> ibo;
  if (!model.indices.empty())
    ibo.emplace(device.createBuffer(gsl::as_span(model.indices)));

  device.run([&]() {
    auto out = device.getOutputSurface();
    device.clear(out, vec4(1.0, 0.0, 1.0, 1.0));

    uniforms::Scene u_scene;
    u_scene.viewMatrix =
        glm::lookAt(glm::vec3{0.0f, 0.0f, -4.0f}, glm::vec3{0.0f, 0.0f, 0.0f},
                    glm::vec3{0.0f, 1.0f, 0.0f});
    u_scene.projMatrix = glm::perspective(45.0f, 1.0f, 0.01f, 10.0f);
    u_scene.viewProjMatrix = u_scene.projMatrix * u_scene.viewMatrix;
    auto u_scene_buf = device.pushDataToUploadBuffer(
        u_scene, GL::kUniformBufferOffsetAlignment);

    uniforms::Object u_object;
    u_object.modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    auto u_object_buf = device.pushDataToUploadBuffer(
        u_object, GL::kUniformBufferOffsetAlignment);

    if (ibo)
      ag::draw(device, out, pipeline,
               ag::DrawIndexed(ag::PrimitiveType::Triangles, 0, ibo->size(), 0),
               ag::VertexBuffer(vbo), ag::IndexBuffer(ibo.value()), u_scene_buf,
               u_object_buf);
    else
      ag::draw(device, out, pipeline,
               ag::DrawArrays(ag::PrimitiveType::Triangles, 0, vbo.size()),
               ag::VertexBuffer(vbo), u_scene_buf, u_object_buf);
  });

  return 0;
}
