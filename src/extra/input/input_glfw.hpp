#ifndef INPUT_GLFW_HPP
#define INPUT_GLFW_HPP

#include "input.hpp"

#include <GLFW/glfw3.h>
#include <vector>

namespace ag {
namespace extra {
namespace input {
class glfw_input_event_source : public input_event_source {
public:
  glfw_input_event_source(rxcpp::subscriber<input_event> subscriber_,
                          GLFWwindow* window_)
      : subscriber(subscriber_), window(window_) {
    glfwSetCharCallback(window, GLFWCharHandler);
    glfwSetCursorEnterCallback(window, GLFWCursorEnterHandler);
    glfwSetCursorPosCallback(window, GLFWCursorPosHandler);
    glfwSetKeyCallback(window, GLFWKeyHandler);
    glfwSetMouseButtonCallback(window, GLFWMouseButtonHandler);
    glfwSetScrollCallback(window, GLFWScrollHandler);
    instance = this;
  }

  ~glfw_input_event_source() override {}

  void on_mouse_button(GLFWwindow* window, int button, int action, int mods) {
    subscriber.on_next(mouse_button_event{
        (unsigned)button, action == GLFW_PRESS ? mouse_button_state::pressed
                                               : mouse_button_state::released});
  }

  void on_cursor_pos(GLFWwindow* window, double xpos, double ypos) {
    subscriber.on_next(cursor_event{(unsigned)xpos, (unsigned)ypos});
  }

  void on_cursor_enter(GLFWwindow* window, int entered) {
    // TODO
  }

  void on_scroll(GLFWwindow* window, double xoffset, double yoffset) {
    subscriber.on_next(mouse_scroll_event{xoffset, yoffset});
  }

  void on_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
    subscriber.on_next(key_event{
        (unsigned)key,
        action == GLFW_PRESS
            ? key_state::pressed
            : (action == GLFW_REPEAT
                   ? key_state::repeat
                   : (action == GLFW_RELEASE ? key_state::released
                                             : key_state::released))});
  }

  void on_char(GLFWwindow* window, unsigned int codepoint) {}

private:
  rxcpp::subscriber<input_event> subscriber;

  GLFWwindow* window;
  static glfw_input_event_source* instance;

  // GLFW event handlers
  static void GLFWMouseButtonHandler(GLFWwindow* window, int button, int action,
                                     int mods) {
    instance->on_mouse_button(window, button, action, mods);
  }

  static void GLFWCursorPosHandler(GLFWwindow* window, double xpos,
                                   double ypos) {
    instance->on_cursor_pos(window, xpos, ypos);
  }

  static void GLFWCursorEnterHandler(GLFWwindow* window, int entered) {
    instance->on_cursor_enter(window, entered);
  }

  static void GLFWScrollHandler(GLFWwindow* window, double xoffset,
                                double yoffset) {
    instance->on_scroll(window, xoffset, yoffset);
  }

  static void GLFWKeyHandler(GLFWwindow* window, int key, int scancode,
                             int action, int mods) {
    instance->on_key(window, key, scancode, action, mods);
  }

  static void GLFWCharHandler(GLFWwindow* window, unsigned int codepoint) {
    instance->on_char(window, codepoint);
  }

  static void GLFWErrorCallback(int error, const char* description) {}
};
}
}
}

#endif // !INPUT_GLFW_HPP
