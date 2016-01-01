#ifndef RESOURCE_SCOPE_HPP
#define RESOURCE_SCOPE_HPP

#include <vector>
#include <memory>

#include <SharedResource.hpp>

namespace ag
{
	template <
		typename D
	>
	class ResourceScope
	{
	public:
		using Texture1DHandle = typename D::Texture1DHandle;
		using Texture2DHandle = typename D::Texture2DHandle;
		using Texture3DHandle = typename D::Texture3DHandle;
		using SamplerHandle = typename D::SamplerHandle;
		using BufferHandle = typename D::BufferHandle;
		using GraphicsPipelineHandle = typename D::GraphicsPipelineHandle;

		shared_resource<Texture1DHandle> addTexture1DHandle(Texture1DHandle handle)
		{
			auto r = shared_resource<Texture1DHandle>(handle);
			textures1D.push_back(r);
			return std::move(r);
		}
		
		shared_resource<Texture2DHandle> addTexture2DHandle(Texture2DHandle handle)
		{
			auto r = shared_resource<Texture2DHandle>(handle);
			textures2D.push_back(r);
			return std::move(r);
		}
		
		shared_resource<Texture3DHandle> addTexture3DHandle(Texture3DHandle handle)
		{
			auto r = shared_resource<Texture3DHandle>(handle);
			textures3D.push_back(handle);
			return std::move(r);
		}
		
		shared_resource<SamplerHandle> addSamplerHandle(SamplerHandle handle)
		{
			auto r = shared_resource<SamplerHandle>(handle);
			samplers.push_back(handle);
			return std::move(r);
		}

		shared_resource<BufferHandle> addBufferHandle(BufferHandle handle)
		{
			auto r = shared_resource<BufferHandle>(handle);
			buffers.push_back(handle);
			return std::move(r);
		}

		shared_resource<GraphicsPipelineHandle> addGraphicsPipelineHandle(GraphicsPipelineHandle handle)
		{
			auto r = shared_resource<GraphicsPipelineHandle>(handle);
			pipelines.push_back(handle);
			return std::move(r);
		}

		void referenceTexture1D(shared_resource<Texture1DHandle> handle) { textures1D.emplace_back(std::move(handle)); }
		void referenceTexture2D(shared_resource<Texture2DHandle> handle) { textures2D.emplace_back(std::move(handle)); }
		void referenceTexture3D(shared_resource<Texture3DHandle> handle) { textures3D.emplace_back(std::move(handle)); }
		void referenceSampler(shared_resource<SamplerHandle> handle) { samplers.emplace_back(std::move(handle)); }
		void referenceBuffer(shared_resource<BufferHandle> handle) { buffers.emplace_back(std::move(handle)); }
		void referenceGraphicsPipeline(shared_resource<GraphicsPipelineHandle> handle) { pipelines.emplace_back(std::move(handle)); }

		void clear()
		{
			textures1D.clear();
			textures2D.clear();
			textures3D.clear();
			samplers.clear();
			buffers.clear();
			pipelines.clear();
		}

		std::vector<shared_resource<Texture1DHandle>> textures1D;
		std::vector<shared_resource<Texture2DHandle>> textures2D;
		std::vector<shared_resource<Texture3DHandle>> textures3D;
		std::vector<shared_resource<SamplerHandle>> samplers;
		std::vector<shared_resource<BufferHandle>> buffers;
		std::vector<shared_resource<GraphicsPipelineHandle>> pipelines;
	};
}

#endif // !RESOURCE_SCOPE_HPP

