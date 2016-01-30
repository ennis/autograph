#ifndef EXTRAS_LOAD_IMAGE_HPP
#define EXTRAS_LOAD_IMAGE_HPP

#include <autograph/copy.hpp>
#include <autograph/device.hpp>
#include <autograph/pixel_format.hpp>
#include <autograph/texture.hpp>

#include <stb_image.h>

namespace ag {
namespace extra {
namespace image_io {

// loads texture data from a file directly in GPU memory
// the texture is loaded using stb_image, and must be in RGBA8/RGB8 format
template <typename D>
ag::Texture2D<ag::RGBA8, D> loadTexture2D(Device<D>& device,
                                          const char* filename) {
  int x, y, comp;
  auto raw_data = stbi_load(filename, &x, &y, &comp, 4);
  auto tex =
      device.createTexture2D<ag::RGBA8>(glm::uvec2((unsigned)x, (unsigned)y));
  auto pixels_data_span =
      gsl::span<const ag::RGBA8>((const ag::RGBA8*)raw_data, x * y);
  ag::copy(device, pixels_data_span, tex);
  return std::move(tex);
}
}
}
}

#endif // !EXTRAS_LOAD_IMAGE_HPP