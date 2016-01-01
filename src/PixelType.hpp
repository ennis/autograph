#ifndef PIXEL_TYPE_HPP
#define PIXEL_TYPE_HPP

#include <glm/glm.hpp>
#include <backend/opengl/OpenGL.hpp>

namespace ag
{
	using namespace gl;

	template <typename T>
	struct PixelTypeTraits
	{
		static constexpr bool IsPixelType = false;
	};

	struct PixelTypeTraitsImpl
	{
		static constexpr bool IsPixelType = true;
	};

	template <> struct PixelTypeTraits<float> : public PixelTypeTraitsImpl {};
	template <> struct PixelTypeTraits<glm::u8vec3> : public PixelTypeTraitsImpl {};
	template <> struct PixelTypeTraits<glm::u8vec4> : public PixelTypeTraitsImpl {};

	using RGBA8 = glm::u8vec4;
	using RGB8 = glm::u8vec3;
	using R32F = glm::f32;
	using R8 = glm::u8;
}

#endif
