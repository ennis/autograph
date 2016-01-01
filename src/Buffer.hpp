#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <SharedResource.hpp>

namespace ag
{
	template <
		typename D
	>
	struct RawBuffer 
	{
		RawBuffer(
			std::size_t byteSize_,
			shared_resource<typename D::BufferHandle> handle_) :
			byteSize(byteSize_),
			handle(std::move(handle_))
		{}

		std::size_t byteSize;
		shared_resource<typename D::BufferHandle> handle;
	};

	template <
		typename D,
		typename T
	>
	struct Buffer : public RawBuffer<D>
	{
		Buffer(shared_resource<typename D::BufferHandle> handle_) :
			RawBuffer<D>(sizeof(T), std::move(handle_))
		{}

	};

	// specialization for array types
	template <
		typename D,
		typename T
	>
	struct Buffer<D, T[]> : public RawBuffer<D>
	{
		Buffer(std::size_t size_, shared_resource<typename D::BufferHandle> handle_) :
			RawBuffer<D>(size_*sizeof(T), std::move(handle_))
		{}

		std::size_t size() const
		{
			// must use this pointer to make byteSize a dependent name
			return this->byteSize / sizeof(T);
		}
	};
}

#endif
