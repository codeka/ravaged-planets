
#include <string>
#include <sstream>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>

#include <framework/effect.h>
#include <framework/misc.h>
#include <framework/framework.h>
#include <framework/colour.h>
#include <framework/graphics.h>
//#include <framework/texture.h>
#include <framework/settings.h>
#include <framework/logging.h>
#include <framework/vector.h>
#include <framework/exception.h>

//#include <process.h>

namespace fs = boost::filesystem;

#if defined(_DEBUG)
	#define CG_CHECKED(fn) \
		fn; \
		check_error(#fn)
#else
	#define CG_CHECKED(fn) fn
#endif

struct effect_data : boost::noncopyable
{
	fs::path filename;
	CGeffect fx;

	inline ~effect_data()
	{
		if (fx != 0)
		{
			cgDestroyEffect(fx);
		}
	}
};

//-------------------------------------------------------------------------
// This is a cache of .fx files, so we don't have to load them over and over...
class effect_cache
{
private:
	typedef std::map<fs::path, boost::shared_ptr<effect_data> > effect_map;
	effect_map _effects;

public:
	boost::shared_ptr<effect_data> get_effect(fs::path const &name);
	void add_effect(fs::path const &name, boost::shared_ptr<effect_data> data);

	void clear_cache();
};

boost::shared_ptr<effect_data> effect_cache::get_effect(fs::path const &name)
{
	effect_map::iterator it = _effects.find(name);
	if (it == _effects.end())
		return boost::shared_ptr<effect_data>();

	return it->second;
}

void effect_cache::add_effect(fs::path const &name, boost::shared_ptr<effect_data> data)
{
	_effects[name] = data;
}

void effect_cache::clear_cache()
{
	_effects.clear();
}

//-----------------------------------------------------------------------------

static CGerror g_last_error = CG_NO_ERROR;

// This is called after every Cg function call (in debug mode) or at least every
// now & then (in release mode) to check for errors
void check_error(char const *fn)
{
	CGerror err = cgGetError();
	if (err == CG_NO_ERROR)
		err = g_last_error;

	if (err == CG_NO_ERROR)
		return;

	BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(fn));
}

// this is called via cgSetErrorCallback() to 
void error_callback()
{
	CGerror err = cgGetError();
	fw::debug << boost::format("ERROR Cg generated error: %1%") % cgGetErrorString(err) << std::endl;
}

//-----------------------------------------------------------------------------

static bool g_initialised = false;
static effect_cache g_cache;
static CGeffect load_effect(fw::graphics *g, fs::path const &full_path);

namespace fw {
	//-------------------------------------------------------------------------
	effect_parameters::effect_parameters()
	{
	}

	effect_parameters::~effect_parameters()
	{
	}

	void effect_parameters::set_technique_name(std::string const &name)
	{
		_technique_name = name;
	}

	void effect_parameters::set_texture(std::string const &name, boost::shared_ptr<texture> const &tex)
	{
		_textures[name] = tex;
	}

	void effect_parameters::set_matrix(std::string const &name, matrix const &m)
	{
		_matrices[name] = m;
	}

	void effect_parameters::set_vector(std::string const &name, vector const &v)
	{
		_vectors[name] = v;
	}

	void effect_parameters::set_colour(std::string const &name, colour const &c)
	{
		_colours[name] = c;
	}

	void effect_parameters::set_scalar(std::string const &name, float f)
	{
		_scalars[name] = f;
	}

	boost::shared_ptr<effect_parameters> effect_parameters::clone()
	{
		boost::shared_ptr<effect_parameters> clone(new effect_parameters());
		clone->_textures = _textures;
		clone->_matrices = _matrices;
		clone->_vectors = _vectors;
		clone->_colours = _colours;
		clone->_scalars = _scalars;
		clone->_technique_name = _technique_name;
		return clone;
	}

	void effect_parameters::apply(effect *e) const
	{
		if (e->_data == 0)
			return;

		CGeffect fx = e->_data->fx;

//		for(std::map<std::string, boost::shared_ptr<texture> >::const_iterator it = _textures.begin();
//			it != _textures.end(); ++it)
//		{
//			CGparameter p = cgGetNamedEffectParameter(fx, it->first.c_str());
//			if (p != 0)
//			{
//				cgGLSetTextureParameter(p, it->second->get_name());
//			}
//		}

		for(std::map<std::string, matrix>::const_iterator it = _matrices.begin();
			it != _matrices.end(); ++it)
		{
			CGparameter p = cgGetNamedEffectParameter(fx, it->first.c_str());
			if (p != 0)
			{
				CG_CHECKED(cgSetParameterValuefc(p, 16, it->second.data()));
			}
		}

		for(std::map<std::string, vector>::const_iterator it = _vectors.begin();
			it != _vectors.end(); ++it)
		{
			CGparameter p = cgGetNamedEffectParameter(fx, it->first.c_str());
			if (p != 0)
			{
				CG_CHECKED(cgSetParameter3fv(p, it->second.data()));
			}
		}

		for(std::map<std::string, colour>::const_iterator it = _colours.begin();
			it != _colours.end(); ++it)
		{
			CGparameter p = cgGetNamedEffectParameter(fx, it->first.c_str());
			if (p != 0)
			{
				CG_CHECKED(cgSetParameter4f(p, it->second.a, it->second.r, it->second.g, it->second.b));
			}
		}

		for(std::map<std::string, float>::const_iterator it = _scalars.begin();
			it != _scalars.end(); ++it)
		{
			CGparameter p = cgGetNamedEffectParameter(fx, it->first.c_str());
			if (p != 0)
			{
				CG_CHECKED(cgSetParameter1f(p, it->second));
			}
		}

		
	}

	//-------------------------------------------------------------------------
	effect::effect()
	{
	}

	effect::~effect()
	{
	}

	void effect::initialise(fs::path const &filename)
	{
		if (!g_initialised)
		{
			cgSetErrorCallback(error_callback);
		}

		fw::graphics *g = fw::framework::get_instance()->get_graphics();

		bool loaded_from_cache = true;
		_data = g_cache.get_effect(filename);
		if (!_data)
		{
			CGeffect fx = load_effect(g, fw::resolve(std::string("~/share/war-worlds/effects/") + filename.string()));
			if (fx != 0)
			{
				loaded_from_cache = false;

				boost::shared_ptr<effect_data> new_data(new effect_data());
				new_data->fx = fx;
				new_data->filename = filename;
				_data = new_data;

				g_cache.add_effect(filename, _data);
			}
		}

		if (_data)
		{
			// find the first valid technique in the file
			_technique = cgGetFirstTechnique(_data->fx);
			while (_technique)
			{
				if (cgValidateTechnique(_technique))
				{
					if (!loaded_from_cache)
					{
						fw::debug << boost::format(" - loading technique \"%1%\" from effect.") % cgGetTechniqueName(_technique) << std::endl;
					}
					break;
				}
				else
				{
					fw::debug << boost::format(" - technique \"%1%\" is not valid, skipping.") % cgGetTechniqueName(_technique) << std::endl;
					_technique = cgGetNextTechnique(_technique);
				}
			}
			
			if (!_technique)
			{
				fw::debug << boost::format("WARNING: no valid technique found in file: %1%") % _data->filename << std::endl;
			}
		}
	}

	void effect::set_technique(std::string const &name)
	{
		if (!_data)
			return;

		CGtechnique new_technique = cgGetNamedTechnique(_data->fx, name.c_str());
		if (new_technique == 0)
		{
			debug << boost::format("WARN: could not find technique for effect file \"%1%\": \"%2%\"")
				% _data->filename % name << std::endl;
		}
		else
		{
			_technique = new_technique;
		}
	}

	effect_pass *effect::begin(boost::shared_ptr<effect_parameters> parameters)
	{
		if (!_data)
			return 0;

		CGtechnique technique = _technique;
		if (parameters && parameters->_technique_name != "")
		{
			technique = cgGetNamedTechnique(_data->fx, parameters->_technique_name.c_str());
			if (technique == 0)
			{
				fw::debug << boost::format("WARN: could not set technique to: \"%1%\"") % parameters->_technique_name << std::endl;
				technique = _technique;
			}
		}

		if (parameters)
		{
			parameters->apply(this);
		}

		return new effect_pass(cgGetFirstPass(technique));
	}

	boost::shared_ptr<effect_parameters> effect::create_parameters()
	{
		boost::shared_ptr<effect_parameters> params(new effect_parameters());
		return params;
	}

	void effect::end(effect_pass *pass)
	{
		if (pass != 0)
			delete pass;
	}

	//-----------------------------------------------------------------------------

	effect_pass::effect_pass(CGpass pass)
		: _curr_pass(pass)
	{
	}

	bool effect_pass::valid() const
	{
		return (_curr_pass != 0);
	}

	void effect_pass::begin_pass()
	{
		CG_CHECKED(cgSetPassState(_curr_pass));
	}

	void effect_pass::end_pass()
	{
		CG_CHECKED(cgResetPassState(_curr_pass));
		_curr_pass = cgGetNextPass(_curr_pass);
	}

}

//-----------------------------------------------------------------------------

CGeffect load_effect(fw::graphics *g, fs::path const &full_path)
{
	// TODO: how to handle optimization, nVidia's nperfhud thingy, etc?

	CGeffect fx = cgCreateEffectFromFile(g->get_cg(), full_path.c_str(), 0);
	if (fx == 0)
	{
		fw::debug << boost::format("ERROR loading effect file: %1%") % full_path.leaf() << std::endl;
		const char *listing = cgGetLastListing(g->get_cg());
		if (listing != 0)
		{
			std::stringstream ss(listing);
			while (!ss.eof())
			{
				char line[1024];
				ss.getline(line, sizeof(line));
				fw::debug << " > " << line << std::endl;
			}
		}
	}
	else
	{
		fw::debug << boost::format("loaded effect from: %1%") % full_path.string() << std::endl;
	}

	return fx;
}
