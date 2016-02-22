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

/*namespace renderpass {
////////////////////////////////////
// RenderPass
template <typename D> class RenderPassBase { public: };

template <typename D, typename Output, typename... Input>
class RenderPass : public RenderPassBase<D> {
public:
  Output apply(Input&&... input);
};

////////////////////////////////////
// Values
// They do not encode the location of the data (GPU or CPU or somewhere else)
// They reference an entry in the renderpass that describes how they are
// computed

template <typename Pixel> struct clear_op {
    image2D<Pixel>& result; 
};

template <typename Pixel> struct image2D {
unsigned width;
unsigned height;
variant<
  clear_op&,
  null_op&> generator;

};

////////////////////////////////////
// Operations

enum class Operations 
{
  ClearImage,
};

// create a new uninitialized 2D image
// width: static
// height: static

template <typename D, typename Pixel>
Image2D<D, Pixel> image2D(RenderPassBase<D>& rp, unsigned width,
                          unsigned height) {
  // allocate a new resource entry in rp: image with static width and height
  // return Image2D { resource_id }
}

// clear an image
// clear color: static
template <typename D, typename Pixel>
Image2D<D, Pixel> clear(RenderPassBase<D>& rp,
                        Image2D<D, Pixel>& image,
                        const ag::ClearColor& clearColor) 
{
  // allocate a new resource entry in rp: image with static width and height
  // create a new operation: clear() 
  // set dependencies for new operation
  // set generator operation for entry
}

}

class Painter : public samples::GLSample<Painter> {
public:
  Painter(unsigned width, unsigned height)
      : GLSample(width, height, "Renderpass") {}

  void render() {
    using namespace glm;
    auto out = device->getOutputSurface();
    ag::clear(*device, out, ag::ClearColor{1.0f, 0.0f, 1.0f, 1.0f});
  }

private:
};*/

int main() {
  //Painter sample(1000, 800);
  //return sample.run();
}
