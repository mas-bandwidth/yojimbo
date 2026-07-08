# yojimbo build. Replaces the old premake5.lua.
#
#   cmake -B build [-DCMAKE_BUILD_TYPE=Debug|Release] [-DYOJIMBO_SYSTEM_SODIUM=ON]
#   cmake --build build -j
#   ./bin/test        # must print "ALL TESTS PASS"
#
# Static libraries: sodium-builtin (unless -DYOJIMBO_SYSTEM_SODIUM=ON), netcode, reliable,
# tlsf, yojimbo. Executables: client, server, loopback, soak, test — all written to bin/.

cmake_minimum_required(VERSION 3.16)
project(Yojimbo LANGUAGES C CXX)

option(YOJIMBO_SYSTEM_SODIUM
    "Link the system-installed libsodium instead of the bundled subset in sodium/" OFF)

# Single-config generators (Make/Ninja) have no build type by default; match the old
# `make config=debug` default of a debug build.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
endif()

# Executables go to bin/ at the repo root, as premake did (so ./bin/test etc. keep working,
# including for multi-config generators like Visual Studio).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${cfg}" CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} "${CMAKE_SOURCE_DIR}/bin")
endforeach()

set(YOJIMBO_INCLUDE_DIRS
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/sodium"
    "${CMAKE_SOURCE_DIR}/tlsf"
    "${CMAKE_SOURCE_DIR}/netcode"
    "${CMAKE_SOURCE_DIR}/reliable"
    "${CMAKE_SOURCE_DIR}/serialize"
)

# yojimbo/netcode/reliable/serialize each derive their _DEBUG vs _RELEASE mode from NDEBUG
# (see the `#if !defined(..._DEBUG) && !defined(..._RELEASE)` blocks in their headers). CMake
# already defines NDEBUG in Release and not in Debug, so the correct mode is selected
# automatically — matching what premake set explicitly.

# Warnings Extra, RTTI Off (yojimbo uses its own lightweight message-type RTTI), fast float —
# matching premake's global settings.
if(MSVC)
    add_compile_options(/W4 /fp:fast $<$<COMPILE_LANGUAGE:CXX>:/GR->)
else()
    add_compile_options(-Wall -Wextra -ffast-math $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
endif()

# --- libsodium backend --------------------------------------------------------------------

if(YOJIMBO_SYSTEM_SODIUM)
    find_library(YOJIMBO_SODIUM_LIB NAMES sodium REQUIRED
        HINTS /opt/homebrew/lib /usr/local/lib /usr/lib)
    message(STATUS "yojimbo: linking system libsodium: ${YOJIMBO_SODIUM_LIB}")
    set(YOJIMBO_SODIUM ${YOJIMBO_SODIUM_LIB})
else()
    # The bundled minimal libsodium (sodium/sodium.c). Warnings off — it is vendored,
    # amalgamated upstream code.
    add_library(sodium-builtin STATIC sodium/sodium.c)
    target_include_directories(sodium-builtin PRIVATE ${YOJIMBO_INCLUDE_DIRS})
    if(MSVC)
        target_compile_options(sodium-builtin PRIVATE /w)
    else()
        target_compile_options(sodium-builtin PRIVATE -w)
    endif()
    set(YOJIMBO_SODIUM sodium-builtin)
endif()

# --- vendored C libraries -----------------------------------------------------------------

add_library(netcode STATIC netcode/netcode.c)
target_include_directories(netcode PRIVATE ${YOJIMBO_INCLUDE_DIRS})
target_compile_definitions(netcode PRIVATE NETCODE_ENABLE_TESTS=1)
target_link_libraries(netcode PUBLIC ${YOJIMBO_SODIUM})

add_library(reliable STATIC reliable/reliable.c)
target_include_directories(reliable PRIVATE ${YOJIMBO_INCLUDE_DIRS})
target_compile_definitions(reliable PRIVATE RELIABLE_ENABLE_TESTS=1)

add_library(tlsf STATIC tlsf/tlsf.c)
target_include_directories(tlsf PRIVATE ${YOJIMBO_INCLUDE_DIRS})

# --- yojimbo library ----------------------------------------------------------------------

file(GLOB YOJIMBO_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/source/*.cpp")
add_library(yojimbo STATIC ${YOJIMBO_SOURCES})
target_include_directories(yojimbo PUBLIC ${YOJIMBO_INCLUDE_DIRS})
target_link_libraries(yojimbo PUBLIC netcode reliable tlsf ${YOJIMBO_SODIUM})
if(NOT WIN32)
    target_link_libraries(yojimbo PUBLIC m)   # ceil/floor in the reliable-ordered channel etc.
endif()

# --- sample programs and tests ------------------------------------------------------------

foreach(app client server loopback soak test)
    add_executable(${app} ${app}.cpp)
    target_link_libraries(${app} PRIVATE yojimbo)
endforeach()

# serialize is header-only; its self-tests are gated on this and called from test.cpp.
target_compile_definitions(test PRIVATE SERIALIZE_ENABLE_TESTS=1)
