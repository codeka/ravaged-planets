
#include <SDL_mixer.h>

#include <framework/audio.h>
#include <framework/exception.h>
#include <framework/logging.h>

namespace fw {

#if defined(_DEBUG)
#define CHECK_ERROR(fn) \
  audio_manager::check_error(fn, #fn)
#else
#define CHECK_ERROR(fn) \
  fn
#endif

void audio_manager::initialize() {
  if (Mix_Init(MIX_INIT_OGG) != MIX_INIT_OGG) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Error initializing SDL_mixer"));
  }

  // TODO: these are all just guesses (paraticularly frequency and chunksize)
  CHECK_ERROR(Mix_OpenAudio(22050 /* frequency */, AUDIO_S16SYS /* format */, 2 /* channels */, 1024 /* chunksize */));

  int frequency;
  uint16_t format;
  int channels;
  CHECK_ERROR(Mix_QuerySpec(&frequency, &format, &channels));

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
      << "[" << frequency << "Hz, " << format_name << " " << channels << " channels]" << std::endl;
}

void audio_manager::destroy() {
  Mix_CloseAudio();
  Mix_Quit();
}

void audio_manager::update() {
}

void audio_manager::check_error(int error_code, char const *fn_name) {
  if (error_code != 0) {
    char *error_msg = Mix_GetError();
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(fn_name) << fw::audio_error_info(error_msg));
  }
}

//-----------------------------------------------------------------------------
std::string to_string(audio_error_info const &err_info) {
  std::string msg = err_info.value();
  return msg;
}

}
