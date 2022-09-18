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
 * this entity.
 */
class audio_component: public entity_component {
public:
  struct cue {
    std::string name;
    std::shared_ptr<fw::audio_buffer> audio;
  };

private:
  std::map<std::string, std::shared_ptr<cue>> _cues;
  std::list<std::shared_ptr<fw::audio_source> > _active_sources;

  /** Goes through and removes all of the sources which have finished playing. */
  void remove_inactive_sources();

public:
  static const int identifier = 800;

  audio_component();
  virtual ~audio_component();

  virtual void apply_template(luabind::object const &tmpl);

  /** Plays the cue with the given name (if the cue doesn't exist, nothing happens). */
  void play_cue(std::string const &name);

  virtual int get_identifier() {
    return identifier;
  }
};

}
