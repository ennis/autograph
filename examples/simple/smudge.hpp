#ifndef SMUDGE_HPP
#define SMUDGE_HPP
// smudge tool

#include "brush_tool.hpp"

class Smudge {
public:
  Smudge(const ToolResources& resources_) : res(resources_) {}

  void beginStroke(const PointerEvent& event) {
    brushPath = {};
    brushProps = brushPropsFromUi(res.ui);
    brushPath.addPointerEvent(event, brushProps,
                              [&](auto splat) { smudge(splat); });
  }

  void continueStroke(const PointerEvent& event) {
    brushPath.addPointerEvent(event, brushProps,
                              [&](auto splat) { smudge(splat); });
  }

  void endStroke(const PointerEvent& event) {
    // brushPath.addPointerEvent(event, brushProps, [&](auto
    // splat){paintSplat(splat);});
  }

private:
  BrushPath brushPath;
  ToolResources& res;
};

#endif