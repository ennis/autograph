#ifndef BRUSH_PATH_HPP
#define BRUSH_PATH_HPP

#include <vector>

#include "ui.hpp"

#include <glm/gtc/random.hpp>

namespace input = ag::extra::input;

struct PointerEvent {
  PointerEvent(unsigned x_, unsigned y_, float pressure_ = 1.0f)
      : positionX(x_), positionY(y_), pressure(pressure_) {}

  unsigned positionX;
  unsigned positionY;
  float pressure = 1.0f;
};

struct BrushProperties {
  float color[3];
  float opacity;
  float opacityJitter;
  float width;
  float widthJitter;
  float spacing;
  float spacingJitter;
  float rotation;
  float rotationJitter;
};

BrushProperties brushPropsFromUi(Ui& ui) {
  BrushProperties props;
  props.color[0] = ui.strokeColor[0];
  props.color[1] = ui.strokeColor[1];
  props.color[2] = ui.strokeColor[2];
  props.opacity = ui.strokeOpacity;
  props.opacityJitter = ui.strokeOpacityJitter;
  props.width = ui.strokeWidth;
  props.widthJitter = ui.brushWidthJitter;
  props.spacing = ui.brushSpacing;
  props.spacingJitter = ui.brushSpacingJitter;
  props.rotation = 0.0f;
  props.rotationJitter = ui.brushRotationJitter;
  return props;
}

// Evaluated properties of the splat to draw
struct SplatProperties {
  glm::vec2 center;
  float color[3];
  float opacity;
  float width; // scale
  float rotation;
};

glm::mat3x4 getSplatTransform(unsigned tipWidth, unsigned tipHeight,
                              const SplatProperties& splat) {
  glm::vec2 scale{1.0f};
  if (tipWidth < tipHeight)
    scale = glm::vec2{1.0f, (float)tipHeight / (float)tipWidth};
  else
    scale = glm::vec2{(float)tipWidth / (float)tipHeight, 1.0f};

  scale *= splat.width;

  return glm::mat3x4(
      glm::scale(glm::rotate(glm::translate(glm::mat3x3(1.0f), splat.center),
                             splat.rotation),
                 scale));
}

ag::Box2D getSplatFootprint(unsigned tipWidth, unsigned tipHeight,
                            const SplatProperties& splat) {
  auto transform = getSplatTransform(tipWidth, tipHeight, splat);
  auto topleft = transform * glm::vec3{-1.0f, -1.0f, 1.0f};
  auto bottomright = transform * glm::vec3{1.0f, 1.0f, 1.0f};
  return ag::Box2D{(unsigned)topleft.x, (unsigned)topleft.y,
                   (unsigned)bottomright.x, (unsigned)bottomright.y};
}

template <typename T> T evalJitter(T ref, T jitter) {
  return ref + glm::linearRand(-jitter, jitter);
}

SplatProperties evalSplat(const BrushProperties& props, glm::vec2 center) {
  SplatProperties ret;
  ret.center = center;
  ret.color[0] = props.color[0];
  ret.color[1] = props.color[1];
  ret.color[2] = props.color[2];
  ret.opacity = evalJitter(props.opacity, props.opacityJitter);
  ret.width = evalJitter(props.width, props.widthJitter);
  ret.rotation = evalJitter(props.rotation,
                            2.0f * glm::pi<float>() * props.rotationJitter);
  return ret;
}

// Brush path: convert a sequence of pointer position to a sequence of
// splat positions.
// TODO smoothing
struct BrushPath {
  // Call this when the mouse has moved
  template <typename F>
  void addPointerEvent(const PointerEvent& ev, const BrushProperties& props,
                       F f) {
    // eval spacing
    auto spacing = evalJitter(props.spacing, props.spacingJitter);
    if (spacing < 0.1f)
      spacing = 0.1f;

    if (pointerEvents.empty()) {
      pointerEvents.push_back(ev);
      f(evalSplat(props, glm::vec2((float)ev.positionX, (float)ev.positionY)));
      return;
    }

    auto last = pointerEvents.back();
    glm::vec2 curF((float)ev.positionX, (float)ev.positionY);
    glm::vec2 lastF((float)last.positionX, (float)last.positionY);
    auto length = glm::distance(lastF, curF);
    auto slack = pathLength;
    pathLength += length;
    auto pos = spacing - slack;

    while (pathLength > spacing) {
      // emit splat
      auto P = glm::mix(lastF, curF, (length > 0.01f) ? pos / length : 0.0f);
      f(evalSplat(props, P));
      pathLength -= spacing;
      pos += spacing;
    }

    pointerEvents.push_back(ev);
  }

  std::vector<PointerEvent> pointerEvents;
  float pathLength;
};

#endif
