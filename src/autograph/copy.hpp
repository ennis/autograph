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
#include "texture.hpp"
#include "pixel_format.hpp"

namespace ag {

// CPU -> Texture1D
template <typename D, typename Pixel,
          typename Storage = typename PixelTypeTraits<Pixel>::storage_type>
void copy(Device<D>& device, gsl::span<Storage> pixels,
          Texture1D<Pixel, D>& texture) {
  // TODO
}

// CPU -> Texture2D
template <typename D, typename Pixel,
          typename Storage = typename PixelTypeTraits<Pixel>::storage_type>
void copy(Device<D>& device, gsl::span<Storage> pixels,
          Texture2D<Pixel, D>& texture) {
  // TODO
}

// CPU -> Texture3D
template <typename D, typename Pixel,
          typename Storage = typename PixelTypeTraits<Pixel>::storage_type>
void copy(Device<D>& device, gsl::span<Storage> pixels,
          Texture3D<Pixel, D>& texture) {
  // TODO
}


}

#endif