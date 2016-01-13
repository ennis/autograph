#ifndef OPENGL_BACKEND_HPP
#define OPENGL_BACKEND_HPP

#include <stdexcept>
#include <deque>
#include <array>

#include <glm/glm.hpp>
#include <optional.hpp>
#include <format.h>
#include <gl_core_4_5.hpp>
#include <GLFW/glfw3.h>
#include <gsl.h>

#include <Utils.hpp>
#include <Surface.hpp>
#include <Texture.hpp>
#include <Device.hpp>
#include <Draw.hpp>

#include "OpenGLPixelType.hpp"
#include "State.hpp"

namespace ag
{
	namespace opengl
	{
		// Data access hints
		enum class DataAccessHints : unsigned
		{
			None = 0,
			GPURead = 1 << 0,
			CPURead = 1 << 1
		};

		// Surface usage hints
		enum class SurfaceUsageHints : unsigned
		{
			None = 0,
			DefaultFramebuffer = 1 << 0,
		};
	}
}

// enable the two previous enums to be used as bitflags
template <> struct is_enum_flags<ag::opengl::SurfaceUsageHints> : public std::true_type {};
template <> struct is_enum_flags<ag::opengl::DataAccessHints> : public std::true_type {};

namespace ag
{
	namespace opengl
	{
        // Wrapper to use GLuint as a unique_ptr handle type
        // http://stackoverflow.com/questions/6265288/unique-ptr-custom-storage-type-example/6272139#6272139
        // TODO move this in a shared header
        struct GLuintHandle {
            GLuint id;

            GLuintHandle(GLuint obj_id) : id(obj_id) { }
            // default and nullptr constructors folded together
            GLuintHandle(std::nullptr_t = nullptr) : id(0) { }
            explicit operator bool() { return id != 0; }
            friend bool operator ==(GLuintHandle l, GLuintHandle r) { return l.id == r.id; }
            friend bool operator !=(GLuintHandle l, GLuintHandle r) { return !(l == r); }
            // default copy ctor and operator= are fine
            // explicit nullptr assignment and comparison unneeded
            // because of implicit nullptr constructor
            // swappable requirement fulfilled by std::swap
        };

		struct VertexAttribute
		{
			unsigned slot;
			GLenum type;
			unsigned size;
			unsigned stride;
			bool normalized;
		};

		struct GraphicsPipelineInfo
		{
			const char* VSSource = nullptr;
			const char* GSSource = nullptr;
			const char* PSSource = nullptr;
			const char* DSSource = nullptr;
			const char* HSSource = nullptr;
			gsl::span<VertexAttribute> vertexAttribs;
			GLRasterizerState rasterizerState;
			GLDepthStencilState depthStencilState;
			GLBlendState blendState;
		};

		struct ComputePipelineInfo
		{
			const char* CSSource = nullptr;
		};

		// Graphics context (OpenGL)
		struct OpenGLBackend
		{
			///////////////////// Timeout values for fence wait operations
			static constexpr unsigned kFenceWaitTimeout = 2000000000;	// in nanoseconds

			///////////////////// Alignment constraints for buffers
			static constexpr unsigned kBufferAlignment = 64;
			static constexpr unsigned kUniformBufferOffsetAlignment = 256;	// TODO do not hardcode this

			///////////////////// arbitrary binding limits
			static constexpr unsigned kMaxTextureUnits = 8;
			static constexpr unsigned kMaxImageUnits = 8;
			static constexpr unsigned kMaxVertexBufferSlots = 8;
			static constexpr unsigned kMaxUniformBufferSlots = 8;
			static constexpr unsigned kMaxShaderStorageBufferSlots = 8;

			struct GraphicsPipeline
			{
				GLuint vao = 0;
				GLuint program = 0;
				GLRasterizerState rasterizerState;
				GLDepthStencilState depthStencilState;
				GLBlendState blendState;
			};

            struct ComputePipeline
            {
                GLuint program = 0;
            };

			struct GLFence
			{
				struct SyncPoint
				{
					GLsync sync;
					uint64_t targetValue;
				};

				uint64_t currentValue;
				std::deque<SyncPoint> syncPoints;
			};

			struct GLbuffer
			{
				GLuint buf_obj;
				BufferUsage usage;
			};

            ///////////////////// Deleters
            struct SamplerDeleter
            {
                using pointer = GLuintHandle;
                void operator()(pointer sampler_obj) {
                    gl::DeleteSamplers(1, &sampler_obj.id);
                }
            };

            struct TextureDeleter
            {
                using pointer = GLuintHandle;
                void operator()(pointer tex_obj) {
                    gl::DeleteTextures(1, &tex_obj.id);
                }
            };

            struct BufferDeleter
            {
                using pointer = GLbuffer*;
                void operator()(pointer buffer) {
                    gl::DeleteBuffers(1, &buffer->buf_obj);
                    delete buffer;
                }
            };

            struct GraphicsPipelineDeleter
            {
                using pointer = GraphicsPipeline*;
                void operator()(pointer pp) {
                    if (pp->vao)
                        gl::DeleteVertexArrays(1, &pp->vao);
                    if (pp->program)
                        gl::DeleteProgram(pp->program);
                    delete pp;

                }
            };

            struct ComputePipelineDeleter
            {
                using pointer = ComputePipeline*;
                void operator()(pointer pp) {
                    if (pp->program)
                        gl::DeleteProgram(pp->program);
                    delete pp;
                }
            };

            struct FenceDeleter
            {
                using pointer = GLFence*;
                void operator()(pointer fence)
                {
                    for (auto s : fence->syncPoints)
                        gl::DeleteSync(s.sync);
                    delete fence;
                }
            };


			///////////////////// associated types
			// buffer handles
            using BufferHandle = std::unique_ptr<void,BufferDeleter>;
			// texture handles
            using Texture1DHandle = std::unique_ptr<void,TextureDeleter>;
            using Texture2DHandle = std::unique_ptr<void,TextureDeleter>;
            using Texture3DHandle = std::unique_ptr<void,TextureDeleter>;
			// sampler handles
            using SamplerHandle = std::unique_ptr<void,SamplerDeleter>;
			// surface handles
            using SurfaceHandle = std::unique_ptr<void,TextureDeleter>;
			// graphics pipeline
            using GraphicsPipelineHandle = std::unique_ptr<void,GraphicsPipelineDeleter>;
            using ComputePipelineHandle = std::unique_ptr<void,ComputePipelineDeleter>;
			// fence handle
            using FenceHandle = std::unique_ptr<void,FenceDeleter>;

			// constructor
			OpenGLBackend();

			// create a swap chain to draw into (color buffer + depth buffer)
			void createWindow(const DeviceOptions& options);
			bool processWindowEvents();


			// textures

			// binds!

			// texturenD::getView() -> GPUAsync<TextureView>
			// buffer<T>::getView() -> GPUAsync<...>

			SurfaceHandle initOutputSurface();

			///////////////////// Resources: Textures
			template <typename TPixel> 
			Texture1DHandle createTexture1D(const Texture1DInfo& info);
			template <typename TPixel> 
			Texture2DHandle createTexture2D(const Texture2DInfo& info);
			template <typename TPixel> 
			Texture3DHandle createTexture3D(const Texture3DInfo& info);

			// used internally
            /*void destroyTexture1D(Texture1DHandle detail, const Texture1DInfo& info);
			void destroyTexture2D(Texture2DHandle detail, const Texture2DInfo& info);
            void destroyTexture3D(Texture3DHandle detail, const Texture3DInfo& info);*/

			///////////////////// Resources: Buffers
			BufferHandle createBuffer(std::size_t size, const void* data, BufferUsage usage);
			// used internally
            //void destroyBuffer(BufferHandle handle);
			// Map buffer data into the CPU virtual address space
            void* mapBuffer(BufferHandle::pointer handle, size_t offset, size_t size);

			///////////////////// Resources: Samplers
			SamplerHandle createSampler(const SamplerInfo& info);
			// used internally
            //void destroySampler(SamplerHandle::pointer detail);

			///////////////////// Resources: Pipelines
			GraphicsPipelineHandle createGraphicsPipeline(const GraphicsPipelineInfo& info);
            ComputePipelineHandle createComputePipeline(const ComputePipelineInfo& info);
			// used internally
            //void destroyGraphicsPipeline(GraphicsPipelineHandle handle);

			///////////////////// Resources: fences
			FenceHandle createFence(uint64_t initialValue);
            void signal(FenceHandle::pointer fence, uint64_t value);	// insert into GPU command stream
            void signalCPU(FenceHandle::pointer fence, uint64_t value);	// CPU-side signal
            uint64_t getFenceValue(FenceHandle::pointer handle);
            //void destroyFence(FenceHandle handle);
            void waitForFence(FenceHandle::pointer handle, uint64_t value);

			///////////////////// Bind
            void bindTexture1D(unsigned slot, Texture1DHandle::pointer handle);
            void bindTexture2D(unsigned slot, Texture2DHandle::pointer handle);
            void bindTexture3D(unsigned slot, Texture3DHandle::pointer handle);
            void bindSampler(unsigned slot, SamplerHandle::pointer handle);
            void bindVertexBuffer(unsigned slot, BufferHandle::pointer handle, size_t offset, size_t size, unsigned stride);
            void bindUniformBuffer(unsigned slot, BufferHandle::pointer handle, size_t offset, size_t size);
            void bindSurface(SurfaceHandle::pointer handle);
            void bindGraphicsPipeline(GraphicsPipelineHandle::pointer handle);

			///////////////////// Clear command
            void clearColor(SurfaceHandle::pointer framebuffer_obj, const glm::vec4& color);
            void clearDepth(SurfaceHandle::pointer framebuffer_obj, float depth);

			///////////////////// Draw calls
			void draw(PrimitiveType primitiveType, unsigned first, unsigned count);

			void swapBuffers();

		private:
			// TODO pImpl?
			void bindFramebufferObject(GLuint framebuffer_obj);
            void bindState();

            Texture1DHandle createTexture1D(unsigned dimensions, GLenum internalFormat);
            Texture2DHandle createTexture2D(glm::uvec2 dimensions, GLenum internalFormat);
            Texture3DHandle createTexture3D(glm::uvec3 dimensions, GLenum internalFormat);
			void destroyTexture(GLuint tex_obj);

			GLuint createProgramFromShaderPipeline(const GraphicsPipelineInfo& info);
            GLuint createComputeProgram(const ComputePipelineInfo& info);
			GLuint createVertexArrayObject(gsl::span<VertexAttribute> attribs);

			struct BindState
			{
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
			};

			// last bound FBO
			GLuint last_framebuffer_obj;
			GLFWwindow* window;
			// bind state
			BindState bind_state;
		};

		template<typename TPixel>
		inline OpenGLBackend::Texture1DHandle OpenGLBackend::createTexture1D(const Texture1DInfo& info)
		{
			return createTexture1D(info.dimensions, OpenGLPixelTypeTraits<TPixel>::InternalFormat);
		}

		template<typename TPixel>
		inline OpenGLBackend::Texture2DHandle OpenGLBackend::createTexture2D(const Texture2DInfo& info)
		{
			return createTexture2D(info.dimensions, OpenGLPixelTypeTraits<TPixel>::InternalFormat);
		}

		template<typename TPixel>
		inline OpenGLBackend::Texture3DHandle OpenGLBackend::createTexture3D(const Texture3DInfo& info)
		{
			return createTexture3D(info.dimensions, OpenGLPixelTypeTraits<TPixel>::InternalFormat);
		}
}
}

#endif // !OPENGL_BACKEND_HPP
