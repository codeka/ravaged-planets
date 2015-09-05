
#include <SDL_mixer.h>

#include <framework/audio.h>
#include <framework/exception.h>
#include <framework/logging.h>

namespace fw {

void audio_manager::initialize() {
  if (Mix_Init(MIX_INIT_OGG) != MIX_INIT_OGG) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Error initializing SDL_mixer"));
  }

  // TODO: these are all just guesses (paraticularly frequency and chunksize)
  Mix_OpenAudio(22050 /* frequency */, AUDIO_S16SYS /* format */, 2 /* channels */, 1024 /* chunksize */);

  SDL_version const *link_version = Mix_Linked_Version();
  fw::debug << "SDL_mixer ("
      << static_cast<int>(link_version->major) << "."
      << static_cast<int>(link_version->minor) << "."
      << static_cast<int>(link_version->patch) << ") initialized" << std::endl;
}

void audio_manager::destroy() {
  Mix_CloseAudio();
  Mix_Quit();
}

void audio_manager::update() {

}

}
