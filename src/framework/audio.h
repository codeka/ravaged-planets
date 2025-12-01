#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include <list>
#include <map>
#include <memory>
#include <string>

namespace fw {
class AudioBuffer;
class AudioSource;

// The audio manager creates audio sources, buffers and manages them all.
class AudioManager {
private:
  ALCdevice* device_ = nullptr;
  ALCcontext* context_ = nullptr;

  std::map<std::string, std::shared_ptr<AudioBuffer>> loaded_buffers_;
  std::list<std::weak_ptr<AudioSource>> audio_sources_;

public:
  void initialize();
  void destroy();
  void update(float dt);

  // Loads an audio_buffer with the given name (a filename that will be fw::resolved).
  std::shared_ptr<AudioBuffer> get_audio_buffer(std::string const &name);

  // Creates a new audio source.
  std::shared_ptr<AudioSource> create_audio_source();
};

// An audio buffer represents a single sound "file". The sound can be playing multiple times at once in different
// audio sources.
class AudioBuffer {
private:
  ALuint id_ = 0;

protected:
  friend class AudioSource;

public:
  AudioBuffer(AudioManager* mgr, std::string const& name);
  ~AudioBuffer();
};

// An audio source actually plays a sound. You must create a source in order to play an AudioBuffer. The audio source
// controls things like the 3D spacial position of the sound and so on.
class AudioSource {
private:
  const AudioManager &manager_;

  struct PlayState {
    ALuint id;
    std::shared_ptr<AudioBuffer> buffer;
  };
  std::list<PlayState> playing_sounds_;

  PlayState& find_free_playstate();

protected:
  friend class AudioManager;
  // Called regularly by the AudioManager to update playback.
  void update(float dt);

public:
  AudioSource(const AudioManager& manager);
  ~AudioSource();

  // Start playing the given sound. Returns true if we were able to play the sound, false if not (e.g. if there's
  // no available channels).
  bool play(std::shared_ptr<AudioBuffer> audio);
};

}
