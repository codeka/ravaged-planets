# Try to find Luabind (and Lua) library
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
)

find_path (LUABIND_INCLUDE_DIR_LUABIND luabind/luabind.hpp
    HINTS
    $ENV{LUABINDDIR}
    PATH_SUFFIXES include
    PATHS ${LUABIND_SEARCH_PATHS}
)
find_path (LUA_INCLUDE_DIR_LUA lua.h
    HINTS
    $ENV{LUABINDDIR}
    PATH_SUFFIXES include include/lua5.2
    PATHS ${LUABIND_SEARCH_PATHS}
)

list(APPEND LUABIND_INCLUDE_DIR ${LUABIND_INCLUDE_DIR_LUABIND})
if(NOT ${LUA_INCLUDE_DIR_LUA} STREQUAL ${LUABIND_INCLUDE_DIR_LUABIND})
  list(APPEND LUABIND_INCLUDE_DIR ${LUA_INCLUDE_DIR_LUA})
endif()

find_library (LUABIND_LIBRARY_LUA NAMES lua5.2 PATHS ${LUABIND_SEARCH_PATHS})
find_library (LUABIND_LIBRARY_LUABIND NAMES luabind PATHS ${LUABIND_SEARCH_PATHS})

set (LUABIND_LIBRARY ${LUABIND_LIBRARY_LUA} ${LUABIND_LIBRARY_LUABIND})

find_package_handle_standard_args(Luabind
    required_vars LUABIND_LIBRARY_LUA LUABIND_LIBRARY_LUABIND LUABIND_INCLUDE_DIR_LUABIND LUA_INCLUDE_DIR_LUA)

if (LUABIND_FOUND)
    mark_as_advanced (LUABIND_INCLUDE_DIR LUABIND_LIBRARY)
endif()