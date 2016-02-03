#ifndef UI_HPP
#define UI_HPP

#include <string>

#include <glm/glm.hpp>

#include <rxcpp/rx.hpp>
#include <extras/input/input.hpp>

namespace input = ag::extras::input;
template <typename T> using subject = rxcpp::rxsub::subject<T>;
template <typename T> using behavior = rxcpp::rxsub::behavior<T>;

template <typename T> struct binding {
  binding(const T& initial_value)
      : behavior(initial_value), subscriber(behavior.get_subscriber()) {}

  auto observable() const { return behavior.get_observable(); }

  void set(const T& value) { subscriber.on_next(value); }

  T value() const { return behavior.get_value(); }

  behavior<T> behavior;
  subscriber<T> subscriber;
};

struct event {
  event() {}
  void signal() { subscriber.on_next(); }
  auto observable() const { return subject.get_observable(); }
  subject<void> subject;
  subscriber<T> subscriber;
};

// ImGui-based user interface
class Ui {
public:
  Ui(GLFWwindow* window, input::Input& input_) : input(input_) {
    ImGui_ImplGlfwGL3_Init(window, true);
  }

  void render(Device& device) {
    ImGui::ColorEdit3("Stroke color", currentStrokeColor);
    ImGui::SliderFloat2("Light pos", lightOffsetXY, -0.5, 0.5);
    ImGui::SliderFloat("Stroke opacity", &strokeOpacity, 0.0, 1.0);
    ImGui::SliderFloat("Stroke width", &strokeWidth, 1.0, 300.0);
    ImGui::PlotHistogram("Illum curve", illumHist, NUM_ILLUM_HISTOGRAM_ENTRIES,
                         0, "", 0.0, 1.0, ImVec2(300, 150));
    if (ImGui::Button("Reload shaders", ImVec2(150, 20)))
      loadShaders();
    ImGui::Checkbox("Use textured brush", &useTexturedBrush);
    ImGui::Checkbox("Show reference shading", &showReferenceShading);
    ImGui::Checkbox("Show isolines", &showIsolines);
    if (ImGui::Button("Save"))
      subSaveCanvas.get_subscriber().on_next();
    ImGui::SameLine();
    if (ImGui::Button("Load"))
      subLoadCanvas.get_subscriber().on_next();

    ImGui::InputText("Path", fileName, 100);
  }

  void poll() {
    // nothing to do
  }

  // 'Reload shaders' button
  event reloadShaders;
  // 'load canvas' button
  event loadCanvas;
  // 'save canvas button'
  event saveCanvas;

  // stroke color in RGB
  binding<glm::vec3> strokeColor;
  // stroke opacity in %
  binding<float> strokeOpacity;
  // brush radius in pixels
  binding<float> strokeWidthf;
  // light position over the painting
  binding<float> lightPosX;
  binding<float> lightPosY;

  // TODO
  binding<bool> useTexturedBrush;
  binding<bool> showReferenceShading;
  binding<bool> showIsolines; 

  binding<std::string> saveFileName = "output.paint";

  // mouse events on canvas (triggered when the mouse is not focused on the canvas)
  observable<MouseButtonEvent> canvasMouseButtons;
  // mouse pointer position (last value + events)
  behavior<MousePointerEvent> canvasMousePointer;

private:
  input::Input& input;
};

#endif