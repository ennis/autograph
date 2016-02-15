#ifndef BRUSH_TOOL_HPP
#define BRUSH_TOOL_HPP

#include "canvas.hpp"
#include "ui.hpp"
#include "brush_path.hpp"
#include "types.hpp"

// Object holding the state of a stroke of the brush tool
class BrushTool {
public:
  BrushTool(Device& device, Canvas& canvas_, const BrushProperties& brushProps_,
            Texture2D<ag::RGBA8>& texStrokeMask_)
      : canvas(canvas_), brushProps(brushProps_),
        texStrokeMask(texStrokeMask_) {
    uniforms::CanvasData uCanvasData;
    uCanvasData.size.x = (float)canvas.width;
    uCanvasData.size.y = (float)canvas.height;
    canvasData = device->pushDataToUploadBuffer(
        uCanvasData, GL::kUniformBufferOffsetAlignment);
  }

private:
  RawBufferSlice canvasData;
  BrushProperties brushProps;
  Canvas& canvas;
  Device& device;
  Texture2D<ag::RGBA8>& texStrokeMask;
};

#endif // !BRUSH_TOOL_HPP