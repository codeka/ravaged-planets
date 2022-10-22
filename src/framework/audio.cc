
#include <SDL2/SDL_mixer.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <framework/audio.h>
#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/paths.h>

namespace fw {

namespace fs = boost::filesystem;

#if defined(_DEBUG)
#define CHECK_ERROR(fn) \
  AudioManager::check_error(fn, #fn)
#else
#define CHECK_ERROR(fn) \
  fn
#endif

void AudioManager::initialize() {
  if (Mix_Init(MIX_INIT_OGG) != MIX_INIT_OGG) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("Error initializing SDL_mixer"));
  }

  CHECK_ERROR(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, /*chunksize=*/ 1024));

  int frequency;
  uint16_t format;
  int channels;
  if (!Mix_QuerySpec(&frequency, &format, &channels)) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("Error querying spec"));
  }

  std::string format_name;
  switch(format) {
    case AUDIO_U8: format_name = "U8"; break;
    case AUDIO_S8: format_name = "S8"; break;
    case AUDIO_U16LSB: format_name = "U16LSB"; break;
    case AUDIO_S16LSB: format_name = "S16LSB"; break;
    case AUDIO_U16MSB: format_name = "U16MSB"; break;
    case AUDIO_S16MSB: format_name = "S16MSB"; break;
    default: format_name = "<unknown>"; break;
  }

  SDL_version const *link_version = Mix_Linked_Version();
  fw::debug << "SDL_mixer ("
      << static_cast<int>(link_version->major) << "."
      << static_cast<int>(link_version->minor) << "."
      << static_cast<int>(link_version->patch) << ") initialized "
      << "[" << frequency << "Hz, " << format_name << ", " << channels << " channels]" << std::endl;
  fw::debug << "  chunk decoders loaded: ";
  for (int i = 0; i < Mix_GetNumChunkDecoders(); i++) {
    if (i != 0) {
      fw::debug << ", ";
    }
    fw::debug << Mix_GetChunkDecoder(i);
  }
  fw::debug << "." << std::endl;
}

void AudioManager::destroy() {
  Mix_CloseAudio();
  Mix_Quit();
}

void AudioManager::update(float dt) {
  // first, remove any dead audio sources
  audio_sources_.erase(
    std::remove_if(
      audio_sources_.begin(),
      audio_sources_.end(),
      [](const std::weak_ptr<AudioSource>& s) {
        // If the underlying shared_ptr is gone, we remove it.
        return s.expired();
      }), audio_sources_.end());

  // Now update them all.
  for (auto s : audio_sources_) {
    if (auto source = s.lock()) {
      source->update(dt);
    }
  }
}

void AudioManager::check_error(int error_code, char const *fn_name) {
  if (error_code != 0) {
    char const *error_msg = Mix_GetError();
    fw::debug << "  error in audio_manager: " << error_code << " - " << error_msg << std::endl;
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info(fn_name) << fw::audio_error_info(error_msg));
  }
}

std::shared_ptr<AudioBuffer> AudioManager::get_audio_buffer(std::string const &name) {
  auto it = loaded_buffers_.find(name);
  if (it != loaded_buffers_.end()) {
    return it->second;
  }

  auto buffer = std::make_shared<AudioBuffer>(this, name);
  loaded_buffers_[name] = buffer;
  return buffer;
}

std::shared_ptr<AudioSource> AudioManager::create_audio_source() {
  std::shared_ptr<AudioSource> audio_source = std::make_shared<AudioSource>(*this);
  audio_sources_.push_back(audio_source);
  return audio_source;
}

//-----------------------------------------------------------------------------

AudioBuffer::AudioBuffer(AudioManager *mgr, std::string const &name) {
  fs::path path = fw::resolve(name);
  fw::debug << "loading sound: %1%" << path << std::endl;
  chunk_ = Mix_LoadWAV(path.string().c_str());
  if (chunk_ == nullptr) {
    fw::debug << "  error loading sound \"" << name << "\": " << SDL_GetError() << std::endl;
  }
}

AudioBuffer::~AudioBuffer() {
  Mix_FreeChunk(chunk_);
  chunk_ = nullptr;
}

//-----------------------------------------------------------------------------
AudioSource::AudioSource(const AudioManager& manager)
  : manager_(manager) {
}

AudioSource::~AudioSource() {
}

// Called regularly by the AudioManager to update playback.
void AudioSource::update(float dt) {
  // TODO: only remove playing_sounds_ if they've finished?
  playing_sounds_.clear();
}

bool AudioSource::play(std::shared_ptr<AudioBuffer> audio) {
  Mix_Chunk* chunk = audio->get_chunk();
  if (chunk == nullptr) {
    return false;
  }

  // TODO: support looping?
  int channel = Mix_PlayChannel(-1, chunk, 0);
  if (channel < 0) {
    fw::debug << "failed to play sound \"" << "TODO" << "\": " << SDL_GetError() << std::endl;
    return false;
  }

  PlayState state;
  state.channel = channel;
  state.buffer = audio;
  playing_sounds_.push_back(state);
}

//-----------------------------------------------------------------------------
std::string to_string(audio_error_info const &err_info) {
  std::string msg = err_info.value();
  return msg;
}

}
