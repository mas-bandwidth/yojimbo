# A project that embeds yojimbo with add_subdirectory, the way a game does (YJ-08).
#
# Two things are checked by configuring and building this, which CI does:
#
#   - yojimbo's tests and examples are NOT built here. They used to default on for a
#     subproject, so an embedding project got five extra executables and, with CTest enabled,
#     two extra entries in its own test suite. `ctest -N` must report no tests.
#   - the library is reachable as Yojimbo::yojimbo from the subdirectory, the same name an
#     installed yojimbo exports, so a project can switch between the two without editing its
#     link line.
#
#   cmake -B build -S tools/subproject -DYOJIMBO_SOURCE_DIR=<repo root>
#   cmake --build build && ctest --test-dir build -N   # Total Tests: 0

cmake_minimum_required(VERSION 3.16)

project(YojimboSubproject LANGUAGES CXX)

set(YOJIMBO_SOURCE_DIR "" CACHE PATH "Path to the yojimbo source tree to embed")
if(NOT YOJIMBO_SOURCE_DIR)
    message(FATAL_ERROR "pass -DYOJIMBO_SOURCE_DIR=<repo root>")
endif()

enable_testing()

add_subdirectory("${YOJIMBO_SOURCE_DIR}" yojimbo-build)

foreach(reserved client server loopback soak test custom_packet_io_test tlsf)
    if(TARGET ${reserved})
        message(FATAL_ERROR "yojimbo defined the global target '${reserved}' in a subproject")
    endif()
endforeach()

add_executable(subproject_app ../consumer/main.cpp)
target_link_libraries(subproject_app PRIVATE Yojimbo::yojimbo)
if(MSVC)
    target_compile_options(subproject_app PRIVATE /fp:precise)
else()
    target_compile_options(subproject_app PRIVATE -ffp-contract=off)
endif()

add_test(NAME subproject_app COMMAND subproject_app)
