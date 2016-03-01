#ifndef DETAIL_HPP
#define DETAIL_HPP

#include "../brush_tool.hpp"
#include "../brush_path.hpp"

class DetailTool : public BrushTool {
public:
  DetailTool(const ToolResources& resources)
      : BrushTool(resources), res(resources) {}

  virtual ~DetailTool() {}

  void beginStroke(const PointerEvent& event) override {
    brushPath = {};
    brushProps = brushPropsFromUi(res.ui);
    brushPath.addPointerEvent(event, brushProps,
                              [this](auto splat) { this->detail(splat); });
  }

  void continueStroke(const PointerEvent& event) override {
    brushPath.addPointerEvent(event, brushProps,
                              [this](auto splat) { this->detail(splat); });
  }

  void endStroke(const PointerEvent& event) override {}

  void detail(const SplatProperties& splat) {
    struct Uniforms {
      glm::vec2 center;
      float width;
    };

    // splat a gaussian kernel on the 1D parameter map, centered on LdotN
    ag::Box2D footprintBox =
        getSplatFootprint((unsigned)splat.width, (unsigned)splat.width, splat);
    // Hack: re-use blur tool shader (same parameters)
    ag::compute(
        res.device, res.pipelines.ppBlurBrush,
        ag::makeThreadGroupCount2D(kShadingCurveSamplesSize, 1u, 16u, 1u),
        Uniforms{splat.center, splat.width}, res.canvas.texShadingTermSmooth,
        RWTextureUnit(0, res.canvas.texDetailMaskLN));
  }

private:
  ToolResources res;
  BrushPath brushPath;
  BrushProperties brushProps;
};

#endif
