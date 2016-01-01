#include <istream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <cctype>

#include <gl_core_4_5.hpp>

#include "fx.hpp"
#include "fxparser.hpp"
#include "../Backend.hpp"

namespace ag {
namespace opengl {
namespace fx {

	namespace 
	{
		unsigned getVertexFormatSize(GLenum format, GLenum size)
		{
			unsigned elemSize = 0;
			switch (format)
			{
			case gl::FLOAT:
				elemSize = 4;
			case gl::INT:
			case gl::UNSIGNED_INT:
				elemSize = 4;
			case gl::SHORT:
			case gl::UNSIGNED_SHORT:
				elemSize = 2;
			case gl::BYTE:
			case gl::UNSIGNED_BYTE:
				elemSize = 1;
			default:
				throw std::logic_error("getVertexFormatSize: invalid OpenGL vertex format");
				break;
			}
			return elemSize * size;
		}
	}

	struct ShaderLibrary::Impl
	{
		std::unique_ptr<FxLibraryParser> parser;
		std::unordered_map<std::string, GLuint> samplerObjects;
		std::unordered_map<std::string, GLuint> vertexArrayObjects;
		std::string fileName;
	};

	ShaderLibrary::ShaderLibrary(const char* fxFile)
	{
		impl = std::make_unique<Impl>();
		impl->parser = std::unique_ptr<FxLibraryParser>(new FxLibraryParser(fxFile));
		impl->parser->parse();
        impl->fileName = fxFile;
	}

	ShaderLibrary::~ShaderLibrary()
	{
		// No resources to free
	}

	GraphicsPipeline<OpenGLBackend> ShaderLibrary::compileGraphicsPipeline(const char* programName)
	{
		auto progNameStr = std::string(programName);
		auto &programs = impl->parser->programs;
		if (programs.find(progNameStr) == programs.end()) {
            throw std::logic_error("Program " + progNameStr + " not found in effect file");
		}
		auto *progSpec = programs.at(progNameStr).get();

		GraphicsPipelineInfo info;
		std::string VSSource;
		std::string GSSource;
		std::string PSSource;
		std::string HSSource;
		std::string DSSource;

		if (progSpec->VS_snippet) {
			std::ostringstream s;
			s << "#version " << progSpec->VS_version << std::endl;
			progSpec->VS_snippet->paste(s);
			VSSource = s.str();
		}
		if (progSpec->GS_snippet) {
			std::ostringstream s;
			s << "#version " << progSpec->GS_version << std::endl;
			progSpec->GS_snippet->paste(s);
			GSSource = s.str();
		}
		if (progSpec->FS_snippet) {
			std::ostringstream s;
			s << "#version " << progSpec->FS_version << std::endl;
			progSpec->FS_snippet->paste(s);
			PSSource = s.str();
		}
		if (progSpec->TCS_snippet) {
			std::ostringstream s;
			s << "#version " << progSpec->TCS_version << std::endl;
			progSpec->TCS_snippet->paste(s);
			HSSource = s.str();
		}
		if (progSpec->TES_snippet) {
			std::ostringstream s;
			s << "#version " << progSpec->TES_version << std::endl;
			progSpec->TES_snippet->paste(s);
			DSSource = s.str();
		}

		info.VSSource = VSSource.c_str();
		info.GSSource = GSSource.c_str();
		info.PSSource = PSSource.c_str();
		info.HSSource = HSSource.c_str();
		info.DSSource = DSSource.c_str();

		return GraphicsPipeline<OpenGLBackend>();
	}
}
