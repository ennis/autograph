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
#include "rect.hpp"
#include "texture.hpp"

namespace ag {

///////////////////// CPU -> Texture copy operations
// These operations do not stall if the texture is created with the 'Dynamic'
// usage flag: the texture data is first copied to a staging
// buffer and is copied to the final texture when the GPU is ready

// CPU -> Texture1D
template <typename D, typename Pixel,
          typename Storage = typename PixelTypeTraits<Pixel>::storage_type>
void copy(Device<D>& device, gsl::span<const Storage> pixels,
          Texture1D<Pixel, D>& texture, unsigned mipLevel = 0) {
  device.backend.updateTexture1D(texture.handle.get(), texture.info, mipLevel,
                                 Box1D{0, texture.info.dimensions},
                                 gsl::as_bytes(pixels));
}

// CPU -> Texture2D
template <typename D, typename Pixel,
          typename Storage = typename PixelTypeTraits<Pixel>::storage_type>
void copy(Device<D>& device, gsl::span<const Storage> pixels,
          Texture2D<Pixel, D>& texture, unsigned mipLevel = 0) {
  device.backend.updateTexture2D(
      texture.handle.get(), texture.info, mipLevel,
      Box2D{0, 0, texture.info.dimensions.x, texture.info.dimensions.y},
      gsl::as_bytes(pixels));
}

// CPU -> Texture3D
template <typename D, typename Pixel,
          typename Storage = typename PixelTypeTraits<Pixel>::storage_type>
void copy(Device<D>& device, gsl::span<const Storage> pixels,
          Texture3D<Pixel, D>& texture, unsigned mipLevel = 0) {
  device.backend.updateTexture3D(texture.handle.get(), texture.info, mipLevel,
                                 Box3D{0, 0, 0, texture.info.dimensions.x,
                                       texture.info.dimensions.y,
                                       texture.info.dimensions.z},
                                 gsl::as_bytes(pixels));
}

///////////////////// Texture -> CPU async readback operations
// returns a waitable std::future that indicates when the span is ready
// future<span> asyncCopy(device, texture, out_pixels, box)

///////////////////// Texture -> CPU sync readback operations
// force a CPU/GPU sync
// void syncCopy(device, texture, out_pixels, box)
// backend impl: run async copy op to unpack buffer, fence, wait for fence, copy
// to CPU main memory, return
template <typename D, typename Pixel,
          typename Storage = typename PixelTypeTraits<Pixel>::storage_type>
void copySync(Device<D>& device, Texture1D<Pixel, D>& texture,
              gsl::span<Storage> outPixels, unsigned mipLevel = 0) {
  // TODO remove this, replace with a shared async transfer API
  device.backend.readTexture1D(texture.handle.get(), texture.info, mipLevel,
                               Box1D{0, texture.info.dimensions},
                               gsl::as_writeable_bytes(outPixels));
}

template <typename D, typename Pixel,
          typename Storage = typename PixelTypeTraits<Pixel>::storage_type>
void copySync(Device<D>& device, Texture2D<Pixel, D>& texture,
              gsl::span<Storage> outPixels, unsigned mipLevel = 0) {
  // TODO remove this, replace with a shared async transfer API
  device.backend.readTexture2D(
      texture.handle.get(), texture.info, mipLevel,
      Box2D{0, 0, texture.info.dimensions.x, texture.info.dimensions.y},
      gsl::as_writeable_bytes(outPixels));
}

///////////////////// Texture -> buffer copy operations
template <typename D, typename Pixel,
          typename Storage = typename PixelTypeTraits<Pixel>::storage_type>
void copy(Device<D>& device, Texture1D<Pixel, D>& texture,
          RawBufferSlice<D>& buffer, const ag::Box1D& region,
          unsigned mipLevel = 0) {
  // TODO should check that the Storage and Buffer types are compatible
  device.backend.copyTextureRegion1D(texture, buffer, region, mipLevel);
}

template <typename D, typename Pixel,
          typename Storage = typename PixelTypeTraits<Pixel>::storage_type>
void copy(Device<D>& device, Texture2D<Pixel, D>& texture,
          RawBufferSlice<D>& buffer, const ag::Box2D& region,
          unsigned mipLevel = 0) {
  // TODO should check that the Storage and Buffer types are compatible
  device.backend.copyTextureRegion2D(texture, buffer, region, mipLevel);
}

// copy operation:
// Texture1D -> Texture1D
// Texture2D -> Texture2D
// Texture3D -> Texture3D
// Texture1D -> Buffer
// Texture2D -> Buffer
// Texture3D -> Buffer
// Buffer -> Texture1D
// Buffer -> Texture2D
// Buffer -> Texture3D
// Buffer -> CPU
// Buffer -> CPU
// Buffer -> CPU
// CPU -> Buffer
// CPU -> Buffer
// CPU -> Buffer

// TODO: copy a mip layer to a texture2D? copy a face of a cube map to a
// texture2D?
// Copy a texture1D to a scanline of a texture2D?
// -> need a more generic method
}

#endif
