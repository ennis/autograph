#ifndef TYPES_HPP
#define TYPES_HPP

#include <autograph/backend/opengl/backend.hpp>
#include <autograph/device.hpp>

using GL = ag::opengl::OpenGLBackend;
using Device = ag::Device<GL>;
template <typename Pixel> using Texture2D = ag::Texture2D<Pixel, GL>;
template <typename Pixel> using Texture1D = ag::Texture1D<Pixel, GL>;
using GraphicsPipeline = ag::GraphicsPipeline<GL>;
using ComputePipeline = ag::ComputePipeline<GL>;
using Mesh = samples::Mesh<GL>;

#endif
