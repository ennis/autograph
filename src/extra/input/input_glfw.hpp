#ifndef INPUT_GLFW_HPP
#define INPUT_GLFW_HPP

#include "input.hpp"

#include <vector>
#include <GLFW/glfw3.hpp>

namespace ag {
namespace extra {
namespace input {
class GLFWInputEventSource : public InputEventSource {
public:
        GLFWInputEventSource(GLFWwindow* window_) : window(window_)
	{
            glfwSetCharCallback(window, GLFWCharHandler);
            glfwSetCursorEnterCallback(window, GLFWCursorEnterHandler);
            glfwSetCursorPosCallback(window, GLFWCursorPosHandler);
            glfwSetKeyCallback(window, GLFWKeyHandler);
            glfwSetMouseButtonCallback(window, GLFWMouseButtonHandler);
            glfwSetScrollCallback(window, GLFWScrollHandler);
            instance = this;
	}

        ~GLFWInputEventSource() override
        {
        }

	void onMouseButton(GLFWwindow* window, int button, int action, int mods) {
            mouseButtonEvents.push_back(MouseButtonEvent {
                                            button,
                                        button, action == GLFW_PRESS ? MouseButtonState::Pressed : MouseButtonState::Release
                                        });
	}

	void onCursorPos(GLFWwindow* window, double xpos, double ypos) {
            mousePointerEvents.push_back()
	}

	void onCursorEnter(GLFWwindow* window, int entered) {}

        void onScroll(GLFWwindow* window, double xoffset, double yoffset) {
            // TODO (scroll event?)
        }

	void onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
            keyEvents.push_back(KeyEvent {
                                    key,
                                    action == GLFW_PRESS ? KeyEventType::Pressed : (
                                    action == GLFW_REPEAT ? KeyEventType::Repeat : (
                                    action == GLFW_RELEASE ? KeyEventType::Release : KeyEventType::Release))
                                });
	}

	void onChar(GLFWwindow* window, unsigned int codepoint) {}

        void poll(InputSubscribers& subscribers) override
        {
            // flush all events
            for (const auto& e : keyEvents) subscribers.sub_keys.on_next(e);
            for (const auto& e : mouseButtonEvents) subscribers.sub_mouse_buttons.on_next(e);
            for (const auto& e : mousePointerEvents) subscribers.sub_mouse_pointer.on_next(e);
            // TODO stylus, I guess
        }

private:
        std::vector<KeyEvent> keyEvents;
        std::vector<MouseButtonEvent> mouseButtonEvents;
        std::vector<MousePointerEvent> mousePointerEvents;

  static GLFWInputBackend* instance;

  // GLFW event handlers
  static void GLFWMouseButtonHandler(GLFWwindow* window, int button, int action,
                                     int mods) {
    instance->onMouseButton(window, button, action, mods);
  }

  static void GLFWCursorPosHandler(GLFWwindow* window, double xpos,
                                   double ypos) {
    instance->onCursorPos(window, xpos, ypos);
  }

  static void GLFWCursorEnterHandler(GLFWwindow* window, int entered) {
    instance->onCursorEnter(window, entered);
  }

  static void GLFWScrollHandler(GLFWwindow* window, double xoffset,
                                double yoffset) {
    instance->onScroll(window, xoffset, yoffset);
  }

  static void GLFWKeyHandler(GLFWwindow* window, int key, int scancode,
                             int action, int mods) {
    instance->onKey(window, key, scancode, action, mods);
  }

  static void GLFWCharHandler(GLFWwindow* window, unsigned int codepoint) {
    instance->onChar(window, codepoint);
  }

  static void GLFWErrorCallback(int error, const char* description) {}
};
}
}
}

#endif // !INPUT_GLFW_HPP
