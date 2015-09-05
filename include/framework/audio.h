#pragma once

#include <framework/exception.h>

namespace fw {

// this is the type of error info we include with exception's when an error is detected by SDL_mixer
typedef boost::error_info<struct tag_audioerr, std::string> audio_error_info;

// converts an audio_error_info into a string
std::string to_string(audio_error_info const &err_info);

class audio_manager {
private:
public:
  void initialize();
  void destroy();
  void update();

  static void check_error(int error_code, char const *fn_name);
};

}
