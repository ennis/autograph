#ifndef FENCE_HPP
#define FENCE_HPP

#include <cstdint>

namespace ag {
using FenceValue = uint64_t;

// Linked to a thread
template <typename D> struct Fence {
  FenceValue getValue() { backend.getFenceValue(handle); }

  void signal(FenceValue newValue) { backend.signal(handle, newValue); }

  D& backend;
  typename D::FenceHandle handle;
};
}

#endif // !FENCE_HPP
