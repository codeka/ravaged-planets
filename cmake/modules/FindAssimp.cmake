# Based on https://github.com/ivansafrin/Polycode/blob/master/CMake/FindAssimp.cmake
# Based on http://freya3d.org/browser/CMakeFind/FindAssimp.cmake
# Based on http://www.daimi.au.dk/~cgd/code/extensions/Assimp/FindAssimp.cmake
# - Try to find Assimp
# Once done this will define
#
#  ASSIMP_FOUND - system has Assimp
#  ASSIMP_INCLUDE_DIR - the Assimp include directory
#  ASSIMP_LIBRARY - Link these to use Assimp
#  ASSIMP_LIBRARIES

SET(ASSIMP_SEARCH_PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
)

find_path (ASSIMP_INCLUDE_DIR assimp/Importer.hpp
    HINTS
    $ENV{ASSIMPDIR}
    PATH_SUFFIXES include
    PATHS ${ASSIMP_SEARCH_PATHS}
)

find_library (ASSIMP_LIBRARY_DEBUG NAMES assimpd libassimpd libassimp_d PATHS ${ASSIMP_SEARCH_PATHS})
find_library (ASSIMP_LIBRARY_RELEASE NAMES assimp libassimp PATHS ${ASSIMP_SEARCH_PATHS})

if (ASSIMP_INCLUDE_DIR AND ASSIMP_LIBRARY_RELEASE)
    set(ASSIMP_FOUND TRUE)
endif()

if (ASSIMP_LIBRARY_RELEASE)
    set (ASSIMP_LIBRARY ${ASSIMP_LIBRARY_RELEASE})
endif()

if (ASSIMP_LIBRARY_DEBUG AND ASSIMP_LIBRARY_RELEASE)
    set (ASSIMP_LIBRARY debug ${ASSIMP_LIBRARY_DEBUG} optimized ${ASSIMP_LIBRARY_RELEASE} )
endif()

if (ASSIMP_FOUND)
    mark_as_advanced (ASSIMP_INCLUDE_DIR ASSIMP_LIBRARY ASSIMP_LIBRARIES)
endif()

find_package_handle_standard_args(Assimp required_vars ASSIMP_LIBRARY ASSIMP_INCLUDE_DIR)
