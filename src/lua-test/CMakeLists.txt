
file(GLOB LUA_TEST_FILES
    *.cc
)

add_executable(lua-test
    ${LUA_TEST_FILES}
)

target_link_libraries(lua-test
    framework
)

install(TARGETS lua-test RUNTIME DESTINATION bin)
