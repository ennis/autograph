#include "boilerplate.hpp"

#include <vector>

#include <glm/glm.hpp>
#include <optional.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <shaderpp/shaderpp.hpp>
#include <Error.hpp>
#include <Device.hpp>
#include <Draw.hpp>

namespace samples {
void Boilerplate::initialize(ag::Device<GL>& device) {
  namespace fs = filesystem;

  auto path = fs::path::getcwd();
  // try several parent paths until we get the right one
  if (!(path / "examples/assets").is_directory()) {
    path = path.parent_path();
    if (!(path / "examples/assets").is_directory()) {
      path = path.parent_path();
      if (!(path / "examples/assets").is_directory()) {
        path = path.parent_path();
        if (!(path / "examples/assets").is_directory()) {
          path = path.parent_path();
          if (!(path / "examples/assets").is_directory()) {
            path = path.parent_path();
            if (!(path / "examples/assets").is_directory()) {
              ag::failWith("Samples asset directory not found");
            }
          }
        }
      }
    }
  }

  projectRoot = path;
  assetsRoot = path / "examples/assets";
  loadPipelines(device);
  loadSamplers(device);
}

void Boilerplate::loadPipelines(ag::Device<GL>& device) {
  using namespace ag::opengl;
  using namespace shaderpp;

  VertexAttribute vertexAttribs_2D[] = {
      VertexAttribute{0, gl::FLOAT, 2, 2 * sizeof(float), false},
      VertexAttribute{0, gl::FLOAT, 2, 2 * sizeof(float), false}};

  {
    ShaderSource src_default("../../../examples/common/glsl/default.glsl");
    ag::opengl::GraphicsPipelineInfo info;
    auto VSSource =
        src_default.preprocess(PipelineStage::Vertex, nullptr, nullptr);
    auto PSSource =
        src_default.preprocess(PipelineStage::Pixel, nullptr, nullptr);
    info.VSSource = VSSource.c_str();
    info.PSSource = PSSource.c_str();
    info.vertexAttribs = gsl::as_span(vertexAttribs_2D);
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
    info.vertexAttribs = gsl::as_span(vertexAttribs_2D);
    ppCopyTex = device.createGraphicsPipeline(info);
    const char* defines[] = {"USE_MASK"};
    VSSource = src_copy.preprocess(PipelineStage::Vertex, defines, nullptr);
    PSSource = src_copy.preprocess(PipelineStage::Pixel, defines, nullptr);
    ppCopyTexWithMask = device.createGraphicsPipeline(info);
  }
}

void Boilerplate::loadMeshShader(ag::Device<GL>& device, const char* mesh)
{

}

void Boilerplate::loadSamplers(ag::Device<GL>& device) {
  ag::SamplerInfo info;
  info.addrU = ag::TextureAddressMode::Repeat;
  info.addrV = ag::TextureAddressMode::Repeat;
  info.addrW = ag::TextureAddressMode::Repeat;
  info.minFilter = ag::TextureFilter::Nearest;
  info.magFilter = ag::TextureFilter::Nearest;
  samNearestRepeat = device.createSampler(info);
  info.minFilter = ag::TextureFilter::Linear;
  info.magFilter = ag::TextureFilter::Linear;
  samLinearRepeat = device.createSampler(info);
  info.addrU = ag::TextureAddressMode::Clamp;
  info.addrV = ag::TextureAddressMode::Clamp;
  info.addrW = ag::TextureAddressMode::Clamp;
  info.minFilter = ag::TextureFilter::Nearest;
  info.magFilter = ag::TextureFilter::Nearest;
  samNearestClamp = device.createSampler(info);
  info.minFilter = ag::TextureFilter::Linear;
  info.magFilter = ag::TextureFilter::Linear;
  samLinearClamp = device.createSampler(info);
}

Mesh Boilerplate::loadMesh(const char* asset_path) {
  auto full_path = assetsRoot / asset_path;
  Assimp::Importer importer;

  const aiScene* scene = importer.ReadFile(
      full_path.c_str(), aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph |
                             aiProcess_Triangulate |
                             aiProcess_JoinIdenticalVertices |
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

Mesh Boilerplate::loadMeshPipeline(const char* asset_path)
{
  
}

}
}