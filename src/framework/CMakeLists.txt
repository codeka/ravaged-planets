
file(GLOB FRAMEWORK_FILES
    *.cc
)

if(WIN32)
    file(GLOB ARCH_FILES
        arch/win32/*.cc
    )
elseif(UNIX)
    file(GLOB ARCH_FILES
        arch/linux/*.cc
    )
else()
    message(FATAL_ERROR "Your platform is not supported at the moment.")
endif()

add_library(framework
    ${FRAMEWORK_FILES}
    ${ARCH_FILES}
)

target_link_libraries(framework 
    ${SDL2_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_IOSTREAMS_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_SIGNALS_LIBRARY}
)

