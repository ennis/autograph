#ifndef SAMPLE_HPP
#define SAMPLE_HPP
#include <array>
#include <cmath>

#include <filesystem/path.h>

#include <autograph/backend/opengl/backend.hpp>
#include <autograph/device.hpp>
#include <autograph/draw.hpp>
#include <autograph/pipeline.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <shaderpp/shaderpp.hpp>

#include "uniforms.hpp"

namespace samples {

using GL = ag::opengl::OpenGLBackend;

struct Vertex2D {
  float x;
  float y;
  float tx;
  float ty;
};

struct Vertex3D {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 tangent;
  glm::vec2 texcoords;
};

template <typename D> struct Mesh {
  std::vector<Vertex3D> vertices;
  std::vector<unsigned int> indices;
  ag::Buffer<D, Vertex3D[]> vbo;
  std::experimental::optional<ag::Buffer<D, unsigned int[]>> ibo;
};

template <typename D, typename RenderTarget, typename... ShaderResources>
void drawMesh(Mesh<D>& mesh, ag::Device<D>& device, RenderTarget&& rt,
              ag::GraphicsPipeline<D>& pipeline,
              ShaderResources&&... resources) {
  if (mesh.ibo)
    ag::draw(device, rt, pipeline, ag::DrawIndexed(ag::PrimitiveType::Triangles,
                                                   0, mesh.ibo->size(), 0),
             ag::VertexBuffer(mesh.vbo), ag::IndexBuffer(mesh.ibo.value()),
             std::forward<ShaderResources>(resources)...);
  else
    ag::draw(device, rt, pipeline,
             ag::DrawArrays(ag::PrimitiveType::Triangles, 0, mesh.vbo.size()),
             ag::VertexBuffer(mesh.vbo),
             std::forward<ShaderResources>(resources)...);
}

std::array<float, 16> perspectiveFovRH(float fovY, float aspectRatio,
                                       float zNear, float zFar) {
  std::array<float, 16> out;

  float tanHalfFovy = std::tan(fovY / 2.0f);

  out[0] = 1.0f / (aspectRatio * tanHalfFovy);
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  out[4] = 0;
  out[5] = -1.0f / (tanHalfFovy);
  out[6] = 0;
  out[7] = 0;
  out[8] = 0;
  out[9] = 0;
  out[10] = zFar / (zNear - zFar);
  out[11] = -1.0f;
  out[12] = 0;
  out[13] = 0;
  out[14] = zNear * zFar / (zNear - zFar);
  out[15] = 0;

  return out;
}

constexpr ag::opengl::VertexAttribute kMeshVertexDesc[] = {
    {0, gl::FLOAT, 3, 3 * sizeof(float), false},
    {0, gl::FLOAT, 3, 3 * sizeof(float), false},
    {0, gl::FLOAT, 3, 3 * sizeof(float), false},
    {0, gl::FLOAT, 2, 2 * sizeof(float), false}};

constexpr ag::opengl::VertexAttribute vertexAttribs_2D[] = {
	{ 0, gl::FLOAT, 2, 2 * sizeof(float), false },
	{ 0, gl::FLOAT, 2, 2 * sizeof(float), false } };

// utility methods and shared resources for the samples
// OpenGL sample class, needs glfw
template <typename Derived> class GLSample {
public:
  GLSample(unsigned width_, unsigned height_, const char* window_title_)
      : width(width_), height(height_) {
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
    ag::DeviceOptions options;
    options.framebufferWidth = width;
    options.framebufferHeight = height;
    options.windowTitle = std::string(window_title_);
    device = std::make_unique<ag::Device<GL>>(gl, options);
    loadPipelines();
    loadSamplers();
  }

  void makeCopyRect(unsigned tex_width, unsigned tex_height,
                    unsigned target_width, unsigned target_height,
                    glm::vec2 pos, float scale, gsl::span<Vertex2D, 6> out) {
    float xleft = (pos.x / target_width) * 2.0f - 1.0f;
    float ytop = (pos.y / target_height) * 2.0f - 1.0f;
    float xright = xleft + ((float)tex_width / target_width * scale) * 2.0f;
    float ybottom = ytop + ((float)tex_height / target_height * scale) * 2.0f;

    out[0] = Vertex2D{xleft, ytop, 0.0, 0.0};
    out[2] = Vertex2D{xright, ytop, 1.0, 0.0};
    out[1] = Vertex2D{xleft, ybottom, 0.0, 1.0};
    out[3] = Vertex2D{xleft, ybottom, 0.0, 1.0};
    out[5] = Vertex2D{xright, ytop, 1.0, 0.0};
    out[4] = Vertex2D{xright, ybottom, 1.0, 1.0};
  }

  template <typename SurfaceTy, typename T>
  void copyTex(ag::Texture2D<T, GL>& tex, SurfaceTy&& out_surface,
               unsigned target_width, unsigned target_height, glm::vec2 pos,
               float scale) {
    Vertex2D target_rect[6];
    makeCopyRect(tex.info.dimensions.x, tex.info.dimensions.y, target_width,
                 target_height, pos, scale, target_rect);
    ag::draw(*device, out_surface, ppCopyTex,
             ag::DrawArrays(ag::PrimitiveType::Triangles,
                            gsl::span<Vertex2D>(target_rect)),
             glm::vec2(target_width, target_height), tex);
  }

  ag::Texture2D<ag::RGBA8, GL> loadTexture2D(const char* asset_path) {
    auto full_path = samplesRoot / asset_path;
    return std::move(
        ag::extra::image_io::loadTexture2D(*device, full_path.str().c_str()));
  }

  Mesh<GL> loadMesh(const char* asset_path) {
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
        for (unsigned i = 0; i < mesh->mNumVertices; ++i)
          vertices[i].normal = glm::vec3(
              mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
      for (unsigned i = 0; i < mesh->mNumFaces; ++i) {
        indices[i * 3 + 0] = mesh->mFaces[i].mIndices[0];
        indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
        indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
      }
    }

    auto vbo = device->createBufferFromSpan(gsl::span<const Vertex3D>(vertices));
    auto ibo = device->createBufferFromSpan(gsl::span<const unsigned int>(indices));

    return Mesh<GL>{std::move(vertices), std::move(indices), std::move(vbo),
                    std::move(ibo)};
  }

  ag::GraphicsPipeline<GL>
  loadGraphicsPipeline(const char* path,
                       ag::opengl::GraphicsPipelineInfo& baseInfo,
                       gsl::span<const char*> defines) {
    using namespace shaderpp;
    ShaderSource src((samplesRoot / path).str().c_str());
    auto VSSource = src.preprocess(PipelineStage::Vertex, defines, nullptr);
    auto PSSource = src.preprocess(PipelineStage::Pixel, defines, nullptr);
    baseInfo.VSSource = VSSource.c_str();
    baseInfo.PSSource = PSSource.c_str();
    return device->createGraphicsPipeline(baseInfo);
  }

  ag::ComputePipeline<GL> loadComputePipeline(const char* path,
                                              gsl::span<const char*> defines) {
    using namespace shaderpp;
    ag::opengl::ComputePipelineInfo info;
    ShaderSource src((samplesRoot / path).str().c_str());
    auto CSSource = src.preprocess(PipelineStage::Compute, defines, nullptr);
    info.CSSource = CSSource.c_str();
    return device->createComputePipeline(info);
  }

  int run() {
    device->run([this]() {
      // TODO camera and stuff
      // Note: we use the DirectX convention, so -1 is at the top, +1 is at the
      // bottom
      sceneData.viewMatrix =
          glm::lookAt(glm::vec3{0.0f, 0.0f, -4.0f}, glm::vec3{0.0f, 0.0f, 0.0f},
                      glm::vec3{0.0f, 1.0f, 0.0f});
      auto projMat = perspectiveFovRH(45.0f, 1.0f, 0.01f, 10.0f);
      sceneData.projMatrix =
          glm::mat4(projMat[0], projMat[1], projMat[2], projMat[3], projMat[4],
                    projMat[5], projMat[6], projMat[7], projMat[8], projMat[9],
                    projMat[10], projMat[11], projMat[12], projMat[13],
                    projMat[14], projMat[15]);
      sceneData.viewProjMatrix = sceneData.projMatrix * sceneData.viewMatrix;
      cbSceneData = device->pushDataToUploadBuffer(
          sceneData, GL::kUniformBufferOffsetAlignment);

      // TODO handle input here
      static_cast<Derived*>(this)->render();
    });
    return 0;
  }

protected:
  unsigned width;
  unsigned height;
  ag::opengl::OpenGLBackend gl;
  std::unique_ptr<ag::Device<GL>> device;
  // autograph source tree root (contains src,ext,examples)
  filesystem::path projectRoot;
  // samples root
  filesystem::path samplesRoot;

  ag::GraphicsPipeline<GL> ppDefault;
  ag::GraphicsPipeline<GL> ppCopyTex;
  ag::GraphicsPipeline<GL> ppCopyTexWithMask;

  ag::Sampler<GL> samLinearClamp;
  ag::Sampler<GL> samNearestClamp;
  ag::Sampler<GL> samLinearRepeat;
  ag::Sampler<GL> samNearestRepeat;

  samples::uniforms::Scene sceneData;
  ag::RawBufferSlice<GL> cbSceneData;

private:
  void loadPipelines() {
    using namespace ag::opengl;
    using namespace shaderpp;


    ShaderSource src_copy(
        (samplesRoot / "common/glsl/copy_tex.glsl").str().c_str());
    ag::opengl::GraphicsPipelineInfo info;
    auto VSSource =
        src_copy.preprocess(PipelineStage::Vertex, nullptr, nullptr);
    auto PSSource = src_copy.preprocess(PipelineStage::Pixel, nullptr, nullptr);
    info.VSSource = VSSource.c_str();
    info.PSSource = PSSource.c_str();
    info.vertexAttribs = gsl::as_span(vertexAttribs_2D);
    ppCopyTex = device->createGraphicsPipeline(info);
    const char* defines[] = {"USE_MASK"};
    VSSource = src_copy.preprocess(PipelineStage::Vertex, defines, nullptr);
    PSSource = src_copy.preprocess(PipelineStage::Pixel, defines, nullptr);
    info.VSSource = VSSource.c_str();
    info.PSSource = PSSource.c_str();
    ppCopyTexWithMask = device->createGraphicsPipeline(info);
  }
  void loadSamplers() {
    ag::SamplerInfo info;
    info.addrU = ag::TextureAddressMode::Repeat;
    info.addrV = ag::TextureAddressMode::Repeat;
    info.addrW = ag::TextureAddressMode::Repeat;
    info.minFilter = ag::TextureFilter::Nearest;
    info.magFilter = ag::TextureFilter::Nearest;
    samNearestRepeat = device->createSampler(info);
    info.minFilter = ag::TextureFilter::Linear;
    info.magFilter = ag::TextureFilter::Linear;
    samLinearRepeat = device->createSampler(info);
    info.addrU = ag::TextureAddressMode::Clamp;
    info.addrV = ag::TextureAddressMode::Clamp;
    info.addrW = ag::TextureAddressMode::Clamp;
    info.minFilter = ag::TextureFilter::Nearest;
    info.magFilter = ag::TextureFilter::Nearest;
    samNearestClamp = device->createSampler(info);
    info.minFilter = ag::TextureFilter::Linear;
    info.magFilter = ag::TextureFilter::Linear;
    samLinearClamp = device->createSampler(info);
  }
};
}

#endif
