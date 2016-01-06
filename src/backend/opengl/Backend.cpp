#include "Backend.hpp"

#include <iostream>
#include <ostream>
#include <sstream>

#include <format.h>

namespace ag
{
	namespace opengl
	{
		namespace
		{
			GLFWwindow* createGlfwWindow(const DeviceOptions& options)
			{
				GLFWwindow* window;
				if (!glfwInit())
					return nullptr;

				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, gl::TRUE_);
				window = glfwCreateWindow(options.framebufferWidth, options.framebufferHeight, "Painter", NULL, NULL);
				if (!window)
				{
					glfwTerminate();
					return nullptr;
				}

				glfwMakeContextCurrent(window);
				if (!gl::sys::LoadFunctions()) {
					std::cerr << "Failed to load OpenGL functions" << std::endl;
				}

				return window;
			}

			// debug output callback
			void APIENTRY debugCallback(
				GLenum source,
				GLenum type,
				GLuint id,
				GLenum severity,
				GLsizei length,
				const GLubyte *msg,
				void *data)
			{
				if (severity != gl::DEBUG_SEVERITY_LOW && severity != gl::DEBUG_SEVERITY_NOTIFICATION)
					std::clog << "(GL) " << msg << std::endl;
			}

			void setDebugCallback()
			{
				gl::Enable(gl::DEBUG_OUTPUT_SYNCHRONOUS);
				gl::DebugMessageCallback((GLDEBUGPROC)debugCallback, nullptr);
				gl::DebugMessageControl(gl::DONT_CARE, gl::DONT_CARE, gl::DONT_CARE, 0, nullptr, true);
				gl::DebugMessageInsert(
					gl::DEBUG_SOURCE_APPLICATION,
					gl::DEBUG_TYPE_MARKER,
					1111,
					gl::DEBUG_SEVERITY_NOTIFICATION, -1,
					"Started logging OpenGL messages");
			}

			GLenum textureAddressModeToGLenum(TextureAddressMode mode)
			{
				switch (mode)
				{
				case TextureAddressMode::Clamp: return gl::CLAMP_TO_EDGE;
				case TextureAddressMode::Mirror: return gl::MIRRORED_REPEAT;
				case TextureAddressMode::Repeat: return gl::REPEAT;
				default: return gl::REPEAT;
				}
			}

			GLenum textureFilterToGLenum(TextureFilter filter)
			{
				switch (filter)
				{
				case TextureFilter::Nearest: return gl::NEAREST;
				case TextureFilter::Linear: return gl::LINEAR;
				default: return gl::NEAREST;
				}
			}

			GLenum primitiveTypeToGLenum(PrimitiveType primitiveType)
			{
				switch (primitiveType)
				{
				case PrimitiveType::Triangles: return gl::TRIANGLES;
				case PrimitiveType::Lines: return gl::LINES;
				case PrimitiveType::Points: return gl::POINTS;
				default: return gl::POINTS;
				}
			}

			GLuint compileShader(GLenum stage, const char* source, std::ostream& infoLog)
			{
				GLuint obj = gl::CreateShader(stage);
				const char *shaderSources[1] = { source };
				gl::ShaderSource(obj, 1, shaderSources, NULL);
				gl::CompileShader(obj);
				GLint status = gl::TRUE_;
				GLint logsize = 0;
				gl::GetShaderiv(obj, gl::COMPILE_STATUS, &status);
				gl::GetShaderiv(obj, gl::INFO_LOG_LENGTH, &logsize);
				if (status != gl::TRUE_)
				{
					if (logsize != 0) {
						char *logbuf = new char[logsize];
						gl::GetShaderInfoLog(obj, logsize, &logsize, logbuf);
						infoLog << logbuf;
						delete[] logbuf;
						gl::DeleteShader(obj);
					}
					return 0;
				}
				return obj;
			}

			bool linkProgram(GLuint program, std::ostream& infoLog)
			{
				GLint status = gl::TRUE_;
				GLint logsize = 0;
				gl::LinkProgram(program);
				gl::GetProgramiv(program, gl::LINK_STATUS, &status);
				gl::GetProgramiv(program, gl::INFO_LOG_LENGTH, &logsize);
				if (status != gl::TRUE_) {
					if (logsize != 0) {
						char *logbuf = new char[logsize];
						gl::GetProgramInfoLog(program, logsize, &logsize, logbuf);
						infoLog << logbuf;
						delete[] logbuf;
					}
					return true;
				}
				return false;
			}
		}

		// create a swap chain to draw into (color buffer + depth buffer)
		void OpenGLBackend::createWindow(const DeviceOptions & options)
		{
			window = createGlfwWindow(options);
			setDebugCallback();
		}

		bool OpenGLBackend::processWindowEvents()
		{
			return !!glfwWindowShouldClose(window);
		}

		OpenGLBackend::SurfaceHandle OpenGLBackend::initOutputSurface()
		{
			return 0;
		}

		void OpenGLBackend::destroyTexture1D(Texture1DHandle detail, const Texture1DInfo & info)
		{
			destroyTexture(detail);
		}

		void OpenGLBackend::destroyTexture2D(Texture2DHandle detail, const Texture2DInfo & info)
		{
			destroyTexture(detail);
		}

		void OpenGLBackend::destroyTexture3D(Texture3DHandle detail, const Texture3DInfo & info)
		{
			destroyTexture(detail);
		}

		OpenGLBackend::BufferHandle OpenGLBackend::createBuffer(std::size_t size, const void * data)
		{
			// XXX is the size is small, alloc in a buffer pool
			GLuint buf_obj;
			gl::CreateBuffers(1, &buf_obj);
			gl::NamedBufferStorage(buf_obj, size, data, 0);
			return buf_obj;
		}

		void OpenGLBackend::destroyBuffer(BufferHandle handle)
		{
			gl::DeleteBuffers(1, &handle);
		}

		void OpenGLBackend::bindTexture1D(unsigned slot, Texture1DHandle handle)
		{
			assert(slot < kMaxTextureUnits);
			if (bind_state.textures[slot] != handle)
			{
				bind_state.textures[slot] = handle;
				bind_state.textureUpdated = true;
			}
		}

		void OpenGLBackend::bindTexture2D(unsigned slot, Texture2DHandle handle)
		{
			assert(slot < kMaxTextureUnits);
			if (bind_state.textures[slot] != handle)
			{
				bind_state.textures[slot] = handle;
				bind_state.textureUpdated = true;
			}
		}

		void OpenGLBackend::bindTexture3D(unsigned slot, Texture3DHandle handle)
		{
			assert(slot < kMaxTextureUnits);
			if (bind_state.textures[slot] != handle)
			{
				bind_state.textures[slot] = handle;
				bind_state.textureUpdated = true;
			}
		}

		void OpenGLBackend::bindSampler(unsigned slot, SamplerHandle handle)
		{
			assert(slot < kMaxTextureUnits);
			if (bind_state.samplers[slot] != handle)
			{
				bind_state.samplers[slot] = handle;
				bind_state.samplersUpdated = true;
			}
		}

		OpenGLBackend::SamplerHandle OpenGLBackend::initSampler(const SamplerInfo & info)
		{
			GLuint sampler_obj;
			gl::CreateSamplers(1, &sampler_obj);
			gl::SamplerParameteri(sampler_obj, gl::TEXTURE_MIN_FILTER, textureFilterToGLenum(info.minFilter));
			gl::SamplerParameteri(sampler_obj, gl::TEXTURE_MAG_FILTER, textureFilterToGLenum(info.magFilter));
			gl::SamplerParameteri(sampler_obj, gl::TEXTURE_WRAP_R, textureAddressModeToGLenum(info.addrU));
			gl::SamplerParameteri(sampler_obj, gl::TEXTURE_WRAP_S, textureAddressModeToGLenum(info.addrV));
			gl::SamplerParameteri(sampler_obj, gl::TEXTURE_WRAP_T, textureAddressModeToGLenum(info.addrW));
			return sampler_obj;
		}

		void OpenGLBackend::destroySampler(SamplerHandle handle)
		{
			gl::DeleteSamplers(1, &handle);
		}

		const char* getShaderStageName(GLenum stage)
		{
			switch (stage)
			{
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

		GLuint compileAndAttachShader(GLuint program_obj, GLenum stage, const char* source)
		{
			std::ostringstream infoLog;
			GLuint shader_obj = compileShader(stage, source, infoLog);
			if (!shader_obj)
			{
				fmt::print("===============================================================\n");
				fmt::print("Shader compilation error (stage: {})\n", getShaderStageName(stage));
				fmt::print("Compilation log follows:\n\n{}\n\n", infoLog.str());
				return 0;
			}
			gl::AttachShader(program_obj, shader_obj);
			return shader_obj;
		}

		OpenGLBackend::GraphicsPipelineHandle OpenGLBackend::createGraphicsPipeline(const GraphicsPipelineInfo & info)
		{
			auto pp = new GraphicsPipeline;
			pp->program = createProgramFromShaderPipeline(info);
			pp->vao = createVertexArrayObject(info.vertexAttribs);
			pp->blendState = info.blendState;
			pp->depthStencilState = info.depthStencilState;
			pp->rasterizerState = info.rasterizerState;
			return pp;
		}

		void OpenGLBackend::destroyGraphicsPipeline(GraphicsPipelineHandle handle)
		{
			if (handle->vao)
				gl::DeleteVertexArrays(1, &handle->vao);
			if (handle->program)
				gl::DeleteProgram(handle->program);
			delete handle;
		}

		void OpenGLBackend::destroyTexture(GLuint tex_obj)
		{
			gl::DeleteTextures(1, &tex_obj);
		}

		GLuint OpenGLBackend::createProgramFromShaderPipeline(const GraphicsPipelineInfo & info)
		{
			// compile programs
			GLuint vs_obj = 0;
			GLuint fs_obj = 0;
			GLuint gs_obj = 0;
			GLuint tcs_obj = 0;
			GLuint tes_obj = 0;
			GLuint program_obj = gl::CreateProgram();
			bool compilation_error = false;
			vs_obj = compileAndAttachShader(program_obj, gl::VERTEX_SHADER, info.VSSource);
			if (!vs_obj) compilation_error = true;
			fs_obj = compileAndAttachShader(program_obj, gl::FRAGMENT_SHADER, info.PSSource);
			if (!fs_obj) compilation_error = true;
			if (info.GSSource) {
				gs_obj = compileAndAttachShader(program_obj, gl::GEOMETRY_SHADER, info.GSSource);
				if (!gs_obj) compilation_error = true;
			}
			if (info.DSSource) {
				tes_obj = compileAndAttachShader(program_obj, gl::TESS_EVALUATION_SHADER, info.DSSource);
				if (!tes_obj) compilation_error = true;
			}
			if (info.HSSource) {
				tcs_obj = compileAndAttachShader(program_obj, gl::TESS_CONTROL_SHADER, info.HSSource);
				if (!tcs_obj) compilation_error = true;
			}

			bool link_error = false;
			if (!compilation_error)
			{
				std::ostringstream linkInfoLog;
				link_error = linkProgram(program_obj, linkInfoLog);
				if (link_error) {
					fmt::print("===============================================================\n");
					fmt::print("Shader link error\n");
					fmt::print("Compilation log follows:\n\n{}\n\n", linkInfoLog.str());
				}
			}

			if (vs_obj) gl::DeleteShader(vs_obj);
			if (fs_obj) gl::DeleteShader(fs_obj);
			if (gs_obj) gl::DeleteShader(gs_obj);
			if (tcs_obj) gl::DeleteShader(tcs_obj);
			if (tes_obj) gl::DeleteShader(tes_obj);
			if (link_error) {
				gl::DeleteProgram(program_obj);
				program_obj = 0;
			}

			return program_obj;
		}

		GLuint OpenGLBackend::createVertexArrayObject(gsl::span<VertexAttribute> attribs)
		{
			GLuint strides[OpenGLBackend::kMaxVertexBufferSlots] = { 0 };
			GLuint vertex_array_obj;
			gl::CreateVertexArrays(1, &vertex_array_obj);
			for (int attribindex = 0; attribindex < attribs.size(); ++attribindex)
			{
				const auto& a = attribs[attribindex];
				assert(a.slot < OpenGLBackend::kMaxVertexBufferSlots);
				gl::EnableVertexArrayAttrib(vertex_array_obj, attribindex);
				gl::VertexArrayAttribFormat(
					vertex_array_obj,
					attribindex,
					a.size,
					a.type,
					a.normalized,
					strides[a.slot]);
				gl::VertexArrayAttribBinding(vertex_array_obj, attribindex, a.slot);
				strides[a.slot] += a.stride;
			}
			gl::BindVertexArray(0);
			return vertex_array_obj;
		}


		///////////////////// Clear command

		void OpenGLBackend::bindVertexBuffer(unsigned slot, BufferHandle handle, unsigned stride)
		{
			gl::BindVertexBuffer(slot, handle, 0, stride);
		}

		void OpenGLBackend::bindSurface(SurfaceHandle handle)
		{
			bindFramebufferObject(handle);
		}

		void OpenGLBackend::bindGraphicsPipeline(GraphicsPipelineHandle handle)
		{
            gl::UseProgram(handle->program);
            gl::BindVertexArray(handle->vao);
            // TODO optimize state changes
            if (handle->depthStencilState.depthTestEnable)
                gl::Enable(gl::DEPTH_TEST);
            else
                gl::Disable(gl::DEPTH_TEST);
		}

		void OpenGLBackend::clearColor(SurfaceHandle framebuffer_obj, const glm::vec4 & color)
		{
			// bind the framebuffer
			bindFramebufferObject(framebuffer_obj);
			// set clear color
			gl::ClearColor(color.r, color.g, color.b, color.a);
			// clear
			gl::Clear(gl::COLOR_BUFFER_BIT);
		}

		void OpenGLBackend::clearDepth(SurfaceHandle framebuffer_obj, float depth)
		{
			gl::BindFramebuffer(gl::FRAMEBUFFER, framebuffer_obj);
			// set clear color
			gl::ClearDepth(depth);
			// clear
			gl::Clear(gl::DEPTH_BUFFER_BIT);
		}

		void OpenGLBackend::draw(PrimitiveType primitiveType, unsigned first, unsigned count)
		{
			bindState();
			gl::DrawArrays(primitiveTypeToGLenum(primitiveType), first, count);
		}

		void OpenGLBackend::swapBuffers()
		{
			glfwSwapBuffers(window);
			glfwPollEvents();
		}

		void OpenGLBackend::bindFramebufferObject(GLuint framebuffer_obj)
		{
			if (last_framebuffer_obj != framebuffer_obj) {
				last_framebuffer_obj = framebuffer_obj;
				gl::BindFramebuffer(gl::FRAMEBUFFER, framebuffer_obj);
			}
		}

		void OpenGLBackend::bindState()
		{
			if (bind_state.textureUpdated) {
				gl::BindTextures(0, kMaxTextureUnits, bind_state.textures);
				bind_state.textureUpdated = false;
			}
			if (bind_state.samplersUpdated) {
				gl::BindSamplers(0, kMaxTextureUnits, bind_state.samplers);
				bind_state.samplersUpdated = false;
			}
			if (bind_state.imagesUpdated) {
				gl::BindImageTextures(0, kMaxImageUnits, bind_state.images);
				bind_state.imagesUpdated = false;
			}
            if (bind_state.uniformBuffersUpdated) {
                gl::BindBuffersRange(gl::UNIFORM_BUFFER, 0, kMaxUniformBufferSlots, bind_state.uniformBuffers, nullptr, nullptr);
                bind_state.uniformBuffersUpdated = false;
            }
		}
		
		GLuint OpenGLBackend::createTexture1D(unsigned dimensions, GLenum internalFormat)
		{
			GLuint tex_obj;
			gl::CreateTextures(gl::TEXTURE_1D, 1, &tex_obj);
			gl::TextureStorage1D(tex_obj, 1, internalFormat, dimensions);
			return tex_obj;
		}

		GLuint OpenGLBackend::createTexture2D(glm::uvec2 dimensions, GLenum internalFormat)
		{
			GLuint tex_obj;
			gl::CreateTextures(gl::TEXTURE_2D, 1, &tex_obj);
			gl::TextureStorage2D(tex_obj, 1, internalFormat, dimensions.x, dimensions.y);
			return tex_obj;
		}

		GLuint OpenGLBackend::createTexture3D(glm::uvec3 dimensions, GLenum internalFormat)
		{
			GLuint tex_obj;
			gl::CreateTextures(gl::TEXTURE_3D, 1, &tex_obj);
			gl::TextureStorage3D(tex_obj, 1, internalFormat, dimensions.x, dimensions.y, dimensions.z);
			return tex_obj;
		}
	}
}
