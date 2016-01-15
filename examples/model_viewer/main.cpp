#include <iostream>
#include <fstream>
#include <vector>

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <docopt.h>

#include <shaderpp/shaderpp.hpp>
#include <backend/opengl/Backend.hpp>
#include <Device.hpp>
#include <Draw.hpp>
#include <Error.hpp>

using GL = ag::opengl::OpenGLBackend;

struct Vertex 
{
	glm::vec3 position;
	glm::vec3 normal;
};

ag::GraphicsPipeline<GL> loadPipeline(ag::Device<GL>& device)
{
    using namespace ag::opengl;
    using namespace shaderpp;

    VertexAttribute vertexAttribs[] = {
        VertexAttribute { 0, gl::FLOAT, 3, 3 * sizeof(float), false },   // positions
        VertexAttribute { 0, gl::FLOAT, 3, 3 * sizeof(float), false }    // normals
    };

    ShaderSource src("../../../examples/shaders/default.glsl");
    ag::opengl::GraphicsPipelineInfo info;
    auto VSSource = src.preprocess(PipelineStage::Vertex, nullptr, nullptr);
    auto PSSource = src.preprocess(PipelineStage::Pixel, nullptr, nullptr);
    info.VSSource = VSSource.c_str();
    info.PSSource = PSSource.c_str();
    info.vertexAttribs = gsl::as_span(vertexAttribs);
    return device.createGraphicsPipeline(info);
}

std::vector<Vertex> loadModel(const char* path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, 
        	aiProcess_Triangulate |
        	aiProcess_JoinIdenticalVertices |
        	aiProcess_SortByPType);

    if (!scene) 
    	ag::failWith("Could not load scene");
    
    //std::vector<Vertex> vertices;
    //vertices.reserve();
}

static const char kUsage[] = R"(Model viewer
Usage:
    model_viewer <file>
)";

int main(int argc, const char** argv)
{ 
	auto args = docopt::docopt(kUsage,
					 { argv + 1, argv + argc },
					 true, // show help if requested
					 "Model viewer");// version string

	auto model = loadModel(args["<file>"].asString().c_str());

    using namespace glm;
	ag::opengl::OpenGLBackend gl;
	ag::DeviceOptions opts;
	ag::Device<GL> device(gl, opts);
	auto pipeline = loadPipeline(device);

	device.run([&]() {
		auto out = device.getOutputSurface();
		device.clear(out, vec4(1.0, 0.0, 1.0, 1.0));
	});

	return 0;
}
