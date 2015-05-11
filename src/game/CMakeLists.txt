
file(GLOB GAME_FILES
    *.cc
    screens/*.cc
    world/*.cc
)

add_executable(rp
    ${GAME_FILES}
)

target_link_libraries(rp
    framework
)

install(TARGETS rp RUNTIME DESTINATION bin)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/ravagedplanets.conf" DESTINATION etc/ravaged-planets)

