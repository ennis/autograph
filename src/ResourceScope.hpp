#ifndef RESOURCE_SCOPE_HPP
#define RESOURCE_SCOPE_HPP

#include <vector>
#include <memory>

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

        void addTexture1DHandle(Texture1DHandle handle)
		{
            textures1D.emplace_back(std::move(handle));
		}
		
        void addTexture2DHandle(Texture2DHandle handle)
		{
            textures2D.emplace_back(std::move(handle));
		}
		
        void addTexture3DHandle(Texture3DHandle handle)
		{
            textures3D.emplace_back(std::move(handle));
		}
		
        void addSamplerHandle(SamplerHandle handle)
		{
            samplers.emplace_back(std::move(handle));
		}

        void addBufferHandle(BufferHandle handle)
		{
            buffers.emplace_back(std::move(handle));
		}

        void addGraphicsPipelineHandle(GraphicsPipelineHandle handle)
		{
            pipelines.emplace_back(std::move(handle));
		}

		void clear()
		{
			textures1D.clear();
			textures2D.clear();
			textures3D.clear();
			samplers.clear();
			buffers.clear();
			pipelines.clear();
		}

        std::vector<Texture1DHandle> textures1D;
        std::vector<Texture2DHandle> textures2D;
        std::vector<Texture3DHandle> textures3D;
        std::vector<SamplerHandle> samplers;
        std::vector<BufferHandle> buffers;
        std::vector<GraphicsPipelineHandle> pipelines;
	};
}

#endif // !RESOURCE_SCOPE_HPP

