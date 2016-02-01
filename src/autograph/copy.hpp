#ifndef COPY_HPP
#define COPY_HPP

// Copy operations:
// - between GPU memory regions
// - upload from CPU to GPU
// - readback from GPU to CPU
// and, orthogonally:
// - between textures
// - between buffers

#include "buffer.hpp"
#include "device.hpp"
#include "pixel_format.hpp"
#include "texture.hpp"
#include "rect.hpp"

namespace ag {

///////////////////// CPU -> Texture copy operations

// These operations do not stall if the texture is created with the 'Dynamic'
// usage flag: the texture data is first copied to a staging
// buffer and is copied to the final texture when the GPU is ready

// CPU -> Texture1D
template <typename D, typename Pixel,
          typename Storage = typename PixelTypeTraits<Pixel>::storage_type>
void copy(Device<D>& device, gsl::span<const Storage> pixels,
          Texture1D<Pixel, D>& texture, unsigned mipLevel = 0) 
{
  device.backend.updateTexture1D(texture.handle.get(), texture.info, mipLevel,
                                 Box1D{0, texture.info.dimensions},
                                 gsl::as_bytes(pixels));
}

// CPU -> Texture2D
template <typename D, typename Pixel,
          typename Storage = typename PixelTypeTraits<Pixel>::storage_type>
void copy(Device<D>& device, gsl::span<const Storage> pixels,
          Texture2D<Pixel, D>& texture, unsigned mipLevel = 0) 
{
  device.backend.updateTexture2D(
      texture.handle.get(), texture.info, mipLevel,
      Box2D{0, 0, texture.info.dimensions.x, texture.info.dimensions.y},
      gsl::as_bytes(pixels));
}

// CPU -> Texture3D
template <typename D, typename Pixel,
          typename Storage = typename PixelTypeTraits<Pixel>::storage_type>
void copy(Device<D>& device, gsl::span<const Storage> pixels,
          Texture3D<Pixel, D>& texture, unsigned mipLevel = 0) 
{
  device.backend.updateTexture3D(texture.handle.get(), texture.info, mipLevel,
                                 Box3D{0, 0, 0, texture.info.dimensions.x,
                                       texture.info.dimensions.y,
                                       texture.info.dimensions.z},
                                 gsl::as_bytes(pixels));
}
}

#endif