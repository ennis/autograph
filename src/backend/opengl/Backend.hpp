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
#include <SharedResource.hpp>

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


			///////////////////// associated types
			// buffer handles
			using BufferHandle = GLbuffer*;
			// texture handles
			using Texture1DHandle = GLuint;
			using Texture2DHandle = GLuint;
			using Texture3DHandle = GLuint;
			// sampler handles
			using SamplerHandle = GLuint;
			// surface handles
			using SurfaceHandle = GLuint;
			// graphics pipeline
			using GraphicsPipelineHandle = GraphicsPipeline*;
			// fence handle
			using FenceHandle = GLFence*;

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
			void destroyTexture1D(Texture1DHandle detail, const Texture1DInfo& info);
			void destroyTexture2D(Texture2DHandle detail, const Texture2DInfo& info);
			void destroyTexture3D(Texture3DHandle detail, const Texture3DInfo& info);

			///////////////////// Resources: Buffers
			BufferHandle createBuffer(std::size_t size, const void* data, BufferUsage usage);
			// used internally
			void destroyBuffer(BufferHandle handle);
			// Map buffer data into the CPU virtual address space
			void* mapBuffer(BufferHandle handle, size_t offset, size_t size);

			///////////////////// Resources: Samplers
			SamplerHandle createSampler(const SamplerInfo& info);
			// used internally
			void destroySampler(SamplerHandle detail);

			///////////////////// Resources: Pipelines
			GraphicsPipelineHandle createGraphicsPipeline(const GraphicsPipelineInfo& info);
			// used internally
			void destroyGraphicsPipeline(GraphicsPipelineHandle handle);

			///////////////////// Resources: fences
			FenceHandle createFence(uint64_t initialValue);
			void signal(FenceHandle fence, uint64_t value);	// insert into GPU command stream
			void signalCPU(FenceHandle fence, uint64_t value);	// CPU-side signal
			uint64_t getFenceValue(FenceHandle handle);
			void destroyFence(FenceHandle handle);
			void waitForFence(FenceHandle handle, uint64_t value);

			///////////////////// Bind
			void bindTexture1D(unsigned slot, Texture1DHandle handle);
			void bindTexture2D(unsigned slot, Texture2DHandle handle);
			void bindTexture3D(unsigned slot, Texture3DHandle handle);
			void bindSampler(unsigned slot, SamplerHandle handle);
			void bindVertexBuffer(unsigned slot, BufferHandle handle, size_t offset, size_t size, unsigned stride);
			void bindUniformBuffer(unsigned slot, BufferHandle handle, size_t offset, size_t size);
			void bindSurface(SurfaceHandle handle);
			void bindGraphicsPipeline(GraphicsPipelineHandle handle);

			///////////////////// Clear command
			void clearColor(SurfaceHandle framebuffer_obj, const glm::vec4& color);
			void clearDepth(SurfaceHandle framebuffer_obj, float depth);

			///////////////////// Draw calls
			void draw(PrimitiveType primitiveType, unsigned first, unsigned count);

			void swapBuffers();

		private:
			// TODO pImpl?
			void bindFramebufferObject(GLuint framebuffer_obj);
            void bindState();

			GLuint createTexture1D(unsigned dimensions, GLenum internalFormat);
			GLuint createTexture2D(glm::uvec2 dimensions, GLenum internalFormat);
			GLuint createTexture3D(glm::uvec3 dimensions, GLenum internalFormat);
			void destroyTexture(GLuint tex_obj);

			GLuint createProgramFromShaderPipeline(const GraphicsPipelineInfo& info);
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
