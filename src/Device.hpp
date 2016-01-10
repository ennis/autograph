#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <algorithm>
#include <memory>

#include <gsl.h>

#include <Surface.hpp>
#include <ResourceScope.hpp>
#include <Texture.hpp>
#include <Buffer.hpp>
#include <Pipeline.hpp>
#include <UploadBuffer.hpp>
#include <Fence.hpp>
#include <Error.hpp>

namespace ag
{
	struct DeviceOptions
	{
		std::string windowTitle = "Title";
		unsigned framebufferWidth = 640;
		unsigned framebufferHeight = 480;
		bool fullscreen = false;
		unsigned maxFramesInFlight = 3;
	};

	inline FenceValue getFrameExpirationDate(unsigned frame_id)
	{
		// Frame N expires when the fence has reached the value N+1
		return frame_id + 1;
	}

	template <
		typename T,
		typename F
	>
	void cleanupHandles(std::vector<shared_resource<T> >& resources, F deleter)
	{
		auto begin_remove = std::remove_if(resources.begin(), resources.end(), [](const shared_resource<T> r) {
			return r.is_unique();
		});

		for (auto it = begin_remove; it != resources.end(); ++it)
		{
			deleter(*it);
		}

		resources.erase(begin_remove, resources.end());
	}

	template <typename D>
	struct Frame
	{
		//typename D::FenceHandle fence;
		ResourceScope<D> scope;
	};

	template <
		typename D
	>
	class Device
	{
	public:
		Device(D& backend_, const DeviceOptions& options_) : options(options_), backend(backend_), frame_id(0)
		{
			backend.createWindow(options);
			frameFence = backend.createFence(0);
			default_upload_buffer = std::make_unique<UploadBuffer<D> >(backend_, 3*256);
		}

		Surface<D, float, RGBA8> getOutputSurface()
		{
			Surface<D, float, RGBA8> surface;
			// TODO: move-construct?
			surface.handle = backend.initOutputSurface();
			return surface;
		}

		template <
			typename F
		>
		void run(F render_fn)
		{
			while (!backend.processWindowEvents())
			{
				render_fn();
				endFrame();
				backend.swapBuffers();
			}
		}

		template <
			typename TDepth,
			typename... TColors
		>
		void clear(Surface<D, TDepth, TColors...>& surface, const glm::vec4& color)
		{
			backend.clearColor(surface.handle, color);
		}

		///////////////////// createTexture1D
		template <typename TPixel>
		Texture1D<TPixel, D> createTexture1D(glm::uint width)
		{
			Texture1DInfo info{ width };
			return Texture1D<TPixel, D>{
				info,
				scope.addTexture1DHandle(backend.template createTexture1D<TPixel>(info))
			};
		}

		///////////////////// createTexture2D
		template <typename TPixel>
		Texture2D<TPixel, D> createTexture2D(glm::uvec2 dimensions)
		{
			Texture2DInfo info{ dimensions };
			return Texture2D<TPixel, D>{
				info,
				scope.addTexture2DHandle(backend.template createTexture2D<TPixel>(info))
			};
		}

		///////////////////// createTexture3D
		template <typename TPixel>
		Texture3D<TPixel, D> createTexture3D(glm::uvec3 dimensions)
		{
			Texture3DInfo info{ dimensions };
			return Texture3D<TPixel, D>{
				info,
				scope.addTexture3DHandle(backend.template createTexture3D<TPixel>(info))
			};
		}

		///////////////////// createSampler
		Sampler<D> createSampler(const SamplerInfo& info)
		{
			return Sampler<D>{
				info,
				scope.addSamplerHandle(backend.createSampler(info))
			};
		}

        ///////////////////// createBuffer(T)
        template <typename T>
        Buffer<D, T> createBuffer(const T& data)
        {
            return Buffer<D, T> (
                scope.addBufferHandle(backend.createBuffer(sizeof(T), &data, BufferUsage::Default))
            );
        }

		///////////////////// createBuffer(span)
		template <typename T>
		Buffer<D, T[]> createBuffer(gsl::span<T> data)
		{
			return Buffer<D, T[]> (
				data.size(),
				scope.addBufferHandle(backend.createBuffer(data.size_bytes(), data.data(), BufferUsage::Default))
			);
		}


		///////////////////// Upload heap management
		template <typename T>
		RawBufferSlice<D> pushDataToUploadBuffer(const T& value, size_t alignment = alignof(T))
		{
			RawBufferSlice<D> out_slice;
			if (!default_upload_buffer->uploadRaw(&value, sizeof(T), alignment, getFrameExpirationDate(frame_id), out_slice)) {
				// upload failed, meh, I should sync here
				failWith("Upload buffer is full");
			}
			return std::move(out_slice);
		}

		///////////////////// end-of-frame cleanup
		void endFrame()
		{
			// sync on frame N-(max-in-flight)
			frame_id++;
			backend.signal(frameFence, frame_id);	// this should be a command queue API
			if (frame_id >= options.maxFramesInFlight) {
				backend.waitForFence(frameFence, getFrameExpirationDate(frame_id - options.maxFramesInFlight));
				default_upload_buffer->reclaim(getFrameExpirationDate(frame_id - options.maxFramesInFlight));
			}
		}

		///////////////////// references
		ResourceScope<D>& getFrameScope()
		{
			return in_flight.back().scope;
		}

		//RingBuffer<D>& getUploadBuffer()

		///////////////////// pipeline 
		template <
			typename Arg
		>
		GraphicsPipeline<D> createGraphicsPipeline(Arg&& arg)
		{
			return GraphicsPipeline<D> {
				scope.addGraphicsPipelineHandle(backend.createGraphicsPipeline(std::forward<Arg>(arg)))
			};
		}


		//private:
		DeviceOptions options;
		D& backend;
		ResourceScope<D> scope;
		std::vector<Frame<D> > in_flight;

		typename D::FenceHandle frameFence;
		unsigned frame_id;

		// the default upload buffer
		std::unique_ptr<UploadBuffer<D> > default_upload_buffer;
	};
}

#endif // !DEVICE_HPP
