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

	////////////////////////// IndexType
	enum class IndexType
	{
		UShort,
		UInt
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
		typename D::BufferHandle::pointer buffer;
		size_t offset;
		size_t size;
		size_t stride;
		size_t count;

		void draw(Device<D>& device, BindContext& context)
		{
			device.backend.bindVertexBuffer(context.vertexBufferBindingIndex++, buffer, offset, size, stride);
			device.backend.draw(primitiveType, 0, count);
		}
	};

	////////////////////////// Draw command: DrawArrays (reduced form)
	struct DrawArrays0_
	{
		PrimitiveType primitiveType;
		size_t first;
		size_t count;

		template <typename D>
		void draw(Device<D>& device, BindContext& context)
		{
			device.backend.draw(primitiveType, 0, count);
		}
	};
	
	inline DrawArrays0_ DrawArrays(PrimitiveType primitiveType, size_t first, size_t count)
	{
		return DrawArrays0_{ primitiveType, first, count };
	}

	////////////////////////// Draw command: DrawIndexed (reduced form)
	struct DrawIndexed0_
	{
		PrimitiveType primitiveType;
		size_t first;
		size_t count;
		size_t baseVertex;
		
		template <typename D>
		void draw(Device<D>& device, BindContext& context)
		{
			device.backend.drawIndexed(primitiveType, 0, count, baseVertex);
		}
	};

	inline DrawIndexed0_ DrawIndexed(PrimitiveType primitiveType, size_t first, size_t count, size_t baseVertex)
	{
		return DrawIndexed0_{ primitiveType, first, count, baseVertex };
	}


	// Immediate version (put vertex data in the default upload buffer)
	template <
		typename TVertex
	>
	struct DrawArraysImmediate_
	{
		PrimitiveType primitiveType;
		gsl::span<TVertex> vertices;

		template <typename D>
		void draw(Device<D>& device, BindContext& context)
		{
			// upload to default upload buffer
			auto slice = device.pushDataToUploadBuffer(vertices);
			device.backend.bindVertexBuffer(context.vertexBufferBindingIndex++, slice.handle, slice.offset, slice.byteSize, sizeof(TVertex));
			device.backend.draw(primitiveType, 0, vertices.size());
		}
	};

	template <
		typename D,
		typename TVertex
	>
	DrawArrays_<D> DrawArrays(PrimitiveType primitiveType, const Buffer<D, TVertex[]>& vertex_buffer)
	{
		return DrawArrays_<D>{primitiveType, vertex_buffer.handle.get(), 0, vertex_buffer.byteSize(), sizeof(TVertex), vertex_buffer.size()};
	}
	
	template <
		typename TVertex
	>
	DrawArraysImmediate_<TVertex> DrawArrays(PrimitiveType primitiveType, gsl::span<TVertex> vertices)
	{
		return DrawArraysImmediate_<TVertex>{primitiveType, vertices};
	}

	////////////////////////// Binder: vertex buffer
	template <
		typename VertexTy,
		typename D
	>
	struct VertexBuffer_ 
	{
		const Buffer<D, VertexTy[]>& buf;
	};

	template <typename VertexTy, typename D>
	VertexBuffer_<VertexTy, D> VertexBuffer(const Buffer<D, VertexTy[]>& buf)
	{
		return VertexBuffer_<VertexTy, D> {buf};
	}

	////////////////////////// Binder: index buffer
	template <
		typename T,
		typename D
	>
	struct IndexBuffer_
	{
		const Buffer<D, T[]>& buf;
	};

	template <typename T, typename D>
	IndexBuffer_<T, D> IndexBuffer(const Buffer<D, T[]>& buf)
	{
		static_assert(std::is_same<T, unsigned short>::value || std::is_same<T, unsigned int>::value, "Unsupported index type");
		return IndexBuffer_<T, D> {buf};
	}
	
	////////////////////////// Binder: vertex array (vertices on the CPU)
	template <
		typename VertexTy
	>
	struct VertexArray_
	{
		gsl::span<const VertexTy> data;
	};

	template <typename VertexTy, typename D>
	VertexArray_<VertexTy> VertexArray(gsl::span<const VertexTy> data)
	{
		return VertexArray_<VertexTy> {data};
	}

	////////////////////////// Binder: texture unit
	template <
        typename TextureTy,
        typename D
	>
    struct TextureUnit_
	{
        TextureUnit_(unsigned unit_, const TextureTy& tex_, const Sampler<D>& sampler_) :
			unit(unit_),
			tex(tex_),
			sampler(sampler_)
		{}

		unsigned unit;
		const TextureTy& tex;
        const Sampler<D>& sampler;
	};

    template <typename T, typename D>
    TextureUnit_<Texture1D<T,D>, D> TextureUnit(unsigned unit_, const Texture1D<T,D>& tex_, const Sampler<D>& sampler_)
    {
        return TextureUnit_<Texture1D<T,D>, D>(unit_, tex_, sampler_);
    }

    template <typename T, typename D>
    TextureUnit_<Texture2D<T,D>, D> TextureUnit(unsigned unit_, const Texture2D<T,D>& tex_, const Sampler<D>& sampler_)
    {
        return TextureUnit_<Texture2D<T,D>, D>(unit_, tex_, sampler_);
    }

    template <typename T, typename D>
    TextureUnit_<Texture3D<T,D>, D> TextureUnit(unsigned unit_, const Texture3D<T,D>& tex_, const Sampler<D>& sampler_)
    {
        return TextureUnit_<Texture3D<T,D>, D>(unit_, tex_, sampler_);
    }

    ////////////////////////// Binder: uniform slot
    template <
        typename ResTy	// Buffer, BufferSlice or just a value
    >
    struct Uniform_
    {
        Uniform_(unsigned slot_, const ResTy& buf_) :
            slot(slot_),
            buf(buf_)
        {}

        unsigned slot;
        const ResTy& buf;
    };

    template <
        typename ResTy
    >
    Uniform_<ResTy> Uniform(unsigned slot_, const ResTy& buf_)
    {
        return Uniform_<ResTy>(slot_, buf_);
    }

    ////////////////////////// bindOne<T> template declaration
    template <
         typename D,
         typename T
    >
    void bindOne(Device<D>& device, BindContext& context, const T& value);

	////////////////////////// Bind<VertexBuffer_>
	template <typename D, typename T>
	void bindOne(Device<D>& device, BindContext& context, const VertexBuffer_<T, D>& vbuf)
	{
		device.backend.bindVertexBuffer(context.vertexBufferBindingIndex++, vbuf.buf.handle.get(), 0, vbuf.buf.byteSize, sizeof(T));
	}

	////////////////////////// Bind<VertexArray_>
	template <typename D, typename TVertex>
	void bindOne(Device<D>& device, BindContext& context, const VertexArray_<TVertex>& vbuf)
	{
		auto slice = device.pushDataToUploadBuffer(vbuf.data);
		device.backend.bindVertexBuffer(context.vertexBufferBindingIndex++, slice.handle, slice.offset, slice.byteSize, sizeof(TVertex));
	}

	////////////////////////// Bind<IndexBuffer_<T> >
	template <typename D, typename T>
	void bindOne(Device<D>& device, BindContext& context, const IndexBuffer_<T, D>& ibuf)
	{
		IndexType indexType;
		if (std::is_same<T, unsigned short>::value) indexType = IndexType::UShort;
		if (std::is_same<T, unsigned int>::value) indexType = IndexType::UInt;
		device.backend.bindIndexBuffer(ibuf.buf.handle.get(), 0, ibuf.buf.byteSize, indexType);
	}

    ////////////////////////// Bind<Texture1D>
    template <
        typename D,
        typename TPixel
    >
    void bindOne(Device<D>& device, BindContext& context, const Texture1D<TPixel, D>& tex)
    {
        device.backend.bindTexture1D(context.textureBindingIndex++, tex.handle.get());
    }

    ////////////////////////// Bind<Texture2D>
    template <
        typename D,
        typename TPixel
    >
    void bindOne(Device<D>& device, BindContext& context, const Texture2D<TPixel, D>& tex)
    {
        device.backend.bindTexture2D(context.textureBindingIndex++, tex.handle.get());
    }

    ////////////////////////// Bind<Texture3D>
    template <
        typename D,
        typename TPixel
    >
    void bindOne(Device<D>& device, BindContext& context, const Texture3D<TPixel, D>& tex)
    {
        device.backend.bindTexture3D(context.textureBindingIndex++, tex.handle.get());
    }


    ////////////////////////// Bind<Sampler>
    template <
        typename D
    >
    void bindOne(Device<D>& device, BindContext& context, const Sampler<D>& sampler)
    {
        device.backend.bindSampler(context.samplerBindingIndex++, sampler.handle.get());
    }

    ////////////////////////// Bind<TextureUnit<>>
    template <
        typename D,
        typename TextureTy
    >
    void bindOne(Device<D>& device, BindContext& context, const TextureUnit_<TextureTy, D>& tex_unit)
    {
        context.textureBindingIndex = tex_unit.unit;
        context.samplerBindingIndex = tex_unit.unit;
        bindOne(device, context, tex_unit.sampler);
        bindOne(device, context, tex_unit.tex);
    }

    ////////////////////////// Bind<RawBufferSlice>
    template <
        typename D
    >
    void bindOne(Device<D>& device, BindContext& context, const RawBufferSlice<D>& buf_slice)
    {
        device.backend.bindUniformBuffer(context.uniformBufferBindingIndex++, buf_slice.handle, buf_slice.offset, buf_slice.byteSize);
    }

    ////////////////////////// Bind<T>
    template <
         typename D,
         typename T
    >
    void bindOne(Device<D>& device, BindContext& context, const T& value)
    {
        // allocate a temporary uniform buffer from the default upload buffer
        auto slice = device.pushDataToUploadBuffer(value, D::kUniformBufferOffsetAlignment);
        bindOne(device, context, slice);
    }

    ////////////////////////// bindImpl<T>: recursive binding of draw resources
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
        TSurface&& surface,
        GraphicsPipeline<D>& graphicsPipeline,
        Drawable&& drawable,
        TShaderResources&&... resources)
	{
		BindContext context;
		bindImpl(device, context, resources...);
        device.backend.bindSurface(surface.handle.get());
        device.backend.bindGraphicsPipeline(graphicsPipeline.handle.get());
        drawable.draw(device, context);
	}

    template <
        typename D,
        typename... TShaderResources
    >
    void compute(
        Device<D>& device,
        ComputePipeline<D>& computePipeline,
        TShaderResources&&... resources)
    {
        BindContext context;
        bindImpl(device, context, resources...);
        device.backend.bindComputePipeline(computePipeline.handle.get());
    }
	
}

#endif // !DRAW_HPP
