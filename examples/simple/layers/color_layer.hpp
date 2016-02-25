#ifndef COLOR_LAYER_HPP
#define COLOR_LAYER_HPP

#include "../layer.hpp"

// dynamic color layer (color curves)
struct DynamicColorLayer : public TLayer<LayerType::DynamicColor>
{
	void draw(Device& device, Texture2D<ag::RGBA8>& target, const ag::Box2D& rect) override 
	{
		// 
	}
};

// static color layer (non-varying)
struct ColorLayer : public TLayer<LayerType::Constant>
{

};

#endif