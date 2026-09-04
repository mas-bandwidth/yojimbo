# Prints yojimbo's version, for scripts that need it without parsing CMakeLists.txt:
#
#   cmake -P cmake/PrintVersion.cmake
#
# Read from include/yojimbo_config.h, which is what consumers read, and checked against the
# version project() declares at every configure.

include("${CMAKE_CURRENT_LIST_DIR}/YojimboDependencies.cmake")
yojimbo_read_header_version("${CMAKE_CURRENT_LIST_DIR}/../include/yojimbo_config.h" YOJIMBO version)
message("${version}")
