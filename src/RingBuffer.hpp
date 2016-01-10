#ifndef RING_BUFFER_HPP
#define RING_BUFFER_HPP

#include <vector>
#include <queue>
#include <algorithm>

#include <optional.hpp>

#include <Buffer.hpp>
#include <Fence.hpp>

namespace ag
{
	namespace {
		size_t moveRingPtr(size_t ptr, size_t offset, size_t ring_size)
		{
			ptr += offset;
			ptr = ptr % ring_size;
			return ptr;
		}
	}


	// A ring buffer, used in the implementation of upload buffers
	template <typename D>
	class RingBuffer
	{
	public:
		RingBuffer(RawBuffer<D> buffer_) : 
			buffer(std::move(buffer_)),
			write_ptr(0),
			begin_ptr(0),
			used(0)
		{
		}

		// can block on fences
		template <typename T>
		bool allocate(FenceValue expirationDate, BufferSlice<D,T>& slice)
		{
			return allocateRaw(expirationDate, sizeof(T), alignof(T), slice);
		}

		// can block on fences
		template <typename T>
		bool allocateArray(FenceValue expirationDate, size_t count, BufferSlice<D, T[]>& slice)
		{
			return allocateRaw(expirationDate, count*sizeof(T), alignof(T), slice);
		}

		bool allocateRaw(FenceValue expirationDate, size_t size, size_t align, RawBufferSlice<D>& slice)
		{
			size_t offset = 0;
			if (!tryAllocateContiguousFreeSpace(expirationDate, size, align, offset))
				return false;
			slice.handle = buffer.handle.get();
			slice.offset = offset;
			slice.byteSize = size;
			return true;
		}

		bool alignOffset(size_t align, size_t size, size_t& ptr, size_t space)
		{
			size_t off = ptr & (align - 1);
			if (off > 0)
				off = align - off;
			if (space < off || space - off < size)
				return false;
			else
			{
				ptr = ptr + off;
				return true;
			}
		}
				
		bool tryAllocateContiguousFreeSpace(FenceValue expirationDate, size_t size, size_t align, size_t& alloc_begin)
		{
			size_t buf_size = buffer.byteSize;
			assert(needed < buf_size);
			if ((begin_ptr < write_ptr) || ((begin_ptr == write_ptr) && (used == 0))) {
				size_t slack_space = buf_size - write_ptr;
				// try to put the buffer in the slack space at the end
				if (!alignOffset(align, size, write_ptr, slack_space)) {
					// else, try to put it at the beginning (which is always correctly aligned)
					if (size > begin_ptr)
						return false;
					write_ptr = 0;
				}
			}
			else { // begin_ptr > write_ptr
				// reclaim space in the middle
				if (alignOffset(align, size, write_ptr, begin_ptr - write_ptr)) {
					alloc_begin = write_ptr;
				}
				else
					return false;
			}

			alloc_begin = write_ptr;
			used += size;
			write_ptr += size;
			fencedRegions.push(FencedRegion{ expirationDate, alloc_begin, alloc_begin + size });
			return true;
		}

		void reclaim(FenceValue date)
		{
			while (fencedRegions.front().expirationDate < date) {
				auto& r = fencedRegions.front();
				begin_ptr = r.end_ptr;	// there may be some alignment space that we would want to reclaim
				used -= r.end_ptr - r.begin_ptr;
				fencedRegions.pop();
			}
		}


	private:
		struct FencedRegion
		{
			// device fence
			FenceValue expirationDate;
			// offset to the beginning of the fenced region in the ring buffer
			size_t begin_ptr;
			// offset to the end of the fenced region
			size_t end_ptr;
		};

		// start of free space in the ring
		size_t write_ptr;
		// end of free space in the ring
		size_t begin_ptr;
		// used space
		size_t used;

		RawBuffer<D> buffer;
		std::queue<FencedRegion> fencedRegions;
	};
}

#endif // !RING_BUFFER_HPP