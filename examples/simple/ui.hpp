#ifndef UI_HPP
#define UI_HPP

#include <string>

#include <glm/glm.hpp>

#include <extra/input/input.hpp>
#include <rxcpp/rx-subjects.hpp>
#include <rxcpp/rx.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"

#include "canvas.hpp"
#include "tool.hpp"
#include "types.hpp"

namespace input = ag::extra::input;
namespace rxsub = rxcpp::rxsub;

// dummy event type
struct event_t {};
template <typename T> struct binding {
  binding(const T& initial_value)
      : behavior(initial_value), subscriber(behavior.get_subscriber()) {}

  auto observable() const { return behavior.get_observable(); }

  void set(const T& value) { subscriber.on_next(value); }

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

struct BrushTipTexture {
  std::string name;
  Texture2D<ag::RGBA8> tex;
};

// ImGui-based user interface
class Ui {
public:
  Ui(GLFWwindow* window, input::input2& input_)
      : input(input_), histH(kShadingCurveSamplesSize),
        histS(kShadingCurveSamplesSize), histV(kShadingCurveSamplesSize),
        histBlurRadius(kShadingCurveSamplesSize),
        cursorPosition(std::make_tuple(0u, 0u)) {

    using namespace input;

    std::fill(begin(histH), end(histH), 0.0f);
    std::fill(begin(histS), end(histS), 0.0f);
    std::fill(begin(histV), end(histV), 0.0f);
    // do not register callbacks
    ImGui_ImplGlfwGL3_Init(window, false);

    canvasMouseButtons =
        input_filter<mouse_button_event>(input.events())
            .filter([this](auto ev) {
              return (ev.state == input::mouse_button_state::pressed)
                         ? !(lastMouseButtonOnGUI =
                                 ImGui::IsMouseHoveringAnyWindow())
                         : !(lastMouseButtonOnGUI = false);
            });

    canvasMousePointer =
        input_filter<cursor_event>(input.events()).filter([this](auto ev) {
          return !lastMouseButtonOnGUI;
        });

    canvasMouseScroll = input_filter<mouse_scroll_event>(input.events())
                            .filter([this](auto ev) {
                              return !ImGui::IsMouseHoveringAnyWindow();
                            });

    // cursor position
    input_filter<cursor_event>(input.events())
        .subscribe(subscription, [this](const cursor_event& ev) {
          this->cursorPosition.get_subscriber().on_next(
              std::make_tuple(ev.positionX, ev.positionY));
        });
  }

  ~Ui() { subscription.unsubscribe(); }

  void render(Device& device) {
    ImGui_ImplGlfwGL3_NewFrame();
    ImGui::ColorEdit3("Stroke color", strokeColor.data());
    ImGui::SliderFloat2("Light pos", lightPosXY.data(), -1.5, 1.5);
    ImGui::SliderFloat("Stroke opacity", &strokeOpacity, 0.0, 1.0);
    ImGui::SliderFloat("Stroke width", &strokeWidth, 1.0, 300.0);

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
    case Tool::Camera:
      nActiveTool = 5;
      break;
    }

    const char* toolNames[] = {"None",   "Brush",  "Blur",
                               "Smudge", "Select", "Camera"};
    ImGui::Combo("Tool", &nActiveTool, toolNames, 6);

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
    case 5:
      activeTool = Tool::Camera;
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

    const char* tipNames[] = {"Round", "Textured"};
    ImGui::Combo("Brush tip", &nBrushTip, tipNames, 2);

    switch (nBrushTip) {
    case 0:
      brushTip = BrushTip::Round;
      break;
    case 1:
      brushTip = BrushTip::Textured;
      break;
    }

    if (!brushTipTextures.empty()) {
      std::vector<const char*> tipTexNames;
      tipTexNames.reserve(brushTipTextures.size());
      for (const auto& tip : brushTipTextures)
        tipTexNames.push_back(tip.name.c_str());
      ImGui::Combo("Tip texture", &selectedBrushTip, tipTexNames.data(),
                   (int)tipTexNames.size());
    }

    ImGui::SliderFloat("Smoothness", &brushSmoothness, 0.0f, 1.0f);
    ImGui::SliderFloat("Brush opacity jitter", &strokeOpacityJitter, 0.0f,
                       1.0f);
    ImGui::SliderFloat("Brush width jitter", &brushWidthJitter, 0.0f, 100.0f);
    ImGui::SliderFloat("Brush rotation jitter", &brushRotationJitter, 0.0f,
                       1.0f);
    ImGui::SliderFloat("Brush position jitter", &brushPositionJitter, 0.0f,
                       50.0f);
    ImGui::SliderFloat("Brush spacing", &brushSpacing, 1.0f, 50.0f);
    ImGui::SliderFloat("Brush spacing jitter", &brushSpacingJitter, 1.0f,
                       50.0f);

    ImGui::Checkbox("Use textured brush", &useTexturedBrush);
    ImGui::Checkbox("Override shading curve", &overrideShadingCurve);
    ImGui::Checkbox("Show reference shading", &showReferenceShading);
    ImGui::Checkbox("Show isolines", &showIsolines);
    ImGui::Checkbox("DEBUG - Show base color", &showBaseColor);
    ImGui::Checkbox("DEBUG - Show shading offsets", &showHSVOffset);

    if (ImGui::Button("Save"))
      saveCanvas.signal();
    ImGui::SameLine();
    if (ImGui::Button("Load"))
      loadCanvas.signal();

    ImGui::InputText("Path", saveFileName, 100);

    ImGui::PlotHistogram("H curve", histH.data(), kShadingCurveSamplesSize, 0,
                         "", 0.0, 1.0,
                         ImVec2((float)kShadingCurveSamplesSize, 60.0f));
    ImGui::PlotHistogram("S curve", histS.data(), kShadingCurveSamplesSize, 0,
                         "", 0.0, 1.0,
                         ImVec2((float)kShadingCurveSamplesSize, 60.0f));
    ImGui::PlotHistogram("V curve", histV.data(), kShadingCurveSamplesSize, 0,
                         "", 0.0, 1.0,
                         ImVec2((float)kShadingCurveSamplesSize, 60.0f));
    ImGui::PlotHistogram("Blur radius", histBlurRadius.data(),
                         kShadingCurveSamplesSize, 0, "", 0.0, 1.0,
                         ImVec2((float)kShadingCurveSamplesSize, 60.0f));

    ImGui::Render();
  }

  // 'Reload shaders' button
  event reloadShaders;
  // 'load canvas' button
  event loadCanvas;
  // 'save canvas button'
  event saveCanvas;

  // stroke color in RGB
  std::array<float, 3> strokeColor{{0.0f, 0.0f, 0.0f}};
  // stroke opacity in %
  float strokeOpacity = 1.0f;
  float strokeOpacityJitter = 0.0f;
  // brush radius in pixels
  float strokeWidth = 5.0f;
  // light position over the painting
  std::array<float, 2> lightPosXY{{0.0f, 0.0f}};

  // TODO
  bool useTexturedBrush = false;
  bool showReferenceShading = true;
  bool showIsolines = true;
  bool overrideShadingCurve = true;

  // debug
  bool showHSVOffset = false;
  bool showBaseColor = false;

  char saveFileName[100] = "output.paint";
  std::vector<BrushTipTexture> brushTipTextures;
  int selectedBrushTip = 0;

  Tool activeTool = Tool::Brush;
  BrushTip brushTip = BrushTip::Round;
  // brush tip rotation jitter
  float brushRotationJitter = 0.0f;
  float brushPositionJitter = 0.0f;
  float brushWidthJitter = 0.0f;
  float brushSpacing = 1.0f;
  float brushSpacingJitter = 0.0f;
  float brushSmoothness;

  // histograms
  std::vector<float> histH;
  std::vector<float> histS;
  std::vector<float> histV;
  std::vector<float> histBlurRadius;

  // current cursor position on the canvas
  rxcpp::rxsub::behavior<std::tuple<unsigned, unsigned>> cursorPosition;

  // mouse events on canvas (triggered when the mouse is not focused on the
  // canvas)
  rxcpp::observable<input::mouse_button_event> canvasMouseButtons;
  // mouse pointer position (last value + events)
  rxcpp::observable<input::cursor_event> canvasMousePointer;
  // scroll events
  rxcpp::observable<input::mouse_scroll_event> canvasMouseScroll;

private:
  input::input2& input;
  rxcpp::composite_subscription subscription;
  bool lastMouseButtonOnGUI;
  unsigned mouseX;
  unsigned mouseY;
};

#endif
