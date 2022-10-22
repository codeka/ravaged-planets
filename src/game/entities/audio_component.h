#pragma once

#include <list>
#include <map>
#include <memory>

#include <game/entities/entity.h>

namespace fw {
class audio_source;
class audio_buffer;
}

namespace ent {

/**
 * This class provide audio cues and so on that other components can reference to play audio from
 * this Entity.
 */
class AudioComponent: public EntityComponent {
public:
  struct Cue {
    std::string name;
    std::shared_ptr<fw::audio_buffer> audio;
  };

private:
  std::map<std::string, std::shared_ptr<Cue>> cues_;
  std::list<std::shared_ptr<fw::audio_source>> active_sources_;

  // Goes through and removes all of the sources which have finished playing.
  void remove_inactive_sources();

public:
  static const int identifier = 800;

  AudioComponent();
  virtual ~AudioComponent();

  void apply_template(fw::lua::Value tmpl) override;

  /** Plays the Cue with the given name (if the Cue doesn't exist, nothing happens). */
  void play_cue(std::string const &name);

  virtual int get_identifier() {
    return identifier;
  }
};

}
