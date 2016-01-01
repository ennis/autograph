#ifndef SHARED_RESOURCE_HPP
#define SHARED_RESOURCE_HPP

#include <memory>
#include <atomic>

template <typename T>
struct null_deleter
{
	void operator()(T) {}
};

template <
	typename T,
	typename Deleter = null_deleter<T>
>
class shared_resource 
{
public:
	shared_resource() :
		handle(T()),
		shared_block(nullptr)
	{
	}

	shared_resource(T handle_) : 
		handle(handle_),
		shared_block(new SharedBlock)
	{
	}

	shared_resource(const shared_resource<T>& right) :
		handle(right.handle),
		shared_block(right.shared_block)
	{
		if (shared_block)
			shared_block->strong_refs++;
	}

	shared_resource(shared_resource<T>&& right) :
		handle(right.handle),
		shared_block(right.shared_block)
	{
		right.shared_block = nullptr;
	}

	~shared_resource()
	{
		deinit();
	}

	shared_resource& operator=(const shared_resource& right)
	{
		deinit();
		shared_block = right.shared_block;
		if (shared_block)
			shared_block->strong_refs++;
		handle = right.handle;
		return *this;
	}

	shared_resource& operator=(shared_resource&& right)
	{
		deinit();
		shared_block = right.shared_block;
		right.shared_block = nullptr;
		handle = right.handle;
		return *this;
	}

	T get() const
	{
		return handle;
	}

	bool is_unique() const
	{
		if (!shared_block)
			throw std::logic_error("called is_unique() on an empty resource");
		return shared_block->strong_refs == 1;
	}

private:
	void deinit()
	{
		if (shared_block)
		{
			if (shared_block->strong_refs == 1)
			{
				deleter(handle);
				delete shared_block;
			}
			else {
				shared_block->strong_refs--;
			}
		}
	}

	struct SharedBlock {
		SharedBlock() : strong_refs(1)
		{}

		std::atomic_uint strong_refs;
	};

	SharedBlock* shared_block;
	T handle;
	Deleter deleter;
};

#endif // !SHARED_RESOURCE_HPP
