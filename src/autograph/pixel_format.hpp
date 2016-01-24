#ifndef PIXEL_TYPE_HPP
#define PIXEL_TYPE_HPP

#include <cstdint>

namespace ag {

enum class PixelFormat {
  // 32x4
  Uint32x4,
  Sint32x4,
  Float4,
  // 32x3
  Uint32x3,
  Sint32x3,
  Float3,
  // 32x2
  Float2,
  // 16x4
  Uint16x4,
  Sint16x4,
  Unorm16x4,
  Snorm16x4,
  Float16x4,
  // 16x2
  Uint16x2,
  Sint16x2,
  Unorm16x2,
  Snorm16x2,
  Float16x2,
  // 8x4
  Uint8x4,
  Sint8x4,
  Unorm8x4,
  Snorm8x4,
  // 8x3
  Uint8x3,
  Sint8x3,
  Unorm8x3,
  Snorm8x3,
  // 8x2
  Uint8x2,
  Sint8x2,
  Unorm8x2,
  Snorm8x2,
  // 10_10_10_2
  Unorm10x3_1x2,
  Snorm10x3_1x2,
  // Compressed formats
  BC1, // DXT1
  BC2, // DXT3
  BC3, // DXT5
  UnormBC4,
  SnormBC4,
  UnormBC5,
  SnormBC5,
  // Single
  Uint32,
  Sint32,
  Uint16,
  Sint16,
  Unorm16,
  Snorm16,
  //
  Uint8,
  Sint8,
  Unorm8,
  Snorm8,
  // TODO
  Depth32,
  Depth24,
  Depth16,
  Float16,
  Float,
  Max
};

template <typename T> struct PixelTypeTraits {
  static constexpr bool kIsPixelType = false;
};

template <PixelFormat Format> struct PixelTypeTraitsImpl {
  static constexpr bool kIsPixelType = true;
  static constexpr PixelFormat kFormat = Format;
};

template <>
struct PixelTypeTraits<float> : public PixelTypeTraitsImpl<PixelFormat::Float> {
};
template <>
struct PixelTypeTraits<float[2]>
    : public PixelTypeTraitsImpl<PixelFormat::Float2> {};
template <>
struct PixelTypeTraits<float[3]>
    : public PixelTypeTraitsImpl<PixelFormat::Float3> {};
template <>
struct PixelTypeTraits<float[4]>
    : public PixelTypeTraitsImpl<PixelFormat::Float4> {};

template <>
struct PixelTypeTraits<uint8_t>
    : public PixelTypeTraitsImpl<PixelFormat::Uint8> {};
template <>
struct PixelTypeTraits<uint8_t[2]>
    : public PixelTypeTraitsImpl<PixelFormat::Uint8x2> {};
template <>
struct PixelTypeTraits<uint8_t[3]>
    : public PixelTypeTraitsImpl<PixelFormat::Uint8x3> {};
template <>
struct PixelTypeTraits<uint8_t[4]>
    : public PixelTypeTraitsImpl<PixelFormat::Uint8x4> {};

// wrapper type for normalized pixel formats
template <typename T> struct Normalized { T value; };

template <>
struct PixelTypeTraits<Normalized<uint8_t>>
    : public PixelTypeTraitsImpl<PixelFormat::Unorm8> {};
template <>
struct PixelTypeTraits<Normalized<uint8_t>[2]>
    : public PixelTypeTraitsImpl<PixelFormat::Unorm8x2> {};
template <>
struct PixelTypeTraits<Normalized<uint8_t>[3]>
    : public PixelTypeTraitsImpl<PixelFormat::Unorm8x3> {};
template <>
struct PixelTypeTraits<Normalized<uint8_t>[4]>
    : public PixelTypeTraitsImpl<PixelFormat::Unorm8x4> {};

using RGBA8 = Normalized<uint8_t>[4];
using RGB8 = Normalized<uint8_t>[3];
using RG8 = Normalized<uint8_t>[3];
using R32F = float;
using R8 = Normalized<uint8_t>;
}

#endif
