#ifndef COMPUTE_HPP
#define COMPUTE_HPP

#include "device.hpp"
#include "bind.hpp"

namespace ag {

struct ThreadGroupCount {
  ThreadGroupCount(unsigned sizeX_, unsigned sizeY_ = 1, unsigned sizeZ_ = 1)
      : sizeX(sizeX_), sizeY(sizeY_), sizeZ(sizeZ_) {}

  unsigned sizeX;
  unsigned sizeY;
  unsigned sizeZ;
};

template <typename D, typename... TShaderResources>
void compute(Device<D>& device, ComputePipeline<D>& computePipeline,
             ThreadGroupCount threadGroupCount,
             TShaderResources&&... resources) {
  BindContext context;
  bindImpl(device, context, resources...);
  device.backend.bindComputePipeline(computePipeline.handle.get());
  device.backend.dispatchCompute(threadGroupCount.sizeX, threadGroupCount.sizeY,
                                 threadGroupCount.sizeZ);
}
}

#endif // !COMPUTE_HPP