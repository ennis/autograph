#ifndef PAINTER_HPP
#define PAINTER_HPP

#include <fstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <autograph/backend/opengl/backend.hpp>
#include <autograph/device.hpp>
#include <autograph/draw.hpp>
#include <autograph/pipeline.hpp>
#include <autograph/pixel_format.hpp>
#include <autograph/surface.hpp>

#include <extra/image_io/load_image.hpp>

#include "../common/sample.hpp"
#include "../common/uniforms.hpp"


class Painter : public samples::GLSample<Painter> {
public:

  Painter(unsigned width, unsigned height)
      : GLSample(width, height, "Painter") {

    ag::opengl::GraphicsPipelineInfo gpinfo;
    gpinfo.depthStencilState.depthTestEnable = false;
    gpinfo.depthStencilState.depthWriteEnable = false;
    gpinfo.vertexAttribs = samples::kMeshVertexDesc;
  }

  void render() {
    using namespace glm;
    auto out = device->getOutputSurface();
    ag::clear(*device, out, ag::ClearColor{1.0f, 0.0f, 1.0f, 1.0f});
    ag::clear(*device, texRender, ag::ClearColor{0.0f, 1.0f, 0.0f, 1.0f});
    /*samples::drawMesh(bunnyMesh, *device, out, pipeline, cbSceneData,
                      glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)));*/

    copyTex(texDefault, out, width, height, {20, 20}, 1.0);
  }

  // create textures
  // load pipelines

  // render normal map
  // render isolines 

private:
	// rendered normal map of object
	Texture2D<ag::RGBA8> texNormalMap;
	// painting stencil (use a dedicated stencil buffer?)
	Texture2D<ag::R8> texStencil;
	// stroke mask target
	Texture2D<ag::R8> texStrokeMask;

	

  Texture2D< 
};

#endif