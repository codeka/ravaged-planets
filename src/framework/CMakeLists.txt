
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
    SOURCE_GROUP(src\\arch\\win32 REGULAR_EXPRESSION arch/win32/.*\\.cc)
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
    ${PROTO_SRCS}
)

target_link_libraries(framework 
    ${SDL2_LIBRARY}
	${SDL2main_LIBRARY}
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
)

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

