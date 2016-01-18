#ifndef OPENGL_PIXELTYPE_HPP
#define OPENGL_PIXELTYPE_HPP

#include <gl_core_4_5.hpp>
#include <PixelType.hpp>

namespace ag {
namespace opengl {
template <typename T> struct OpenGLPixelTypeTraits {};

template <GLenum ExternalFormat_, unsigned NumComponents_,
          GLenum InternalFormat_>
struct OpenGLPixelTypeTraitsImpl {
  static constexpr GLenum ExternalFormat = ExternalFormat_;
  static constexpr unsigned NumComponents = NumComponents_;
  static constexpr GLenum InternalFormat = InternalFormat_;
};

template <>
struct OpenGLPixelTypeTraits<float>
    : public OpenGLPixelTypeTraitsImpl<gl::FLOAT, 1, gl::R32F> {};
template <>
struct OpenGLPixelTypeTraits<ag::RGB8>
    : public OpenGLPixelTypeTraitsImpl<gl::UNSIGNED_BYTE, 3, gl::RGB8> {};
template <>
struct OpenGLPixelTypeTraits<ag::RGBA8>
    : public OpenGLPixelTypeTraitsImpl<gl::UNSIGNED_BYTE, 4, gl::RGBA8> {};
}
}

#endif // !OPENGL_PIXELTYPE_HPP
