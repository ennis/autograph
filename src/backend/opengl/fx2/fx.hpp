#ifndef OPENGL_FX_HPP
#define OPENGL_FX_HPP

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include <gl_core_4_5.hpp>

#include <Pipeline.hpp>

#include "../Backend.hpp"

namespace ag {
namespace opengl {
namespace fx {
	class ShaderLibrary
	{
	public:
		ShaderLibrary(const char* fxFilePath);
		~ShaderLibrary();

		GraphicsPipeline<OpenGLBackend> compileGraphicsPipeline(const char* programName);

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

}
}
}
	


#endif