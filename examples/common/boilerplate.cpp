#include "boilerplate.hpp"

#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <optional.hpp>

#include <Device.hpp>
#include <Draw.hpp>
#include <Error.hpp>
#include <shaderpp/shaderpp.hpp>

namespace samples {
void Framework::initialize() {
  namespace fs = filesystem;

  auto path = fs::path::getcwd();
  // try several parent paths until we get the right one
  if (!(path / "examples/examples.txt").is_file()) {
    path = path.parent_path();
    if (!(path / "examples/examples.txt").is_file()) {
      path = path.parent_path();
      if (!(path / "examples/examples.txt").is_file()) {
        path = path.parent_path();
        if (!(path / "examples/examples.txt").is_file()) {
          path = path.parent_path();
          if (!(path / "examples/examples.txt").is_file()) {
            path = path.parent_path();
            if (!(path / "examples/examples.txt").is_file()) {
              ag::failWith("Samples root directory not found");
            }
          }
        }
      }
    }
  }

  projectRoot = path;
  samplesRoot = path / "examples";
  loadPipelines();
  loadSamplers();
}

void Framework::loadPipelines() {
  using namespace ag::opengl;
  using namespace shaderpp;

  VertexAttribute vertexAttribs_2D[] = {
      VertexAttribute{0, gl::FLOAT, 2, 2 * sizeof(float), false},
      VertexAttribute{0, gl::FLOAT, 2, 2 * sizeof(float), false}};

  {
    ShaderSource src_copy(
        (samplesRoot / "common/glsl/copy_tex.glsl").str().c_str());
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
	info.VSSource = VSSource.c_str();
	info.PSSource = PSSource.c_str();
    ppCopyTexWithMask = device.createGraphicsPipeline(info);
  }
}

ag::GraphicsPipeline<GL>
Framework::loadMeshShaderPipeline(ag::opengl::GraphicsPipelineInfo& baseInfo,
                                  const char* mesh,
                                  gsl::span<const char*> defines) {
  using namespace shaderpp;
  ag::opengl::VertexAttribute vertexAttribs[] = {
      ag::opengl::VertexAttribute{0, gl::FLOAT, 3, 3 * sizeof(float), false},
      ag::opengl::VertexAttribute{0, gl::FLOAT, 3, 3 * sizeof(float), false},
      ag::opengl::VertexAttribute{0, gl::FLOAT, 3, 3 * sizeof(float), false},
      ag::opengl::VertexAttribute{0, gl::FLOAT, 2, 2 * sizeof(float), false}};

  ShaderSource src((samplesRoot / mesh).str().c_str());
  auto VSSource = src.preprocess(PipelineStage::Vertex, defines, nullptr);
  auto PSSource = src.preprocess(PipelineStage::Pixel, defines, nullptr);
  baseInfo.VSSource = VSSource.c_str();
  baseInfo.PSSource = PSSource.c_str();
  baseInfo.vertexAttribs = gsl::as_span(vertexAttribs);
  return device.createGraphicsPipeline(baseInfo);
}

void Framework::loadSamplers() {
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

Mesh<GL> Framework::loadMesh(const char* asset_path) {
  auto full_path = samplesRoot / asset_path;
  Assimp::Importer importer;

  const aiScene* scene = importer.ReadFile(
      full_path.str().c_str(),
      aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph |
          aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
          aiProcess_SortByPType);

  if (!scene)
    ag::failWith("Could not load scene");

  // load the first mesh
  std::vector<Vertex3D> vertices;
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

  auto vbo = device.createBuffer(gsl::as_span(vertices));
  auto ibo = device.createBuffer(gsl::as_span(indices));

  return Mesh<GL>{std::move(vertices), std::move(indices), std::move(vbo),
                  std::move(ibo)};
}
}