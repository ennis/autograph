#ifndef CANVAS_HPP
#define CANVAS_HPP

#include "types.hpp"

struct Canvas 
{
	Canvas(Device& device, unsigned width_, unsigned height_) 
	{}

	unsigned width;
	unsigned height;

	// Average shading curve
	Texture1D<float> texAvgShadingCurve;
	// lit-sphere
	Texture2D<float> texLitSphere;

	// HSV offset 
	Texture2D<RGBA8> texHSVOffset;
	// Blur radius
	Texture2D<R8> texBlurRadius;
	// Blur radius / shading space
	Texture2D<R8> texBlurRadiusShading;
	// TODO shading detail map?
};

#endif