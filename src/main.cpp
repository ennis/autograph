#include <iostream>
#include <fstream>
#include <glm/glm.hpp>

#include <PixelType.hpp>
#include <Surface.hpp>
#include <backend/opengl/Backend.hpp>
#include <Device.hpp>
#include <ResourceScope.hpp>
#include <Draw.hpp>
#include <Pipeline.hpp>

using GL = ag::opengl::OpenGLBackend;

std::string loadShaderSource(const char* path)
{
	std::ifstream fileIn(path, std::ios::in);
	if (!fileIn.is_open()) {
		fmt::print("Could not open shader file {}", path);
		throw std::runtime_error("Could not open shader file");
	}
	std::string str;
	str.assign(
		(std::istreambuf_iterator<char>(fileIn)),
		std::istreambuf_iterator<char>());
    return std::move(str);
}

struct Pipelines
{
	Pipelines(ag::Device<GL>& device)
	{
		reload(device);
	}

	void reload(ag::Device<GL>& device)
	{
		using namespace ag::opengl;

		VertexAttribute vertexAttribs[] = {
			VertexAttribute { 0, gl::FLOAT, 3, 3 * sizeof(float), false }
		};

		ag::opengl::GraphicsPipelineInfo info;
		auto VSSource = loadShaderSource("../examples/shaders/default.vert");
		auto PSSource = loadShaderSource("../examples/shaders/default.frag");
		info.VSSource = VSSource.c_str();
		info.PSSource = PSSource.c_str();
        info.vertexAttribs = gsl::as_span(vertexAttribs);
		pipeline = device.createGraphicsPipeline(info);
	}

	ag::GraphicsPipeline<GL> pipeline;
};

int main()
{
	using namespace glm;
	ag::opengl::OpenGLBackend gl;
	ag::DeviceOptions opts;
	ag::Device<GL> device(gl, opts);

	Pipelines pp(device);

    auto tex = device.createTexture2D<ag::RGBA8>({ 512, 512 });
	glm::vec3 vbo_data[] = {
		{0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{1.0f, 0.0f, 0.0f} };
	auto vbo = device.createBuffer(gsl::span<glm::vec3>(vbo_data));

    ag::SamplerInfo linearSamplerInfo;
    linearSamplerInfo.addrU = ag::TextureAddressMode::Repeat;
    linearSamplerInfo.addrV = ag::TextureAddressMode::Repeat;
    linearSamplerInfo.addrW = ag::TextureAddressMode::Repeat;
    linearSamplerInfo.minFilter = ag::TextureFilter::Nearest;
    linearSamplerInfo.magFilter = ag::TextureFilter::Nearest;
    auto sampler = device.createSampler(linearSamplerInfo);


	device.run([&]() {
		auto out = device.getOutputSurface();
		device.clear(out, vec4(1.0, 0.0, 1.0, 1.0));

        ag::draw(
			device,
			out,
            pp.pipeline,
			ag::DrawArrays(ag::PrimitiveType::Triangles, gsl::span<glm::vec3>(vbo_data)),
            glm::mat4(0.5f),
            ag::TextureUnit(0, tex, sampler)
            );
	});

	return 0;
}
