
#include <framework/audio.h>
#include <framework/framework.h>
#include <framework/lua.h>

#include <game/entities/audio_component.h>
#include <game/entities/entity_factory.h>

namespace ent {

// register the component with the entity_factory
ENT_COMPONENT_REGISTER("Audio", AudioComponent);

AudioComponent::AudioComponent() {
}

AudioComponent::~AudioComponent() {
}

void AudioComponent::apply_template(fw::lua::Value tmpl) {
	fw::AudioManager* audio_manager = fw::Framework::get_instance()->get_audio_manager();

	for (auto& kvp : tmpl) {
		std::string key = kvp.key<std::string>();
		if (key == "Cues") {
			auto cues = kvp.value<fw::lua::Value>();
			for (auto& cue_kvp : cues) {
				auto cue_value = kvp.value<fw::lua::Value>();
				Cue cue;
				cue.name = cue_value["Name"];
				cue.audio_buffer = audio_manager->get_audio_buffer(cue_value["Filename"] );
				if (!cue.audio_buffer) {
					fw::debug << "  error loading cue \"" << cue.name << "\", could not load audio file." << std::endl;
				} else {
					cues_[cue.name] = cue;
				}
			}
		}
	}

}

void AudioComponent::initialize() {
	if (cues_.size() == 0) {
		// Don't bother initializing if there aren't any cues.
		return;
	}

	source_ = fw::Framework::get_instance()->get_audio_manager()->create_audio_source();
}

void AudioComponent::remove_inactive_sources() {
	//std::remove_if(active_sources_.begin(), active_sources_.end(), &is_active);
}

void AudioComponent::play_cue(std::string const &name) {
	auto it = cues_.find(name);
	if (it == cues_.end()) {
		fw::debug << "error: no cue\"" << name << "\"" << std::endl;
		return;
	}

	fw::debug << "playing sound \"" << name << "\" \"" << it->second.name << "\"" << std::endl;
	source_->play(it->second.audio_buffer);
}

}
