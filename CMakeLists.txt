cmake_minimum_required (VERSION 3.1)

# The eclipse generator still doesn't generate the correct values for __cplusplus unless I also do this.
# We need to run this BEFORE project() because the command project() runs to determine compiler support
# will not run it otherwise.
if(${CMAKE_EXTRA_GENERATOR} MATCHES "Eclipse CDT4")
    set(CMAKE_CXX_COMPILER_ARG1 "-std=c++11" CACHE STRING "C++ version for eclipse" FORCE)
    set(CMAKE_ECLIPSE_VERSION "4.4" CACHE STRING "Eclipse version" FORCE)
endif()

project (RavagedPlanets)

# Add our custom CMAKE modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# Make sure we compile with C++11 features
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG)
endif()

if(WIN32)
    # The following command changes \ to / in the Program Files Path so CMake will not complain about bad escape
    # sequences.
    string (REPLACE "\\" "/" PGM_FILES $ENV{PROGRAMFILES})

    set (CMAKE_INSTALL_PREFIX ${PGM_FILES}/Codeka/Ravaged-Planets
         CACHE STRING "Default Install Path" FORCE)
endif()

if (CMAKE_COMPILER_IS_GNUCC)
    # Make sure we compile with -rdynamic so that stack traces can include method names.
    add_definitions(-rdynamic)
endif()

include_directories(include external/include external/include/SDL)
if (UNIX)
    link_directories(external/lib/linux)
endif()

# Configure the config.h file
configure_file(include/config.h.in include/config.h ESCAPE_QUOTES)

# Make sure generated files are #includable.
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

# Look for the various libraries we depend on.
find_package(Boost COMPONENTS filesystem iostreams program_options signals system REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

if(UNIX)
    # I don't know why, but libSDL is importing symbols from libX11.
    find_package(X11 REQUIRED)
    list(APPEND SDL2_LIBRARY ${X11_LIBRARIES} pthread)
endif()

include_directories(${Boost_INCLUDE_DIR})
include_directories(${SDL2_INCLUDE_DIR})
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${GLEW_INCLUDE_DIR})

add_subdirectory(src/version-number)
add_subdirectory(src/framework)
add_subdirectory(src/font-test)
add_subdirectory(src/game)
add_subdirectory(src/particle-test)

# Be sure to install the "data" directory into /share/war-worlds
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/"
        DESTINATION share/ravaged-planets
        REGEX "\\.git" EXCLUDE
)

