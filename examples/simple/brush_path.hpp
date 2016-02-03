#ifndef BRUSH_PATH_HPP
#define BRUSH_PATH_HPP

#include <vector>

#include "ui.hpp"

// Brush path: convert a sequence of mouse pointer events to a sequence of 
// splat positions. 
// TODO smoothing
struct BrushPath {
  template <typename F>
  void addPointerEvent(const MousePointerEvent& ev, float spacing, F f) {
    if (pointerEvents.empty()) {
      pointerEvents.push_back(ev);
      f(glm::vec2((float)ev.x, (float)ev.y));
      return;
    }

    auto last = pointerEvents.back();
    glm::vec2 curF((float)ev.x, (float)ev.y);
    glm::vec2 lastF((float)last.x, (float)last.y);
    auto length = glm::distance(lastF, curF);
    auto slack = pathLength;
    pathLength += length;
    auto pos = spacing - slack;

    while (pathLength > spacing) {
      // emit splat
      auto P = glm::mix(lastF, curF, (length > 0.01) ? pos / length : 0.0f);
      f(P);
      pathLength -= spacing;
      pos += spacing;
    }

    pointerEvents.push_back(cursorPos);
  }

  std::vector<MousePointerEvent> pointerEvents;
  float pathLength;
};


#endif