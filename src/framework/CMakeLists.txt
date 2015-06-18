
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

add_custom_command(
    OUTPUT model_file.pb.h model_file.pb.cc
    COMMAND ${PROTOC_COMMAND} --cpp_out=. --proto_path=${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/model_file.proto
    MAIN_DEPENDENCY model_file.proto
)
set_source_files_properties(
    model_file.pb.h model_file_pb.cc
    PROPERTIES GENERATED TRUE
)

add_library(framework
    ${FRAMEWORK_FILES}
    ${ARCH_FILES}
    model_file.pb.cc
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
    freetype
    png
    z
    SDL2_static_2.0.3
    protobuf
    dl # not sure why SDL needs this
)

