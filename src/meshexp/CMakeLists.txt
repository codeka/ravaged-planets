
file(GLOB MESHEXP_FILES
    *.cc
)

if (${ASSIMP_FOUND})
    add_executable(meshexp
        ${MESHEXP_FILES}
    )

    target_include_directories(meshexp
        PUBLIC ${ASSIMP_INCLUDE_DIR}
    )

    target_link_libraries(meshexp
        framework
        ${ASSIMP_LIBRARY}
    )

    install(TARGETS meshexp RUNTIME DESTINATION bin)
endif()
