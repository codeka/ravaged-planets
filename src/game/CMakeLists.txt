
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

file(GLOB GAME_HEADERS
    *.h
    ai/*.h
    editor/*.h
    editor/tools/*.h
    editor/windows/*.h
    entities/*.h
    screens/*.h
    screens/hud/*.h
    screens/title/*.h
    session/*.h
    simulation/*.h
    world/*.h
)

add_custom_command(
   OUTPUT version.cc
   COMMAND version-number "${CMAKE_CURRENT_SOURCE_DIR}" "$<CONFIG>" "${CMAKE_CURRENT_BINARY_DIR}/version.cc"
   DEPENDS version-number
)

add_executable(rp WIN32
    ${GAME_FILES}
    ${GAME_HEADERS}
    version.cc
)

if(MSVC)
    # Set /EHsc so we can have proper unwind semantics in Visual C++
    target_compile_options(rp PUBLIC /EHsc)
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        # /MDd = multi-threaded debug DLL
        target_compile_options(rp PUBLIC /MDd)
        #target_compile_options(rp PUBLIC /ZI)
        target_compile_options(rp PUBLIC /Od)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
    endif()
    target_compile_options(rp PUBLIC /MP)
endif()

target_link_libraries(rp framework)

install(TARGETS rp RUNTIME DESTINATION bin)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/deploy/default.conf" DESTINATION etc/ravaged-planets/default.conf)
