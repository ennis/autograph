#ifndef SMUDGE_HPP
#define SMUDGE_HPP
// smudge tool

#include "brush_tool.hpp"

class Smudge 
{
public:
 Smudge(const ToolResources& resources_) : res(resources_) {
    texSmudgeFootprint = res.device.createTexture2D<ag::RGBA8>(glm::uvec2{512, 512});
  }

    void beginStroke(const PointerEvent& event) {
        ag::clear(res.device, texSmudgeFootprint, ag::ClearColor {0.0f, 0.0f, 0.0f, 0.0f});
    brushPath = {};
    brushProps = brushPropsFromUi(res.ui);
    brushPath.addPointerEvent(event, brushProps,
                              [this](auto splat) { this->smudge(splat, true); });
  }

  void continueStroke(const PointerEvent& event) {
    brushPath.addPointerEvent(event, brushProps,
                              [this](auto splat) { this->smudge(splat, false); });
  }

  void endStroke(const PointerEvent& event) {
    // brushPath.addPointerEvent(event, brushProps, [&](auto
    // splat){paintSplat(splat);});

  }

  void smudge(const SplatProperties& splat, bool first)
  {
    ag::Box2D footprintBox;
    if (res.ui.brushTip == BrushTip::Textured) {
      auto dim = res.ui.brushTipTextures[res.ui.selectedBrushTip].tex.info.dimensions;
      footprintBox = getSplatFootprint(dim.x, dim.y, splat);
    } else
      footprintBox = getSplatFootprint(splat.width, splat.width, splat);

    struct SmudgeUniforms {
      glm::uvec2 origin;
      glm::uvec2 size;
      float opacity;
    };

    SmudgeUniforms u{{footprintBox.xmin, footprintBox.ymin},
                     {footprintBox.width(), footprintBox.height()},
                     first ? 0.0f : res.ui.strokeOpacity};

    if (res.ui.brushTip == BrushTip::Textured)
      ag::compute(res.device, res.pipelines.ppSmudge, ag::ThreadGroupCount{ footprintBox.width(), footprintBox.height(), 1u},
          glm::vec2 {(float)res.canvas.width, (float)res.canvas.height},
          RWTextureUnit(0, res.canvas.texBaseColorUV),
          RWTextureUnit(1, texSmudgeFootprint), u,
          res.ui.brushTipTextures[res.ui.selectedBrushTip].tex);
  }


private:
    Texture2D<ag::RGBA8> texSmudgeFootprint;
  BrushPath brushPath;
  BrushProperties brushProps;
        ToolResources res;
};

  using SmudgeTool = BrushTool<Smudge>;

#endif
