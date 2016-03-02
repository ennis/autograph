#ifndef COLOR_LAYER_HPP
#define COLOR_LAYER_HPP

#include "../canvas.hpp"

// dynamic color layer (color curves)
class DynamicColorLayer : public TLayer<LayerType::DynamicColor>
{
public:
    DynamicColorLayer(gsl::span<float> initCurveH, gsl::span<float> initCurveS, gsl::span<float> initCurveV)
    void draw(LayerContext& context) override
    {

    }

private:
    // details are merged with
    // used for histogram calculation

};

// static color layer (non-varying)
// use for drawing contours, etc.
class ColorLayer : public TLayer<LayerType::Constant>
{
public:
    void draw(LayerContext& context) override
    {

    }

private:
    Texture2D<ag::RGBA8> tex;
};

#endif
