# Try to find SDL_mixer library
# Once done, this will define:
#
#  SDL_mixer_FOUND - system has SDL_mixer
#  SDL_mixer_INCLUDE_DIR - the SDL_mixer include directory
#  SDL_mixer_LIBRARY - Link these to use SDL_mixer

SET(SDL_mixer_SEARCH_PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /opt/local
)

find_path (SDL_mixer_INCLUDE_DIR SDL_mixer.h
    HINTS
    $ENV{SDL_mixerDIR}
    PATH_SUFFIXES include include/SDL
    PATHS ${SDL_mixer_SEARCH_PATHS}
)

find_library (SDL_mixer_LIBRARY NAMES SDL_mixer PATHS ${SDL_mixer_SEARCH_PATHS})

find_package_handle_standard_args(SDL_mixer
    required_vars SDL_mixer_LIBRARY SDL_mixer_INCLUDE_DIR)

if (SDL_mixer_FOUND)
    mark_as_advanced (SDL_mixer_INCLUDE_DIR SDL_mixer_LIBRARY)
endif()
