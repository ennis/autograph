#ifndef OPENGL_BACKEND_HPP
#define OPENGL_BACKEND_HPP

#include <array>
#include <deque>
#include <stdexcept>

// this must be included before glfw3
#include <gl_core_4_5.hpp>

#include <GLFW/glfw3.h>
#include <format.h>
#include <glm/glm.hpp>
#include <gsl.h>

#include "../../device.hpp"
#include "../../draw.hpp"
#include "../../optional.hpp"
#include "../../rect.hpp"
#include "../../surface.hpp"
#include "../../texture.hpp"
#include "../../utils.hpp"

#include "state.hpp"

namespace ag {
namespace opengl {
// Data access hints
enum class DataAccessHints : unsigned {
  None = 0,
  GPURead = 1 << 0,
  CPURead = 1 << 1
};

// Surface usage hints
enum class SurfaceUsageHints : unsigned {
  None = 0,
  DefaultFramebuffer = 1 << 0,
};
}
}

// enable the two previous enums to be used as bitflags
template <>
struct is_enum_flags<ag::opengl::SurfaceUsageHints> : public std::true_type {};
template <>
struct is_enum_flags<ag::opengl::DataAccessHints> : public std::true_type {};

namespace ag {
namespace opengl {

struct GLPixelFormat {
  GLenum internalFormat;
  GLenum externalFormat;
  GLenum type;
  int numComponents;
};

inline GLPixelFormat pixelFormatToGL(PixelFormat format) {
  switch (format) {
  case PixelFormat::Uint8:
    return GLPixelFormat{gl::R8UI, gl::RED_INTEGER, gl::UNSIGNED_BYTE, 1};
  case PixelFormat::Uint8x2:
    return GLPixelFormat{gl::RG8UI, gl::RG_INTEGER, gl::UNSIGNED_BYTE, 2};
  case PixelFormat::Uint8x3:
    return GLPixelFormat{gl::RGB8UI, gl::RGB_INTEGER, gl::UNSIGNED_BYTE, 3};
  case PixelFormat::Uint8x4:
    return GLPixelFormat{gl::RGBA8UI, gl::RGBA_INTEGER, gl::UNSIGNED_BYTE, 4};
  case PixelFormat::Unorm8:
    return GLPixelFormat{gl::R8, gl::RED, gl::UNSIGNED_BYTE, 1};
  case PixelFormat::Unorm8x2:
    return GLPixelFormat{gl::RG8, gl::RG, gl::UNSIGNED_BYTE, 2};
  case PixelFormat::Unorm8x3:
    return GLPixelFormat{gl::RGB8, gl::RGB, gl::UNSIGNED_BYTE, 3};
  case PixelFormat::Unorm8x4:
    return GLPixelFormat{gl::RGBA8, gl::RGBA, gl::UNSIGNED_BYTE, 4};
  case PixelFormat::Float:
    return GLPixelFormat{gl::R32F, gl::RED, gl::FLOAT, 1};
  case PixelFormat::Uint32:
    return GLPixelFormat{gl::R32UI, gl::RED_INTEGER, gl::UNSIGNED_INT, 1};
  case PixelFormat::Depth32:
    return GLPixelFormat{gl::DEPTH_COMPONENT32F, 0, 0, 1}; // TODO
  case PixelFormat::Depth24_Stencil8:
    return GLPixelFormat{gl::DEPTH24_STENCIL8, 0, 0, 1};
  case PixelFormat::Snorm10x3_1x2: // Oops, does not exist in OpenGL
    return GLPixelFormat{gl::RGB10_A2, gl::RGBA, gl::UNSIGNED_INT_10_10_10_2,
                         4};
  case PixelFormat::Unorm10x3_1x2:
    return GLPixelFormat{gl::RGB10_A2, gl::RGBA, gl::UNSIGNED_INT_10_10_10_2,
                         4};
  default:
    failWith("TODO");
  }
}

// Wrapper to use GLuint as a unique_ptr handle type
// http://stackoverflow.com/questions/6265288/unique-ptr-custom-storage-type-example/6272139#6272139
// TODO move this in a shared header
struct GLuintHandle {
  GLuint id;
  GLuintHandle(GLuint obj_id) : id(obj_id) {}
  // default and nullptr constructors folded together
  GLuintHandle(std::nullptr_t = nullptr) : id(0) {}
  explicit operator bool() { return id != 0; }
  friend bool operator==(GLuintHandle l, GLuintHandle r) {
    return l.id == r.id;
  }
  friend bool operator!=(GLuintHandle l, GLuintHandle r) { return !(l == r); }
  // default copy ctor and operator= are fine
  // explicit nullptr assignment and comparison unneeded
  // because of implicit nullptr constructor
  // swappable requirement fulfilled by std::swap
};

struct VertexAttribute {
  unsigned slot;
  GLenum type;
  unsigned size;
  unsigned stride;
  bool normalized;
};

struct GraphicsPipelineInfo {
  const char* VSSource = nullptr;
  const char* GSSource = nullptr;
  const char* PSSource = nullptr;
  const char* DSSource = nullptr;
  const char* HSSource = nullptr;
  gsl::span<const VertexAttribute> vertexAttribs;
  GLRasterizerState rasterizerState;
  GLDepthStencilState depthStencilState;
  GLBlendState blendState;
};

struct ComputePipelineInfo {
  const char* CSSource = nullptr;
};

// Graphics context (OpenGL)
struct OpenGLBackend {
private:
  // shortcut
  using D = OpenGLBackend;

public:
  ///////////////////// Timeout values for fence wait operations
  static constexpr unsigned kFenceWaitTimeout = 2000000000; // in nanoseconds

  ///////////////////// Alignment constraints for buffers
  static constexpr unsigned kBufferAlignment = 64;
  static constexpr unsigned kUniformBufferOffsetAlignment =
      256; // TODO do not hardcode this

  ///////////////////// arbitrary binding limits
  static constexpr unsigned kMaxTextureUnits = 16;
  static constexpr unsigned kMaxImageUnits = 8;
  static constexpr unsigned kMaxVertexBufferSlots = 8;
  static constexpr unsigned kMaxUniformBufferSlots = 8;
  static constexpr unsigned kMaxShaderStorageBufferSlots = 8;

  struct GraphicsPipeline {
    GLuint vao = 0;
    GLuint program = 0;
    GLRasterizerState rasterizerState;
    GLDepthStencilState depthStencilState;
    GLBlendState blendState;
  };

  struct ComputePipeline {
    GLuint program = 0;
  };

  struct GLFence {
    struct SyncPoint {
      GLsync sync;
      uint64_t targetValue;
    };

    uint64_t currentValue;
    std::deque<SyncPoint> syncPoints;
  };

  struct GLbuffer {
    GLuint buf_obj;
    BufferUsage usage;
  };

  ///////////////////// Deleters
  struct SamplerDeleter {
    using pointer = GLuintHandle;
    void operator()(pointer sampler_obj) {
      gl::DeleteSamplers(1, &sampler_obj.id);
    }
  };

  struct TextureDeleter {
    using pointer = GLuintHandle;
    void operator()(pointer tex_obj) { gl::DeleteTextures(1, &tex_obj.id); }
  };

  struct BufferDeleter {
    using pointer = GLbuffer*;
    void operator()(pointer buffer) {
      gl::DeleteBuffers(1, &buffer->buf_obj);
      delete buffer;
    }
  };

  struct GraphicsPipelineDeleter {
    using pointer = GraphicsPipeline*;
    void operator()(pointer pp) {
      if (pp->vao)
        gl::DeleteVertexArrays(1, &pp->vao);
      if (pp->program)
        gl::DeleteProgram(pp->program);
      delete pp;
    }
  };

  struct ComputePipelineDeleter {
    using pointer = ComputePipeline*;
    void operator()(pointer pp) {
      if (pp->program)
        gl::DeleteProgram(pp->program);
      delete pp;
    }
  };

  struct FenceDeleter {
    using pointer = GLFence*;
    void operator()(pointer fence) {
      for (auto s : fence->syncPoints)
        gl::DeleteSync(s.sync);
      delete fence;
    }
  };

  ///////////////////// associated types
  // buffer handles
  using BufferHandle = std::unique_ptr<void, BufferDeleter>;
  // texture handles
  using Texture1DHandle = std::unique_ptr<void, TextureDeleter>;
  using Texture2DHandle = std::unique_ptr<void, TextureDeleter>;
  using Texture3DHandle = std::unique_ptr<void, TextureDeleter>;
  // sampler handles
  using SamplerHandle = std::unique_ptr<void, SamplerDeleter>;
  // surface handles
  using SurfaceHandle = std::unique_ptr<void, TextureDeleter>;
  // graphics pipeline
  using GraphicsPipelineHandle = std::unique_ptr<void, GraphicsPipelineDeleter>;
  using ComputePipelineHandle = std::unique_ptr<void, ComputePipelineDeleter>;
  // fence handle
  using FenceHandle = std::unique_ptr<void, FenceDeleter>;

  // constructor
  OpenGLBackend();

  // create a swap chain to draw into (color buffer + depth buffer)
  void createWindow(const DeviceOptions& options);
  bool processWindowEvents();
  GLFWwindow* getWindow() { return window; }

  SurfaceHandle initOutputSurface();

  ///////////////////// Resources: Textures
  Texture1DHandle createTexture1D(const Texture1DInfo& info);
  Texture2DHandle createTexture2D(const Texture2DInfo& info);
  Texture3DHandle createTexture3D(const Texture3DInfo& info);

  // used internally
  /*void destroyTexture1D(Texture1DHandle detail, const Texture1DInfo& info);
              void destroyTexture2D(Texture2DHandle detail, const Texture2DInfo&
  info);
  void destroyTexture3D(Texture3DHandle detail, const Texture3DInfo& info);*/

  ///////////////////// Resources: Buffers
  BufferHandle createBuffer(std::size_t size, const void* data,
                            BufferUsage usage);
  // used internally
  // void destroyBuffer(BufferHandle handle);
  // Map buffer data into the CPU virtual address space
  void* mapBuffer(BufferHandle::pointer handle, size_t offset, size_t size);

  ///////////////////// Resources: Samplers
  SamplerHandle createSampler(const SamplerInfo& info);
  // used internally
  // void destroySampler(SamplerHandle::pointer detail);

  ///////////////////// Resources: Pipelines
  GraphicsPipelineHandle
  createGraphicsPipeline(const GraphicsPipelineInfo& info);
  ComputePipelineHandle createComputePipeline(const ComputePipelineInfo& info);
  // used internally
  // void destroyGraphicsPipeline(GraphicsPipelineHandle handle);

  ///////////////////// Resources: fences
  FenceHandle createFence(uint64_t initialValue);
  void signal(FenceHandle::pointer fence,
              uint64_t value); // insert into GPU command stream
  void signalCPU(FenceHandle::pointer fence, uint64_t value); // CPU-side signal
  uint64_t getFenceValue(FenceHandle::pointer handle);
  // void destroyFence(FenceHandle handle);
  void waitForFence(FenceHandle::pointer handle, uint64_t value);

  ///////////////////// Bind
  void bindTexture1D(unsigned slot, Texture1DHandle::pointer handle);
  void bindTexture2D(unsigned slot, Texture2DHandle::pointer handle);
  void bindTexture3D(unsigned slot, Texture3DHandle::pointer handle);
  void bindRWTexture1D(unsigned slot, Texture1DHandle::pointer handle);
  void bindRWTexture2D(unsigned slot, Texture2DHandle::pointer handle);
  void bindRWTexture3D(unsigned slot, Texture3DHandle::pointer handle);
  void bindSampler(unsigned slot, SamplerHandle::pointer handle);
  void bindVertexBuffer(unsigned slot, BufferHandle::pointer handle,
                        size_t offset, size_t size, unsigned stride);
  void bindIndexBuffer(BufferHandle::pointer handle, size_t offset, size_t size,
                       IndexType type);
  void bindUniformBuffer(unsigned slot, BufferHandle::pointer handle,
                         size_t offset, size_t size);
  void bindGraphicsPipeline(GraphicsPipelineHandle::pointer handle);
  void bindComputePipeline(ComputePipelineHandle::pointer handle);

  ///////////////////// Render targets
  void bindSurface(SurfaceHandle::pointer handle);
  void bindRenderTexture(unsigned slot, Texture2DHandle::pointer handle);
  void bindDepthRenderTexture(Texture2DHandle::pointer handle);

  ///////////////////// Clear command
  void clearColor(SurfaceHandle::pointer framebuffer_obj,
                  const ag::ClearColor& color);
  void clearDepth(SurfaceHandle::pointer framebuffer_obj, float depth);

  ///////////////////// Clear texture when Pixel is a floating point pixel type
  template <typename Pixel>
  void clearTexture1DFloat(Texture1D<Pixel, D>& tex, const ag::Box1D& region,
                           const ag::ClearColor& color) {
    gl::ClearTexImage(tex.handle.get().id, 0, gl::RGBA, gl::FLOAT, color.rgba);
  }

  template <typename Pixel>
  void clearTexture2DFloat(Texture2D<Pixel, D>& tex, const ag::Box2D& region,
                           const ag::ClearColor& color) {
    gl::ClearTexImage(tex.handle.get().id, 0, gl::RGBA, gl::FLOAT, color.rgba);
  }

  template <typename Pixel>
  void clearTexture3DFloat(Texture3D<Pixel, D>& tex, const ag::Box3D& region,
                           const ag::ClearColor& color) {
    gl::ClearTexImage(tex.handle.get().id, 0, gl::RGBA_INTEGER, gl::FLOAT,
                      color.rgba);
  }

  ///////////////////// Clear texture when Pixel is an integer pixel type
  template <typename IPixel>
  void clearTexture1DInteger(Texture1D<IPixel, D>& tex, const ag::Box1D& region,
                             const ag::ClearColorInt& color) {

    gl::ClearTexImage(tex.handle.get().id, 0, gl::RGBA_INTEGER,
                      gl::UNSIGNED_INT, color.rgba);
  }

  template <typename IPixel>
  void clearTexture2DInteger(Texture2D<IPixel, D>& tex, const ag::Box2D& region,
                             const ag::ClearColorInt& color) {

    gl::ClearTexImage(tex.handle.get().id, 0, gl::RGBA_INTEGER,
                      gl::UNSIGNED_INT, color.rgba);
  }

  template <typename IPixel>
  void clearTexture3DInteger(Texture3D<IPixel, D>& tex, const ag::Box3D& region,
                             const ag::ClearColorInt& color) {
    gl::ClearTexImage(tex.handle.get().id, 0, gl::RGBA_INTEGER,
                      gl::UNSIGNED_INT, color.rgba);
  }

  ///////////////////// Clear texture when Pixel is a depth pixel type
  template <typename Depth>
  void clearTexture2DDepth(Texture2D<Depth, D>& tex, const ag::Box2D& region,
                           float depth) {
    gl::ClearTexImage(tex.handle.get().id, 0, gl::DEPTH_COMPONENT, gl::FLOAT,
                      &depth);
  }

  ///////////////////// Copy tex region to buffer
  template <typename Pixel>
  void copyTextureRegion1D(Texture1D<Pixel, D>& src, RawBufferSlice<D>& dest,
                           const ag::Box1D& region, unsigned mipLevel) {
    const auto& gl_fmt = pixelFormatToGL(src.info.format);
    gl::BindBuffer(gl::PIXEL_UNPACK_BUFFER, dest.handle->buf_obj);
    gl::GetTextureSubImage(src.handle.get().id, mipLevel, region.xmin, 0, 0,
                           region.width(), 1, 1, gl_fmt.externalFormat,
                           gl_fmt.type, dest.byteSize, (void*)dest.offset);
  }

  template <typename Pixel>
  void copyTextureRegion2D(Texture2D<Pixel, D>& src, RawBufferSlice<D>& dest,
                           const ag::Box2D& region, unsigned mipLevel) {
    const auto& gl_fmt = pixelFormatToGL(src.info.format);
    gl::BindBuffer(gl::PIXEL_UNPACK_BUFFER, dest.handle->buf_obj);
    gl::GetTextureSubImage(src.handle.get().id, mipLevel, region.xmin,
                           region.ymin, 0, region.width(), region.height(), 1,
                           gl_fmt.externalFormat, gl_fmt.type, dest.byteSize,
                           (void*)dest.offset);
  }

  ///////////////////// Texture upload

  // These are blocking
  void updateTexture1D(Texture1DHandle::pointer handle,
                       const Texture1DInfo& info, unsigned mipLevel,
                       Box1D region, gsl::span<const gsl::byte> data);
  void updateTexture2D(Texture2DHandle::pointer handle,
                       const Texture2DInfo& info, unsigned mipLevel,
                       Box2D region, gsl::span<const gsl::byte> data);
  void updateTexture3D(Texture3DHandle::pointer handle,
                       const Texture3DInfo& info, unsigned mipLevel,
                       Box3D region, gsl::span<const gsl::byte> data);
  void readTexture1D(Texture1DHandle::pointer handle, const Texture1DInfo& info,
                     unsigned mipLevel, Box1D region,
                     gsl::span<gsl::byte> outData);
  void readTexture2D(Texture2DHandle::pointer handle, const Texture2DInfo& info,
                     unsigned mipLevel, Box2D region,
                     gsl::span<gsl::byte> outData);
  void readTexture3D(Texture3DHandle::pointer handle, const Texture3DInfo& info,
                     unsigned mipLevel, Box3D region,
                     gsl::span<gsl::byte> outData);

  // staged texture upload: copy buffer data to texture
  /*void copyTextureRegion1D(Texture1DHandle::pointer src_handle, Box1D
     src_region, PixelFormat src_format,
          Texture1DHandle::pointer dest_handle, unsigned dest_offset,
     PixelFormat dest_format);*/

  ///////////////////// Draw calls
  void draw(PrimitiveType primitiveType, unsigned first, unsigned count);
  void drawIndexed(PrimitiveType primitiveType, unsigned first, unsigned count,
                   unsigned baseVertex);

  ///////////////////// Compute
  void dispatchCompute(unsigned threadGroupCountX, unsigned threadGroupCountY,
                       unsigned threadGroupCountZ);

  void swapBuffers();

private:
  // TODO pImpl?
  void bindFramebufferObject(GLuint framebuffer_obj);
  void bindState();

  GLuint createProgramFromShaderPipeline(const GraphicsPipelineInfo& info);
  GLuint createComputeProgram(const ComputePipelineInfo& info);
  GLuint createVertexArrayObject(gsl::span<const VertexAttribute> attribs);

  struct BindState {
    std::array<GLuint, kMaxVertexBufferSlots> vertexBuffers;
    std::array<GLintptr, kMaxVertexBufferSlots> vertexBufferOffsets;
    std::array<GLsizei, kMaxVertexBufferSlots> vertexBufferStrides;
    bool vertexBuffersUpdated = false;
    std::array<GLuint, kMaxTextureUnits> textures;
    bool textureUpdated = false;
    std::array<GLuint, kMaxTextureUnits> samplers;
    bool samplersUpdated = false;
    std::array<GLuint, kMaxImageUnits> images;
    bool imagesUpdated = false;
    std::array<GLuint, kMaxUniformBufferSlots> uniformBuffers;
    std::array<GLsizeiptr, kMaxUniformBufferSlots> uniformBufferSizes;
    std::array<GLintptr, kMaxUniformBufferSlots> uniformBufferOffsets;
    bool uniformBuffersUpdated = false;
    std::array<GLuint, kMaxShaderStorageBufferSlots> shaderStorageBuffers;
    bool shaderStorageBuffersUpdated = false;
    GLuint indexBuffer;
    GLenum indexBufferType;
  };

  // last bound FBO
  GLuint last_framebuffer_obj;
  GLuint render_to_texture_fbo;
  GLFWwindow* window;
  // bind state
  BindState bind_state;
};
}
}

#endif // !OPENGL_BACKEND_HPP
