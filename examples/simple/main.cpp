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

#include <shaderpp/shaderpp.hpp>

using GL = ag::opengl::OpenGLBackend;

struct Pipelines
{
	Pipelines(ag::Device<GL>& device)
	{
		reload(device);
	}

	void reload(ag::Device<GL>& device)
	{
		using namespace ag::opengl;
		using namespace shaderpp;

		VertexAttribute vertexAttribs[] = {
			VertexAttribute { 0, gl::FLOAT, 3, 3 * sizeof(float), false }
		};

		{
			ShaderSource src_default("../../../examples/shaders/default.glsl");
			ag::opengl::GraphicsPipelineInfo info;
			auto VSSource = src_default.preprocess(PipelineStage::Vertex, nullptr, nullptr);
			auto PSSource = src_default.preprocess(PipelineStage::Pixel, nullptr, nullptr);
			info.VSSource = VSSource.c_str();
			info.PSSource = PSSource.c_str();
	        info.vertexAttribs = gsl::as_span(vertexAttribs);
			pp_default = device.createGraphicsPipeline(info);
		}

		/*{
			ShaderSource src_compute("../../../examples/shaders/compute.glsl");
			ag::opengl::ComputePipelineInfo info;
			auto CSSource = src_compute.preprocess(PipelineStage::Compute, nullptr, nullptr);
			info.CSSource = CSSource.c_str();
			pp_compute = device.createComputePipeline(info);
		}*/
	}

	ag::GraphicsPipeline<GL> pp_default;
	ag::ComputePipeline<GL> pp_compute;
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
            // device
			device,
            // render target
			out,
            // pipeline
            pp.pp_default,
            // drawable
			ag::DrawArrays(ag::PrimitiveType::Triangles, gsl::span<glm::vec3>(vbo_data)),
            // resources
            glm::mat4(2.0f),
            ag::TextureUnit(0, tex, sampler)
            );
	});

	return 0;
}
