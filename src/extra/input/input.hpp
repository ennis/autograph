#ifndef INPUT_HPP
#define INPUT_HPP

#include <cstdint>

#include <rxcpp/rx.hpp>
#include <rxcpp/rx-subjects.hpp>

#include <eggs/variant.hpp>

#include <iostream>

namespace ag {
namespace extra {
namespace input {

enum class mouse_button_state { pressed, released };
enum class key_state { pressed, released, repeat };

struct mouse_button_event {
  unsigned button;
  mouse_button_state state;
};

struct cursor_event {
  // in client units (pixels)
  unsigned positionX;
  unsigned positionY;
};

struct mouse_move_event {
  double dx; // mysterious device units
  double dy;
};

struct mouse_scroll_event {
	/*mouse_scroll_event(double x, double y) : dx(x), dy(y)
	{}

	mouse_scroll_event(const mouse_scroll_event& rhs): dx(rhs.dx), dy(rhs.dy) {
		std::clog << "Copy\n";
	}
	mouse_scroll_event(mouse_scroll_event&& rhs) : dx(rhs.dx), dy(rhs.dy) {
		std::clog << "Move\n";
	}

	mouse_scroll_event& operator=(const mouse_scroll_event& rhs) {
		std::clog << "Assign\n";
		dx = rhs.dx;
		dy = rhs.dy;
	}
	*/
  double dx;
  double dy;
};

struct key_event {
  uint32_t code;
  key_state state;
};

struct stylus_proximity_event {
  // TODO
  // Touch, Hover, Leave
};

struct stylus_properties_event {
  double x;
  double y;
  double pressure;
  double tilt;
};

using input_event =
    eggs::variant<mouse_button_event, cursor_event, mouse_move_event,
                  mouse_scroll_event, key_event, stylus_proximity_event,
                  stylus_properties_event>;

struct input_event_source {
  virtual ~input_event_source() {}
};

class input2 {
public:
  template <typename EventSource, typename... Args>
  void make_event_source(Args&&... args) {
    // TODO: event source must inherit from input_event_source
    auto src = std::make_unique<EventSource>(event_stream.get_subscriber(),
                                             std::forward<Args>(args)...);
    event_sources.emplace_back(std::move(src));
  }

  auto events() { return event_stream.get_observable(); }

private:
  rxcpp::rxsub::subject<input_event> event_stream;
  std::vector<std::unique_ptr<input_event_source>> event_sources;
};

// helpers
template <typename Type>
auto input_filter(const rxcpp::observable<input_event>& event_stream) {
  using namespace rxcpp;
  return event_stream.filter([](const auto& ev) { return !!ev.target<Type>(); })
	  .map([](const auto& ev) { return *ev.target<Type>(); });
}
}
}
}
#endif
