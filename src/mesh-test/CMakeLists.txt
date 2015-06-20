
file(GLOB MESH_TEST_FILES
    *.cc
)

add_executable(mesh-test
    ${MESH_TEST_FILES}
)

target_link_libraries(mesh-test
    framework
)

install(TARGETS mesh-test RUNTIME DESTINATION bin)

