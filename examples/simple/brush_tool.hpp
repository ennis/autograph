#ifndef BRUSH_TOOL_HPP
#define BRUSH_TOOL_HPP

#include <autograph/compute.hpp>

#include "../common/sample.hpp" // for Vertex2D
#include "brush_path.hpp"
#include "canvas.hpp"
#include "pipelines.hpp"
#include "types.hpp"
#include "ui.hpp"
#include "camera.hpp"

enum class StrokeStatus { InsideStroke, OutsideStroke };

// State needed by all tools
struct ToolResources {
  Device& device;
  Pipelines& pipelines;
  Canvas& canvas;
  Ui& ui;
  //Camera& camera;
  //
  Sampler& samLinearRepeat;
  Sampler& samLinearClamp;
  Sampler& samNearestRepeat;
  Sampler& samNearestClamp;
  //
  Buffer<samples::Vertex2D[]>& vboQuad;
};

class ToolInstance {
public:
  virtual ~ToolInstance() {}
};

// Object holding the state of a stroke of a brush-like tool
// Subclass and override beginStroke(Device&, PointerEvent),
// continueStroke(Device&, PointerEvent), endStroke(Device&, PointerEvent)
class BrushTool : public ToolInstance {
public:
  BrushTool(const ToolResources& resources) : ui(resources.ui) {
      fmt::print(std::clog, "Creating BrushTool\n");
    resources.ui.canvasMouseButtons.subscribe(subscription, [this](const input::mouse_button_event& ev) {
      if (ev.button == GLFW_MOUSE_BUTTON_LEFT &&
          ev.state == input::mouse_button_state::pressed) {
		auto cursor_pos = this->ui.cursorPosition.get_value();
        PointerEvent pointerEvent{std::get<0>(cursor_pos), std::get<1>(cursor_pos), 1.0f}; // no pressure yet :(
        strokeStatus = StrokeStatus::InsideStroke;
        this->beginStroke(pointerEvent);
      } else if (ev.button == GLFW_MOUSE_BUTTON_LEFT &&
                 ev.state == input::mouse_button_state::released) {
		  auto cursor_pos = this->ui.cursorPosition.get_value();
		  PointerEvent pointerEvent{ std::get<0>(cursor_pos), std::get<1>(cursor_pos), 1.0f }; // no pressure yet :(
		  this->endStroke(pointerEvent);
        strokeStatus = StrokeStatus::OutsideStroke;
      }
    });

    resources.ui.cursorPosition.get_observable().subscribe(subscription, [this](const std::tuple<unsigned,unsigned>& pos) {
      if (strokeStatus == StrokeStatus::InsideStroke) {
        PointerEvent pointerEvent{ std::get<0>(pos), std::get<1>(pos),
                                  1.0f}; // no pressure yet :(
        this->continueStroke(pointerEvent);
      }
    });
  }

  virtual void beginStroke(const PointerEvent& ev) = 0;
  virtual void endStroke(const PointerEvent& ev) = 0;
  virtual void continueStroke(const PointerEvent& ev) = 0;

  virtual ~BrushTool() {
      fmt::print(std::clog, "Deleting BrushTool\n");
      subscription.unsubscribe();
  }

protected:
  Ui& ui;
  StrokeStatus strokeStatus = StrokeStatus::OutsideStroke;
  rxcpp::composite_subscription subscription;
};

// color brush tool callbacks
// noncopyable, nonmovable
class ColorBrushTool : public BrushTool {
public:
  ColorBrushTool(const ToolResources& resources_)
      : BrushTool(resources_), res(resources_) {
    // allocate stroke mask
          fmt::print(std::clog, "Init ColorBrushTool\n");
    texStrokeMask = res.device.createTexture2D<ag::RGBA8>(
        {res.canvas.width, res.canvas.height});
  }

  virtual ~ColorBrushTool()
  {}

  void beginStroke(const PointerEvent& event) override {
    // reset state:
    //    clear stroke mask
    //    clear brush path
    // get brush properties from UI
    // add point to brush path
    ag::clear(res.device, texStrokeMask,
              ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
    brushPath = {};
    brushProps = brushPropsFromUi(res.ui);
    brushPath.addPointerEvent(event, brushProps,
                              [this](auto splat) { this->paintSplat(splat); });
  }

  void continueStroke(const PointerEvent& event) override {
    brushPath.addPointerEvent(event, brushProps,
                              [this](auto splat) { this->paintSplat(splat); });
  }

  void endStroke(const PointerEvent& event) override {
	  fmt::print("Flatten\n");
    ag::compute(res.device, res.pipelines.ppFlattenStroke,
		ag::makeThreadGroupCount2D(res.canvas.width, res.canvas.height, 16, 16),
                glm::vec2{res.canvas.width, res.canvas.height},
                glm::vec4{brushProps.color[0], brushProps.color[1],
                          brushProps.color[2], brushProps.opacity},
                texStrokeMask, ag::RWTextureUnit(0, res.canvas.texBaseColorUV));
  }

  //
  void previewCanvas(Texture2D<ag::RGBA8>& texTarget) {}

  void paintSplat(const SplatProperties& splat) {
    // draw splat to stroke mask
	  fmt::print("Splat {} {} {}\n", splat.center.x, splat.center.y, splat.width);
    uniforms::Splat uSplat;
    uSplat.center = splat.center;
    uSplat.width = splat.width;
    uSplat.smoothness = splat.smoothness;
    if (res.ui.brushTip == BrushTip::Round) {
      auto dim =
          res.ui.brushTipTextures[res.ui.selectedBrushTip].tex.info.dimensions;
      uSplat.transform = getSplatTransform((unsigned)dim.x, (unsigned)dim.y, splat);
    } else
      uSplat.transform = getSplatTransform((unsigned)splat.width, (unsigned)splat.width, splat);

    if (res.ui.brushTip == BrushTip::Round)
      ag::draw(res.device, texStrokeMask,
               res.pipelines.ppDrawRoundSplatToStrokeMask,
               ag::DrawArrays(ag::PrimitiveType::Triangles, res.vboQuad),
               glm::vec2{res.canvas.width, res.canvas.height}, uSplat);
    else if (res.ui.brushTip == BrushTip::Textured)
      ag::draw(res.device, texStrokeMask,
               res.pipelines.ppDrawTexturedSplatToStrokeMask,
               ag::DrawArrays(ag::PrimitiveType::Triangles, res.vboQuad),
               ag::TextureUnit(
                   0, res.ui.brushTipTextures[res.ui.selectedBrushTip].tex,
                   res.samLinearClamp),
               glm::vec2{res.canvas.width, res.canvas.height}, uSplat);
  }

private:
  RawBufferSlice canvasData;
  ToolResources res;
  BrushProperties brushProps;
  BrushPath brushPath;
  Texture2D<ag::RGBA8> texStrokeMask;
};

#endif // !BRUSH_TOOL_HPP
