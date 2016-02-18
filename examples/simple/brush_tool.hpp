#ifndef BRUSH_TOOL_HPP
#define BRUSH_TOOL_HPP

#include "canvas.hpp"
#include "ui.hpp"
#include "brush_path.hpp"
#include "types.hpp"
#include "pipelines.hpp"
#include "../common/sample.hpp" // for Vertex2D

enum class StrokeStatus
{
    InsideStroke,
    OutsideStroke
};

// State needed by all tools
struct ToolResources
{
    Device& device;
    Pipelines& pipelines;
    Canvas& canvas;
    Ui& ui;
    //
    Sampler& samLinearRepeat;
    Sampler& samLinearClamp;
    Sampler& samNearestRepeat;
    Sampler& samNearestClamp;
    //
    Buffer<samples::Vertex2D[]>& vboQuad;
};

// Object holding the state of a stroke of a brush-like tool
// Subclass and override beginStroke(Device&, PointerEvent), continueStroke(Device&, PointerEvent), endStroke(Device&, PointerEvent)
template<typename Callbacks>
class BrushTool {
public:
  BrushTool(ToolResources& resources)
  {
    canvasMouseButtons = resources.ui.canvasMouseButtons;
    canvasMousePointer = resources.ui.canvasMousePointer;
    callbacks = std::make_unique<Callbacks>(resources);
    canvasMouseButtons.subscribe([this](auto ev) {
          if (ev.button == GLFW_MOUSE_BUTTON_LEFT &&
              ev.state == input::MouseButtonState::Pressed)
          {
              unsigned x, y;
              this->ui.getPointerPosition(x, y);
                PointerEvent pointerEvent {x, y, 1.0f}; // no pressure yet :(
strokeStatus = StrokeStatus::InsideStroke;
this->callbacks->beginStroke(pointerEvent);
          } else if (ev.button == GLFW_MOUSE_BUTTON_LEFT &&
                     ev.state == input::MouseButtonState::Released) {
              unsigned x, y;
              this->ui.getPointerPosition(x, y);
                PointerEvent pointerEvent {x, y, 1.0f}; // no pressure yet :(
           this->callbacks->endStroke(pointerEvent);
              strokeStatus = StrokeStatus::OutsideStroke;
          }
    });

    canvasMousePointer.subscribe([this](auto ev) {
        if (strokeStatus == StrokeStatus::InsideStroke) {
            PointerEvent pointerEvent {ev.positionX, ev.positionY, 1.0f}; // no pressure yet :(
            this->callbacks->continueStroke(pointerEvent);
        }
    });
  }

protected:
  std::unique_ptr<Callbacks> callbacks;
  StrokeStatus strokeStatus = StrokeStatus::OutsideStroke;
  rxcpp::observable<input::MouseButtonEvent> canvasMouseButtons;
  rxcpp::observable<input::MousePointerEvent> canvasMousePointer;
};


// color brush tool callbacks
class ColorBrush
{
public:
    ColorBrush(const ToolResources& resources_) : res(resources_)
    {
        // allocate stroke mask
        // subscribe to some interesting brush events
    }

    void beginStroke(const PointerEvent& event)
    {
        // reset state:
        //    clear stroke mask
        //    clear brush path
        // get brush properties from UI
        // add point to brush path
        ag::clear(res.device, texStrokeMask, ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
        brushPath = {};
        brushProps = brushPropsFromUi(res.ui);
        brushPath.addPointerEvent(event, brushProps, [&](auto splat){paintSplat(splat);});
    }

    void continueStroke(const PointerEvent& event)
    {
        brushPath.addPointerEvent(event, brushProps, [&](auto splat){paintSplat(splat);});
    }

    void endStroke(const PointerEvent& event)
    {
        //brushPath.addPointerEvent(event, brushProps, [&](auto splat){paintSplat(splat);});
    }

    //
    void previewCanvas(Texture2D<ag::RGBA8>& texTarget)
    {

    }

    void paintSplat(const SplatProperties& splat)
    {
        // draw splat to stroke mask
        uniforms::Splat uSplat;
        uSplat.center = splat.center;
        uSplat.width = splat.width;
        if (res.ui.brushTip == BrushTip::Round) {
          auto dim = res.ui.brushTipTextures[res.ui.selectedBrushTip].tex.info.dimensions;
          uSplat.transform = getSplatTransform(dim.x, dim.y, splat);
        } else
          uSplat.transform = getSplatTransform(splat.width, splat.width, splat);

        if (res.ui.brushTip == BrushTip::Round)
          ag::draw(res.device, res.canvas.texStrokeMask,
                   res.pipelines.ppDrawRoundSplatToStrokeMask,
                   ag::DrawArrays(ag::PrimitiveType::Triangles, res.vboQuad),
                   canvasData, uSplat);
        else if (res.ui.brushTip == BrushTip::Textured)
          ag::draw(res.device, res.canvas.texStrokeMask,
                   res.pipelines.ppDrawTexturedSplatToStrokeMask,
                   ag::DrawArrays(ag::PrimitiveType::Triangles, res.vboQuad),
                   ag::TextureUnit(0,
                                   res.ui.brushTipTextures[res.ui.selectedBrushTip].tex,
                                   res.samLinearClamp),
                   canvasData, uSplat);
    }

private:
    RawBufferSlice canvasData;
    ToolResources res;
    BrushProperties brushProps;
    BrushPath brushPath;
    Texture2D<ag::RGBA8> texStrokeMask;
};

#endif // !BRUSH_TOOL_HPP
