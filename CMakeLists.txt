cmake_minimum_required(VERSION 3.5)

project(yojimbo VERSION 1.0)
# Includedirs  
include_directories(
    .
    include
    serialize
    sodium
    netcode
    reliable
    tlsf
)
# Definitions
add_compile_definitions(
    # Debug
    $<$<CONFIG:Debug>:YOJIMBO_DEBUG>
    $<$<CONFIG:Debug>:NETCODE_DEBUG>
    $<$<CONFIG:Debug>:RELIABLE_DEBUG>
    # Release
    $<$<CONFIG:Release>:YOJIMBO_RELEASE>
    $<$<CONFIG:Release>:NETCODE_RELEASE>
    $<$<CONFIG:Release>:RELIABLE_RELEASE>
)
# Out directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/run)

# SODIUM
file(GLOB sodium_SRC
    IF(WIN32)
        "sodium/*.c"
        "sodium/*.h"
    ELSE()
        "sodium/*.S"
    ENDIF()
)
add_library(sodium STATIC ${sodium_SRC})

# NETCODE
add_library(netcode STATIC 
    netcode/netcode.c
    netcode/netcode.h
)
target_compile_definitions(netcode PRIVATE NETCODE_ENABLE_TESTS=1)

# RELIABLE
add_library(reliable STATIC 
    reliable/reliable.c
    reliable/reliable.h
)
target_compile_definitions(reliable PRIVATE RELIABLE_ENABLE_TESTS=1=1)

# TLSF
add_library(tlsf STATIC 
    tlsf/tlsf.c
    tlsf/tlsf.h
)

# YOJIMBO
file(GLOB yojimbo_SRC
    "include/*.h"
    "source/*.cpp"
)
add_library(yojimbo STATIC ${yojimbo_SRC})

# CLIENT
add_executable(client
    client.cpp
    shared.h
)
target_link_libraries(client
    yojimbo
    sodium
    tlsf
    netcode
    reliable
)

# SERVER
add_executable(server
    server.cpp
    shared.h
)
target_link_libraries(server
    yojimbo
    sodium
    tlsf
    netcode
    reliable
)

# LOOPBACK
add_executable(loopback
    loopback.cpp
    shared.h
)
target_link_libraries(loopback
    yojimbo
    sodium
    tlsf
    netcode
    reliable
)

# SOAK
add_executable(soak
    soak.cpp
    shared.h
)
target_link_libraries(soak
    yojimbo
    sodium
    tlsf
    netcode
    reliable
)

# TEST
add_executable(test
    test.cpp
    shared.h
)
target_link_libraries(test
    yojimbo
    sodium
    tlsf
    netcode
    reliable
)
target_compile_definitions(test PRIVATE SERIALIZE_ENABLE_TESTS=1)