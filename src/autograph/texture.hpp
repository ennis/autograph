#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <glm/glm.hpp>

#include "pixel_format.hpp"

namespace ag {
enum class TextureAddressMode { Repeat, Clamp, Mirror };

enum class TextureFilter { Nearest, Linear };

////////////////////////// Sampler
struct SamplerInfo {
  TextureAddressMode addrU = TextureAddressMode::Repeat;
  TextureAddressMode addrV = TextureAddressMode::Repeat;
  TextureAddressMode addrW = TextureAddressMode::Repeat;
  TextureFilter minFilter = TextureFilter::Nearest;
  TextureFilter magFilter = TextureFilter::Linear;
};

template <typename D> struct Sampler {
  SamplerInfo info;
  typename D::SamplerHandle handle;
};

////////////////////////// Texture1D
struct Texture1DInfo {
  glm::uint dimensions;
  PixelFormat format;
};

template <typename T, typename D> struct Texture1D {
  Texture1DInfo info;
  typename D::TextureHandle handle;
};

////////////////////////// Texture2D
struct Texture2DInfo {
  glm::uvec2 dimensions;
  PixelFormat format;
};

template <typename T, typename D> struct Texture2D {
  Texture2DInfo info;
  typename D::TextureHandle handle;
};

////////////////////////// Texture3D
struct Texture3DInfo {
  glm::uvec3 dimensions;
  PixelFormat format;
};

template <typename T, typename D> struct Texture3D {
  Texture3DInfo info;
  typename D::TextureHandle handle;
};

////////////////////////// TextureDataRaw
/*template <typename D>
struct TextureDataRaw 
{
  enum class Type { Texture1D, Texture2D, Texture3D};
  union {
    struct {

    } texture1D;
    struct {

    } texture2D;
    struct {

    } texture3D;
    struct {
      typename D::BufferHandle handle;
    } buffer;

  } u;

};*/

}

#endif // !TEXTURE_HPP
