#ifndef PIXEL_TYPE_HPP
#define PIXEL_TYPE_HPP

#include <glm/glm.hpp>
#include <glm/gtc/packing.hpp>
#include <gl_core_4_5.hpp>

namespace ag {

template <typename T> struct PixelTypeTraits {
  static constexpr bool IsPixelType = false;
};

struct PixelTypeTraitsImpl {
  static constexpr bool IsPixelType = true;
};

template <> struct PixelTypeTraits<float> : public PixelTypeTraitsImpl {};
template <> struct PixelTypeTraits<glm::u8vec3> : public PixelTypeTraitsImpl {};
template <> struct PixelTypeTraits<glm::u8vec4> : public PixelTypeTraitsImpl {};

struct Unorm16x4 { glm::u64 v; };
struct Unorm16x2 { glm::u32 v; };
struct Unorm16 { glm::u16 v; };
struct Snorm16x4 { glm::u64 v; };
struct Snorm16x2 { glm::u32 v; };
struct Snorm16 { glm::u16 v; };

using RGBA8 = glm::u8vec4;
using RGB8 = glm::u8vec3;
using R32F = glm::f32;
using R8 = glm::u8;

}

#endif
