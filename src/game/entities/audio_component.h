#pragma once

#include <list>
#include <map>
#include <memory>

#include <framework/audio.h>
#include <game/entities/entity.h>

namespace ent {

/**
 * This class provide audio cues and so on that other components can reference to play audio from
 * this Entity.
 */
class AudioComponent: public EntityComponent {
public:
  struct Cue {
    std::string name;
    std::shared_ptr<fw::AudioBuffer> audio_buffer;
  };

private:
  std::map<std::string, Cue> cues_;
  std::shared_ptr<fw::AudioSource> source_;

  // Goes through and removes all of the sources which have finished playing.
  void remove_inactive_sources();

public:
  static const int identifier = 800;

  AudioComponent();
  virtual ~AudioComponent();

  void apply_template(fw::lua::Value tmpl) override;
  void initialize() override;

  /** Plays the Cue with the given name (if the Cue doesn't exist, nothing happens). */
  void play_cue(std::string const &name);

  virtual int get_identifier() {
    return identifier;
  }
};

}
