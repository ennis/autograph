#include "shaderpp.hpp"

#include <fstream>
#include <sstream>

#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>

namespace shaderpp
{
	namespace {
		std::string loadSource(const char* path)
		{
			std::ifstream fileIn(path, std::ios::in);
			if (!fileIn.is_open()) {
                std::cerr << "Could not open file " << path << std::endl;
				throw std::runtime_error("Could not open file");
			}
			std::string str;
			str.assign(
				(std::istreambuf_iterator<char>(fileIn)),
				std::istreambuf_iterator<char>());
			return str;
		}
	}

    ShaderSource::ShaderSource(const char* path_) : source(loadSource(path_)), path(std::string(path_))
    {
    }

    std::string ShaderSource::preprocess(
    	PipelineStage stage, 
    	gsl::span<const char*> defines,
    	gsl::span<const char*> includePaths)
    {
        using lex_iterator_type =
            boost::wave::cpplexer::lex_iterator<
                boost::wave::cpplexer::lex_token<> >;
        using context_type = boost::wave::context<
                std::string::iterator, lex_iterator_type>;

        context_type ctx(source.begin(), source.end(), path.c_str());

        ctx.add_include_path(".");

        for (auto p : includePaths)
        	ctx.add_include_path(p);
        for (auto d : defines)
            ctx.add_macro_definition(d);

        switch (stage)
        {
        case PipelineStage::Vertex:
        	ctx.add_macro_definition("_VERTEX_");
        	break;
        case PipelineStage::Geometry:
        	ctx.add_macro_definition("_GEOMETRY_");
        	break;
        case PipelineStage::Pixel:
        	ctx.add_macro_definition("_PIXEL_");
        	break;
        case PipelineStage::Hull:
            ctx.add_macro_definition("_HULL_");
        	break;
        case PipelineStage::Domain:
            ctx.add_macro_definition("_DOMAIN_");
        	break;
        case PipelineStage::Compute:
        	ctx.add_macro_definition("_COMPUTE_");
        	break;
        }

        context_type::iterator_type first = ctx.begin();
        context_type::iterator_type last = ctx.end();

        std::ostringstream os;

        while (first != last) {
            os << (*first).get_value();
            ++first;
        }

        return std::move(os.str());
    }

}
