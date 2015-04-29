//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//

#include <framework/bitmap.h>
#include <framework/misc.h>
#include <framework/colour.h>
#include <framework/logging.h>
#include <framework/graphics.h>
#include <framework/exception.h>
//#include "texture.h"

#include "soil/SOIL.h"

namespace fs = boost::filesystem;

namespace fw {
	//-------------------------------------------------------------------------
	// This class contains the actual bitmap data, which is actually a Direct3D texture.
	struct bitmap_data : private boost::noncopyable
	{
		int width;
		int height;
		std::vector<uint32_t> argb;

		// This is the number of bitmaps that have a reference to us.
		int ref_count;

		// constructs a new bitmap_data (just set everything to 0)
		inline bitmap_data()
		{
			width = height = 0;
			ref_count = 1;
		}
	};

	static void argb_2_rgba(std::vector<uint32_t> const &src, std::vector<uint32_t> &dest, int size);
	static void rgba_2_argb(uint32_t const *src, std::vector<uint32_t> &dest, int size);

	void argb_2_rgba(std::vector<uint32_t> const &src, std::vector<uint32_t> &dest, int size)
	{
		dest.resize(size);
		for(int i = 0; i < size; i++)
		{
			dest[i] = ((src[i] & 0xff000000) >> 24) || ((src[i] & 0x00ffffff) << 8);
		}
	}

	void rgba_2_argb(uint32_t const *src, std::vector<uint32_t> &dest, int size)
	{
		dest.resize(size);
		for(int i = 0; i < size; i++)
		{
			dest[i] = ((src[i] & 0xffffff00) >> 8) || ((src[i] & 0xff) << 24);
		}
	}

	
	//-------------------------------------------------------------------------
/*
	static void copy_pixels(IDirect3DTexture9 *dest_texture, uint32_t const *src_argb, int width, int height);
	static void copy_pixels(IDirect3DSurface9 *dest_surface, uint32_t const *src_argb, int width, int height);
	static void copy_pixels(uint32_t *dest_argb, IDirect3DTexture9 *src_texture, int width, int height);
	static void copy_pixels_A8R8G8B8(uint32_t *dest_argb, D3DSURFACE_DESC const &src_surface_desc, D3DLOCKED_RECT const &src_rect);
	static void copy_pixels(uint32_t *dest_argb, IDirect3DSurface9 *src_surface, int width, int height, bool allow_recurse = true);

	void copy_pixels(IDirect3DTexture9 *dest_texture, uint32_t const *src_argb, int width, int height)
	{
		IDirect3DSurface9 *surface;
		HRESULT hr = dest_texture->GetSurfaceLevel(0, &surface);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(fw::exception(hr));

		copy_pixels(surface, src_argb, width, height);

		surface->Release();
	}

	void copy_pixels(IDirect3DSurface9 *dest_surface, uint32_t const *src_argb, int width, int height)
	{
		D3DLOCKED_RECT data;
		HRESULT hr = dest_surface->LockRect(&data, 0, D3DLOCK_DISCARD);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(fw::exception(hr));

		uint32_t *bits = reinterpret_cast<uint32_t *>(data.pBits);
		int pitch = data.Pitch / sizeof(uint32_t);
		for(int y = 0; y < height; y++)
		{
			memcpy(bits + (y * pitch), src_argb + (y * width), width * sizeof(uint32_t));
		}

		hr = dest_surface->UnlockRect();
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(fw::exception(hr));
	}

	void copy_pixels(uint32_t *dest_argb, IDirect3DTexture9 *src_texture, int width, int height)
	{
		IDirect3DSurface9 *surface;
		HRESULT hr = src_texture->GetSurfaceLevel(0, &surface);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(fw::exception(hr));

		copy_pixels(dest_argb, surface, width, height);

		surface->Release();
	}

	void copy_pixels_A8R8G8B8(uint32_t *dest_argb, D3DSURFACE_DESC const &src_surface_desc, D3DLOCKED_RECT const &src_rect)
	{
		for(UINT row = 0; row < src_surface_desc.Height; row++)
		{
			char const *row_data = reinterpret_cast<char const *>(src_rect.pBits) + (src_rect.Pitch * row);
			uint32_t const *pixels = reinterpret_cast<uint32_t const *>(row_data);

			for(UINT col = 0; col < src_surface_desc.Width; col++)
			{
				uint32_t pixel = pixels[col];
				if (src_surface_desc.Format == D3DFMT_X8R8G8B8)
					pixel |= 0xFF000000;

				dest_argb[(row * src_surface_desc.Width) + col] = pixel;
			}
		}
	}

	void copy_pixels(uint32_t *dest_argb, IDirect3DSurface9 *src_surface, int width, int height, bool allow_recurse = true)
	{
		D3DSURFACE_DESC desc;
		HRESULT hr = src_surface->GetDesc(&desc);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(fw::exception(hr));

		if (width != static_cast<int>(desc.Width) || height != static_cast<int>(desc.Height))
			BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("get_pixels only supports passing width/height that's the same as the surface"));

		if (desc.Format == D3DFMT_A8R8G8B8 || desc.Format == D3DFMT_X8R8G8B8)
		{
			D3DLOCKED_RECT rect;
			hr = src_surface->LockRect(&rect, 0, D3DLOCK_READONLY);
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(fw::exception(hr));

			copy_pixels_A8R8G8B8(dest_argb, desc, rect);

			hr = src_surface->UnlockRect();
			if (FAILED(hr))
			{
				BOOST_THROW_EXCEPTION(fw::exception(hr));
			}
		}
		else if (!allow_recurse)
		{
			BOOST_THROW_EXCEPTION(fw::exception()
				<< fw::message_error_info("Cannot get pixel data from surface! desc.Format = " + desc.Format));
		}
		else
		{
			// we have to do things a little bit differently if we don't support
			// the format (and, to be honest, we don't really support many formats...)
			// we'll have to save the surface to a file "in memory" then load a new
			// surface with a format we *do* support, and load that one...
			//
			// This is useful, for example, for fetching the data from a DXT1 texture where we'd
			// have to implement a DXT1 decoding algorithm. I *could* do that, but I'm too lazy.

			ID3DXBuffer *buffer = 0;
			hr = ::D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_BMP, src_surface, 0, 0);
			if (FAILED(hr))
			{
				BOOST_THROW_EXCEPTION(fw::exception(hr));
			}

			// create the temporary surface that'll hold the data
			IDirect3DSurface9 *tmp_surface = 0;
			hr = fw::framework::get_instance()->get_graphics()->get_device()->CreateOffscreenPlainSurface(
								desc.Width, desc.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &tmp_surface, 0);
			if (FAILED(hr))
			{
				BOOST_THROW_EXCEPTION(fw::exception(hr));
			}

			// load the in-memory buffer back into the new surface
			hr = ::D3DXLoadSurfaceFromFileInMemory(tmp_surface, 0, 0, buffer->GetBufferPointer(),
				buffer->GetBufferSize(), 0, D3DX_DEFAULT, 0, 0);
			if (FAILED(hr))
			{
				BOOST_THROW_EXCEPTION(fw::exception(hr));
			}

			// now call get_pixels recursively. We know it's a format we support now
			// (because we created it like that)
			copy_pixels(dest_argb, tmp_surface, width, height, false);

			// and release the temporary objects (note: there is a leak here in the case
			// of exception, but the process will die if there's an exception here)
			tmp_surface->Release();
			buffer->Release();
		}
	}
*/
	//-------------------------------------------------------------------------

	void blit(fw::bitmap const &src, fw::texture &dest)
	{
/*
		int width = src.get_width();
		int height = src.get_height();

		if (dest.get_width() != width || dest.get_height() != height)
		{
			// we want the texture to be exactly the same width/height as us, so
			// if it's not, resize it (log a warning though, cause we should try
			// to avoid this as much as possible - the caller can resize the texture
			// if they really need to)
			debug << boost::format("warning: resizing texture to fix bitmap size: (%1%, %2%), original texture size was: (%3%, %4%)")
							% width % height % dest.get_width() % dest.get_height() << std::endl;
			dest.create(0, width, height);
		}

		// grab the image data from the bitmap in a nice, easy to use format (RGBA, 8 bits per pixel)
		std::vector<uint32_t> const &buffer = src.get_pixels();
		copy_pixels(dest.get_d3dtexture(), &buffer[0], width, height);
*/
	}

	//-------------------------------------------------------------------------
	bitmap::bitmap()
		: _data(0)
	{
	}

	bitmap::bitmap(int width, int height, uint32_t *argb /*= 0*/)
		: _data(0)
	{
		prepare_write(width, height);

		if (argb != 0)
		{
			memcpy(&_data->argb[0], argb, width * height * sizeof(uint32_t));
		}
	}

	bitmap::bitmap(fs::path const &filename)
		: _data(0)
	{
		load_bitmap(filename);
	}

	bitmap::bitmap(uint8_t const *data, size_t data_size)
		: _data(0)
	{
		load_bitmap(data, data_size);
	}

	bitmap::bitmap(texture const &tex)
		: _data(0)
	{
//		IDirect3DTexture9 *texture = tex.get_d3dtexture();
//		load_bitmap(texture);
	}

	bitmap::bitmap(bitmap const &copy)
	{
		_data = copy._data;
		_data->ref_count ++;
	}

	bitmap::~bitmap()
	{
		release();
	}

	bitmap &bitmap::operator =(fw::bitmap const &copy)
	{
		// ignore self assignment
		if (this == &copy)
			return (*this);

		release();

		_data = copy._data;
		_data->ref_count ++;

		return (*this);
	}
	
	void bitmap::release()
	{
		if (_data != 0)
		{
			_data->ref_count --;
			if (_data->ref_count == 0)
			{
				delete _data;
			}
		}
	}

	void bitmap::prepare_write(int width, int height)
	{
		if (_data == 0 || _data->ref_count > 1)
		{
			// we'll have to create a new bitmap_data if there's currently no data or
			// the ref_count is > 1
			if (_data != 0)
				_data->ref_count --;

			_data = new bitmap_data();
		}

		// make sure the pixel buffer is big enough to hold the required width/height
		_data->argb.resize(width * height);
		_data->width = width;
		_data->height = height;
	}

	void bitmap::load_bitmap(fs::path const &filename)
	{
		debug << boost::format("loading image: \"%1%\"") % filename << std::endl;
		prepare_write(0, 0);

		int channels;
		unsigned char *pixels = SOIL_load_image(filename.string().c_str(), &_data->width, &_data->height,
												&channels, SOIL_LOAD_RGBA);
		if (pixels == 0)
		{
			BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(SOIL_last_result()));
		}

		// copy pixels from what SOIL returned into our own buffer...
		rgba_2_argb(reinterpret_cast<uint32_t *>(pixels), _data->argb, _data->width * _data->height);

		// don't need this anymore
		SOIL_free_image_data(pixels);
	}

	// Populates our bitmap_data with data from the given in-memory file
	void bitmap::load_bitmap(uint8_t const *data, size_t data_size)
	{
		int channels;
		unsigned char *pixels = SOIL_load_image_from_memory(reinterpret_cast<unsigned char const *>(data), data_size,
						&_data->width, &_data->height, &channels, SOIL_LOAD_RGBA);
		if (pixels == 0)
		{
			BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(SOIL_last_result()));
		}

		// copy pixels from what SOIL returned into our own buffer...
		rgba_2_argb(reinterpret_cast<uint32_t *>(pixels), _data->argb, _data->width * _data->height);

		// don't need this anymore
		SOIL_free_image_data(pixels);
	}

	void bitmap::save_bitmap(fs::path const &filename) const
	{
		if (_data == 0)
			return;

		debug << boost::format("saving image: \"%1%\"") % filename << std::endl;

		fs::path path(filename);
		if (fs::exists(path))
		{
			fs::remove(path);
		}

		std::vector<uint32_t> pixels;
		argb_2_rgba(_data->argb, pixels, _data->argb.size());

		int res = SOIL_save_image(filename.c_str(), SOIL_SAVE_TYPE_BMP,
								  _data->width, _data->height, 4,
								  reinterpret_cast<unsigned char *>(&pixels[0]));
		if (res == 0)
		{
			BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(SOIL_last_result()));
		}
	}

	int bitmap::get_width() const
	{
		if (_data == 0)
			return 0;

		return _data->width;
	}

	int bitmap::get_height() const
	{
		if (_data == 0)
			return 0;

		return _data->height;
	}

	std::vector<uint32_t> const &bitmap::get_pixels() const
	{
		return _data->argb;
	}

	void bitmap::get_pixels(std::vector<uint32_t> &argb) const
	{
		if (_data == 0)
			return;

		int width = get_width();
		int height = get_height();

		// make sure it's big enough to hold the data
		argb.resize(width * height);
		memcpy(&argb[0], &_data->argb[0], width * height * sizeof(uint32_t));
	}

	void bitmap::set_pixels(std::vector<uint32_t> const &argb)
	{
		int width = get_width();
		int height = get_height();

		prepare_write(width, height);
		memcpy(&_data->argb[0], &argb[0], width * height * sizeof(uint32_t));
	}

	fw::colour bitmap::get_pixel(int x, int y)
	{
		// this is currently built on top of get_pixels, which is a bit of a pain...

		if (_data->argb.size() == 0)
		{
			get_pixels(_data->argb);
		}

		int width = get_width();
		return fw::colour::from_rgba(_data->argb[(width * y) + x]);
	}

	void bitmap::resize(int new_width, int new_height, int quality)
	{
		int curr_width = get_width();
		int curr_height = get_height();

		if (curr_width == new_width && curr_height == new_height)
			return;

		if (new_width > curr_width || new_height > curr_height)
		{
			BOOST_THROW_EXCEPTION(fw::exception()
				<< fw::message_error_info("up-scaling is not supported"));
		}

		int scale_x = curr_width / new_width;
		if (scale_x < 1)
			scale_x = 1;

		int scale_y = curr_height / new_height;
		if (scale_y < 1)
			scale_y = 1;

		std::vector<uint32_t> resized(new_width * new_height);
		for(int y = 0; y < new_height; y++)
		{
			for(int x = 0; x < new_width; x++)
			{
				// we get the average colour of the pixels in the rectangle
				// that the current pixel occupies

				fw::colour average(0, 0, 0, 0);
				int n = 0;
				for(int y1 = (y * scale_y); y1 < (y + 1) * scale_y; y1++)
				{
					if (y1 >= curr_height)
						break;

					for(int x1 = (x * scale_x); x1 < (x + 1) * scale_x; x1++)
					{
						if (x1 >= curr_width)
							break;

						average += fw::colour::from_argb(_data->argb[(y1 * curr_width) + x1]);
						n++;
					}
				}

				average /= static_cast<double>(n);
				average = average.clamp();
				resized[(y * new_width) + x] = average.to_argb();
			}
		}

		prepare_write(new_width, new_height);
		set_pixels(resized);
	}

}