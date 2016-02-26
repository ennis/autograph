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
enum class MouseButton : unsigned { Left, Right, Middle };

enum class StylusEventType { };


/*enum class input_event_type {
  mouse_button_event,
  mouse_move_event,
  key_event,
  stylus_proximity_event,
  stylus_properties_event
};*/

enum class mouse_button_state 
{
  pressed, released
};

enum class key_state 
{
  pressed, released, repeat
};

struct mouse_button_event 
{
  unsigned button;
  mouse_button_state state;
};

struct key_event 
{
  uint32_t code;
  key_state state;
};

struct stylus_proximity_event 
{
  // TODO
  // Touch, Hover, Leave
};

struct stylus_properties_event 
{
  double x;
  double y;
  double pressure;
  double tilt;
};

/*using input_event = eggs::variant<
  mouse_button_event,
  cursor_event,
  mouse_move_event, // raw values + cursor delta
  key_event,
  stylus_proximity_event,
  stylus_properties_event
>;*/


/*class input2 
{
public:
  auto events() 
};*/

//////////////////// Event types
struct KeyEvent {
  uint32_t code;
  KeyEventType type;
};

// sent when the stylus has moved on the surface or near the surface of the tablet
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
  unsigned positionX; // in client area pixels
  unsigned positionY;
};

struct MouseScrollEvent {
    double delta;   // in some mysterious units
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

struct InputSubjects {
  rxcpp::rxsub::subject<KeyEvent> keys;
  rxcpp::rxsub::subject<MouseButtonEvent> mouse_buttons;
  rxcpp::rxsub::subject<MousePointerEvent> mouse_pointer;
  rxcpp::rxsub::subject<MouseScrollEvent> mouse_scroll;
  rxcpp::rxsub::subject<StylusEvent> stylus;
};


class InputEventSource {
public:
  virtual ~InputEventSource() {}

  virtual void poll(InputSubjects& subjects) = 0;
};

//////////////////// input
class Input {
public:
  Input()
  {
  }

  void registerEventSource(std::unique_ptr<InputEventSource>&& eventSource) {
    eventSources.emplace_back(std::move(eventSource));
  }

  auto keys() const { return subjects.keys.get_observable(); }
  auto mouseButtons() const {  return subjects.mouse_buttons.get_observable(); }
  auto stylus() const { return subjects.stylus.get_observable(); }
  auto mousePointer() const { return subjects.mouse_pointer.get_observable(); }
  auto mouseScroll() const { return subjects.mouse_scroll.get_observable(); }

  // returns a behavior that hold the last known key state of the specified key
  auto keyState(uint32_t keyCode, KeyState init_state = KeyState::Released) {
    auto b = rxcpp::rxsub::behavior<KeyState>(init_state);
        subjects.keys.get_observable().filter([=](auto ev) { return ev.code == keyCode; })
        .subscribe(
            [=](auto ev) { b.get_subscriber().on_next(KeyState::Released); });
    return b;
  }

  void poll() {
    for (auto& src : eventSources)
      src->poll(subjects);
  }

private:
  // subjects
  InputSubjects subjects;

  // backends (input event sources)
  std::vector<std::unique_ptr<InputEventSource>> eventSources;
};
}
}
}

#endif
