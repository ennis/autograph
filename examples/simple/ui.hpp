#ifndef UI_HPP
#define UI_HPP

#include <string>

#include <glm/glm.hpp>

#include <rxcpp/rx.hpp>
#include <rxcpp/rx-subjects.hpp>
#include <extra/input/input.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"

#include "tool.hpp"

namespace input = ag::extra::input;
namespace rxsub = rxcpp::rxsub;

// dummy event type
struct event_t {};
template <typename T> struct binding {
  binding(const T &initial_value)
      : behavior(initial_value), subscriber(behavior.get_subscriber()) {}

  auto observable() const { return behavior.get_observable(); }

  void set(const T &value) { subscriber.on_next(value); }

  T value() const { return behavior.get_value(); }

  rxsub::behavior<T> behavior;
  rxcpp::subscriber<T> subscriber;
};

struct event {
  event() {}
  void signal() { subject.get_subscriber().on_next(event_t()); }
  auto observable() const { return subject.get_observable(); }
  rxsub::subject<event_t> subject;
};

// ImGui-based user interface
class Ui {
public:
  Ui(GLFWwindow *window, input::Input &input_)
      : input(input_), lastMouseButtonOnGUI(false), activeTool(Tool::None),
        brushTip(BrushTip::Round), brushRotationJitter(0.0f),
        brushPositionJitter(0.0f), brushWidthJitter(0.0f), brushSpacing(1.0f),
        brushSpacingJitter(0.0f) {
    // do not register callbacks
    ImGui_ImplGlfwGL3_Init(window, false);

    canvasMouseButtons = input.mouseButtons().filter([this](auto ev) {
      return (ev.state == input::MouseButtonState::Pressed)
                 ? (lastMouseButtonOnGUI = ImGui::IsMouseHoveringAnyWindow())
                 : (lastMouseButtonOnGUI = false);
    });

    canvasMousePointer = input.mousePointer().filter(
        [this](auto ev) { return !lastMouseButtonOnGUI; });
  }

  void render(Device &device) {

    ImGui::ColorEdit3("Stroke color", strokeColor);
    ImGui::SliderFloat2("Light pos", lightPosXY, -0.5, 0.5);
    ImGui::SliderFloat("Stroke opacity", &strokeOpacity, 0.0, 1.0);
    ImGui::SliderFloat("Stroke width", &strokeWidth, 1.0, 300.0);
    /*ImGui::PlotHistogram("Illum curve", illumHist,
       NUM_ILLUM_HISTOGRAM_ENTRIES,
                         0, "", 0.0, 1.0, ImVec2(300, 150));*/
    /*if (ImGui::Button("Reload shaders", ImVec2(150, 20)))
      loadShaders();*/

    int nActiveTool = 0;
    switch (activeTool) {
    case Tool::None:
      nActiveTool = 0;
      break;
    case Tool::Brush:
      nActiveTool = 1;
      break;
    case Tool::Blur:
      nActiveTool = 2;
      break;
    case Tool::Smudge:
      nActiveTool = 3;
      break;
    case Tool::Select:
      nActiveTool = 4;
      break;
    }

    const char *toolNames[] = {"None", "Brush", "Blur", "Smudge", "Select"};
    ImGui::Combo("Tool", &nActiveTool, toolNames, 5);

    switch (nActiveTool) {
    case 0:
      activeTool = Tool::None;
      break;
    case 1:
      activeTool = Tool::Brush;
      break;
    case 2:
      activeTool = Tool::Blur;
      break;
    case 3:
      activeTool = Tool::Smudge;
      break;
    case 4:
      activeTool = Tool::Select;
      break;
    }

    int nBrushTip = 0;
    switch (brushTip) {
    case BrushTip::Round:
      nBrushTip = 0;
      break;
    case BrushTip::Textured:
      nBrushTip = 1;
      break;
    }

    const char *tipNames[] = {"Round", "Textured"};
    ImGui::Combo("Brush tip", &nBrushTip, tipNames, 2);

    switch (nBrushTip) {
    case 0:
      brushTip = BrushTip::Round;
    case 1:
      brushTip = BrushTip::Textured;
    }

    ImGui::SliderFloat("Brush opacity jitter", &strokeOpacityJitter, 0.0f, 1.0f);
    ImGui::SliderFloat("Brush width jitter", &brushWidthJitter, 0.0f, 100.0f);
    ImGui::SliderFloat("Brush rotation jitter", &brushRotationJitter, 0.0f,
                       1.0f);
    ImGui::SliderFloat("Brush position jitter", &brushPositionJitter, 0.0f,
                       50.0f);
    ImGui::SliderFloat("Brush spacing", &brushSpacing, 1.0f, 50.0f);
    ImGui::SliderFloat("Brush spacing jitter", &brushSpacingJitter, 1.0f,
                       50.0f);

    ImGui::Checkbox("Use textured brush", &useTexturedBrush);
    ImGui::Checkbox("Show reference shading", &showReferenceShading);
    ImGui::Checkbox("Show isolines", &showIsolines);

    if (ImGui::Button("Save"))
      saveCanvas.signal();
    ImGui::SameLine();
    if (ImGui::Button("Load"))
      loadCanvas.signal();

    ImGui::InputText("Path", saveFileName, 100);
  }

  void poll() {}

  // 'Reload shaders' button
  event reloadShaders;
  // 'load canvas' button
  event loadCanvas;
  // 'save canvas button'
  event saveCanvas;

  // stroke color in RGB
  float strokeColor[3];
  // stroke opacity in %
  float strokeOpacity;
  float strokeOpacityJitter;
  // brush radius in pixels
  float strokeWidth;
  // light position over the painting
  float lightPosXY[2];

  // TODO
  bool useTexturedBrush;
  bool showReferenceShading;
  bool showIsolines;

  char saveFileName[100] = "output.paint";

  Tool activeTool;
  BrushTip brushTip;
  // brush tip rotation jitter
  float brushRotationJitter;
  float brushPositionJitter;
  float brushWidthJitter;
  float brushSpacing;
  float brushSpacingJitter;

  // mouse events on canvas (triggered when the mouse is not focused on the
  // canvas)
  rxcpp::observable<input::MouseButtonEvent> canvasMouseButtons;
  // mouse pointer position (last value + events)
  rxcpp::observable<input::MousePointerEvent> canvasMousePointer;
  //

private:
  input::Input &input;
  bool lastMouseButtonOnGUI;
};

#endif
