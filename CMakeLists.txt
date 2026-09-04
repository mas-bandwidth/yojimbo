# yojimbo build. Replaces the old premake5.lua.
#
#   cmake -B build [-DCMAKE_BUILD_TYPE=Debug|Release] [-DYOJIMBO_SYSTEM_SODIUM=ON]
#   cmake --build build -j
#   ctest --test-dir build --output-on-failure     # or ./build/bin/test directly
#
# Produces one library, libyojimbo, with the vendored netcode, reliable, tlsf and libsodium
# objects folded in. Executables: client, server, loopback, soak, test — all written to
# <build>/bin (multi-config generators: <build>/bin/<Config>).
#
# For package managers (e.g. homebrew): -DYOJIMBO_SYSTEM_DEPS=ON builds against
# system-installed serialize, reliable and netcode instead of the vendored copies, and
# `cmake --install` installs the yojimbo headers and library. Package managers that ship
# tlsf as its own package additionally set -DYOJIMBO_SYSTEM_TLSF=ON (see the tlsf backend
# section below).

cmake_minimum_required(VERSION 3.16)

# A no-op build prints nothing (rowan-new#110). The per-target "Built target" lines are
# the only output a rebuild with nothing to do produces, and they carry no information; a
# real build still prints its [ x%] progress lines, and errors are untouched. This sits
# above project() because CMAKE_TARGET_MESSAGES initializes the TARGET_MESSAGES global
# property there and has no effect if set after it, and under a top-level test so a consumer
# that pulls this library in through FetchContent keeps its own messages.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_TARGET_MESSAGES OFF)
endif()

project(Yojimbo VERSION 1.12.0 LANGUAGES C CXX)

option(YOJIMBO_SYSTEM_SODIUM
    "Link the system-installed libsodium instead of the bundled subset in sodium/" OFF)

option(YOJIMBO_SYSTEM_DEPS
    "Build against system-installed serialize, reliable and netcode instead of the vendored copies" OFF)

# Same reasoning: a consumer embedding yojimbo in their own build installs their program, not
# yojimbo's headers and archive into their prefix.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(YOJIMBO_INSTALL_DEFAULT ON)
else()
    set(YOJIMBO_INSTALL_DEFAULT OFF)
endif()

option(YOJIMBO_INSTALL
    "Generate the install target (yojimbo headers and library)" ${YOJIMBO_INSTALL_DEFAULT})

option(YOJIMBO_SYSTEM_TLSF
    "Build against a system-installed tlsf instead of the vendored copy" OFF)

# Default ON for a top-level build (someone building yojimbo itself wants its tests and
# examples) and OFF for a consumer that pulls this in with add_subdirectory or FetchContent
# (YJ-08): they want the library, not five more executables and two more CTest entries in their
# own project. Same rule as CMAKE_TARGET_MESSAGES and CMAKE_BUILD_TYPE above.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(YOJIMBO_BUILD_TESTS_DEFAULT ON)
else()
    set(YOJIMBO_BUILD_TESTS_DEFAULT OFF)
endif()

option(YOJIMBO_BUILD_TESTS
    "Build the test, client/server example and soak programs" ${YOJIMBO_BUILD_TESTS_DEFAULT})

# Single-config generators (Make/Ninja) have no build type by default; match the old
# `make config=debug` default of a debug build.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
endif()

# Executables go under the build tree, never into the source tree (YJ-06). Every build tree used
# to write to <source>/bin, so a Debug and a Release build of the same checkout overwrote each
# other's ./bin/test and whichever ran last was whatever had been built last — a test step could
# silently exercise the other configuration. Multi-config generators additionally get one
# directory per configuration, instead of the single flattened path they were forced into.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${cfg}" CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} "${CMAKE_BINARY_DIR}/bin/${cfg}")
endforeach()

# yojimbo/netcode/reliable/serialize each derive their _DEBUG vs _RELEASE mode from NDEBUG
# (see the `#if !defined(..._DEBUG) && !defined(..._RELEASE)` blocks in their headers). CMake
# already defines NDEBUG in Release and not in Debug, so the correct mode is selected
# automatically — matching what premake set explicitly.

# Warnings Extra. RTTI stays ON (YJ-05): yojimbo has public polymorphic classes — Allocator,
# MessageFactory, Adapter, ClientInterface, ServerInterface — that consumers derive from, and the
# compiler default is RTTI on. Building the library with -fno-rtti emitted their vtables without
# the matching typeinfo, so an ordinary consumer that used dynamic_cast or typeid on one of them
# failed to link on "typeinfo for yojimbo::Allocator". The public ABI is built the way consumers
# build. Messages still carry their own lightweight type tag on the wire; that is a separate
# thing and unaffected.
# No fast math: yojimbo and the vendored libraries build with strict IEEE floating point.
# (The old premake build set fast float globally; that is deliberately not carried forward —
# the serializer round-trips floats bit-exactly and fast math has no business in that path.)
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# --- tlsf backend -------------------------------------------------------------------------
#
# tlsf is a private implementation detail of yojimbo's per-client allocators. By default the
# vendored copy compiles directly into libyojimbo so the archive is self-contained. A package
# manager that ships tlsf as its own package (e.g. vcpkg) sets -DYOJIMBO_SYSTEM_TLSF=ON:
# yojimbo then stops embedding tlsf objects and links the system library publicly instead.
# Design from the vcpkg port review — vicroms's use-system-tlsf patch, adopted upstream so
# no package carries a patch.

if(YOJIMBO_SYSTEM_TLSF)
    find_path(YOJIMBO_TLSF_INCLUDE_DIR tlsf.h REQUIRED)
    find_library(YOJIMBO_TLSF_LIBRARY tlsf REQUIRED)
    message(STATUS "yojimbo: linking system tlsf: ${YOJIMBO_TLSF_LIBRARY}")
    set(YOJIMBO_TLSF_INCLUDE_DIRS "${YOJIMBO_TLSF_INCLUDE_DIR}")
else()
    set(YOJIMBO_TLSF_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/tlsf")
endif()

include(GNUInstallDirs)

# --- version ---------------------------------------------------------------------------------
#
# project() above and the YOJIMBO_*_VERSION macros in yojimbo_config.h are two statements of the
# same fact, and consumers read the second. They must agree, and the documentation takes its
# version from here rather than repeating it a third time (YJ-12).

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/YojimboDependencies.cmake")
yojimbo_read_header_version("${CMAKE_CURRENT_SOURCE_DIR}/include/yojimbo_config.h" YOJIMBO YOJIMBO_HEADER_VERSION)
if(NOT YOJIMBO_HEADER_VERSION VERSION_EQUAL PROJECT_VERSION)
    message(FATAL_ERROR
        "yojimbo: include/yojimbo_config.h says ${YOJIMBO_HEADER_VERSION} but project() says "
        "${PROJECT_VERSION}. Both have to move together.")
endif()

# --- dependency manifest --------------------------------------------------------------------
#
# dependencies.manifest is the one place serialize, reliable and netcode are versioned: the
# version vendored here, the upstream tag it came from (which the system-deps CI job installs)
# and the floor a system-installed copy must meet.

yojimbo_read_dependency_manifest("${CMAKE_CURRENT_SOURCE_DIR}/dependencies.manifest")

if(YOJIMBO_SYSTEM_DEPS)

    # --- system dependencies (e.g. homebrew) ------------------------------------------------
    #
    # Build against installed serialize (header-only), reliable and netcode instead of the
    # vendored copies. This is the package-manager configuration: those libraries come from
    # their own packages and receive their own updates. The system netcode supplies its own
    # crypto, so the bundled libsodium is not used at all here (YOJIMBO_SYSTEM_SODIUM is
    # moot). tlsf stays built-in — it is an implementation detail of yojimbo's per-client
    # allocators, not a public dependency.
    #
    # The three arrive as versioned imported targets whose versions are read from the installed
    # headers and checked against the manifest floors at configure time. The same module runs
    # again inside the installed YojimboConfig.cmake, so a consumer of this install resolves
    # them identically.

    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/YojimboSystemDeps.cmake")
    yojimbo_find_system_deps("${CMAKE_CURRENT_SOURCE_DIR}/dependencies.manifest")

    set(YOJIMBO_INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${YOJIMBO_TLSF_INCLUDE_DIRS}"
        "${YOJIMBO_SERIALIZE_INCLUDE_DIR}"
        "${YOJIMBO_NETCODE_INCLUDE_DIR}"
        "${YOJIMBO_RELIABLE_INCLUDE_DIR}"
    )

else()

    # The vendored copies must be exactly what the manifest says they are, so the tags the
    # system-deps job installs really are the code the default build compiles.
    foreach(dep serialize reliable netcode)
        string(TOUPPER "${dep}" DEP)
        yojimbo_read_header_version("${CMAKE_CURRENT_SOURCE_DIR}/${dep}/${dep}.h" ${DEP} vendored_version)
        if(NOT vendored_version VERSION_EQUAL YOJIMBO_DEP_${dep}_VENDORED)
            message(FATAL_ERROR
                "yojimbo: vendored ${dep} is ${vendored_version} but dependencies.manifest says "
                "${YOJIMBO_DEP_${dep}_VENDORED}. Update the manifest with the re-vendor.")
        endif()
    endforeach()

    set(YOJIMBO_INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/sodium"
        "${YOJIMBO_TLSF_INCLUDE_DIRS}"
        "${CMAKE_CURRENT_SOURCE_DIR}/netcode"
        "${CMAKE_CURRENT_SOURCE_DIR}/reliable"
        "${CMAKE_CURRENT_SOURCE_DIR}/serialize"
    )

    # --- libsodium backend ----------------------------------------------------------------

    if(YOJIMBO_SYSTEM_SODIUM)
        find_library(YOJIMBO_SODIUM_LIBRARY NAMES sodium REQUIRED
            HINTS /opt/homebrew/lib /usr/local/lib /usr/lib)
        message(STATUS "yojimbo: linking system libsodium: ${YOJIMBO_SODIUM_LIBRARY}")
        add_library(Yojimbo::sodium UNKNOWN IMPORTED)
        set_target_properties(Yojimbo::sodium PROPERTIES IMPORTED_LOCATION "${YOJIMBO_SODIUM_LIBRARY}")
    else()
        # The bundled minimal libsodium (sodium/sodium.c). Warnings off — it is vendored,
        # amalgamated upstream code.
        add_library(sodium-builtin OBJECT sodium/sodium.c)
        target_include_directories(sodium-builtin PRIVATE ${YOJIMBO_INCLUDE_DIRS})
        set_target_properties(sodium-builtin PROPERTIES POSITION_INDEPENDENT_CODE ON)
        if(MSVC)
            target_compile_options(sodium-builtin PRIVATE /w)
        else()
            # -fno-sanitize=nonnull-attribute belongs to these sources and nothing else (SEC-05).
            # netcode encrypts zero-length plaintexts and libsodium's AEAD then does
            # memcpy(dst, NULL, 0) -- benign UB in vendored upstream code. CI used to put the
            # exemption in the global sanitizer flags, which switched the check off for every
            # first-party source too. It is a no-op when no sanitizer is enabled.
            target_compile_options(sodium-builtin PRIVATE -w -fno-sanitize=nonnull-attribute)
        endif()
        set(YOJIMBO_SODIUM_OBJECTS $<TARGET_OBJECTS:sodium-builtin>)
    endif()

    # --- vendored C libraries ---------------------------------------------------------------
    #
    # OBJECT libraries, folded into libyojimbo below (YJ-03). They used to be separate static
    # archives that libyojimbo linked PUBLIC and the install rule never shipped, so the default
    # install published a libyojimbo.a with netcode, reliable and libsodium symbols undefined
    # and no way for a consumer to satisfy them. The bundled install is one self-contained
    # archive; a consumer who wants those libraries as their own packages builds with
    # -DYOJIMBO_SYSTEM_DEPS=ON.

    add_library(netcode OBJECT netcode/netcode.c)
    target_include_directories(netcode PRIVATE ${YOJIMBO_INCLUDE_DIRS})
    target_compile_definitions(netcode PRIVATE NETCODE_ENABLE_TESTS=1)
    set_target_properties(netcode PROPERTIES POSITION_INDEPENDENT_CODE ON)

    add_library(reliable OBJECT reliable/reliable.c)
    target_include_directories(reliable PRIVATE ${YOJIMBO_INCLUDE_DIRS})
    target_compile_definitions(reliable PRIVATE RELIABLE_ENABLE_TESTS=1)
    set_target_properties(reliable PROPERTIES POSITION_INDEPENDENT_CODE ON)

    set(YOJIMBO_VENDORED_OBJECTS
        $<TARGET_OBJECTS:netcode>
        $<TARGET_OBJECTS:reliable>
        ${YOJIMBO_SODIUM_OBJECTS})

endif()

# --- strict floating point ------------------------------------------------------------------

# yojimbo's wire arithmetic lives in serialize.h, and serialize's compressed float quantizes
# with two DISTINCT roundings on write and on read -- the product must round to float32 before
# the constant is added. The vendored serialize pins those roundings in-source with an
# optimization barrier, so the compressed-float wire bytes are identical under every
# -ffp-contract setting. -ffast-math / -Ofast remain unsupported: they license reciprocal
# approximation and reassociation, which change the wire in ways no barrier can pin.
#
# Every target here that compiles serialize.h still builds strict, exactly as serialize's own
# CMakeLists.txt does for its executables:
#   - GCC/Clang: -ffp-contract=off.
#   - MSVC: /fp:precise, which since VS 2022 does not imply contraction.
#
# This is Glenn's standing estate policy (2026-08-24): "generally speaking, network libraries
# we work on require -ffp-contract=off" -- belt and braces on top of the in-source barrier,
# and the certification setting for golden vectors. The flag is PRIVATE so nothing leaks
# into consumers' own flags; consumers that compile serialize.h themselves set it too, and
# BUILDING.md states that as policy.

if(MSVC)
    set(YOJIMBO_STRICT_FP_FLAGS /fp:precise)
else()
    set(YOJIMBO_STRICT_FP_FLAGS -ffp-contract=off)
endif()

# --- yojimbo library ----------------------------------------------------------------------

# Static by default; -DBUILD_SHARED_LIBS=ON builds it shared (the built-in objects are
# position independent, so they fold in either way).
#
# tlsf.c is compiled directly into yojimbo's own sources (rather than linked in from the
# tlsf target above) so that libyojimbo.a is self-contained: a package manager installs a
# single archive, with no separate libtlsf.a for consumers to also link against. tlsf is a
# private implementation detail of yojimbo's per-client allocators, not a public dependency.
# The exception is YOJIMBO_SYSTEM_TLSF: there the package manager ships tlsf as its own
# package, so yojimbo embeds nothing and links it publicly instead.

file(GLOB YOJIMBO_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
if(NOT YOJIMBO_SYSTEM_TLSF)
    list(APPEND YOJIMBO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tlsf/tlsf.c")
endif()
add_library(yojimbo ${YOJIMBO_SOURCES} ${YOJIMBO_VENDORED_OBJECTS})
add_library(Yojimbo::yojimbo ALIAS yojimbo)
target_include_directories(yojimbo PUBLIC
    "$<BUILD_INTERFACE:${YOJIMBO_INCLUDE_DIRS}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_compile_options(yojimbo PRIVATE ${YOJIMBO_STRICT_FP_FLAGS})
set_target_properties(yojimbo PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

if(YOJIMBO_SYSTEM_DEPS)
    target_link_libraries(yojimbo PUBLIC Yojimbo::netcode Yojimbo::reliable Yojimbo::serialize)
elseif(YOJIMBO_SYSTEM_SODIUM)
    target_link_libraries(yojimbo PUBLIC Yojimbo::sodium)
endif()
if(NOT WIN32)
    target_link_libraries(yojimbo PUBLIC m)   # ceil/floor in the reliable-ordered channel etc.
endif()
if(YOJIMBO_SYSTEM_TLSF)
    target_link_libraries(yojimbo PUBLIC Yojimbo::tlsf)
endif()

# --- install ------------------------------------------------------------------------------
#
# Two coherent installs, both consumable with nothing but what they put in the prefix (YJ-03):
#
#   default (vendored)   libyojimbo carries the netcode, reliable, tlsf and libsodium objects,
#                        and the serialize, netcode, reliable and sodium headers are installed
#                        beside yojimbo's own. yojimbo.h includes serialize.h and consumers
#                        include netcode.h for netcode_address_t, so the headers have to be
#                        there. Nothing else is needed on the system.
#
#   -DYOJIMBO_SYSTEM_DEPS=ON   the package-manager install. serialize, reliable and netcode come
#                        from their own packages, so their headers are NOT installed here;
#                        YojimboConfig.cmake requires them instead, at the manifest's floors.
#
# Both export Yojimbo::yojimbo through YojimboConfig.cmake, so a consumer writes
# find_package(Yojimbo REQUIRED) and links one target.

if(YOJIMBO_INSTALL)

    install(TARGETS yojimbo EXPORT YojimboTargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    file(GLOB YOJIMBO_PUBLIC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
    install(FILES ${YOJIMBO_PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    if(NOT YOJIMBO_SYSTEM_DEPS)
        install(FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/serialize/serialize.h"
                "${CMAKE_CURRENT_SOURCE_DIR}/netcode/netcode.h"
                "${CMAKE_CURRENT_SOURCE_DIR}/reliable/reliable.h"
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
        if(NOT YOJIMBO_SYSTEM_SODIUM)
            install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/sodium/sodium.h"
                    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
        endif()
    endif()

    if(NOT YOJIMBO_SYSTEM_TLSF)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/tlsf/tlsf.h"
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    endif()

    set(YOJIMBO_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/Yojimbo)

    install(EXPORT YojimboTargets
            FILE YojimboTargets.cmake
            NAMESPACE Yojimbo::
            DESTINATION ${YOJIMBO_CMAKE_DIR})

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/YojimboConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/YojimboConfig.cmake"
        INSTALL_DESTINATION ${YOJIMBO_CMAKE_DIR})
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/YojimboConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/YojimboConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/YojimboConfigVersion.cmake"
            DESTINATION ${YOJIMBO_CMAKE_DIR})

    # The system-deps config resolves its dependencies with the same module and the same
    # manifest yojimbo's own build used, so both ship with the package.
    if(YOJIMBO_SYSTEM_DEPS)
        install(FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/YojimboSystemDeps.cmake"
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/YojimboDependencies.cmake"
                "${CMAKE_CURRENT_SOURCE_DIR}/dependencies.manifest"
                DESTINATION ${YOJIMBO_CMAKE_DIR})
    endif()

endif()

# --- documentation --------------------------------------------------------------------------
#
# `cmake --build <build> --target docs` runs doxygen with the project version in the
# environment, which is where doxygen.config's PROJECT_NUMBER comes from -- it used to say 1.0
# for every release. doxygen.config also fails the build on any documentation warning, so an
# undocumented public member or a @param that no longer matches its signature is a build error
# and not a line that scrolls past (YJ-12). Only offered for a top-level build with doxygen
# installed; nothing else depends on it.

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_custom_target(docs
            COMMAND ${CMAKE_COMMAND} -E env "YOJIMBO_VERSION=${PROJECT_VERSION}"
                    ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/doxygen.config"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "yojimbo: generating documentation with doxygen (warnings are errors)"
            VERBATIM)
    endif()
endif()

# --- sample programs and tests ------------------------------------------------------------

if(YOJIMBO_BUILD_TESTS)

    # CMake target names are global, so they are prefixed; the executables they produce keep the
    # plain names, so build/bin/test and build/bin/client are what they always were. `test` in
    # particular is a name CTest reserves.
    foreach(app client server loopback soak test custom_packet_io_test)
        add_executable(yojimbo_${app} ${app}.cpp)
        set_target_properties(yojimbo_${app} PROPERTIES OUTPUT_NAME ${app})
        target_link_libraries(yojimbo_${app} PRIVATE yojimbo)
        target_compile_options(yojimbo_${app} PRIVATE ${YOJIMBO_STRICT_FP_FLAGS})
    endforeach()

    # serialize is header-only; its self-tests are gated on this and called from test.cpp.
    target_compile_definitions(yojimbo_test PRIVATE SERIALIZE_ENABLE_TESTS=1)

    # The cross-configuration layout test (YJ-04). These two translation units compile the public
    # headers with YOJIMBO_DEBUG and YOJIMBO_RELEASE forced on respectively, whatever the build
    # type is, and report the sizes of Allocator and MessageFactory. test.cpp checks the two agree
    # with each other and with the build's own, so a member that appears only under one NDEBUG
    # setting fails the test instead of silently giving consumers a different layout.
    target_sources(yojimbo_test PRIVATE test_layout_debug.cpp test_layout_release.cpp)

    if(YOJIMBO_SYSTEM_DEPS)
        # System netcode/reliable are built without their embedded test suites, so test.cpp
        # skips the [netcode] and [reliable] sections in this configuration (see test.cpp).
        target_compile_definitions(yojimbo_test PRIVATE YOJIMBO_SYSTEM_DEPS=1)
    endif()

    # CTest registration (YJ-06). `ctest --test-dir <build>` runs the binaries of that build tree
    # and no other, which is the point: it cannot pick up a stale executable another
    # configuration left behind. The examples (client, server, loopback) need a peer and the soak
    # runs until interrupted, so neither is a test; both stay buildable.
    enable_testing()
    add_test(NAME yojimbo_test COMMAND yojimbo_test)
    add_test(NAME yojimbo_custom_packet_io_test COMMAND yojimbo_custom_packet_io_test)

endif()
