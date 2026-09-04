# Clean-prefix consumer for an installed yojimbo (YJ-03).
#
# Nothing in this project references the yojimbo source tree: it configures against an install
# prefix and uses only what find_package(Yojimbo) puts there. CI builds it against both installs
# -- the default bundled one and the -DYOJIMBO_SYSTEM_DEPS one -- so "it installs" means "an
# outside program builds, links and runs against the install", not "the files are present".
#
#   cmake -B build -S tools/consumer -DCMAKE_PREFIX_PATH=<prefix>
#   cmake --build build && ./build/yojimbo_consumer

cmake_minimum_required(VERSION 3.16)

project(YojimboConsumer LANGUAGES CXX)

find_package(Yojimbo REQUIRED)

add_executable(yojimbo_consumer main.cpp)
target_link_libraries(yojimbo_consumer PRIVATE Yojimbo::yojimbo)

# serialize.h's compressed float wants this in every translation unit that compiles it; see
# BUILDING.md. A consumer sets it for itself, exactly as this one does.
if(MSVC)
    target_compile_options(yojimbo_consumer PRIVATE /fp:precise)
else()
    target_compile_options(yojimbo_consumer PRIVATE -ffp-contract=off)
endif()

enable_testing()
add_test(NAME consumer COMMAND yojimbo_consumer)
