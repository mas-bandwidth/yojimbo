find_package(unofficial-sodium CONFIG REQUIRED)
find_package(mbedtls           CONFIG REQUIRED)

#include(yojimboTargets.cmake)
include( "${CMAKE_CURRENT_LIST_DIR}/yojimboTargets.cmake" )
