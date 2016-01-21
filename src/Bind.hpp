#ifndef BIND_HPP
#define BIND_HPP

#include <tuple>

#include <Texture.hpp>
#include <Buffer.hpp>
#include <Device.hpp>
#include <Utils.hpp>

namespace ag {

////////////////////////// PrimitiveType
enum class PrimitiveType { Points, Lines, Triangles };

////////////////////////// IndexType
enum class IndexType { UShort, UInt };

////////////////////////// BindContext
struct BindContext {
  unsigned renderTargetBindingIndex = 0;
  unsigned textureBindingIndex = 0;
  unsigned samplerBindingIndex = 0;
  unsigned vertexBufferBindingIndex = 0;
  unsigned uniformBufferBindingIndex = 0;
};

////////////////////////// Binder: vertex buffer
template <typename VertexTy, typename D> struct VertexBuffer_ {
  const Buffer<D, VertexTy[]>& buf;
};

template <typename VertexTy, typename D>
VertexBuffer_<VertexTy, D> VertexBuffer(const Buffer<D, VertexTy[]>& buf) {
  return VertexBuffer_<VertexTy, D>{buf};
}

////////////////////////// Binder: index buffer
template <typename T, typename D> struct IndexBuffer_ {
  const Buffer<D, T[]>& buf;
};

template <typename T, typename D>
IndexBuffer_<T, D> IndexBuffer(const Buffer<D, T[]>& buf) {
  static_assert(std::is_same<T, unsigned short>::value ||
                    std::is_same<T, unsigned int>::value,
                "Unsupported index type");
  return IndexBuffer_<T, D>{buf};
}

////////////////////////// Binder: vertex array (vertices on the CPU)
template <typename VertexTy> struct VertexArray_ {
  gsl::span<const VertexTy> data;
};

template <typename VertexTy, typename D>
VertexArray_<VertexTy> VertexArray(gsl::span<const VertexTy> data) {
  return VertexArray_<VertexTy>{data};
}

////////////////////////// Binder: texture unit
template <typename TextureTy, typename D> struct TextureUnit_ {
  TextureUnit_(unsigned unit_, const TextureTy& tex_,
               const Sampler<D>& sampler_)
      : unit(unit_), tex(tex_), sampler(sampler_) {}

  unsigned unit;
  const TextureTy& tex;
  const Sampler<D>& sampler;
};

template <typename T, typename D>
TextureUnit_<Texture1D<T, D>, D> TextureUnit(unsigned unit_,
                                             const Texture1D<T, D>& tex_,
                                             const Sampler<D>& sampler_) {
  return TextureUnit_<Texture1D<T, D>, D>(unit_, tex_, sampler_);
}

template <typename T, typename D>
TextureUnit_<Texture2D<T, D>, D> TextureUnit(unsigned unit_,
                                             const Texture2D<T, D>& tex_,
                                             const Sampler<D>& sampler_) {
  return TextureUnit_<Texture2D<T, D>, D>(unit_, tex_, sampler_);
}

template <typename T, typename D>
TextureUnit_<Texture3D<T, D>, D> TextureUnit(unsigned unit_,
                                             const Texture3D<T, D>& tex_,
                                             const Sampler<D>& sampler_) {
  return TextureUnit_<Texture3D<T, D>, D>(unit_, tex_, sampler_);
}

////////////////////////// Binder: uniform slot
template <typename ResTy // Buffer, BufferSlice or just a value
          >
struct Uniform_ {
  Uniform_(unsigned slot_, const ResTy& buf_) : slot(slot_), buf(buf_) {}

  unsigned slot;
  const ResTy& buf;
};

template <typename ResTy>
Uniform_<ResTy> Uniform(unsigned slot_, const ResTy& buf_) {
  return Uniform_<ResTy>(slot_, buf_);
}

////////////////////////// Binder: SurfaceRT
template <typename D, typename T>
void bindRTImpl(Device<D>& device, BindContext& context, Texture2D<T,D>& resource) {
    device.backend.
}

template <typename D, typename T, typename... Rest>
void bindRTImpl(Device<D>& device, BindContext& context, Texture2D<T,D>& resource,
              Rest&&... rest)
{
    bindRTOne(device, context, std::forward<T>(resource));
    bindRTImpl(device, context,  std::forward<Rest>(rest)...);
}

template <typename D, typename TDepth, typename... TPixels>
struct SurfaceRT_
{
    // TODO check for a valid depth format
    Texture2D<TDepth, D>& depth_target;
    std::tuple<Texture2D<TPixels, D>&...> color_targets;

    void bind(Device<D>& device, BindContext& context)
    {
        device.backend.bindDepthRenderTarget(depth_target.handle.get());
        call(bindRTImpl, std::tuple_cat(std::make_tuple(device, context), color_targets));
    }
};

// specialization for depth-less rendering
template <typename D, void, typename... TPixels>
struct SurfaceRT_
{
    std::tuple<Texture2D<TPixels, D>&...> color_targets;

    void bind(Device<D>& device, BindContext& context)
    {
        call(bindRTImpl, std::tuple_cat(std::make_tuple(device, context), color_targets));
    }
};

template <typename D, typename TDepth, typename... TPixels>
SurfaceRT_<D,TDepth,TPixels...> SurfaceRT(Texture2D<TDepth,D>& tex_depth, Texture2D<TPixels,D>& tex_color...)
{
    return SurfaceRT_<D,TDepth,TPixels...> { tex_depth, std::make_tuple(tex_color...); }
}

template <typename D, typename... TPixels>
SurfaceRT_<D,void,TPixels...> SurfaceRTNoDepth(Texture2D<TPixels,D>& tex_color...)
{
    return SurfaceRT_<D,void,TPixels...> { std::make_tuple(tex_color...); }
}

////////////////////////// bindOne<T> template declaration
template <typename D, typename T>
void bindOne(Device<D>& device, BindContext& context, const T& value);

////////////////////////// Bind<VertexBuffer_>
template <typename D, typename T>
void bindOne(Device<D>& device, BindContext& context,
             const VertexBuffer_<T, D>& vbuf) {
  device.backend.bindVertexBuffer(context.vertexBufferBindingIndex++,
                                  vbuf.buf.handle.get(), 0, vbuf.buf.byteSize,
                                  sizeof(T));
}

////////////////////////// Bind<VertexArray_>
template <typename D, typename TVertex>
void bindOne(Device<D>& device, BindContext& context,
             const VertexArray_<TVertex>& vbuf) {
  auto slice = device.pushDataToUploadBuffer(vbuf.data);
  device.backend.bindVertexBuffer(context.vertexBufferBindingIndex++,
                                  slice.handle, slice.offset, slice.byteSize,
                                  sizeof(TVertex));
}

////////////////////////// Bind<IndexBuffer_<T> >
template <typename D, typename T>
void bindOne(Device<D>& device, BindContext& context,
             const IndexBuffer_<T, D>& ibuf) {
  IndexType indexType;
  if (std::is_same<T, unsigned short>::value)
    indexType = IndexType::UShort;
  if (std::is_same<T, unsigned int>::value)
    indexType = IndexType::UInt;
  device.backend.bindIndexBuffer(ibuf.buf.handle.get(), 0, ibuf.buf.byteSize,
                                 indexType);
}

////////////////////////// Bind<Texture1D>
template <typename D, typename TPixel>
void bindOne(Device<D>& device, BindContext& context,
             const Texture1D<TPixel, D>& tex) {
  device.backend.bindTexture1D(context.textureBindingIndex++, tex.handle.get());
}

////////////////////////// Bind<Texture2D>
template <typename D, typename TPixel>
void bindOne(Device<D>& device, BindContext& context,
             const Texture2D<TPixel, D>& tex) {
  device.backend.bindTexture2D(context.textureBindingIndex++, tex.handle.get());
}

////////////////////////// Bind<Texture3D>
template <typename D, typename TPixel>
void bindOne(Device<D>& device, BindContext& context,
             const Texture3D<TPixel, D>& tex) {
  device.backend.bindTexture3D(context.textureBindingIndex++, tex.handle.get());
}

////////////////////////// Bind<Sampler>
template <typename D>
void bindOne(Device<D>& device, BindContext& context,
             const Sampler<D>& sampler) {
  device.backend.bindSampler(context.samplerBindingIndex++,
                             sampler.handle.get());
}

////////////////////////// Bind<TextureUnit<>>
template <typename D, typename TextureTy>
void bindOne(Device<D>& device, BindContext& context,
             const TextureUnit_<TextureTy, D>& tex_unit) {
  context.textureBindingIndex = tex_unit.unit;
  context.samplerBindingIndex = tex_unit.unit;
  bindOne(device, context, tex_unit.sampler);
  bindOne(device, context, tex_unit.tex);
}

////////////////////////// Bind<RawBufferSlice>
template <typename D>
void bindOne(Device<D>& device, BindContext& context,
             const RawBufferSlice<D>& buf_slice) {
  device.backend.bindUniformBuffer(context.uniformBufferBindingIndex++,
                                   buf_slice.handle, buf_slice.offset,
                                   buf_slice.byteSize);
}

////////////////////////// Bind<T>
template <typename D, typename T>
void bindOne(Device<D>& device, BindContext& context, const T& value) {
  // allocate a temporary uniform buffer from the default upload buffer
  auto slice =
      device.pushDataToUploadBuffer(value, D::kUniformBufferOffsetAlignment);
  bindOne(device, context, slice);
}

////////////////////////// bindImpl<T>: recursive binding of draw resources
template <typename D, typename T>
void bindImpl(Device<D>& device, BindContext& context, T&& resource) {
  bindOne(device, context, std::forward<T>(resource));
}

template <typename D, typename T, typename... Rest>
void bindImpl(Device<D>& device, BindContext& context, T&& resource,
              Rest&&... rest) {
  bindOne(device, context, std::forward<T>(resource));
  bindImpl(device, context, std::forward<Rest>(rest)...);
}

////////////////////////// Bind render targets
////////////////////////// BindRT<Texture2D>
template <typename D, typename T>
void bindRT(Device<D>& device, BindContext& context, Texture2D<T,D>& tex)
{
    device.backend.bindRenderTarget(context.renderTargetBindingIndex++, tex.handle.get());
}

template <typename D, typename T>
void bindRT(Device<D>& device, BindContext& context, Texture2D<T,D>& tex)
{
    device.backend.bindRenderTarget(context.renderTargetBindingIndex++, tex.handle.get());
}

}

#endif // !BIND_HPP
