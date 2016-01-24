#ifndef COMMAND_LIST_HPP
#define COMMAND_LIST_HPP

#include "upload_buffer.hpp"

namespace ag
{
	// records list of commands to submit 
	// performs redundant state change elimination
	// still, will not bind the resources one-by-one
	template <typename D>
	class CommandList
	{
	public:
		CommandList(D& backend_, UploadBuffer<D>& upload_buf_, typename D::CommandListHandle handle_) :
			backend(backend_),
			upload_buf(upload_buf_),
			handle(handle_)
		{}

		// bind commands (handles)
		void setTexture1D(unsigned slot, Texture1DHandle::pointer handle);
		void setTexture2D(unsigned slot, Texture2DHandle::pointer handle);
		void setTexture3D(unsigned slot, Texture3DHandle::pointer handle);

		// draw commands

	private:
		struct BindState
		{
			enum class TextureDimension {
				Tex1D, Tex2D, Tex3D
			};
			struct TexBindState {
				TextureDimension dim;
				union {
					Texture1DHandle::pointer tex1d;
					Texture2DHandle::pointer tex2d;
					Texture3DHandle::pointer tex3d;
				} u;
			};

			std::array<TexBindState, 8> textures;
			bool texturesUpdated;
		};

		D& backend;
		UploadBuffer<D>& upload_buf;
		typename D::CommandListHandle handle;
	};
}

#endif // !COMMAND_LIST_HPP