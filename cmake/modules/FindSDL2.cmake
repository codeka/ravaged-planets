# Try to find SDL2 library
# Once done, this will define:
#
#  SDL2_FOUND - system has SDL
#  SDL2_INCLUDE_DIR - the SDL include directory
#  SDL2_LIBRARY - Link these to use SDL

SET(SDL2_SEARCH_PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /opt/local
)

find_path (SDL2_INCLUDE_DIR SDL.h
    HINTS
    $ENV{SDL2DIR}
    PATH_SUFFIXES include
    PATHS ${SDL2_SEARCH_PATHS}
)

find_library (SDL2_LIBRARY NAMES SDL2 PATHS ${SDL2_SEARCH_PATHS})

find_package_handle_standard_args(SDL2
    required_vars SDL2_LIBRARY SDL2_INCLUDE_DIR)

if (SDL2_FOUND)
    mark_as_advanced (SDL2_INCLUDE_DIR SDL2_LIBRARY)
endif()