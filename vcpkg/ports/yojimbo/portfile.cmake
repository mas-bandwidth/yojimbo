vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

# make some functions available, mainly for logging / printing variables
# but also the mbedtls and libsodium config files which their ports are lacking.
# libsodium has some "unofficial" config - I decided to roll my own here. ~Yorlik

list(APPEND CMAKE_MODULE_PATH "${CURRENT_PORT_DIR}/cmake/")
message("CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
include(yojimbo_lib) # load functions

## Download stuff
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO networkprotocol/yojimbo
        REF v1.2.1
        SHA512 1c16331b5fa743c953e776c097e8a9e30c29c76c35632cff87f70294db7d7e04b0ce66ea5d1b7930ce746bd7f2d7203a5abf1c9918571f81fcb7453126b09080
    )

    vcpkg_from_github(
        OUT_SOURCE_PATH NETCODE_SOURCE_PATH
        REPO networkprotocol/netcode
        REF v1.2.1
        SHA512 ac03f83ccf548c0ebfc1dfae69248f301d52351d306cf34ef25e07c88d15e544b55cb4dc09c23db84f90e2aee83688d40591f3ce0ef92dc5dc95b53dab301539
    )

    vcpkg_from_github(
        OUT_SOURCE_PATH RELIABLE_SOURCE_PATH
        REPO networkprotocol/reliable
        REF v1.2
        SHA512 45c7d2734de565f1700942a178d062081c70cdb9d4dc2509e5db616ab23f1e4f7d3f5e214d42267f019e5c1867a304bb537b6023eb78574f7a6ebf1ff43174bb
    )

## move netcode dependencies
## todo: make vcpkg ports for these and just declare them as dependencies like mbedtls and libsodium

    if(WIN32)

        STRING(REPLACE "/" "\\" NETCODE_SOURCE_PATH  ${NETCODE_SOURCE_PATH}  )
        STRING(REPLACE "/" "\\" RELIABLE_SOURCE_PATH ${RELIABLE_SOURCE_PATH} )
        STRING(REPLACE "/" "\\" THESOURCE ${SOURCE_PATH} )
        execute_process(
            COMMAND cmd /C "xcopy ${NETCODE_SOURCE_PATH}  ${THESOURCE}\\netcode.io  /E /I /F /H /R /-Y /Q"
            COMMAND cmd /C "xcopy ${RELIABLE_SOURCE_PATH} ${THESOURCE}\\reliable.io /E /I /F /H /R /-Y /Q"
        )

    elseif(UNIX)
        message("Unix like OS. Copying files ...")
        vprint(NETCODE_SOURCE_PATH)
        vprint(RELIABLE_SOURCE_PATH)
        SET( THESOURCE ${SOURCE_PATH} )
        execute_process(
            COMMAND cp -v -r ${NETCODE_SOURCE_PATH}/.  -t ${THESOURCE}/netcode.io/
            COMMAND cp -v -r ${RELIABLE_SOURCE_PATH}/. -t ${THESOURCE}/reliable.io/
        )
        message("Unix like OS. Copying files ... DONE!")

    else()
        message(FATAL_ERROR "Unrecognized OS.")
    endif()

## Install main CMakeLists.txt into yojimbo source tree
    file(INSTALL ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})

## vcpkg feature Check
    set(BUILD_YOJIMBO_TESTS 0)
    if("tests" IN_LIST FEATURES)
        set(BUILD_YOJIMBO_TESTS 1)
    endif()

## Configure cmake for build phase
    vcpkg_configure_cmake(
        SOURCE_PATH ${SOURCE_PATH}
        GENERATOR "Ninja"
        OPTIONS
            -DSOURCE_PATH=${SOURCE_PATH}
            -DVCPKG_ROOT_DIR=${VCPKG_ROOT_DIR}
            -Dlibsodium_DIR=${CURRENT_PORT_DIR}/cmake
            -Dmbedtls_DIR=${CURRENT_PORT_DIR}/cmake
            -DCURRENT_PORT_DIR=${CURRENT_PORT_DIR}
            -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT_DIR}/scripts/buildsystems/vcpkg.cmake
    )

## On windows install debug info
    vcpkg_copy_pdbs()

## Install artifacts
    vcpkg_install_cmake()

## Install usage and copyright
    file(INSTALL ${CMAKE_CURRENT_LIST_DIR}/usage DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT})
    file(INSTALL ${SOURCE_PATH}/LICENCE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)

## Finalize
    vcpkg_fixup_cmake_targets()
