#ifndef BLUR_HPP
#define BLUR_HPP

#include "../brush_tool.hpp"
#include "../brush_path.hpp"

// Blur brush in LdotN space

class BlurTool : public BrushTool
{
public:
	BlurTool(const ToolResources& resources) : BrushTool(resources), res(resources)
        {
            fmt::print(std::clog, "Init BlurTool\n");
        }


        virtual ~BlurTool()
        {
            fmt::print(std::clog, "Deinit BlurTool\n");
        }

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

            struct Uniforms {
                glm::vec2 center;
                float width;
            };

            // splat a gaussian kernel on the 1D parameter map, centered on LdotN
            ag::Box2D footprintBox = getSplatFootprint(splat.width, splat.width, splat);
            ag::compute(res.device, res.pipelines.ppBlurBrush, ag::makeThreadGroupCount2D(kShadingCurveSamplesSize, 1u, 16u, 1u),
                        Uniforms { splat.center, splat.width },
                        res.canvas.texShadingTermSmooth,
                        RWTextureUnit(0, res.canvas.texBlurParametersLN));

	}

private:
        ToolResources res;
	BrushPath brushPath;
	BrushProperties brushProps;
};

#endif
