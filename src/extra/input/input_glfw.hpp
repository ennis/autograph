#ifndef INPUT_GLFW_HPP
#define INPUT_GLFW_HPP

#include "input.hpp"

#include <GLFW/glfw3.hpp>

namespace ag {
namespace extra {
namespace input {
class GLFWInputBackend : public InputEventSource {
public:
	GLFWInputBackend(GLFWwindow* window_) : window(window_)
	{

	}

	void onMouseButton(GLFWwindow* window, int button, int action, int mods) {
	}

	void onCursorPos(GLFWwindow* window, double xpos, double ypos) {
	}

	void onCursorEnter(GLFWwindow* window, int entered) {}

	void onScroll(GLFWwindow* window, double xoffset, double yoffset) {}

	void onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
	}

	void onChar(GLFWwindow* window, unsigned int codepoint) {}



private:
  static Win32GLFWInputBackend* instance;

  // GLFW event handlers
  static void GLFWMouseButtonHandler(GLFWwindow* window, int button, int action,
                                     int mods) {
    instance->onMouseButton(window, button, action, mods);
  }

  static void GLFWCursorPosHandler(GLFWwindow* window, double xpos,
                                   double ypos) {
    frame_call++;
    // fmt::print("Cursor events in frame {}\n", frame_call);
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
