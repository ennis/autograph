#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <glm/glm.hpp>

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
};

template <typename T, typename D> struct Texture1D {
  Texture1DInfo info;
  typename D::Texture1DHandle handle;
};

////////////////////////// Texture2D
struct Texture2DInfo {
  glm::uvec2 dimensions;
};

template <typename T, typename D> struct Texture2D {
  Texture2DInfo info;
  typename D::Texture2DHandle handle;
};

////////////////////////// Texture3D
struct Texture3DInfo {
  glm::uvec3 dimensions;
};

template <typename T, typename D> struct Texture3D {
  Texture3DInfo info;
  typename D::Texture3DHandle handle;
};
}

#endif // !TEXTURE_HPP
