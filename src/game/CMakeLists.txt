
file(GLOB GAME_FILES
    *.cc
)

add_executable(rp
    ${GAME_FILES}
)

target_link_libraries(rp
    ${SDL2_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_IOSTREAMS_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_SIGNALS_LIBRARY}
    ${OPENGL_LIBRARY}
    ${GLEW_LIBRARY}
    framework
)

install(TARGETS rp RUNTIME DESTINATION bin)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/ravagedplanets.conf" DESTINATION etc/ravaged-planets)

