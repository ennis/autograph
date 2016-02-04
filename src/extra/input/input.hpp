#ifndef INPUT_HPP
#define INPUT_HPP

#include <cstdint>

#include <rxcpp/rx.hpp>
#include <rxcpp/rx-subjects.hpp>

namespace ag {
namespace extra {
namespace input {

enum class KeyEventType { Pressed, Released, Repeat };
enum class KeyState { Pressed, Released };
enum class MouseButtonState { Pressed, Released };

//////////////////// Event types
struct KeyEvent {
  uint32_t code;
  KeyEventType type;
};

struct StylusEvent {
  // StylusDeviceInfo& deviceInfo;
  unsigned positionX;
  unsigned positionY;
  float pressure;
  float tilt;
};

struct MouseButtonEvent {
  unsigned button;
  MouseButtonState state;
};

struct MousePointerEvent {
  unsigned positionX; // in client area
  unsigned positionY;
};

//////////////////// Device info
struct MouseDeviceInfo {
  unsigned numButtons;
};

// It may not be a stylus...
struct StylusDeviceInfo {
  std::string name;
  unsigned sizeX;
  unsigned sizeY;
};

struct InputSubscribers {
  rxcpp::rxsub::subject<KeyEvent>::subscriber_type sub_keys;
  rxcpp::rxsub::subject<MouseButtonEvent>::subscriber_type sub_mouse_buttons;
  rxcpp::rxsub::subject<MousePointerEvent>::subscriber_type sub_mouse_pointer;
  rxcpp::rxsub::subject<StylusEvent>::subscriber_type sub_stylus;
};

// input backends:
// GLFWInput (GLFWwindow*): only glfw (cross-platform)
// Win32Input (HWND)
// GLFW_Win32Input (GLFWwindow*, HWND): GLFW for mouse, win32 for stylus support
//
// Graphics backend:
// DX (HWND)
// OpenGL (GLFWwindow?)

class InputEventSource {
public:
  virtual ~InputEventSource() {}

  virtual void poll(InputSubscribers& subscribers) = 0;
};

//////////////////// input
class Input {
public:
  Input()
      : subscribers(InputSubscribers{subject_keys.get_subscriber(),
                                     subject_mouse_buttons.get_subscriber(),
                                     subject_mouse_pointer.get_subscriber(),
                                     subject_stylus.get_subscriber()}) 
  {
	  obs_keys = subject_keys.get_observable();
	  obs_mouse_buttons = subject_mouse_buttons.get_observable();
	  obs_mouse_pointer = subject_mouse_pointer.get_observable();
	  obs_stylus = subject_stylus.get_observable();
  }

  void registerEventSource(std::unique_ptr<InputEventSource>&& eventSource) {
    eventSources.emplace_back(std::move(eventSource));
  }

  auto& keys() const { return obs_keys; }
  auto& mouseButtons() const { return obs_mouse_buttons; }
  auto& stylus() const { return obs_stylus; }
  auto& mousePointer() const { return obs_mouse_pointer; }

  // returns a behavior that hold the last known key state of the specified key
  auto keyState(uint32_t keyCode, KeyState init_state = KeyState::Released) {
    auto b = rxcpp::rxsub::behavior<KeyState>(init_state);
	subject_keys.get_observable().filter([=](auto ev) { return ev.code == keyCode; })
        .subscribe(
            [=](auto ev) { b.get_subscriber().on_next(KeyState::Released); });
    return b;
  }

  void poll() {
    for (auto& src : eventSources)
      src->poll(subscribers);
  }

private:
  // subjects
  rxcpp::rxsub::subject<KeyEvent> subject_keys;
  rxcpp::rxsub::subject<MouseButtonEvent> subject_mouse_buttons;
  rxcpp::rxsub::subject<MousePointerEvent> subject_mouse_pointer;
  rxcpp::rxsub::subject<StylusEvent> subject_stylus;

  // subscribers
  InputSubscribers subscribers;

  // observables
  rxcpp::observable<KeyEvent> obs_keys;
  rxcpp::observable<MouseButtonEvent> obs_mouse_buttons;
  rxcpp::observable<MousePointerEvent> obs_mouse_pointer;
  rxcpp::observable<StylusEvent> obs_stylus;


  // backends (input event sources)
  std::vector<std::unique_ptr<InputEventSource>> eventSources;
};
}
}
}

#endif
