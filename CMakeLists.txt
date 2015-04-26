cmake_minimum_required (VERSION 2.8)
project (RavagedPlanets)

# add our custom CMAKE modules...
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

include_directories(include external/include)

if (CMAKE_BUILD_TYPE MATCHES Debug)
	add_definitions(-D_DEBUG)
endif()

add_definitions(-DTIXML_USE_STL)

if(WIN32)
	# The following command changes \ to / in the Program Files Path so
	# CMake will not complain about bad escape sequences.
	string (REPLACE "\\" "/" PGM_FILES $ENV{PROGRAMFILES})

	set (CMAKE_INSTALL_PREFIX ${PGM_FILES}/Codeka/Ravaged-Planets
	     CACHE STRING "Default Install Path" FORCE)
endif()

if (CMAKE_COMPILER_IS_GNUCC)
	add_definitions(-std=c++14)
endif()

# configure the config.h file
configure_file(include/config.h.in include/config.h ESCAPE_QUOTES)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

# look for the boost components we need
find_package(Boost COMPONENTS filesystem iostreams program_options signals system thread REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

# look for SDL
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

# look for Cg (for shaders)
#find_package(Cg REQUIRED)
#include_directories(${CG_INCLUDE_DIR})

add_subdirectory(src/framework)
add_subdirectory(src/game)

# be sure to install the "data" directory into /share/war-worlds
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/"
        DESTINATION share/ravaged-planets
        REGEX "\\.git" EXCLUDE
)

