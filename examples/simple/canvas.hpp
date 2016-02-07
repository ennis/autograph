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
//constexpr unsigned kLitSphereWidth = 512;
//constexpr unsigned kLitSphereHeight = 512;

struct Canvas {

  Canvas(Device &device, unsigned width_, unsigned height_)
      : width(width_), height(height_) {
	  texStrokeMask = device.createTexture2D<ag::R8>(glm::uvec2{width, height});
    texNormals = device.createTexture2D<ag::RGBA8>(glm::uvec2{width, height});
    texShadingTerm = device.createTexture2D<float>(glm::uvec2{width, height});
    texAvgShadingCurve =
        device.createTexture1D<ag::R8>(kShadingCurveSamplesSize);
	texHSVOffsetXY = device.createTexture2D<ag::RGBA8>(glm::uvec2{width, height});
	texBlurRadiusXY = device.createTexture2D<ag::R8>(glm::uvec2{width, height});
	texBlurRadiusLN =
        device.createTexture1D<ag::R8>(width);
  }

  unsigned width;
  unsigned height;

  // mask
  Texture2D<ag::R8> texStrokeMask;
  // normals texture
  Texture2D<ag::RGBA8> texNormals;
  // shading
  Texture2D<float> texShadingTerm;

  // Average shading curve
  Texture1D<ag::R8> texAvgShadingCurve;

  // HSV offset
  Texture2D<ag::RGBA8> texHSVOffsetXY;
  // Blur radius
  Texture2D<ag::R8> texBlurRadiusXY;
  // Blur radius / shading space
  Texture1D<ag::R8> texBlurRadiusLN;
  // TODO shading detail map?
};

#endif
