#ifndef CANVAS_HPP
#define CANVAS_HPP

#include "types.hpp"

// NOTE for future reference.
// Do not put constants as static const data members of a class,
// because this is NOT a definition, and thus may produce a linker error
// if no definition is provided elsewhere outside the class.
// It may work when the constant is not "odr-used"
// (see section in http://en.cppreference.com/w/cpp/language/definition)
// but it is hard to see at first glance.
// In addition, providing a definition for the constant in a header
// may violate the one definition rule because static const data members
// have external linkage (can be visible by other translation units).
// Namespace-scope constants do not have this problem since they have internal
// linkage.

// value must match the one defined in shaders
constexpr unsigned kShadingCurveSamplesSize = 256;
// lit sphere resolution
// constexpr unsigned kLitSphereWidth = 512;
// constexpr unsigned kLitSphereHeight = 512;

struct Canvas {

  Canvas(Device &device, unsigned width_, unsigned height_)
      : width(width_), height(height_) {
    texStrokeMask = device.createTexture2D<ag::R8>(glm::uvec2{width, height});
    texNormals = device.createTexture2D<ag::RGBA8>(glm::uvec2{width, height});
    texShadingTerm = device.createTexture2D<float>(glm::uvec2{width, height});

    texShadingProfileLN =
        device.createTexture1D<ag::RGBA8>(kShadingCurveSamplesSize);
    texBlurParametersLN =
        device.createTexture1D<ag::RGBA8>(kShadingCurveSamplesSize);
    texDetailMaskLN =
        device.createTexture1D<ag::R8>(kShadingCurveSamplesSize);

    texBaseColorUV=
            device.createTexture2D<ag::RGBA8>(glm::uvec2{width, height});
    texHSVOffsetUV =
        device.createTexture2D<ag::RGBA8>(glm::uvec2{width, height});
    texBlurParametersUV = device.createTexture2D<ag::RGBA8>(glm::uvec2{width, height});

    ag::clear(device, texBaseColorUV, ag::ClearColor{0.0f, 0.0f, 0.0f, 1.0f});
    ag::clear(device, texHSVOffsetUV, ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
    ag::clear(device, texBlurParametersUV, ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
  }

  unsigned width;
  unsigned height;

  // mask
  Texture2D<ag::R8> texStrokeMask;
  // normals texture
  Texture2D<ag::RGBA8> texNormals;
  // shading
  Texture2D<float> texShadingTerm;

  // L dot N space
  Texture1D<ag::RGBA8> texShadingProfileLN;
  Texture1D<ag::RGBA8> texBlurParametersLN;
  Texture1D<ag::R8> texDetailMaskLN;

  // UV (XY) space
  Texture2D<ag::RGBA8> texBaseColorUV;
  Texture2D<ag::RGBA8> texHSVOffsetUV;
  Texture2D<ag::RGBA8> texBlurParametersUV;
  // TODO shading detail map?
};

#endif
