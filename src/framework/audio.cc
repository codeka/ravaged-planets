
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
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Error initializing SDL_mixer"));
  }

  // TODO: these are all just guesses (paraticularly frequency and chunksize)
  CHECK_ERROR(Mix_OpenAudio(22050 /* frequency */, AUDIO_S16SYS /* format */, 2 /* channels */, 1024 /* chunksize */));

  int frequency;
  uint16_t format;
  int channels;
  if (!Mix_QuerySpec(&frequency, &format, &channels)) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Error querying spec"));
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
  fw::debug << "  decoders loaded: ";
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

void AudioManager::update() {
}

void AudioManager::check_error(int error_code, char const *fn_name) {
  if (error_code != 0) {
    char const *error_msg = Mix_GetError();
    fw::debug << "  error in audio_manager: " << error_code << " - " << error_msg << std::endl;
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(fn_name) << fw::audio_error_info(error_msg));
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

//-----------------------------------------------------------------------------

AudioBuffer::AudioBuffer(AudioManager *mgr, std::string const &name) {
  fs::path path = fw::resolve(name);
  fw::debug << boost::format("loading sound: %1%") % path << std::endl;
  chunk_ = Mix_LoadWAV(path.string().c_str());
}

AudioBuffer::~AudioBuffer() {
  Mix_FreeChunk(chunk_);
  chunk_ = nullptr;
}

//-----------------------------------------------------------------------------
std::string to_string(audio_error_info const &err_info) {
  std::string msg = err_info.value();
  return msg;
}

}
