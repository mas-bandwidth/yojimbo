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
    # The Visual Studio generator does not put per-file flags in compile_commands.json, and
    # CTest runs this file as `cmake -P`, where no project targets exist (TARGET and
    # get_property would fail). CMakeLists.txt therefore resolves each target's COMPILE_OPTIONS
    # at configure time and passes them in as -D variables; check each one here for the flag.
    # The option list is joined with '|' on the way in (an unquoted -D VAR=a;b splits at the
    # ';' and hides /fp:precise behind the first option), so split it back first.
    set(targets sodium-builtin netcode reliable yojimbo yojimbo_test)
    set(vars SODIUM_BUILTIN_OPTS NETCODE_OPTS RELIABLE_OPTS YOJIMBO_OPTS YOJIMBO_TEST_OPTS)
    list(LENGTH targets n)
    math(EXPR last "${n} - 1")
    foreach(i RANGE 0 ${last})
        list(GET targets ${i} tgt)
        list(GET vars ${i} var)
        if(NOT DEFINED ${var})
            message(FATAL_ERROR "target ${tgt}: its compile options were not passed to this check; CMakeLists.txt must resolve them at configure time and pass -D${var}=<options>")
        endif()
        set(opts "${${var}}")
        string(REPLACE "|" ";" opts "${opts}")
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
