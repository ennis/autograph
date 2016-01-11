#ifndef PIPELINE_HPP
#define PIPELINE_HPP

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

        typename D::GraphicsPipelineHandle handle;
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

        typename D::ComputePipelineHandle handle;
	};
}

#endif
