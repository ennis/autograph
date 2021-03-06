#ifndef DRAW_HPP
#define DRAW_HPP

#include <tuple>

#include "bind.hpp"
#include "optional.hpp"
#include "rect.hpp"

namespace ag {

////////////////////////// Drawables
template <typename TIndexSource, typename... TVertexSource>
struct IndexedMesh_ {
  // Primitive type
  PrimitiveType primitiveType;
  // vertex buffers
  std::tuple<TVertexSource...> vertex_sources;
  // index buffer
  TIndexSource index_source;
};

////////////////////////// Draw command: DrawArrays
template <typename D> struct DrawArrays_ {
  PrimitiveType primitiveType;
  typename D::BufferHandle::pointer buffer;
  size_t offset;
  size_t size;
  uint32_t stride;
  uint32_t count;

  void draw(Device<D>& device, BindContext& context) {
    device.backend.bindVertexBuffer(context.vertexBufferBindingIndex++, buffer,
                                    offset, size, stride);
    device.backend.draw(primitiveType, 0, count);
  }
};

////////////////////////// Draw command: DrawArrays (reduced form)
struct DrawArrays0_ {
  PrimitiveType primitiveType;
  uint32_t first;
  uint32_t count;

  template <typename D> void draw(Device<D>& device, BindContext& context) {
    device.backend.draw(primitiveType, 0, count);
  }
};

inline DrawArrays0_ DrawArrays(PrimitiveType primitiveType, uint32_t first,
                               uint32_t count) {
  return DrawArrays0_{primitiveType, first, count};
}

////////////////////////// Draw command: DrawIndexed (reduced form)
struct DrawIndexed0_ {
  PrimitiveType primitiveType;
  uint32_t first;
  uint32_t count;
  uint32_t baseVertex;

  template <typename D> void draw(Device<D>& device, BindContext& context) {
    device.backend.drawIndexed(primitiveType, 0, count, baseVertex);
  }
};

inline DrawIndexed0_ DrawIndexed(PrimitiveType primitiveType, uint32_t first,
                                 uint32_t count, uint32_t baseVertex) {
  return DrawIndexed0_{primitiveType, first, count, baseVertex};
}

// Immediate version (put vertex data in the default upload buffer)
template <typename TVertex> struct DrawArraysImmediate_ {
  PrimitiveType primitiveType;
  gsl::span<TVertex> vertices;

  template <typename D> void draw(Device<D>& device, BindContext& context) {
    // upload to default upload buffer
    auto slice = device.pushDataToUploadBuffer(vertices);
    device.backend.bindVertexBuffer(context.vertexBufferBindingIndex++,
                                    slice.handle, slice.offset, slice.byteSize,
                                    sizeof(TVertex));
    device.backend.draw(primitiveType, 0, (uint32_t)vertices.size());
  }
};

template <typename D, typename TVertex>
DrawArrays_<D> DrawArrays(PrimitiveType primitiveType,
                          const Buffer<D, TVertex[]>& vertex_buffer) {
  return DrawArrays_<D>{
      primitiveType,   vertex_buffer.handle.get(), 0, vertex_buffer.byteSize,
      (uint32_t)sizeof(TVertex), (uint32_t)vertex_buffer.size()};
}

template <typename TVertex>
DrawArraysImmediate_<TVertex> DrawArrays(PrimitiveType primitiveType,
                                         gsl::span<TVertex> vertices) {
  return DrawArraysImmediate_<TVertex>{primitiveType, vertices};
}

////////////////////////// ag::draw (no resources)
template <typename D, typename TSurface, typename Drawable>
void draw(Device<D>& device, TSurface&& surface,
          GraphicsPipeline<D>& graphicsPipeline, Drawable&& drawable) {
  BindContext context;
  bindRenderTarget(device, context, surface);
  device.backend.bindGraphicsPipeline(graphicsPipeline.handle.get());
  drawable.draw(device, context);
}

////////////////////////// ag::draw
template <typename D, typename TSurface, typename Drawable,
          typename... TShaderResources>
void draw(Device<D>& device, TSurface&& surface,
          GraphicsPipeline<D>& graphicsPipeline, Drawable&& drawable,
          TShaderResources&&... resources) {
  BindContext context;
  bindImpl(device, context, resources...);
  bindRenderTarget(device, context, surface);
  device.backend.bindGraphicsPipeline(graphicsPipeline.handle.get());
  drawable.draw(device, context);
}

struct ClearColor {
  float rgba[4];
};

struct ClearColorInt {
  uint32_t rgba[4];
};

////////////////////////// ag::clear(Surface)
template <typename D, typename Depth, typename... Pixels>
void clear(Device<D>& device, Surface<D, Depth, Pixels...>& surface,
           const ClearColor& color,
           std::experimental::optional<const ag::Box2D&> region =
               std::experimental::nullopt) {
  device.backend.clearColor(surface.handle.get(), color);
}

////////////////////////// ag::clear(Surface)
template <typename D, typename Depth, typename... Pixels>
void clearDepth(Device<D>& device, Surface<D, Depth, Pixels...>& surface,
                float depth,
                std::experimental::optional<const ag::Box2D&> region =
                    std::experimental::nullopt) {
  device.backend.clearDepth(surface.handle.get(), depth);
}

////////////////////////// ag::clearDepth(Texture2D)
/// TODO special overload for depth pixel types
template <typename D, typename Depth>
void clearDepth(Device<D>& device, Texture2D<Depth, D>& tex, float depth,
                std::experimental::optional<const ag::Box2D&> region =
                    std::experimental::nullopt) {
  device.backend.template clearTexture2DDepth<Depth>(tex, Box2D{}, depth);
}

////////////////////////// ag::clear(Texture1D)
template <typename D, typename Pixel>
void clear(Device<D>& device, Texture1D<Pixel, D>& tex, const ClearColor& color,
           std::experimental::optional<const ag::Box1D&> region =
               std::experimental::nullopt) {
  device.backend.template clearTexture1DFloat<Pixel>(tex, Box1D{}, color);
}

////////////////////////// ag::clear(Texture2D)
template <typename D, typename Pixel>
void clear(Device<D>& device, Texture2D<Pixel, D>& tex, const ClearColor& color,
           std::experimental::optional<const ag::Box2D&> region =
               std::experimental::nullopt) {
  device.backend.template clearTexture2DFloat<Pixel>(tex, Box2D{}, color);
}

////////////////////////// ag::clear(Texture3D)
template <typename D, typename Pixel>
void clear(Device<D>& device, Texture3D<Pixel, D>& tex, const ClearColor& color,
           std::experimental::optional<const ag::Box3D&> region =
               std::experimental::nullopt) {
  device.backend.template clearTexture3DFloat<Pixel>(tex, Box3D{}, color);
}

////////////////////////// ag::clear(Texture2D<Integer>)
template <typename D, typename IPixel>
void clearInteger(Device<D>& device, Texture1D<IPixel, D>& tex,
                  const ClearColorInt& color,
                  std::experimental::optional<const ag::Box1D&> region =
                      std::experimental::nullopt) {
  device.backend.template clearTexture1DInteger<IPixel>(tex, Box1D{}, color);
}

template <typename D, typename IPixel>
void clearInteger(Device<D>& device, Texture2D<IPixel, D>& tex,
                  const ClearColorInt& color,
                  std::experimental::optional<const ag::Box2D&> region =
                      std::experimental::nullopt) {
  device.backend.template clearTexture2DInteger<IPixel>(tex, Box2D{}, color);
}

template <typename D, typename IPixel>
void clearInteger(Device<D>& device, Texture3D<IPixel, D>& tex,
                  const ClearColorInt& color,
                  std::experimental::optional<const ag::Box3D&> region =
                      std::experimental::nullopt) {
  device.backend.template clearTexture3DInteger<IPixel>(tex, Box3D{}, color);
}
}

#endif // !DRAW_HPP
