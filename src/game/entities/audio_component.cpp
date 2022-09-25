//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "entity.h"
#include "entity_factory.h"
#include "entity_manager.h"
#include "audio_component.h"
#include "../../framework/framework.h"
#include "../../framework/audio_manager.h"
#include "../../framework/audio_buffer.h"
#include "../../framework/audio_source.h"
#include "../../framework/xml.h"
#include "../../framework/exception.h"

namespace ent {

	// register the component with the entity_factory
	ENT_COMPONENT_REGISTER("audio", AudioComponent);

	class audio_component_template : public entity_component_template
	{
	public:
		virtual bool load(fw::XmlElement const &elem);

		AudioComponent::cue_list cues;
	};

	bool audio_component_template::load(fw::XmlElement const &elem)
	{
		entity_component_template::load(elem);

		fw::audio_manager *audio_mgr = fw::Framework::get_instance()->get_audio();

		for(fw::XmlElement child = elem.get_first_child(); child.is_valid(); child = child.get_next_sibling())
		{
			if (child.get_value() == "sound")
			{
				std::string name = child.get_attribute("cue");
				std::string filename = child.get_attribute("filename");

				// populate a new Cue object
				shared_ptr<AudioComponent::Cue> c(new AudioComponent::Cue());
				c->name = name;
				c->audio = audio_mgr->load_file(filename);
				if (c->audio)
				{
					// only add this Cue if the file loaded successfully
					cues[c->name] = c;
				}
			}
			else
			{
				BOOST_THROW_EXCEPTION(fw::exception()
					<< fw::xml_error_info(child)
					<< fw::message_error_info("Invalid child of <component type=\"audio\">, expected <sound>"));
			}
		}

		return true;
	}

	//-------------------------------------------------------------------------

	AudioComponent::AudioComponent()
	{
	}

	AudioComponent::~AudioComponent()
	{
	}

	void AudioComponent::apply_template(shared_ptr<entity_component_template> comp_template)
	{
		shared_ptr<audio_component_template> tmpl(boost::dynamic_pointer_cast<audio_component_template>(comp_template));
		cues_ = tmpl->cues;
	}

	static bool is_active(shared_ptr<fw::audio_source> const &src)
	{
		return src->is_active();
	}
	void AudioComponent::remove_inactive_sources()
	{
		std::remove_if(active_sources_.begin(), active_sources_.end(), &is_active);
	}

	void AudioComponent::play_cue(std::string const &name)
	{
		remove_inactive_sources();

		cue_list::iterator it = cues_.find(name);
		if (it != cues_.end())
		{
			shared_ptr<Cue> &c = (*it).second;

			shared_ptr<fw::audio_source> src(fw::Framework::get_instance()->get_audio()->create_source());
			src->play(c->audio);
			active_sources_.push_back(src);
		}
	}

	entity_component_template *AudioComponent::create_template()
	{
		return new audio_component_template();
	}

}