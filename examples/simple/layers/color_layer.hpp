#ifndef COLOR_LAYER_HPP
#define COLOR_LAYER_HPP

#include "../layer.hpp"

// dynamic color layer (color curves)
struct DynamicColorLayer : public TLayer<LayerType::DynamicColor>
{
    void draw(Device& device, LayerContext& context) override
    {

    }
};

// static color layer (non-varying)
// use for drawing contours, etc.
struct ColorLayer : public TLayer<LayerType::Constant>
{

};

#endif
