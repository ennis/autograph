#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <cstdlib> // size_t
#include <utility> // std::move

namespace ag {
enum class BufferUsage {
  Upload,  // CPU-visible, write-only
  Default, // GPU-visible, cannot be accessed by the CPU
  Readback // CPU-visible, read-only
};

template <typename D> struct RawBuffer {
  RawBuffer() = default;
  RawBuffer(std::size_t byteSize_, typename D::BufferHandle handle_)
      : byteSize(byteSize_), handle(std::move(handle_)) {}

  std::size_t byteSize;
  typename D::BufferHandle handle;
};

template <typename D, typename T> struct Buffer : public RawBuffer<D> {
  Buffer() = default;
  Buffer(typename D::BufferHandle handle_)
      : RawBuffer<D>(sizeof(T), std::move(handle_)) {}
};

// specialization for array types
template <typename D, typename T> struct Buffer<D, T[]> : public RawBuffer<D> {
  Buffer() = default;
  Buffer(std::size_t size_, typename D::BufferHandle handle_)
      : RawBuffer<D>(size_ * sizeof(T), std::move(handle_)) {}

  constexpr std::size_t size() const {
    // must use this pointer to make byteSize a dependent name
    return this->byteSize / sizeof(T);
  }
};

// A slice of a buffer (untyped)
template <typename D> struct RawBufferSlice {
  RawBufferSlice() : handle(nullptr), offset(0), byteSize(0) {}

  RawBufferSlice(typename D::BufferHandle::pointer handle_, size_t offset_,
                 size_t byteSize_)
      : handle(handle_), offset(offset_), byteSize(byteSize_) {}

  typename D::BufferHandle::pointer handle;
  size_t offset;
  size_t byteSize;
};

template <typename D, typename T>
struct BufferSlice : public RawBufferSlice<D> {
  BufferSlice(typename D::BufferHandle::pointer handle_, size_t offset_)
      : RawBufferSlice<D>(handle_, offset_, sizeof(T)) {}
};

// specialization for array types
template <typename D, typename T>
struct BufferSlice<D, T[]> : public RawBufferSlice<D> {
  BufferSlice(typename D::BufferHandle handle_, size_t offset_, size_t size_)
      : RawBufferSlice<D>(handle_, offset_, size_ * sizeof(T)) {}

  constexpr std::size_t size() const { return this->byteSize / sizeof(T); }
};

}

#endif
