
file(GLOB FONT_TEST_FILES
    *.cc
)

add_executable(font-test
    ${FONT_TEST_FILES}
)

target_link_libraries(font-test
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_IOSTREAMS_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_SIGNALS_LIBRARY}
    ${OPENGL_LIBRARY}
    ${GLEW_LIBRARY}
    freetype
    SDL2_static_2.0.3
    dl # not sure why SDL needs this
    framework
)

install(TARGETS font-test RUNTIME DESTINATION bin)

