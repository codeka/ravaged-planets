
file(GLOB FRAMEWORK_FILES
    *.cc
    gui/*.cc
    stb/*.cc
)

if(WIN32)
    file(GLOB ARCH_FILES
        arch/win32/*.cc
    )
elseif(UNIX)
    file(GLOB ARCH_FILES
        arch/linux/*.cc
    )
    set(PROTOC_COMMAND
        LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/../../external/lib/linux
        ${CMAKE_CURRENT_SOURCE_DIR}/../../external/bin/linux/protoc)
else()
    message(FATAL_ERROR "Your platform is not supported at the moment.")
endif()

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS model_file.proto)

add_library(framework
    ${FRAMEWORK_FILES}
    ${ARCH_FILES}
    ${PROTO_SRCS}
)

target_link_libraries(framework 
    ${SDL2_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_IOSTREAMS_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_SIGNALS_LIBRARY}
    ${OPENGL_LIBRARY}
    ${GLEW_LIBRARY}
    ${PROTOBUF_LIBRARY}
    ${LUA_LIBRARY}
    ${CURL_LIBRARY}
    enet
    freetype
    ${PNG_LIBRARY}
    z
    SDL2_static_2.0.3
)

if(UNIX AND NOT APPLE)
    target_link_libraries(framework
        dl # not sure why SDL needs this
        pthread
    )
elseif(APPLE)
    target_link_libraries(framework
        -lobjc "-framework Foundation" "-framework Carbon" "-framework AppKit" "-framework AudioUnit"
        "-framework CoreAudio" "-framework IOKit" "-framework ForceFeedback"
        ${BZIP2_LIBRARY} iconv
    )
endif()

