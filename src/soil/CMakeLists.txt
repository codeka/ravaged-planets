
file(GLOB SOIL_FILES
    *.c
)

add_library(soil
    ${SOIL_FILES}
)

target_include_directories(soil
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include/soil
)

target_link_libraries(soil
    ${OPENGL_LIBRARY}
)

