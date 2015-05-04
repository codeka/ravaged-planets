cmake_minimum_required (VERSION 3.1)
project (RavagedPlanets)

# add our custom CMAKE modules...
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# Make sure we compile with C++14 features
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG)
endif()

if(WIN32)
    # The following command changes \ to / in the Program Files Path so
    # CMake will not complain about bad escape sequences.
    string (REPLACE "\\" "/" PGM_FILES $ENV{PROGRAMFILES})

    set (CMAKE_INSTALL_PREFIX ${PGM_FILES}/Codeka/Ravaged-Planets
         CACHE STRING "Default Install Path" FORCE)
endif()

if (CMAKE_COMPILER_IS_GNUCC)
    # Make sure we compile with -rdynamic so that stack traces can include method names.
    add_definitions(-rdynamic)
endif()

include_directories(include external/include)

# configure the config.h file
configure_file(include/config.h.in include/config.h ESCAPE_QUOTES)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

# look for the various libraries we depend on
find_package(Boost COMPONENTS filesystem iostreams program_options signals system thread REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${SDL2_INCLUDE_DIR})
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${GLEW_INCLUDE_DIR})

# look for Cg (for shaders)
#find_package(Cg REQUIRED)
#include_directories(${CG_INCLUDE_DIR})

add_subdirectory(src/soil)
add_subdirectory(src/framework)
add_subdirectory(src/game)

# be sure to install the "data" directory into /share/war-worlds
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/"
        DESTINATION share/ravaged-planets
        REGEX "\\.git" EXCLUDE
)

