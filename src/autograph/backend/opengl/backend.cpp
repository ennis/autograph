#include "backend.hpp"

#include <algorithm>
#include <iostream>
#include <ostream>
#include <sstream>

#include <format.h>

#include "../../error.hpp"

namespace ag {
namespace opengl {
namespace {
GLFWwindow* createGlfwWindow(const DeviceOptions& options) {
  GLFWwindow* window;
  if (!glfwInit())
    return nullptr;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, gl::TRUE_);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, gl::TRUE_);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow(options.framebufferWidth, options.framebufferHeight,
                            "Painter", NULL, NULL);
  if (!window) {
    glfwTerminate();
    failWith("Could not create OpenGL context");
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  if (!gl::sys::LoadFunctions()) {
    glfwTerminate();
    failWith("Failed to load OpenGL functions");
  }

  std::clog << "OpenGL context information:\n"
            << "\tVersion string: " << gl::GetString(gl::VERSION)
            << "\n\tRenderer: " << gl::GetString(gl::RENDERER) << "\n";

  return window;
}

// debug output callback
void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length, const GLubyte* msg,
                            void* data) {
  if (severity != gl::DEBUG_SEVERITY_LOW &&
      severity != gl::DEBUG_SEVERITY_NOTIFICATION)
    std::clog << "(GL) " << msg << std::endl;
}

void setDebugCallback() {
  gl::Enable(gl::DEBUG_OUTPUT_SYNCHRONOUS);
  gl::DebugMessageCallback((GLDEBUGPROC)debugCallback, nullptr);
  gl::DebugMessageControl(gl::DONT_CARE, gl::DONT_CARE, gl::DONT_CARE, 0,
                          nullptr, true);
  gl::DebugMessageInsert(gl::DEBUG_SOURCE_APPLICATION, gl::DEBUG_TYPE_MARKER,
                         1111, gl::DEBUG_SEVERITY_NOTIFICATION, -1,
                         "Started logging OpenGL messages");
}

GLenum textureAddressModeToGLenum(TextureAddressMode mode) {
  switch (mode) {
  case TextureAddressMode::Clamp:
    return gl::CLAMP_TO_EDGE;
  case TextureAddressMode::Mirror:
    return gl::MIRRORED_REPEAT;
  case TextureAddressMode::Repeat:
    return gl::REPEAT;
  default:
    return gl::REPEAT;
  }
}

GLenum textureFilterToGLenum(TextureFilter filter) {
  switch (filter) {
  case TextureFilter::Nearest:
    return gl::NEAREST;
  case TextureFilter::Linear:
    return gl::LINEAR;
  default:
    return gl::NEAREST;
  }
}

GLenum primitiveTypeToGLenum(PrimitiveType primitiveType) {
  switch (primitiveType) {
  case PrimitiveType::Triangles:
    return gl::TRIANGLES;
  case PrimitiveType::Lines:
    return gl::LINES;
  case PrimitiveType::Points:
    return gl::POINTS;
  default:
    return gl::POINTS;
  }
}

GLPixelFormat pixelFormatToGL(PixelFormat format) {
  switch (format) {
  case PixelFormat::Uint8:
    return GLPixelFormat{gl::R8UI, gl::UNSIGNED_BYTE, 1};
  case PixelFormat::Uint8x2:
    return GLPixelFormat{gl::RG8UI, gl::UNSIGNED_BYTE, 2};
  case PixelFormat::Uint8x3:
    return GLPixelFormat{gl::RGB8UI, gl::UNSIGNED_BYTE, 3};
  case PixelFormat::Uint8x4:
    return GLPixelFormat{gl::RGBA8UI, gl::UNSIGNED_BYTE, 4};
  case PixelFormat::Unorm8:
    return GLPixelFormat{gl::R8, gl::UNSIGNED_BYTE, 1};
  case PixelFormat::Unorm8x2:
    return GLPixelFormat{gl::RG8, gl::UNSIGNED_BYTE, 2};
  case PixelFormat::Unorm8x3:
    return GLPixelFormat{gl::RGB8, gl::UNSIGNED_BYTE, 3};
  case PixelFormat::Unorm8x4:
    return GLPixelFormat{gl::RGBA8, gl::UNSIGNED_BYTE, 4};
  case PixelFormat::Float:
    return GLPixelFormat{gl::R32F, gl::FLOAT, 1};
  default:
    failWith("TODO");
  }
}

GLuint compileShader(GLenum stage, const char* source, std::ostream& infoLog) {
  GLuint obj = gl::CreateShader(stage);
  const char* shaderSources[1] = {source};
  gl::ShaderSource(obj, 1, shaderSources, NULL);
  gl::CompileShader(obj);
  GLint status = gl::TRUE_;
  GLint logsize = 0;
  gl::GetShaderiv(obj, gl::COMPILE_STATUS, &status);
  gl::GetShaderiv(obj, gl::INFO_LOG_LENGTH, &logsize);
  if (status != gl::TRUE_) {
    if (logsize != 0) {
      char* logbuf = new char[logsize];
      gl::GetShaderInfoLog(obj, logsize, &logsize, logbuf);
      infoLog << logbuf;
      delete[] logbuf;
      gl::DeleteShader(obj);
    }
    return 0;
  }
  return obj;
}

bool linkProgram(GLuint program, std::ostream& infoLog) {
  GLint status = gl::TRUE_;
  GLint logsize = 0;
  gl::LinkProgram(program);
  gl::GetProgramiv(program, gl::LINK_STATUS, &status);
  gl::GetProgramiv(program, gl::INFO_LOG_LENGTH, &logsize);
  if (status != gl::TRUE_) {
    if (logsize != 0) {
      char* logbuf = new char[logsize];
      gl::GetProgramInfoLog(program, logsize, &logsize, logbuf);
      infoLog << logbuf;
      delete[] logbuf;
    }
    return true;
  }
  return false;
}
}

// constructor

OpenGLBackend::OpenGLBackend() : last_framebuffer_obj(0) {
  bind_state.indexBuffer = 0;
  bind_state.vertexBuffers.fill(0);
  bind_state.images.fill(0);
  bind_state.textures.fill(0);
  bind_state.samplers.fill(0);
  bind_state.shaderStorageBuffers.fill(0);
  bind_state.uniformBuffers.fill(0);
  bind_state.uniformBufferSizes.fill(0);
  bind_state.uniformBufferOffsets.fill(0);
  // nothing to do, the context is created on window creation
}

// create a swap chain to draw into (color buffer + depth buffer)
void OpenGLBackend::createWindow(const DeviceOptions& options) {
  window = createGlfwWindow(options);
  setDebugCallback();
  gl::CreateFramebuffers(1, &render_to_texture_fbo);
}

bool OpenGLBackend::processWindowEvents() {
  return !!glfwWindowShouldClose(window);
}

OpenGLBackend::SurfaceHandle OpenGLBackend::initOutputSurface() { return 0; }

OpenGLBackend::BufferHandle OpenGLBackend::createBuffer(std::size_t size,
                                                        const void* data,
                                                        BufferUsage usage) {
  GLbitfield flags = 0;
  if (usage == BufferUsage::Readback) {
    flags |= gl::MAP_READ_BIT | gl::MAP_PERSISTENT_BIT | gl::MAP_COHERENT_BIT;
  } else if (usage == BufferUsage::Upload) {
    flags |= gl::MAP_WRITE_BIT | gl::MAP_PERSISTENT_BIT | gl::MAP_COHERENT_BIT;
  } else {
    // default, no client access, update through copy engines
    flags = 0;
  }

  GLuint buf_obj;
  gl::CreateBuffers(1, &buf_obj);
  gl::NamedBufferStorage(buf_obj, size, data, flags);
  return BufferHandle(new GLbuffer{buf_obj, usage}, BufferDeleter());
}

void* OpenGLBackend::mapBuffer(BufferHandle::pointer handle, size_t offset,
                               size_t size) {
  // all our operations are unsynchronized
  GLbitfield flags = gl::MAP_UNSYNCHRONIZED_BIT;
  if (handle->usage == BufferUsage::Readback) {
    flags |= gl::MAP_READ_BIT | gl::MAP_PERSISTENT_BIT | gl::MAP_COHERENT_BIT;
  } else if (handle->usage == BufferUsage::Upload) {
    flags |= gl::MAP_WRITE_BIT | gl::MAP_PERSISTENT_BIT | gl::MAP_COHERENT_BIT;
  } else {
    // cannot map a DEFAULT buffer
    throw std::logic_error(
        "Trying to map a buffer allocated with BufferUsage::Default");
  }
  return gl::MapNamedBufferRange(handle->buf_obj, offset, size, flags);
}

void OpenGLBackend::bindTexture1D(unsigned slot,
                                  Texture1DHandle::pointer handle) {
  assert(slot < kMaxTextureUnits);
  if (bind_state.textures[slot] != handle.id) {
    bind_state.textures[slot] = handle.id;
    bind_state.textureUpdated = true;
  }
}

void OpenGLBackend::bindTexture2D(unsigned slot,
                                  Texture2DHandle::pointer handle) {
  assert(slot < kMaxTextureUnits);
  if (bind_state.textures[slot] != handle.id) {
    bind_state.textures[slot] = handle.id;
    bind_state.textureUpdated = true;
  }
}

void OpenGLBackend::bindTexture3D(unsigned slot,
                                  Texture3DHandle::pointer handle) {
  assert(slot < kMaxTextureUnits);
  if (bind_state.textures[slot] != handle.id) {
    bind_state.textures[slot] = handle.id;
    bind_state.textureUpdated = true;
  }
}

void OpenGLBackend::bindSampler(unsigned slot, SamplerHandle::pointer handle) {
  assert(slot < kMaxTextureUnits);
  if (bind_state.samplers[slot] != handle.id) {
    bind_state.samplers[slot] = handle.id;
    bind_state.samplersUpdated = true;
  }
}

OpenGLBackend::SamplerHandle
OpenGLBackend::createSampler(const SamplerInfo& info) {
  GLuint sampler_obj;
  gl::CreateSamplers(1, &sampler_obj);
  gl::SamplerParameteri(sampler_obj, gl::TEXTURE_MIN_FILTER,
                        textureFilterToGLenum(info.minFilter));
  gl::SamplerParameteri(sampler_obj, gl::TEXTURE_MAG_FILTER,
                        textureFilterToGLenum(info.magFilter));
  gl::SamplerParameteri(sampler_obj, gl::TEXTURE_WRAP_R,
                        textureAddressModeToGLenum(info.addrU));
  gl::SamplerParameteri(sampler_obj, gl::TEXTURE_WRAP_S,
                        textureAddressModeToGLenum(info.addrV));
  gl::SamplerParameteri(sampler_obj, gl::TEXTURE_WRAP_T,
                        textureAddressModeToGLenum(info.addrW));
  return SamplerHandle{GLuintHandle(sampler_obj), SamplerDeleter()};
}

const char* getShaderStageName(GLenum stage) {
  switch (stage) {
  case gl::VERTEX_SHADER:
    return "VERTEX_SHADER";
  case gl::FRAGMENT_SHADER:
    return "FRAGMENT_SHADER";
  case gl::GEOMETRY_SHADER:
    return "GEOMETRY_SHADER";
  case gl::TESS_CONTROL_SHADER:
    return "TESS_CONTROL_SHADER";
  case gl::TESS_EVALUATION_SHADER:
    return "TESS_EVALUATION_SHADER";
  case gl::COMPUTE_SHADER:
    return "COMPUTE_SHADER";
  default:
    return "<invalid>";
  }
}

GLuint compileAndAttachShader(GLuint program_obj, GLenum stage,
                              const char* source) {
  std::ostringstream infoLog;
  GLuint shader_obj = compileShader(stage, source, infoLog);
  if (!shader_obj) {
    fmt::print(
        "===============================================================\n");
    fmt::print("Shader compilation error (stage: {})\n",
               getShaderStageName(stage));
    fmt::print("Compilation log follows:\n\n{}\n\n", infoLog.str());
    return 0;
  }
  gl::AttachShader(program_obj, shader_obj);
  return shader_obj;
}

OpenGLBackend::GraphicsPipelineHandle
OpenGLBackend::createGraphicsPipeline(const GraphicsPipelineInfo& info) {
  auto pp = new GraphicsPipeline;
  pp->program = createProgramFromShaderPipeline(info);
  pp->vao = createVertexArrayObject(info.vertexAttribs);
  pp->blendState = info.blendState;
  pp->depthStencilState = info.depthStencilState;
  pp->rasterizerState = info.rasterizerState;
  return GraphicsPipelineHandle(pp, GraphicsPipelineDeleter());
}

OpenGLBackend::ComputePipelineHandle
OpenGLBackend::createComputePipeline(const ComputePipelineInfo& info) {
  auto pp = new ComputePipeline;
  pp->program = createComputeProgram(info);
  return ComputePipelineHandle(pp, ComputePipelineDeleter());
}

OpenGLBackend::FenceHandle OpenGLBackend::createFence(uint64_t initialValue) {
  auto f = new GLFence;
  f->currentValue = initialValue;
  return FenceHandle(f, FenceDeleter());
}

// This should be a command list API
void OpenGLBackend::signal(FenceHandle::pointer fence, uint64_t value) {
  auto sync = gl::FenceSync(gl::SYNC_GPU_COMMANDS_COMPLETE, 0);
  fence->syncPoints.push_back(GLFence::SyncPoint{sync, value});
}

void OpenGLBackend::signalCPU(FenceHandle::pointer fence, uint64_t value) {
  // unimplemented (useful only for multithreaded applications?)
}

GLenum advanceFence(OpenGLBackend::FenceHandle::pointer handle,
                    uint64_t timeout) {
  auto& targetSyncPoint = handle->syncPoints.front();
  auto waitResult = gl::ClientWaitSync(targetSyncPoint.sync,
                                       gl::SYNC_FLUSH_COMMANDS_BIT, timeout);
  if (waitResult == gl::CONDITION_SATISFIED ||
      waitResult == gl::ALREADY_SIGNALED) {
    handle->currentValue = targetSyncPoint.targetValue;
    gl::DeleteSync(targetSyncPoint.sync);
    handle->syncPoints.pop_front();
  } else if (waitResult == gl::WAIT_FAILED_)
    failWith(fmt::format("Wait failed while waiting for fence (target {})",
                         targetSyncPoint.targetValue));
  return waitResult;
}

uint64_t OpenGLBackend::getFenceValue(FenceHandle::pointer handle) {
  while (!handle->syncPoints.empty()) {
    auto waitResult = advanceFence(handle, 0);
    if (waitResult == gl::TIMEOUT_EXPIRED)
      break;
  }
  return handle->currentValue;
}

void OpenGLBackend::waitForFence(FenceHandle::pointer handle, uint64_t value) {
  while (getFenceValue(handle) < value) {
    auto waitResult = advanceFence(handle, kFenceWaitTimeout);
    if (waitResult == gl::TIMEOUT_EXPIRED)
      failWith(
          fmt::format("Timeout expired while waiting for fence (target {})",
                      handle->syncPoints.front().targetValue));
  }
}

GLuint OpenGLBackend::createComputeProgram(const ComputePipelineInfo& info) {
  GLuint cs_obj = 0;
  GLuint program_obj = gl::CreateProgram();
  cs_obj =
      compileAndAttachShader(program_obj, gl::COMPUTE_SHADER, info.CSSource);
  if (!cs_obj) {
    gl::DeleteProgram(program_obj);
    return 0;
  }
  std::ostringstream linkInfoLog;
  bool link_error = linkProgram(program_obj, linkInfoLog);
  gl::DetachShader(program_obj, cs_obj);
  gl::DeleteShader(cs_obj);
  if (link_error) {
    fmt::print(
        "===============================================================\n");
    fmt::print("Shader link error\n");
    fmt::print("Compilation log follows:\n\n{}\n\n", linkInfoLog.str());
    gl::DeleteProgram(program_obj);
    program_obj = 0;
  }
  return program_obj;
}

GLuint OpenGLBackend::createProgramFromShaderPipeline(
    const GraphicsPipelineInfo& info) {
  // compile programs
  GLuint vs_obj = 0;
  GLuint fs_obj = 0;
  GLuint gs_obj = 0;
  GLuint tcs_obj = 0;
  GLuint tes_obj = 0;
  GLuint program_obj = gl::CreateProgram();
  bool compilation_error = false;
  vs_obj =
      compileAndAttachShader(program_obj, gl::VERTEX_SHADER, info.VSSource);
  if (!vs_obj)
    compilation_error = true;
  fs_obj =
      compileAndAttachShader(program_obj, gl::FRAGMENT_SHADER, info.PSSource);
  if (!fs_obj)
    compilation_error = true;
  if (info.GSSource) {
    gs_obj =
        compileAndAttachShader(program_obj, gl::GEOMETRY_SHADER, info.GSSource);
    if (!gs_obj)
      compilation_error = true;
  }
  if (info.DSSource) {
    tes_obj = compileAndAttachShader(program_obj, gl::TESS_EVALUATION_SHADER,
                                     info.DSSource);
    if (!tes_obj)
      compilation_error = true;
  }
  if (info.HSSource) {
    tcs_obj = compileAndAttachShader(program_obj, gl::TESS_CONTROL_SHADER,
                                     info.HSSource);
    if (!tcs_obj)
      compilation_error = true;
  }

  bool link_error = false;
  if (!compilation_error) {
    std::ostringstream linkInfoLog;
    link_error = linkProgram(program_obj, linkInfoLog);
    if (link_error) {
      fmt::print(
          "===============================================================\n");
      fmt::print("Shader link error\n");
      fmt::print("Compilation log follows:\n\n{}\n\n", linkInfoLog.str());
    }
  }

  if (vs_obj)
    gl::DeleteShader(vs_obj);
  if (fs_obj)
    gl::DeleteShader(fs_obj);
  if (gs_obj)
    gl::DeleteShader(gs_obj);
  if (tcs_obj)
    gl::DeleteShader(tcs_obj);
  if (tes_obj)
    gl::DeleteShader(tes_obj);
  if (link_error) {
    gl::DeleteProgram(program_obj);
    program_obj = 0;
  }

  return program_obj;
}

GLuint OpenGLBackend::createVertexArrayObject(
    gsl::span<const VertexAttribute> attribs) {
  GLuint strides[OpenGLBackend::kMaxVertexBufferSlots] = {0};
  GLuint vertex_array_obj;
  gl::CreateVertexArrays(1, &vertex_array_obj);
  for (int attribindex = 0; attribindex < attribs.size(); ++attribindex) {
    const auto& a = attribs[attribindex];
    assert(a.slot < OpenGLBackend::kMaxVertexBufferSlots);
    gl::EnableVertexArrayAttrib(vertex_array_obj, attribindex);
    gl::VertexArrayAttribFormat(vertex_array_obj, attribindex, a.size, a.type,
                                a.normalized, strides[a.slot]);
    gl::VertexArrayAttribBinding(vertex_array_obj, attribindex, a.slot);
    strides[a.slot] += a.stride;
  }
  gl::BindVertexArray(0);
  return vertex_array_obj;
}

///////////////////// Clear command

void OpenGLBackend::bindVertexBuffer(unsigned slot,
                                     BufferHandle::pointer handle,
                                     size_t offset, size_t size,
                                     unsigned stride) {
  bind_state.vertexBuffers[slot] = handle->buf_obj;
  bind_state.vertexBufferOffsets[slot] = offset;
  bind_state.vertexBufferStrides[slot] = stride;
  bind_state.vertexBuffersUpdated = true;
}

void OpenGLBackend::bindIndexBuffer(BufferHandle::pointer handle, size_t offset,
                                    size_t size, IndexType type) {
  if (type == IndexType::UShort)
    bind_state.indexBufferType = gl::UNSIGNED_SHORT;
  else
    bind_state.indexBufferType = gl::UNSIGNED_INT;
  bind_state.indexBuffer = handle->buf_obj;
}

void OpenGLBackend::bindUniformBuffer(unsigned slot,
                                      BufferHandle::pointer handle,
                                      size_t offset, size_t size) {
  bind_state.uniformBuffers[slot] = handle->buf_obj;
  bind_state.uniformBufferSizes[slot] = size;
  bind_state.uniformBufferOffsets[slot] = offset;
  bind_state.uniformBuffersUpdated = true;
}

void OpenGLBackend::bindSurface(SurfaceHandle::pointer handle) {
  bindFramebufferObject(handle.id);
}

void OpenGLBackend::bindRenderTexture(unsigned slot,
                                      Texture2DHandle::pointer handle) {
  bindFramebufferObject(render_to_texture_fbo);
  gl::NamedFramebufferTexture(render_to_texture_fbo,
                              gl::COLOR_ATTACHMENT0 + slot, handle.id, 0);
}

void OpenGLBackend::bindDepthRenderTexture(Texture2DHandle::pointer handle) {
  bindFramebufferObject(render_to_texture_fbo);
  gl::NamedFramebufferTexture(render_to_texture_fbo, gl::DEPTH_ATTACHMENT,
                              handle.id, 0);
}

void OpenGLBackend::bindGraphicsPipeline(
    GraphicsPipelineHandle::pointer handle) {
  gl::UseProgram(handle->program);
  gl::BindVertexArray(handle->vao);
  // TODO optimize state changes
  if (handle->depthStencilState.depthTestEnable)
    gl::Enable(gl::DEPTH_TEST);
  else
    gl::Disable(gl::DEPTH_TEST);
}

void OpenGLBackend::clearColor(SurfaceHandle::pointer framebuffer_obj,
                               const ag::ClearColor& color) {
  // XXX: clear specific draw buffer?
  gl::ClearNamedFramebufferfv(framebuffer_obj.id, gl::COLOR, 0, color.rgba);
}

void OpenGLBackend::clearDepth(SurfaceHandle::pointer framebuffer_obj,
                               float depth) {
  gl::ClearNamedFramebufferfv(framebuffer_obj.id, gl::DEPTH, 0, &depth);
}

void OpenGLBackend::clearTexture1DFloat(Texture1DHandle::pointer handle, ag::Box1D region, const ag::ClearColor& color)
{
	gl::ClearTexImage(handle.id, 0, gl::RGBA, gl::FLOAT, color.rgba);
}

void OpenGLBackend::clearTexture2DFloat(Texture2DHandle::pointer handle, ag::Box2D region, const ag::ClearColor& color)
{
	gl::ClearTexImage(handle.id, 0, gl::RGBA, gl::FLOAT, color.rgba);
}

void OpenGLBackend::clearTexture3DFloat(Texture3DHandle::pointer handle, ag::Box3D region, const ag::ClearColor& color)
{
	gl::ClearTexImage(handle.id, 0, gl::RGBA, gl::FLOAT, color.rgba);
}

void OpenGLBackend::draw(PrimitiveType primitiveType, unsigned first,
                         unsigned count) {
  bindState();
  gl::DrawArrays(primitiveTypeToGLenum(primitiveType), first, count);
}

void OpenGLBackend::drawIndexed(PrimitiveType primitiveType, unsigned first,
                                unsigned count, unsigned baseVertex) {
  bindState();
  auto indexStride = bind_state.indexBufferType == gl::UNSIGNED_INT ? 4 : 2;
  gl::DrawElementsBaseVertex(
      primitiveTypeToGLenum(primitiveType), count, bind_state.indexBufferType,
      ((const char*)((uintptr_t)first * indexStride)), baseVertex);
}

void OpenGLBackend::dispatchCompute(unsigned threadGroupCountX,
                                    unsigned threadGroupCountY,
                                    unsigned threadGroupCountZ) {
  bindState();
  gl::DispatchCompute(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void OpenGLBackend::swapBuffers() {
  glfwSwapBuffers(window);
  glfwPollEvents();
}

void OpenGLBackend::bindFramebufferObject(GLuint framebuffer_obj) {
  if (last_framebuffer_obj != framebuffer_obj) {
    last_framebuffer_obj = framebuffer_obj;
    gl::BindFramebuffer(gl::FRAMEBUFFER, framebuffer_obj);
  }
}

void OpenGLBackend::bindState() {
  if (bind_state.vertexBuffersUpdated) {
    for (unsigned i = 0; i < kMaxVertexBufferSlots; ++i)
      if (bind_state.vertexBuffers[i])
        gl::BindVertexBuffer(i, bind_state.vertexBuffers[i],
                             bind_state.vertexBufferOffsets[i],
                             bind_state.vertexBufferStrides[i]);
      else
        gl::BindVertexBuffer(i, 0, 0, 0);
    bind_state.vertexBuffersUpdated = false;
  }
  if (bind_state.textureUpdated) {
    gl::BindTextures(0, kMaxTextureUnits, bind_state.textures.data());
    bind_state.textureUpdated = false;
  }
  if (bind_state.samplersUpdated) {
    gl::BindSamplers(0, kMaxTextureUnits, bind_state.samplers.data());
    bind_state.samplersUpdated = false;
  }
  if (bind_state.imagesUpdated) {
    gl::BindImageTextures(0, kMaxImageUnits, bind_state.images.data());
    bind_state.imagesUpdated = false;
  }
  if (bind_state.uniformBuffersUpdated) {
    for (unsigned i = 0; i < kMaxUniformBufferSlots; ++i) {
      if (bind_state.uniformBuffers[i])
        gl::BindBufferRange(gl::UNIFORM_BUFFER, i, bind_state.uniformBuffers[i],
                            bind_state.uniformBufferOffsets[i],
                            bind_state.uniformBufferSizes[i]);
      else
        gl::BindBufferBase(gl::UNIFORM_BUFFER, i, 0);
    }
    bind_state.uniformBuffersUpdated = false;
  }
  if (bind_state.indexBuffer)
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, bind_state.indexBuffer);
}

OpenGLBackend::Texture1DHandle
OpenGLBackend::createTexture1D(const Texture1DInfo& info) {
  GLuint tex_obj;
  auto glfmt = pixelFormatToGL(info.format);
  gl::CreateTextures(gl::TEXTURE_1D, 1, &tex_obj);
  gl::TextureStorage1D(tex_obj, 1, glfmt.internalFormat, info.dimensions);
  return Texture1DHandle(GLuintHandle(tex_obj), TextureDeleter());
}

OpenGLBackend::Texture2DHandle
OpenGLBackend::createTexture2D(const Texture2DInfo& info) {
  GLuint tex_obj;
  auto glfmt = pixelFormatToGL(info.format);
  gl::CreateTextures(gl::TEXTURE_2D, 1, &tex_obj);
  gl::TextureStorage2D(tex_obj, 1, glfmt.internalFormat, info.dimensions.x,
                       info.dimensions.y);
  return Texture2DHandle(GLuintHandle(tex_obj), TextureDeleter());
}

OpenGLBackend::Texture3DHandle
OpenGLBackend::createTexture3D(const Texture3DInfo& info) {
  GLuint tex_obj;
  auto glfmt = pixelFormatToGL(info.format);
  gl::CreateTextures(gl::TEXTURE_3D, 1, &tex_obj);
  gl::TextureStorage3D(tex_obj, 1, glfmt.internalFormat, info.dimensions.x,
                       info.dimensions.y, info.dimensions.z);
  return Texture3DHandle(GLuintHandle(tex_obj), TextureDeleter());
}
}
}
