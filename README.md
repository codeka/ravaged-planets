# Ravaged Planets

A real-time strategy game for Windows, Mac OS X and Linux.

This project is based off a very old project I semi-abandoned several years ago, and just happen to
have the source code still lying around. The original project ran on Windows only (it used DirectX
for rendering) and it relied pretty heavily on CEGUI for rendering the UI. You can see a video of
it here:

[![Original War Worlds](http://img.youtube.com/vi/onEK7niVMxM/mqdefault.jpg)](http://www.youtube.com/watch?v=onEK7niVMxM)

The original source code was never public, and I plan to basically slowly cut'n'paste that code as
I go here bringing the project into the modern era as a cross-platform OpenGL 3.x application. The
name is changed from War Worlds to differentiate it from another project I have which I also
(imaginatively) called [War Worlds](http://www.war-worlds.com).

## Building

The project requires CMake to build. You must have a compiler installed (e.g. Visual Studio 2022
for Windows, gcc for Linux etc). Simply load up the root CMakeLists.txt

### Dependencies

There are a bunch of dependencies, see the CMakeLists.txt file for the complete list.

`vcpkg` can be used for all of the dependecies, I've only tested this on Windows. If you want to use
`vcpkg`, [see here](https://github.com/microsoft/vcpkg)
[and here](https://vcpkg.io/en/docs/users/buildsystems/cmake-integration.html) to get started, then:

```
vcpkg install assimp curl enet glew lua openal-soft protobuf sdl2 freetype
```

### Visual Studio 2022

I've tested Ravaged Planets by just opening the directory locally with Visual Studio 2022. It detects
the CMake configuration and works "out of the box".
[More info here](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio)

For Windows, you probably want to use the "install" target when running, which ensures all of the runtime
files are available. For that you'll need to configure CMAKE_INSTALL_PREFIX to be somewhere you have
non-Admin access to. To configure this, just look what I've done in CMakePresets.json and make your
own CMakeUserPresets.json (don't check that file into git).

## Credits

### Terrain Textures

 * https://opengameart.org/content/hitw-terrain-textures
 * https://opengameart.org/content/terrain-textures-pack-from-stunt-rally-23
 * https://opengameart.org/content/0-ad-textures
