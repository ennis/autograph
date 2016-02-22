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
constexpr unsigned kCSThreadGroupSizeX = 16;
constexpr unsigned kCSThreadGroupSizeY = 16;
constexpr unsigned kCSThreadGroupSizeZ = 1;


///////////////////////////////////////
// Canvas
struct Canvas {

  Canvas(Device& device, unsigned width_, unsigned height_)
      : width(width_), height(height_) {

    texStrokeMask = device.createTexture2D<ag::R8>(glm::uvec2{width, height});

    texHistH = device.createTexture1D<ag::R32UI>(kShadingCurveSamplesSize);
    texHistS = device.createTexture1D<ag::R32UI>(kShadingCurveSamplesSize);
    texHistV = device.createTexture1D<ag::R32UI>(kShadingCurveSamplesSize);
    texHistAccum = device.createTexture1D<ag::R32UI>(kShadingCurveSamplesSize);

    /*ag::clear(device, texHistH, ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
    ag::clear(device, texHistS, ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
    ag::clear(device, texHistV,
              ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
    ag::clear(device, texHistAccum,
              ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});*/

    texDepth = device.createTexture2D<ag::Depth32>(glm::uvec2{width, height});
    texNormals =
        device.createTexture2D<ag::Unorm10x3_1x2>(glm::uvec2{width, height});
    texStencil = device.createTexture2D<ag::R8>(glm::uvec2{width, height});

    texShadingProfileLN =
        device.createTexture1D<ag::RGBA8>(kShadingCurveSamplesSize);
    texBlurParametersLN =
        device.createTexture1D<ag::RGBA8>(kShadingCurveSamplesSize);
    texDetailMaskLN = device.createTexture1D<ag::R8>(kShadingCurveSamplesSize);

    texBaseColorUV =
        device.createTexture2D<ag::RGBA8>(glm::uvec2{width, height});
    texHSVOffsetUV =
        device.createTexture2D<ag::RGBA8>(glm::uvec2{width, height});
    texBlurParametersUV =
        device.createTexture2D<ag::RGBA8>(glm::uvec2{width, height});

    texShadingTerm =
        device.createTexture2D<ag::RGBA8>(glm::uvec2{width, height});
    texShadingTermSmooth =
        device.createTexture2D<ag::RGBA8>(glm::uvec2{width, height});
    texShadingTermSmooth0 =
        device.createTexture2D<ag::RGBA8>(glm::uvec2{width, height});

    ag::clear(device, texBaseColorUV, ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
    ag::clear(device, texHSVOffsetUV, ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
    ag::clear(device, texBlurParametersUV,
              ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
    ag::clear(device, texBlurParametersLN,
              ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
  }

  unsigned width;
  unsigned height;

  // mask
  Texture2D<ag::R8> texStrokeMask;

  // rendered from geometry
  Texture2D<ag::Depth32> texDepth;
  Texture2D<ag::Unorm10x3_1x2> texNormals;
  Texture2D<ag::R8> texStencil;

  Texture2D<ag::RGBA8> texShadingTerm;
  Texture2D<ag::RGBA8> texShadingTermSmooth;
  Texture2D<ag::RGBA8> texShadingTermSmooth0;

  //
  Texture1D<ag::R32UI> texHistH;
  Texture1D<ag::R32UI> texHistS;
  Texture1D<ag::R32UI> texHistV;
  Texture1D<ag::R32UI> texHistAccum;

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

int divRoundUp(int numToRound, int multiple) {
  return (numToRound + multiple - 1) / multiple;
}

// Invoke the 'evaluate' CS
template <typename... Resources>
void previewCanvas(Device& device, Canvas& canvas, Texture2D<ag::RGBA8>& out,
                   ComputePipeline& pipeline, RawBufferSlice& canvasData, Sampler& sampler,
                   Resources&&... resources) {
  ag::compute(
      device, pipeline,
      ag::ThreadGroupCount{
          (unsigned)divRoundUp(canvas.width, kCSThreadGroupSizeX),
          (unsigned)divRoundUp(canvas.height, kCSThreadGroupSizeY), 1u},
      canvasData,
      ag::TextureUnit(0, canvas.texShadingProfileLN, sampler),
      ag::TextureUnit(1, canvas.texBlurParametersLN, sampler),
      ag::TextureUnit(2, canvas.texDetailMaskLN, sampler),
      ag::TextureUnit(3, canvas.texBaseColorUV, sampler),
      ag::TextureUnit(4, canvas.texHSVOffsetUV, sampler),
      ag::TextureUnit(5, canvas.texBlurParametersUV, sampler),
      ag::TextureUnit(6, canvas.texShadingTermSmooth, sampler),
      ag::TextureUnit(7, canvas.texStencil, sampler),
      ag::RWTextureUnit(0, out), std::forward<Resources>(resources)...);
}

// apply a CS over a region of the canvas
/*template <typename... Resources>
void applyComputeShaderOverRect(Device& device, Canvas& canvas, const ag::Box2D& rect,
                                ComputePipeline& pipeline,
                                RawBufferSlice& canvasData,
                                Resources&&... resources) {
  ag::compute(device, pipeline,
              ag::ThreadGroupCount{
                  (unsigned)divRoundUp(rect.width(), kCSThreadGroupSizeX),
                  (unsigned)divRoundUp(rect.height(), kCSThreadGroupSizeY),
                  1u},
              canvasData, std::forward<Resources>(resources)...);
}*/



#endif
