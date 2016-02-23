#ifndef COMPUTE_HPP
#define COMPUTE_HPP

#include "bind.hpp"
#include "device.hpp"

namespace ag {

struct ThreadGroupCount {
  ThreadGroupCount(unsigned sizeX_, unsigned sizeY_ = 1, unsigned sizeZ_ = 1)
      : sizeX(sizeX_), sizeY(sizeY_), sizeZ(sizeZ_) {}

  unsigned sizeX;
  unsigned sizeY;
  unsigned sizeZ;
};

int divRoundUp(int numToRound, int multiple) {
  return (numToRound + multiple - 1) / multiple;
}

ThreadGroupCount makeThreadGroupCount2D(unsigned globalSizeX,
                                        unsigned globalSizeY,
                                        unsigned blockSizeX,
                                        unsigned blockSizeY) {
  return ThreadGroupCount{
      (unsigned)divRoundUp((int)globalSizeX, (int)blockSizeX),
      (unsigned)divRoundUp((int)globalSizeY, (int)blockSizeY), 1};
}

ThreadGroupCount
makeThreadGroupCount3D(unsigned globalSizeX, unsigned globalSizeY,
                       unsigned globalSizeZ, unsigned blockSizeX,
                       unsigned blockSizeY, unsigned blockSizeZ) {
  return ThreadGroupCount{
      (unsigned)divRoundUp((int)globalSizeX, (int)blockSizeX),
      (unsigned)divRoundUp((int)globalSizeY, (int)blockSizeY),
      (unsigned)divRoundUp((int)globalSizeZ, (int)blockSizeZ)};
}

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