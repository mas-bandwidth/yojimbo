# check_compile_commands_fp.cmake
#
# Verifies that every .c / .cpp compilation in the build records the strict floating-point
# contraction setting: -ffp-contract=off (GCC/Clang) or /fp:precise (MSVC). This is the
# repository's FP-contraction policy (see CMakeLists.txt / BUILDING.md). It runs as a CTest
# from the build's compile_commands.json, so it sees the OBJECT libraries (netcode, reliable,
# sodium-builtin) whose objects are folded into libyojimbo — the same objects target_compile_options
# on libyojimbo does not reach.
#
# On the Visual Studio generator the database carries no flags, so the target property is
# checked instead and this file says so.

if(NOT DEFINED GENERATOR)
    if(DEFINED CMAKE_GENERATOR)
        set(GENERATOR "${CMAKE_GENERATOR}")
    else()
        set(GENERATOR "")
    endif()
endif()

if(GENERATOR MATCHES "Visual Studio")
    # The Visual Studio generator does not put per-file flags in compile_commands.json.
    # Check the target property instead, exactly where the flag must land.
    foreach(tgt sodium-builtin netcode reliable yojimbo yojimbo_test)
        if(NOT TARGET ${tgt})
            message(FATAL_ERROR "target ${tgt} not found; cannot verify /fp:precise")
        endif()
        get_property(opts TARGET ${tgt} PROPERTY COMPILE_OPTIONS)
        list(FIND opts "/fp:precise" idx)
        if(idx EQUAL -1)
            message(FATAL_ERROR "FP contraction is NOT off: target ${tgt} is missing /fp:precise (options: ${opts})")
        endif()
    endforeach()
    message(STATUS "OK: Visual Studio generator — FP contraction off verified via target properties")
    return()
endif()

set(db "${BUILD_DIR}/compile_commands.json")
if(NOT EXISTS "${db}")
    message(FATAL_ERROR "compile_commands.json not found at ${db}; configure this build with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
endif()

file(READ "${db}" json)
string(JSON count LENGTH "${json}")
math(EXPR last "${count} - 1")
set(failures "")

foreach(i RANGE 0 ${last})
    string(JSON entry GET "${json}" ${i})
    string(JSON file GET "${entry}" "file")
    if(NOT file MATCHES "\\.(c|cpp|cc|C|cxx)$")
        continue()
    endif()
    set(cmd "")
    if(entry MATCHES "\"command\"")
        string(JSON cmd GET "${entry}" "command")
    else()
        string(JSON nargs LENGTH "${entry}" "arguments")
        math(EXPR lastarg "${nargs} - 1")
        foreach(a RANGE 0 ${lastarg})
            string(JSON arg GET "${entry}" "arguments" ${a})
            string(APPEND cmd "${arg} ")
        endforeach()
    endif()
    set(ok OFF)
    if(cmd MATCHES "-ffp-contract=off" OR cmd MATCHES "/fp:precise")
        set(ok ON)
    endif()
    if(NOT ok)
        string(APPEND failures "${file} (missing -ffp-contract=off)\n")
    endif()
endforeach()

if(failures)
    message(FATAL_ERROR "FP contraction is NOT off on the following compilations:\n${failures}")
endif()
message(STATUS "OK: every .c/.cpp compiles with FP contraction off (-ffp-contract=off)")
