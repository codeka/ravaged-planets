
file(GLOB GAME_FILES
    *.cc
    ai/*.cc
    editor/*.cc
    editor/tools/*.cc
    editor/windows/*.cc
    entities/*.cc
    screens/*.cc
    screens/hud/*.cc
    screens/title/*.cc
    session/*.cc
    simulation/*.cc
    world/*.cc
)

# These are used so that we get nice organization of files in Xcode/Visual Studio
file(GLOB GAME_HEADERS
    ../../include/game/*.h
    ../../include/game/ai/*.h
    ../../include/game/editor/*.h
    ../../include/game/editor/tools/*.h
    ../../include/game/editor/windows/*.h
    ../../include/game/entities/*.h
    ../../include/game/screens/*.h
    ../../include/game/screens/hud/*.h
    ../../include/game/screens/title/*.h
    ../../include/game/session/*.h
    ../../include/game/simulation/*.h
    ../../include/game/world/*.h
)
SOURCE_GROUP(src REGULAR_EXPRESSION [^/]+\\.cc)
SOURCE_GROUP(src\\ai REGULAR_EXPRESSION ai/.+\\.cc)
SOURCE_GROUP(src\\editor REGULAR_EXPRESSION editor/.+\\.cc)
SOURCE_GROUP(src\\editor\\tools REGULAR_EXPRESSION editor/tools/.+\\.cc)
SOURCE_GROUP(src\\editor\\windows REGULAR_EXPRESSION editor/windows/.+\\.cc)
SOURCE_GROUP(src\\entities REGULAR_EXPRESSION entities/.+\\.cc)
SOURCE_GROUP(src\\screens REGULAR_EXPRESSION screens/.+\\.cc)
SOURCE_GROUP(src\\screens\\hud REGULAR_EXPRESSION screens/hud/.+\\.cc)
SOURCE_GROUP(src\\screens\\title REGULAR_EXPRESSION screens/title/.+\\.cc)
SOURCE_GROUP(src\\session REGULAR_EXPRESSION session/.+\\.cc)
SOURCE_GROUP(src\\simulation REGULAR_EXPRESSION simulation/.+\\.cc)
SOURCE_GROUP(src\\world REGULAR_EXPRESSION world/.+\\.cc)
SOURCE_GROUP(inc REGULAR_EXPRESSION [^/]+\\.h)
SOURCE_GROUP(inc\\ai REGULAR_EXPRESSION ai/.+\\.h)
SOURCE_GROUP(inc\\editor REGULAR_EXPRESSION editor/.+\\.h)
SOURCE_GROUP(inc\\editor\\tools REGULAR_EXPRESSION editor/tools/.+\\.h)
SOURCE_GROUP(inc\\editor\\windows REGULAR_EXPRESSION editor/windows/.+\\.h)
SOURCE_GROUP(inc\\entities REGULAR_EXPRESSION entities/.+\\.h)
SOURCE_GROUP(inc\\screens REGULAR_EXPRESSION screens/.+\\.h)
SOURCE_GROUP(inc\\screens\\hud REGULAR_EXPRESSION screens/hud/.+\\.h)
SOURCE_GROUP(inc\\screens\\title REGULAR_EXPRESSION screens/title/.+\\.h)
SOURCE_GROUP(inc\\session REGULAR_EXPRESSION session/.+\\.h)
SOURCE_GROUP(inc\\simulation REGULAR_EXPRESSION simulation/.+\\.h)
SOURCE_GROUP(inc\\world REGULAR_EXPRESSION world/.+\\.h)

add_custom_command(
   OUTPUT version.cc
   COMMAND version-number ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BUILD_TYPE} ${CMAKE_CURRENT_BINARY_DIR}/version.cc
   DEPENDS version-number
)

add_executable(rp WIN32
    ${GAME_FILES}
    ${GAME_HEADERS}
    version.cc
)

target_link_libraries(rp
    framework
)

install(TARGETS rp RUNTIME DESTINATION bin)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/ravagedplanets.conf" DESTINATION etc/ravaged-planets)
