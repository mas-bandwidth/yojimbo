# Reads dependencies.manifest into variables. Used by the top-level build, by the installed
# YojimboConfig.cmake, and (as plain text) by the CI job that installs the system dependencies,
# so the tags CI pins, the versions the build checks and the versions a consumer checks all come
# from one file.
#
# Sets, for each dependency:
#   YOJIMBO_DEP_<name>_VENDORED   version of the copy in this repository
#   YOJIMBO_DEP_<name>_TAG        upstream git tag carrying that version
#   YOJIMBO_DEP_<name>_MINIMUM    floor a system-installed copy must meet
# and YOJIMBO_DEPENDENCIES with the names.

function(yojimbo_read_dependency_manifest manifest)
    file(STRINGS "${manifest}" lines)
    set(names "")
    foreach(line ${lines})
        string(STRIP "${line}" line)
        if(line STREQUAL "" OR line MATCHES "^#")
            continue()
        endif()
        string(REGEX MATCHALL "[^ \t]+" fields "${line}")
        list(LENGTH fields count)
        if(NOT count EQUAL 4)
            message(FATAL_ERROR "yojimbo: malformed line in ${manifest}: ${line}")
        endif()
        list(GET fields 0 name)
        list(GET fields 1 vendored)
        list(GET fields 2 tag)
        list(GET fields 3 minimum)
        set(YOJIMBO_DEP_${name}_VENDORED "${vendored}" PARENT_SCOPE)
        set(YOJIMBO_DEP_${name}_TAG "${tag}" PARENT_SCOPE)
        set(YOJIMBO_DEP_${name}_MINIMUM "${minimum}" PARENT_SCOPE)
        list(APPEND names ${name})
    endforeach()
    set(YOJIMBO_DEPENDENCIES "${names}" PARENT_SCOPE)
endfunction()

# Reads the version macros out of an installed or vendored header. Every one of the three
# dependencies spells its version the same way -- <PREFIX>_VERSION_MAJOR / _MINOR / _PATCH -- so
# one reader covers all of them, and it works against a plain header with no package config,
# which is what these projects currently install.
function(yojimbo_read_header_version header prefix out_version)
    if(NOT EXISTS "${header}")
        message(FATAL_ERROR "yojimbo: cannot read a version from ${header}: no such file")
    endif()
    file(READ "${header}" contents)
    set(parts "")
    foreach(component MAJOR MINOR PATCH)
        if(NOT contents MATCHES "#define[ \t]+${prefix}_VERSION_${component}[ \t]+([0-9]+)")
            message(FATAL_ERROR "yojimbo: ${header} does not define ${prefix}_VERSION_${component}")
        endif()
        list(APPEND parts "${CMAKE_MATCH_1}")
    endforeach()
    list(JOIN parts "." version)
    set(${out_version} "${version}" PARENT_SCOPE)
endfunction()
