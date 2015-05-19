
file(GLOB PARTICLE_TEST_FILES
    *.cc
)

add_executable(particle-test
    ${PARTICLE_TEST_FILES}
)

target_link_libraries(particle-test
    framework
)

install(TARGETS particle-test RUNTIME DESTINATION bin)

