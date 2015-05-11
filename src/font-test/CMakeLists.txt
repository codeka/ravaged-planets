
file(GLOB FONT_TEST_FILES
    *.cc
)

add_executable(font-test
    ${FONT_TEST_FILES}
)

target_link_libraries(font-test
    framework
)

install(TARGETS font-test RUNTIME DESTINATION bin)

