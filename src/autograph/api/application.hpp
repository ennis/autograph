#ifndef APPLICATION_HPP
#define APPLICATION_HPP

namespace ag 
{
	// Main application object
	// Handles event loop, window creation in a api-agnostic manner
	template <typename API>
	class Application
	{
	public:

		// options: initWidth, initHeight, fullscreen, can be resized, etc.
		Window<API> createWindow(/*options*/);
		// createPresentableSurface(window) -> Surface<typename API::GraphicsAPI, RGBA8, float>

		// observable: client area size
		// observable: other system events?

	private:
		API& api;
	};
}

#endif