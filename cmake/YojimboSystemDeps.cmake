# Resolves the system-installed serialize, reliable and netcode into versioned IMPORTED targets.
#
# Run both by yojimbo's own build (-DYOJIMBO_SYSTEM_DEPS=ON) and by the installed
# YojimboConfig.cmake, so a consumer of that install resolves the dependencies the same way, to
# the same floors, from the same manifest. Bare find_path / find_library results carry no version
# and no usage requirements, which is how CI came to link a netcode inside a published advisory
# range (SEC-04).
#
# Creates: Yojimbo::serialize (header only), Yojimbo::reliable, Yojimbo::netcode, and
# Yojimbo::sodium when a system libsodium is present. Each carries its include directory, and the
# three versioned ones carry their discovered version in the YOJIMBO_VERSION property.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/YojimboDependencies.cmake")

function(yojimbo_require_version name found minimum header)
    if(found VERSION_LESS minimum)
        message(FATAL_ERROR
            "yojimbo: the installed ${name} is ${found}, but yojimbo requires ${minimum} or later "
            "(from ${header}). See dependencies.manifest.")
    endif()
    message(STATUS "yojimbo: system ${name} ${found} (>= ${minimum})")
endfunction()

macro(yojimbo_find_system_deps manifest)

    yojimbo_read_dependency_manifest("${manifest}")

    find_path(YOJIMBO_SERIALIZE_INCLUDE_DIR serialize.h REQUIRED)
    find_path(YOJIMBO_NETCODE_INCLUDE_DIR netcode.h REQUIRED)
    find_library(YOJIMBO_NETCODE_LIBRARY netcode REQUIRED)
    find_path(YOJIMBO_RELIABLE_INCLUDE_DIR reliable.h REQUIRED)
    find_library(YOJIMBO_RELIABLE_LIBRARY reliable REQUIRED)

    # Needed at link time when the installed netcode is a static archive (a shared netcode
    # already carries its crypto). Optional: absent is fine when netcode is shared.
    find_library(YOJIMBO_SODIUM_LIBRARY sodium)

    yojimbo_read_header_version("${YOJIMBO_SERIALIZE_INCLUDE_DIR}/serialize.h" SERIALIZE YOJIMBO_SERIALIZE_VERSION)
    yojimbo_read_header_version("${YOJIMBO_NETCODE_INCLUDE_DIR}/netcode.h" NETCODE YOJIMBO_NETCODE_VERSION)
    yojimbo_read_header_version("${YOJIMBO_RELIABLE_INCLUDE_DIR}/reliable.h" RELIABLE YOJIMBO_RELIABLE_VERSION)

    yojimbo_require_version(serialize "${YOJIMBO_SERIALIZE_VERSION}" "${YOJIMBO_DEP_serialize_MINIMUM}"
                            "${YOJIMBO_SERIALIZE_INCLUDE_DIR}/serialize.h")
    yojimbo_require_version(netcode "${YOJIMBO_NETCODE_VERSION}" "${YOJIMBO_DEP_netcode_MINIMUM}"
                            "${YOJIMBO_NETCODE_INCLUDE_DIR}/netcode.h")
    yojimbo_require_version(reliable "${YOJIMBO_RELIABLE_VERSION}" "${YOJIMBO_DEP_reliable_MINIMUM}"
                            "${YOJIMBO_RELIABLE_INCLUDE_DIR}/reliable.h")

    if(NOT TARGET Yojimbo::serialize)
        add_library(Yojimbo::serialize INTERFACE IMPORTED)
        set_target_properties(Yojimbo::serialize PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${YOJIMBO_SERIALIZE_INCLUDE_DIR}"
            YOJIMBO_VERSION "${YOJIMBO_SERIALIZE_VERSION}")
    endif()

    if(NOT TARGET Yojimbo::netcode)
        add_library(Yojimbo::netcode UNKNOWN IMPORTED)
        set_target_properties(Yojimbo::netcode PROPERTIES
            IMPORTED_LOCATION "${YOJIMBO_NETCODE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${YOJIMBO_NETCODE_INCLUDE_DIR}"
            YOJIMBO_VERSION "${YOJIMBO_NETCODE_VERSION}")
        if(WIN32)
            # The system netcode needs these and a plain find_library result cannot carry them.
            set_property(TARGET Yojimbo::netcode APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES ws2_32 iphlpapi)
        endif()
    endif()

    if(NOT TARGET Yojimbo::reliable)
        add_library(Yojimbo::reliable UNKNOWN IMPORTED)
        set_target_properties(Yojimbo::reliable PROPERTIES
            IMPORTED_LOCATION "${YOJIMBO_RELIABLE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${YOJIMBO_RELIABLE_INCLUDE_DIR}"
            YOJIMBO_VERSION "${YOJIMBO_RELIABLE_VERSION}")
    endif()

    if(YOJIMBO_SODIUM_LIBRARY AND NOT TARGET Yojimbo::sodium)
        add_library(Yojimbo::sodium UNKNOWN IMPORTED)
        set_target_properties(Yojimbo::sodium PROPERTIES
            IMPORTED_LOCATION "${YOJIMBO_SODIUM_LIBRARY}")
        set_property(TARGET Yojimbo::netcode APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES Yojimbo::sodium)
    endif()

endmacro()
