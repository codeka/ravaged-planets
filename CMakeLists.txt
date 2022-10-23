cmake_minimum_required (VERSION 3.20)

project (RavagedPlanets)

# Add our custom CMAKE modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# Make sure we compile with C++17 features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_CXX_STANDARD_REQUIRED on)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG)
endif()

if(WIN32)
    # The following command changes \ to / in the Program Files Path so CMake will not complain
    # about bad escape sequences.
    string (REPLACE "\\" "/" PGM_FILES $ENV{PROGRAMFILES})

    if (NOT DEFINED CMAKE_INSTALL_PREFIX)
      set (CMAKE_INSTALL_PREFIX ${PGM_FILES}/Codeka/Ravaged-Planets
           CACHE STRING "Default Install Path" FORCE)
    endif()
endif()

if (CMAKE_COMPILER_IS_GNUCC)
    # Make sure we compile with -rdynamic so that stack traces can include method names.
    add_definitions(-rdynamic)
endif()

# Make sure we can include our own include files.
include_directories(include)
include_directories(src)

# Make sure generated files are #includable.
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

# Look for the various libraries we depend on.
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Lua REQUIRED)
find_package(CURL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Freetype REQUIRED)
find_package(ENet REQUIRED)
find_package(Assimp) # not required, no meshexp if not found.
find_package(Boost COMPONENTS filesystem iostreams program_options system REQUIRED)

if(LINUX)
    # I don't know why, but libSDL is importing symbols from libX11.
    find_package(X11 REQUIRED)
    list(APPEND SDL2_LIBRARY ${X11_LIBRARIES} pthread)
elseif(APPLE)
    find_package(BZip2 REQUIRED)
endif()

add_subdirectory(src/version-number)
add_subdirectory(src/framework)
add_subdirectory(src/meshexp)
add_subdirectory(src/font-test)
add_subdirectory(src/lua-test)
add_subdirectory(src/particle-test)
add_subdirectory(src/mesh-test)
add_subdirectory(src/game)

# Be sure to install the "deploy" directory into /share/ravaged-planets
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/deploy/"
        DESTINATION share/ravaged-planets
        REGEX "\\.git" EXCLUDE
)
