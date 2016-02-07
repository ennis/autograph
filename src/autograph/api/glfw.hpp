#ifndef GLFW_HPP
#define GLFW_HPP

#include <GLFW/glfw3.hpp>
#include "../device.hpp"
#include "opengl_driver.hpp"

namespace ag {
namespace api {
namespace glfw {

	Window
		- getSurface -> ag::Surface (optional)
		- getNativeHandle -> ...
		- inputEvents -> observable<InputEvent>
	
	class API 
	{
	public:
		using GraphicsDriver = ag::api::opengl::OpenGLDriver;

	private:
		using D = ag::api::opengl::OpenGLDriver;

		// createWindow -> std::unique_ptr<Window>
		// events? received through the window?
		// seat: display + input device set
		// a window is associated to a seat
		// an APPLICATION is associated to a seat

		// The life of an event
		// Event -> event loop -> dispatch to window/object -> window -> handlers
		// Input: associated to window OR application?

		// Bikeshedding: rename window to view (shiny new terminology for the same thing)
		// on mobile platforms, there is no concept of window though

		std::unique_ptr<Window> createWindow();

	private:
		GLFWwindow* window;
	};
}
}
}

#endif