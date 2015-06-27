# Try to find ENet library
# Once done, this will define:
#
#  ENET_FOUND - system has ENet
#  ENET_INCLUDE_DIR - the ENet include directory
#  ENET_LIBRARY - Link these to use ENet

SET(ENET_SEARCH_PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr/
)

find_path (ENET_INCLUDE_DIR enet/enet.h
    HINTS
    $ENV{ENETDIR}
    PATH_SUFFIXES include
    PATHS ${ENET_SEARCH_PATHS}
)

find_library (ENET_LIBRARY NAMES enet PATHS ${ENET_SEARCH_PATHS})

find_package_handle_standard_args(ENet
    required_vars ENET_LIBRARY ENET_INCLUDE_DIR)

if (ENET_FOUND)
    mark_as_advanced (ENET_INCLUDE_DIR ENET_LIBRARY)
endif()