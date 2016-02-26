#ifndef LAYER_HPP
#define LAYER_HPP

#include "types.hpp"

enum class LayerType {
  // Shading curves
  DynamicColor,
  // Constant color
  Constant,
  // LdotN space blur
  Blur,
  // Detail layer
  Detail,
  // Global layer mask (editable)
  GlobalMask,
  ////////////// Special layers (read-only)
  Special_LdotN,
  Special_SmoothedLdotN,
  Special_Normals,
  // also: occlusion, etc.
};

struct LayerContext {
  Device& device;
  Texture2D<ag::RGBA8>& target;
  ag::Box2D dirtyRect; // read/write
                       // scene parameters and shared G-buffers
                       // layer masks, etc.
};

enum class LayerFlags {
  ReadOnly = (1 << 0), // cannot be the target of brush operations
  ProcessOnCPU = (1 << 1),
  NonLocalProcess = (1 << 2)
};

struct Layer {
  Layer(LayerType type_) : type(type_) {}

  virtual ~Layer() {}

  virtual void draw(Device& device, LayerContext& context) = 0;

  LayerType type;
  LayerFlags flags;
};

template <LayerType Type> struct TLayer : public Layer {
  TLayer() : Layer(Type) {}
};

#endif
