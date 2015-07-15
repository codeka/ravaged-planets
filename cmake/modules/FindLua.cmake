# Try to find Lua library
# Once done, this will define:
#
#  LUA_FOUND - system has Lua
#  LUA_INCLUDE_DIR - the Lua include directory
#  LUA_LIBRARY - Link these to use Lua

SET(LUA_SEARCH_PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
)

find_path (LUA_INCLUDE_DIR lua.h
    HINTS
    $ENV{LUADIR}
    PATH_SUFFIXES include include/lua5.2 include/lua-5.2
    PATHS ${LUA_SEARCH_PATHS}
)

find_library (LUA_LIBRARY NAMES lua5.2 lua-5.2 PATHS ${LUA_SEARCH_PATHS})

find_package_handle_standard_args(Lua
    required_vars LUA_LIBRARY LUA_INCLUDE_DIR)

if (LUA_FOUND)
    mark_as_advanced (LUA_INCLUDE_DIR LUA_LIBRARY)
endif()