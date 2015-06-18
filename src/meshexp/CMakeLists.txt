
file(GLOB MESHEXP_FILES
    *.cc
)

add_executable(meshexp
    ${MESHEXP_FILES}
)

target_link_libraries(meshexp
    framework
    assimp
)

install(TARGETS meshexp RUNTIME DESTINATION bin)

