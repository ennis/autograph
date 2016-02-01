#ifndef INPUT_HPP
#define INPUT_HPP

#include <cstdint>

#include <rxcpp/rx-subjects.hpp>
#include <rxcpp/rx.hpp>

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
  StylusDeviceInfo& deviceInfo;
  unsigned positionX;
  unsigned positionY;
  float pressure;
  float tilt;
};

struct MouseButtonEvent {
  unsigned button;
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

struct InputSubscribers
{
	rxcpp::rxsub::subject<KeyEvent>::subscriber_type sub_keys;
	rxcpp::rxsub::subject<MouseButtonEvent>::subscriber_type sub_mouse_buttons;
	rxcpp::rxsub::subject<MousePointerEvent>::subscriber_type sub_mouse_pointer;
	rxcpp::rxsub::subject<StylusEvent>::subscriber_type subject_stylus;
};

// input backends:
// GLFWInput (GLFWwindow*): only glfw (cross-platform)
// Win32Input (HWND)
// GLFW_Win32Input (GLFWwindow*, HWND): GLFW for mouse, win32 for stylus support
//
// Graphics backend:
// DX (HWND)
// OpenGL (GLFWwindow?)

class InputEventSource
{
public:
	virtual ~InputEventSource();
	virtual void poll(InputSubscribers& subscribers) = 0;
};

//////////////////// input
class Input {
public:
  Input(typename D::InputOptions& options) : driver(options) {}

  auto keys() const { return obs_keys; }
  auto mouseButtons() const { return obs_mouse_buttons; }
  auto stylus() const { return obs_stylus; }
  auto mousePointer() const { return obs_mouse_pointer; }

  // returns a behavior that hold the last known key state of the specified key
  auto keyState(uint32_t keyCode, KeyState init_state = KeyState::Released) {
    auto b = rxcpp::rxsub::behavior<KeyState>(init_state);
    obs_keys.filter([=](auto ev) { return ev.keyCode == key; })
        .subscribe([](auto ev) { b.on_next(ev.type); });
    return b;
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
  std::vector<std::unique_ptr<InputEventSource> > eventSources;
};
}
}
}

#endif