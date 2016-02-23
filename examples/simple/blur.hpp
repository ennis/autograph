#ifndef BLUR_HPP
#define BLUR_HPP

#include "brush_tool.hpp"
#include "brush_path.hpp"

// Blur brush in LdotN space

class BlurTool : public BrushTool
{
public:
	BlurTool(const ToolResources& resources) : BrushTool(resources), res(resources)
	{}


	virtual ~SmudgeTool()
	{}

	void beginStroke(const PointerEvent& event) override {
		brushPath = {};
		brushProps = brushPropsFromUi(res.ui);
		brushPath.addPointerEvent(
			event, brushProps, [this](auto splat) { this->blur(splat); });
	}

	void continueStroke(const PointerEvent& event) override {
		brushPath.addPointerEvent(
			event, brushProps, [this](auto splat) { this->blur(splat); });
	}

	void endStroke(const PointerEvent& event) override {
	}

	void blur(const SplatProperties& splat)  {
		// sample LdotN under cursor center
	}

private:
	ToolResources& res;
	BrushPath brushPath;
	BrushProperties brushProps;
};

#endif