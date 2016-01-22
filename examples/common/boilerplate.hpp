#ifndef BOILERPLATE_HPP
#define BOILERPLATE_HPP

#include <filesystem/path.h>

#include <Device.hpp>
#include <Draw.hpp>
#include <Pipeline.hpp>
#include <backend/opengl/Backend.hpp>

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

template <typename D, typename RenderTarget, typename Drawable,
          typename... ShaderResources>
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

// utility methods and shared resources for the samples
struct Framework {

  Framework(ag::Device<GL>& device_) : device(device_) { initialize(); }

  void initialize();

  void makeCopyRect(unsigned tex_width, unsigned tex_height,
                    unsigned target_width, unsigned target_height,
                    glm::vec2 pos, float scale, gsl::span<Vertex2D, 6> out) {
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
  void copyTex(ag::Texture2D<T, GL>& tex, SurfaceTy&& out_surface,
               unsigned target_width, unsigned target_height, glm::vec2 pos,
               float scale) {
    Vertex2D target_rect[6];
    makeCopyRect(tex.info.dimensions.width, tex.info.dimensions.height,
                 target_width, target_height, pos, scale, target_rect);
    ag::draw(device, out_surface, ppCopyTex,
             ag::DrawArrays(ag::PrimitiveType::Triangles,
                            gsl::span<Vertex2D>(target_rect)),
             glm::vec2(target_width, target_height), tex);
  }

  Mesh<GL> loadMesh(const char* asset_path);
  ag::GraphicsPipeline<GL>
  loadMeshShaderPipeline(ag::opengl::GraphicsPipelineInfo& baseInfo,
                         const char* mesh, gsl::span<const char*> defines);

  ag::Device<GL>& device;
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

private:
  void loadPipelines();
  void loadSamplers();
};

}

#endif
