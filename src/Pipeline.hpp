#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <SharedResource.hpp>

namespace ag
{
	template <
		typename D
	>
	struct GraphicsPipeline
	{
		void bind(D& backend)
		{
			backend.bindGraphicsPipeline(handle.get());
		}

		shared_resource<typename D::GraphicsPipelineHandle> handle;
	};

	template <
		typename D
	>
	struct ComputePipeline
	{
		void bind(D& backend)
		{
			backend.bindComputePipeline(handle.get());
		}

		shared_resource<typename D::ComputePipelineHandle> handle;
	};
}

#endif