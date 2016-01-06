#ifndef DRAW_HPP
#define DRAW_HPP

#include <tuple>

#include <Texture.hpp>
#include <Buffer.hpp>
#include <Device.hpp>

namespace ag
{
	////////////////////////// PrimitiveType
	enum class PrimitiveType
	{
		Points,
		Lines, 
		Triangles
	};


	////////////////////////// BindContext
	struct BindContext
	{
		unsigned textureBindingIndex = 0;
		unsigned samplerBindingIndex = 0;
		unsigned vertexBufferBindingIndex = 0;
		unsigned uniformBufferBindingIndex = 0;
	};

	////////////////////////// Drawables
	template <
		typename TIndexSource,
		typename ... TVertexSource
	>
	struct IndexedMesh_
	{
		// Primitive type
		PrimitiveType primitiveType;
		// vertex buffers
		std::tuple<TVertexSource...> vertex_sources;
		// index buffer
		TIndexSource index_source;
	};

    ////////////////////////// Draw command: DrawArrays
	template <
		typename D
	>
	struct DrawArrays_
	{
		PrimitiveType primitiveType;
		const RawBuffer<D>& vertex_buffer;
		size_t count;
		unsigned stride;

		void draw(Device<D>& device, BindContext& context)
		{
			device.backend.bindVertexBuffer(context.vertexBufferBindingIndex++, vertex_buffer.handle.get(), stride);
			device.backend.draw(primitiveType, 0, count);
		}
	};

	template <
		typename D,
		typename TVertex
	>
	DrawArrays_<D> DrawArrays(PrimitiveType primitiveType, const Buffer<D, TVertex[]>& vertex_buffer)
	{
		return DrawArrays_<D>{primitiveType, vertex_buffer, vertex_buffer.size(), sizeof(TVertex)};
	}


	////////////////////////// Binder: texture unit
	template <
		typename TextureTy,
		typename SamplerTy
	>
    struct TextureUnit_
	{
        TextureUnit_(unsigned unit_, const TextureTy& tex_, const SamplerTy& sampler_) :
			unit(unit_),
			tex(tex_),
			sampler(sampler_)
		{}

		unsigned unit;
		const TextureTy& tex;
		const SamplerTy& sampler;
	};

    template <
        typename TextureTy,
        typename SamplerTy
    >
    TextureUnit_<TextureTy, SamplerTy> TextureUnit(unsigned unit_, const TextureTy& tex_, const SamplerTy& sampler_)
    {
        return TextureUnit_<TextureTy, SamplerTy>(unit_, tex_, sampler_);
    }

    ////////////////////////// Binder: uniform slot
    template <
        typename BufferTy
    >
    struct Uniform_
    {
        Uniform_(unsigned slot_, const BufferTy& buf_) :
            slot(slot_),
            buf(buf_)
        {}

        unsigned slot;
        const BufferTy& buf;
    };

    template <
        typename BufferTy
    >
    Uniform_<BufferTy> Uniform(unsigned slot_, const BufferTy& buf_)
    {
        return Uniform_<BufferTy>(slot_, buf_);
    }

	namespace 
	{
		////////////////////////// Bind<Texture1D>
		template <
			typename D,
			typename TPixel
		>
		void bindOne(Device<D>& device, BindContext& context, const Texture1D<TPixel, D>& tex)
		{
			device.backend.bindTexture1D(context.textureBindingIndex++, tex.handle.get());
			device.getFrameScope().referenceTexture1D(tex.handle);
		}

		////////////////////////// Bind<Texture2D>
		template <
			typename D,
			typename TPixel
		>
		void bindOne(Device<D>& device, BindContext& context, const Texture2D<TPixel, D>& tex)
		{
			device.backend.bindTexture2D(context.textureBindingIndex++, tex.handle.get());
			device.getFrameScope().referenceTexture2D(tex.handle);
		}

		////////////////////////// Bind<Texture3D>
		template <
			typename D,
			typename TPixel
		>
		void bindOne(Device<D>& device, BindContext& context, const Texture3D<TPixel, D>& tex)
		{
			device.backend.bindTexture3D(context.textureBindingIndex++, tex.handle.get());
			device.getFrameScope().referenceTexture3D(tex.handle);
		}


        ////////////////////////// Bind<Sampler>
        template <
            typename D,
            typename TPixel
        >
        void bindOne(Device<D>& device, BindContext& context, const Sampler<D>& sampler)
        {
            device.backend.bindSampler(context.samplerBindingIndex++, sampler.handle.get());
            device.getFrameScope().referenceSampler(sampler.handle);
        }

		////////////////////////// Bind<TextureUnit<>>
		template <
			typename D,
			typename TextureTy,
			typename SamplerTy
		>
        void bindOne(Device<D>& device, BindContext& context, const TextureUnit_<TextureTy, SamplerTy>& tex_unit)
		{
			context.textureBindingIndex = tex_unit.unit;
			context.samplerBindingIndex = tex_unit.unit;
            bindOne(device, context, tex_unit.sampler);
			bindOne(device, context, tex_unit.tex);
		}

        ////////////////////////// Bind<T>
        template <
             typename D,
             typename T
        >
        void bindOne(Device<D>& device, BindContext& context, const T& value)
        {
            // allocate a temporary uniform buffer
            auto tmp_buf = device.createBuffer(value);
            device.backend.bindUniformBuffer(context.uniformBufferBindingIndex++, tmp_buf.handle.get());
            device.getFrameScope().referenceBuffer(tmp_buf.handle);
        }

		template <
			typename D,
			typename T
		>
		void bindImpl(Device<D>& device, BindContext& context, T&& resource)
		{
			bindOne(device, context, std::forward<T>(resource));
		}

		template <
			typename D,
			typename T,
			typename ... Rest
		> 
		void bindImpl(Device<D>& device, BindContext& context, T&& resource, Rest&&... rest)
		{
			bindOne(device, context, std::forward<T>(resource));
			bindImpl(device, context, std::forward<Rest>(rest)...);
		}

	}
	
	template <
		typename D,
		typename TSurface,
		typename Drawable
	>
	void draw(
		Device<D>& device,
		TSurface &&surface,
        GraphicsPipeline<D>& graphicsPipeline,
		Drawable &&drawable)
	{
		BindContext context;
        device.backend.bindSurface(surface.handle);
        device.backend.bindGraphicsPipeline(graphicsPipeline.handle.get());
		drawable.draw(device, context);
	}


	template <
		typename D,
		typename TSurface,
		typename Drawable,
		typename... TShaderResources
	>
	void draw(
		Device<D>& device,
		TSurface &&surface,
        GraphicsPipeline<D>& graphicsPipeline,
		Drawable &&drawable,
		TShaderResources &&... resources)
	{
		BindContext context;
		bindImpl(device, context, resources...);
        device.backend.bindSurface(surface.handle);
        device.backend.bindGraphicsPipeline(graphicsPipeline.handle.get());
        drawable.draw(device, context);
	}
	
}

#endif // !DRAW_HPP