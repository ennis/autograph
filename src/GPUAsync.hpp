#ifndef GPUASYNC_HPP
#define GPUASYNC_HPP

namespace ag
{
	// A future for a value of type T
	template <
		typename T,
		typename D
	>
	struct GPUAsync
	{
		typename D::template GPUAsyncDetail<T> detail;
	};
}

#endif // !GPUASYNC_HPP
