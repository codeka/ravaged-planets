#pragma once

#include <map>
#include <memory>
#include <string>
#include <framework/exception.h>

struct Mix_Chunk;

namespace fw {
class audio_buffer;

// this is the type of error info we include with exception's when an error is detected by SDL_mixer
typedef boost::error_info<struct tag_audioerr, std::string> audio_error_info;

// converts an audio_error_info into a string
std::string to_string(audio_error_info const &err_info);

/** The audio manager creates audio sources, buffers and manages them all. */
class audio_manager {
private:
  std::map<std::string, std::shared_ptr<audio_buffer>> _loaded_buffers;

public:
  void initialize();
  void destroy();
  void update();

  static void check_error(int error_code, char const *fn_name);

  // Loads an audio_buffer with the given name (a filename that will be fw::resolved).
  std::shared_ptr<audio_buffer> get_audio_buffer(std::string const &name);
};

/**
 * An audio buffer represents a single sound "file". The sound can be playing multiple times at once in different
 * audio sources.
 */
class audio_buffer {
private:
  friend class audio_manager;

  Mix_Chunk *_chunk;

  audio_buffer(audio_manager *mgr, std::string const &name);
public:
  ~audio_buffer();
};

}
