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

#include <eggs/variant.hpp>

using GL = ag::opengl::OpenGLBackend;

namespace renderpass {

struct node;

////////////////////////////////////
// dummy types
struct graph_edge {
	graph_edge() = default;
  graph_edge(unsigned ni, unsigned pi) : node_index{ni}, port_index{pi} {}
  unsigned node_index = 0;
  unsigned port_index = 0;
};

struct image : public graph_edge {
	image() = default;
  image(unsigned ni, unsigned pi) : graph_edge{ni, pi} {}
};

template <typename Pixel> struct image_1d : public image {
	image_1d() = default;
  image_1d(unsigned ni, unsigned pi) : image{ni, pi} {}
};

template <typename Pixel> struct image_2d : public image {
	image_2d() = default;
	image_2d(unsigned ni, unsigned pi) : image{ ni, pi } {}
};

template <typename Pixel> struct image_3d : public image {
	image_3d() = default;
	image_3d(unsigned ni, unsigned pi) : image{ ni, pi } {}
};

struct buffer : public graph_edge {
	buffer() = default;
	buffer(unsigned ni, unsigned pi) : graph_edge{ ni, pi } {}
};

template <typename T> struct typed_buffer : public buffer {
	typed_buffer() = default;
	typed_buffer(unsigned ni, unsigned pi) : buffer{ ni, pi } {}
};

// partial specialization for array types
//template <typename T> struct typed_buffer<T[]> : public buffer {};

////////////////////////////////////
// nodes
struct clear_node {
  unsigned width;
  unsigned height;
  ag::PixelFormat format;
  ag::ClearColor clear_color;
};

using node = eggs::variant<clear_node>;

////////////////////////////////////
// render pass
struct render_pass_graph {
  template <typename N> unsigned add_node(N&& n) {
    nodes.emplace_back(std::move(n));
  }

  std::vector<node> nodes;
};

////////////////////////////////////
// Operations
template <typename Pixel>
image_2d<Pixel> clear(render_pass_graph& g, unsigned width, unsigned height,
                      const ag::ClearColor& clear_color) {
  unsigned n =
      g.add_node(clear_node{width, height, Pixel::kFormat, clear_color});
  image_2d<Pixel> img{n, 0};
  return img;
}

}

class Renderpass : public samples::GLSample<Renderpass> {
public:
	Renderpass(unsigned width, unsigned height)
      : GLSample(width, height, "Renderpass") 
  {
	  namespace rp = renderpass;
	  rp::render_pass_graph g;
	  auto v = rp::clear<ag::RGBA8>(g, width, height, ag::ClearColor{ 1.0f, 0.0f, 0.0f, 1.0f });
	  // rp() -> image_2d<...>
	  // rp(dynamic_tag) -> edge<image_2d<...>> 
  }

  void render() {
    using namespace glm;
    auto out = device->getOutputSurface();
    ag::clear(*device, out, ag::ClearColor{1.0f, 0.0f, 1.0f, 1.0f});
  }

private:
};

int main() {
	Renderpass sample(1000, 800);
   return sample.run();
}
