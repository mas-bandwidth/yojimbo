# yojimbo build. Replaces the old premake5.lua.
#
#   cmake -B build [-DCMAKE_BUILD_TYPE=Debug|Release] [-DYOJIMBO_SYSTEM_SODIUM=ON]
#   cmake --build build -j
#   ./bin/test        # must print "ALL TESTS PASS"
#
# Static libraries: sodium-builtin (unless -DYOJIMBO_SYSTEM_SODIUM=ON), netcode, reliable,
# tlsf, yojimbo. Executables: client, server, loopback, soak, test — all written to bin/.
#
# For package managers (e.g. homebrew): -DYOJIMBO_SYSTEM_DEPS=ON builds against
# system-installed serialize, reliable and netcode instead of the vendored copies, and
# `cmake --install` installs the yojimbo headers and library.

cmake_minimum_required(VERSION 3.16)
project(Yojimbo VERSION 1.6.1 LANGUAGES C CXX)

option(YOJIMBO_SYSTEM_SODIUM
    "Link the system-installed libsodium instead of the bundled subset in sodium/" OFF)

option(YOJIMBO_SYSTEM_DEPS
    "Build against system-installed serialize, reliable and netcode instead of the vendored copies" OFF)

option(YOJIMBO_INSTALL
    "Generate the install target (yojimbo headers and library)" ON)

option(YOJIMBO_BUILD_TESTS
    "Build the test, client/server example and soak programs" ON)

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

# yojimbo/netcode/reliable/serialize each derive their _DEBUG vs _RELEASE mode from NDEBUG
# (see the `#if !defined(..._DEBUG) && !defined(..._RELEASE)` blocks in their headers). CMake
# already defines NDEBUG in Release and not in Debug, so the correct mode is selected
# automatically — matching what premake set explicitly.

# Warnings Extra, RTTI Off (yojimbo uses its own lightweight message-type RTTI).
# No fast math: yojimbo and the vendored libraries build with strict IEEE floating point.
# (The old premake build set fast float globally; that is deliberately not carried forward —
# the serializer round-trips floats bit-exactly and fast math has no business in that path.)
if(MSVC)
    add_compile_options(/W4 $<$<COMPILE_LANGUAGE:CXX>:/GR->)
else()
    add_compile_options(-Wall -Wextra $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
endif()

if(YOJIMBO_SYSTEM_DEPS)

    # --- system dependencies (e.g. homebrew) ------------------------------------------------
    #
    # Build against installed serialize (header-only), reliable and netcode instead of the
    # vendored copies. This is the package-manager configuration: those libraries come from
    # their own packages and receive their own updates. The system netcode supplies its own
    # crypto, so the bundled libsodium is not used at all here (YOJIMBO_SYSTEM_SODIUM is
    # moot). tlsf stays built-in — it is an implementation detail of yojimbo's per-client
    # allocators, not a public dependency.

    find_path(YOJIMBO_SERIALIZE_INCLUDE_DIR serialize.h REQUIRED)
    find_path(YOJIMBO_NETCODE_INCLUDE_DIR netcode.h REQUIRED)
    find_library(YOJIMBO_NETCODE_LIBRARY netcode REQUIRED)
    find_path(YOJIMBO_RELIABLE_INCLUDE_DIR reliable.h REQUIRED)
    find_library(YOJIMBO_RELIABLE_LIBRARY reliable REQUIRED)

    # Needed at link time when the installed netcode is a static archive (a shared netcode
    # already carries its crypto). Optional: absent is fine when netcode is shared.
    find_library(YOJIMBO_SODIUM_LIBRARY sodium)

    set(YOJIMBO_INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/tlsf"
        "${YOJIMBO_SERIALIZE_INCLUDE_DIR}"
        "${YOJIMBO_NETCODE_INCLUDE_DIR}"
        "${YOJIMBO_RELIABLE_INCLUDE_DIR}"
    )

else()

    set(YOJIMBO_INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/sodium"
        "${CMAKE_SOURCE_DIR}/tlsf"
        "${CMAKE_SOURCE_DIR}/netcode"
        "${CMAKE_SOURCE_DIR}/reliable"
        "${CMAKE_SOURCE_DIR}/serialize"
    )

    # --- libsodium backend ----------------------------------------------------------------

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
        set_target_properties(sodium-builtin PROPERTIES POSITION_INDEPENDENT_CODE ON)
        if(MSVC)
            target_compile_options(sodium-builtin PRIVATE /w)
        else()
            target_compile_options(sodium-builtin PRIVATE -w)
        endif()
        set(YOJIMBO_SODIUM sodium-builtin)
    endif()

    # --- vendored C libraries ---------------------------------------------------------------

    add_library(netcode STATIC netcode/netcode.c)
    target_include_directories(netcode PRIVATE ${YOJIMBO_INCLUDE_DIRS})
    target_compile_definitions(netcode PRIVATE NETCODE_ENABLE_TESTS=1)
    set_target_properties(netcode PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(netcode PUBLIC ${YOJIMBO_SODIUM})

    add_library(reliable STATIC reliable/reliable.c)
    target_include_directories(reliable PRIVATE ${YOJIMBO_INCLUDE_DIRS})
    target_compile_definitions(reliable PRIVATE RELIABLE_ENABLE_TESTS=1)
    set_target_properties(reliable PROPERTIES POSITION_INDEPENDENT_CODE ON)

endif()

# Standalone tlsf archive for regular development (e.g. anyone building straight from this
# tree). Not linked into yojimbo and not installed — see below, yojimbo compiles tlsf.c
# directly into its own archive so that only libyojimbo.a ships to package consumers.
add_library(tlsf STATIC tlsf/tlsf.c)
target_include_directories(tlsf PRIVATE "${CMAKE_SOURCE_DIR}/tlsf")
set_target_properties(tlsf PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- yojimbo library ----------------------------------------------------------------------

# Static by default; -DBUILD_SHARED_LIBS=ON builds it shared (the built-in objects are
# position independent, so they fold in either way).
#
# tlsf.c is compiled directly into yojimbo's own sources (rather than linked in from the
# tlsf target above) so that libyojimbo.a is self-contained: a package manager installs a
# single archive, with no separate libtlsf.a for consumers to also link against. tlsf is a
# private implementation detail of yojimbo's per-client allocators, not a public dependency.

file(GLOB YOJIMBO_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/source/*.cpp")
add_library(yojimbo ${YOJIMBO_SOURCES} "${CMAKE_SOURCE_DIR}/tlsf/tlsf.c")
target_include_directories(yojimbo PUBLIC ${YOJIMBO_INCLUDE_DIRS})
set_target_properties(yojimbo PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

if(YOJIMBO_SYSTEM_DEPS)
    target_link_libraries(yojimbo PUBLIC ${YOJIMBO_NETCODE_LIBRARY} ${YOJIMBO_RELIABLE_LIBRARY})
    if(YOJIMBO_SODIUM_LIBRARY)
        target_link_libraries(yojimbo PUBLIC ${YOJIMBO_SODIUM_LIBRARY})
    endif()
    if(WIN32)
        # The system netcode needs these; a plain find_library result can't carry them.
        target_link_libraries(yojimbo PUBLIC ws2_32 iphlpapi)
    endif()
else()
    target_link_libraries(yojimbo PUBLIC netcode reliable ${YOJIMBO_SODIUM})
endif()
if(NOT WIN32)
    target_link_libraries(yojimbo PUBLIC m)   # ceil/floor in the reliable-ordered channel etc.
endif()

# --- install ------------------------------------------------------------------------------

# Installs the yojimbo headers and library. The installed headers include serialize.h (and
# consumers may include netcode.h for netcode_address_t), so this is intended for
# YOJIMBO_SYSTEM_DEPS builds where those headers come from their own packages into the same
# prefix. The vendored copies are never installed — they belong to their own projects.

if(YOJIMBO_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS yojimbo
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    file(GLOB YOJIMBO_PUBLIC_HEADERS "${CMAKE_SOURCE_DIR}/include/*.h")
    install(FILES ${YOJIMBO_PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

# --- sample programs and tests ------------------------------------------------------------

if(YOJIMBO_BUILD_TESTS)

    foreach(app client server loopback soak test)
        add_executable(${app} ${app}.cpp)
        target_link_libraries(${app} PRIVATE yojimbo)
    endforeach()

    # serialize is header-only; its self-tests are gated on this and called from test.cpp.
    target_compile_definitions(test PRIVATE SERIALIZE_ENABLE_TESTS=1)

    if(YOJIMBO_SYSTEM_DEPS)
        # System netcode/reliable are built without their embedded test suites, so test.cpp
        # skips the [netcode] and [reliable] sections in this configuration (see test.cpp).
        target_compile_definitions(test PRIVATE YOJIMBO_SYSTEM_DEPS=1)
    endif()

endif()
