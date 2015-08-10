# Try to find Luabind library
# Once done, this will define:
#
#  LUABIND_FOUND - system has Luabind
#  LUABIND_INCLUDE_DIR - the Luabind include directory
#  LUABIND_LIBRARY - Link these to use Luabind

SET(LUABIND_SEARCH_PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /opt/local
)

find_path (LUABIND_INCLUDE_DIR luabind/luabind.hpp
    HINTS
    $ENV{LUABINDDIR}
    PATH_SUFFIXES include
    PATHS ${LUABIND_SEARCH_PATHS}
)

find_library (LUABIND_LIBRARY NAMES luabind PATHS ${LUABIND_SEARCH_PATHS})

find_package_handle_standard_args(Luabind
    required_vars LUABIND_LIBRARY LUABIND_INCLUDE_DIR)

if (LUABIND_FOUND)
    mark_as_advanced (LUABIND_INCLUDE_DIR LUABIND_LIBRARY)
endif()
