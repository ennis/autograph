#ifndef DRAW_HPP
#define DRAW_HPP

#include <tuple>

#include <Bind.hpp>

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
  size_t stride;
  size_t count;

  void draw(Device<D>& device, BindContext& context) {
    device.backend.bindVertexBuffer(context.vertexBufferBindingIndex++, buffer,
                                    offset, size, stride);
    device.backend.draw(primitiveType, 0, count);
  }
};

////////////////////////// Draw command: DrawArrays (reduced form)
struct DrawArrays0_ {
  PrimitiveType primitiveType;
  size_t first;
  size_t count;

  template <typename D> void draw(Device<D>& device, BindContext& context) {
    device.backend.draw(primitiveType, 0, count);
  }
};

inline DrawArrays0_ DrawArrays(PrimitiveType primitiveType, size_t first,
                               size_t count) {
  return DrawArrays0_{primitiveType, first, count};
}

////////////////////////// Draw command: DrawIndexed (reduced form)
struct DrawIndexed0_ {
  PrimitiveType primitiveType;
  size_t first;
  size_t count;
  size_t baseVertex;

  template <typename D> void draw(Device<D>& device, BindContext& context) {
    device.backend.drawIndexed(primitiveType, 0, count, baseVertex);
  }
};

inline DrawIndexed0_ DrawIndexed(PrimitiveType primitiveType, size_t first,
                                 size_t count, size_t baseVertex) {
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
    device.backend.draw(primitiveType, 0, vertices.size());
  }
};

template <typename D, typename TVertex>
DrawArrays_<D> DrawArrays(PrimitiveType primitiveType,
                          const Buffer<D, TVertex[]>& vertex_buffer) {
  return DrawArrays_<D>{primitiveType, vertex_buffer.handle.get(), 0,
                        vertex_buffer.byteSize(), sizeof(TVertex),
                        vertex_buffer.size()};
}

template <typename TVertex>
DrawArraysImmediate_<TVertex> DrawArrays(PrimitiveType primitiveType,
                                         gsl::span<TVertex> vertices) {
  return DrawArraysImmediate_<TVertex>{primitiveType, vertices};
}

template <typename D, typename TSurface, typename Drawable>
void draw(Device<D>& device, TSurface&& surface,
          GraphicsPipeline<D>& graphicsPipeline, Drawable&& drawable) {
  BindContext context;
  bindRenderTarget(device, context, surface);
  device.backend.bindGraphicsPipeline(graphicsPipeline.handle.get());
  drawable.draw(device, context);
}

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
}

#endif // !DRAW_HPP
