cmake_minimum_required (VERSION 3.20.0)

project(yojimbo)

message(STATUS "Building for platform ${CMAKE_SYSTEM_NAME}")

# netcode.io
add_library(netcode-io STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/netcode.io/netcode.h
  ${CMAKE_CURRENT_SOURCE_DIR}/netcode.io/netcode.c
)
target_include_directories(netcode-io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/netcode.io)
target_link_libraries(netcode-io PUBLIC sodium)


# reliable.io
add_library(reliable-io STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/reliable.io/reliable.h
  ${CMAKE_CURRENT_SOURCE_DIR}/reliable.io/reliable.c
)
target_include_directories(reliable-io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/reliable.io)

# yojimbo
add_library(yojimbo STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/yojimbo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/yojimbo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tlsf/tlsf.h
  ${CMAKE_CURRENT_SOURCE_DIR}/tlsf/tlsf.c
)

# Add support for linux at some point
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	target_include_directories(yojimbo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/windows/")
	target_include_directories(reliable-io PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/windows/")
	target_include_directories(netcode-io PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/windows/")
endif()

target_include_directories(yojimbo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(yojimbo PUBLIC netcode-io reliable-io mbedtls mbedx509 mbedcrypto sodium)
