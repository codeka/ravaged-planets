
file(GLOB GAME_FILES
    *.cc
    editor/*.cc
    editor/tools/*.cc
    editor/windows/*.cc
    screens/*.cc
    world/*.cc
)

add_custom_command(
   OUTPUT version.cc
   COMMAND version-number ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BUILD_TYPE} ${CMAKE_CURRENT_BINARY_DIR}/version.cc
   DEPENDS version-number
)

add_executable(rp
    ${GAME_FILES}
    version.cc
)

target_link_libraries(rp
    framework
)

install(TARGETS rp RUNTIME DESTINATION bin)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/ravagedplanets.conf" DESTINATION etc/ravaged-planets)

