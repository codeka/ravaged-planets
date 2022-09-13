
file(GLOB FRAMEWORK_FILES
    *.cc
    gui/*.cc
    stb/*.cc
)

# These are used so that we get nice organization of files in Xcode/Visual Studio
file(GLOB FRAMEWORK_HEADERS
    ../../include/framework/*.h
    ../../include/framework/gui/*.h
    ../../include/framework/stb/*.h
)
SOURCE_GROUP(src
  REGULAR_EXPRESSION [^/]+\\.cc)
SOURCE_GROUP(src\\gui
  REGULAR_EXPRESSION gui/.*\\.cc)
SOURCE_GROUP(src\\stb
  REGULAR_EXPRESSION stb/.*\\.cc)
SOURCE_GROUP(inc
  REGULAR_EXPRESSION [^/]+\\.h)
SOURCE_GROUP(inc\\gui
  REGULAR_EXPRESSION gui/.*\\.h)
SOURCE_GROUP(inc\\stb
  REGULAR_EXPRESSION stb/.*\\.h)

if(WIN32)
    file(GLOB ARCH_FILES
        arch/win32/*.cc
    )
    file(GLOB ARCH_HEADERS
        ../../include/framework/arch/win32/*.h
    )
    SOURCE_GROUP(src\\arch\\win32 REGULAR_EXPRESSION arch/win32/.*\\.cc)
    SOURCE_GROUP(inc\\arch\\win32 REGULAR_EXPRESSION arch/win32/.*\\.h)
elseif(UNIX)
    file(GLOB ARCH_FILES
        arch/linux/*.cc
    )
    SOURCE_GROUP(src\\arch\\linux REGULAR_EXPRESSION arch/linux/.*\\.cc)
    set(PROTOC_COMMAND
        LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/../../external/lib/linux
        ${CMAKE_CURRENT_SOURCE_DIR}/../../external/bin/linux/protoc)
else()
    message(FATAL_ERROR "Your platform is not supported at the moment.")
endif()

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS model_file.proto)

add_library(framework
    ${FRAMEWORK_FILES}
    ${FRAMEWORK_HEADERS}
    ${ARCH_FILES}
    ${ARCH_HEADERS}
    ${PROTO_SRCS}
)

target_link_libraries(framework ${Boost_FILESYSTEM_LIBRARY})
target_link_libraries(framework ${Boost_IOSTREAMS_LIBRARY})
target_link_libraries(framework ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(framework ${Boost_THREAD_LIBRARY})
target_link_libraries(framework ${Boost_SYSTEM_LIBRARY})
target_link_libraries(framework ${Boost_SIGNALS_LIBRARY})
target_link_libraries(framework ${OPENGL_LIBRARY})
target_link_libraries(framework ${PROTOBUF_LIBRARY})
target_link_libraries(framework ${LUA_LIBRARIES})
target_link_libraries(framework ${CURL_LIBRARY})
target_link_libraries(framework ${ENET_LIBRARY})
target_link_libraries(framework ${FREETYPE_LIBRARY})
target_link_libraries(framework ${PNG_LIBRARY})

target_link_libraries(framework SDL2::SDL2main)
target_link_libraries(framework SDL2::SDL2)
target_link_libraries(framework SDL2_mixer::SDL2_mixer)
target_link_libraries(framework ${GLEW_LIBRARY})

if(MSVC)
    # Set /EHsc so we can have proper unwind semantics in Visual C++
    target_compile_options(framework PUBLIC /EHsc)
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        # /MDd = multi-threaded debug DLL
        target_compile_options(framework PUBLIC /MDd)
        target_compile_options(framework PUBLIC /ZI)
        target_compile_options(framework PUBLIC /Od)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
    endif()
    target_compile_options(framework PUBLIC /MP)
endif()


if(UNIX)
   target_link_libraries(framework
       z
   )
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(framework
        dl
        pthread
    )
elseif(APPLE)
    target_link_libraries(framework
        -lobjc "-framework Foundation" "-framework Carbon" "-framework AppKit" "-framework AudioUnit"
        "-framework CoreAudio" "-framework IOKit" "-framework ForceFeedback"
        ${BZIP2_LIBRARY} iconv
    )
elseif(WIN32)
    target_link_libraries(framework
        winmm
        version
        imm32
    )
endif()

install(TARGETS framework RUNTIME DESTINATION bin)
