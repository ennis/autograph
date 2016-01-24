#ifndef SURFACE_HPP
#define SURFACE_HPP

#include "pixel_format.hpp"

namespace ag {
// A type representing a target for a draw operation
template <typename D, typename TDepth, typename... TColors> struct Surface {
  typename D::SurfaceHandle handle;
  int width;
  int height;
};
}

#endif
