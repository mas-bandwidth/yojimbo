cmake_minimum_required(VERSION 3.16)


MESSAGE("*********************************************************************")
MESSAGE("******  Yojimbo -- START --")

list(APPEND CMAKE_MODULE_PATH "${CURRENT_PORT_DIR}/cmake/")
include("${CURRENT_PORT_DIR}/cmake/yojimbo_lib.cmake")

find_package(libsodium REQUIRED)
find_package(mbedtls   REQUIRED)

#############################################################
## Project Definition
#############################################################
project(Yojimbo C CXX)

############################################################
## Yojimbo Static Library
############################################################
##
  add_library(yojimbo STATIC
      ${SOURCE_PATH}/yojimbo.cpp
      ${SOURCE_PATH}/tlsf/tlsf.c
      ${SOURCE_PATH}/netcode.io/netcode.c
      ${SOURCE_PATH}/reliable.io/reliable.c  )

  target_link_libraries( yojimbo PRIVATE sodium mbedtls mbedx509 mbedcrypto )

  target_compile_definitions( yojimbo PUBLIC NETCODE_ENABLE_TESTS=0 RELIABLE_ENABLE_TESTS=0 )      
  
  target_include_directories( yojimbo 
    PRIVATE 
        ${SOURCE_PATH} 
        ${SOURCE_PATH}/netcode.io 
        ${SOURCE_PATH}/reliable.io 
   )

  ############################################################
  ## Yojimbo Static TEST Library and Test App
  ############################################################
  ##
 ##if(${YOJIMBO_TESTS}) 
    ## library
    add_library(yojimbo_test STATIC
      ${SOURCE_PATH}/yojimbo.cpp
      ${SOURCE_PATH}/tlsf/tlsf.c
      ${SOURCE_PATH}/netcode.io/netcode.c
      ${SOURCE_PATH}/reliable.io/reliable.c
    )
    target_link_libraries( yojimbo_test sodium mbedtls mbedx509 mbedcrypto )    
    target_compile_definitions( yojimbo_test PUBLIC NETCODE_ENABLE_TESTS=1 RELIABLE_ENABLE_TESTS=1 )    
    target_include_directories( yojimbo_test 
      INTERFACE
          $<BUILD_INTERFACE:${SOURCE_PATH}> 
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include> 
      PRIVATE 
          "${SOURCE_PATH}/netcode.io" 
          "${SOURCE_PATH}/reliable.io" 
    )
    
    target_include_directories( yojimbo PRIVATE "${VCPKG_LIBSODIUM_BASE}/include"  )
    target_include_directories( yojimbo PRIVATE "${VCPKG_MBEDTLS_BASE}/include"  )
            
    ## executable
    add_executable(testapp ${SOURCE_PATH}/test.cpp )
    target_link_libraries(testapp yojimbo_test)    
    ##
  ##endif()  
    
  ############################################################
  ## Client App Target
  ############################################################
  ##
      add_executable(client ${SOURCE_PATH}/client.cpp )
      target_link_libraries( client yojimbo )
    
  ############################################################
  ## Server App Target
  ############################################################
  ##
      add_executable(server ${SOURCE_PATH}/server.cpp )
      target_link_libraries( server yojimbo )
    
  ############################################################
  ## Secure Client App Target
  ############################################################
  ##
      add_executable(secure_client ${SOURCE_PATH}/secure_client.cpp )
      target_link_libraries( secure_client yojimbo )
    
  ############################################################
  ## Secure Server App Target
  ############################################################
  ##
      add_executable(secure_server ${SOURCE_PATH}/secure_server.cpp )
      target_link_libraries( secure_server yojimbo )
    
  ############################################################
  ## Client Server App Target
  ############################################################
  ##
      add_executable(client_server ${SOURCE_PATH}/client_server.cpp )
      target_link_libraries( client_server yojimbo )
    
  ############################################################
  ## Loopback App Target
  ############################################################
  ##
      add_executable(loopback ${SOURCE_PATH}/loopback.cpp )
      target_link_libraries( loopback yojimbo )
    
  ############################################################
  ## Soak App Target
  ############################################################
  ##
      add_executable(soak ${SOURCE_PATH}/soak.cpp )
      target_link_libraries( soak yojimbo )
    
############################################################
## Install
############################################################
##
  # the ugly hack
  file(INSTALL ${CURRENT_PORT_DIR}/cmake/libsodiumConfig.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/share/yojimbo)
  file(INSTALL ${CURRENT_PORT_DIR}/cmake/mbedtlsConfig.cmake   DESTINATION ${CMAKE_INSTALL_PREFIX}/share/yojimbo)

  #if(YOJIMBO_TESTS)
      SET(TEST_INSTALL testapp)
  #endif()
  
  ## yojimbo + EXPORT
  install(
      TARGETS
          yojimbo
      EXPORT yojimbo_target
      #RUNTIME  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
      LIBRARY  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
      ARCHIVE  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib     
      INCLUDES DESTINATION ${CMAKE_INSTALL_PREFIX}/include    )
  
  ## install cmake files for find_package
  install(EXPORT yojimbo_target
      DESTINATION ${CMAKE_INSTALL_PREFIX}/share/yojimbo
      NAMESPACE yojimbo::
      FILE yojimboConfig.cmake
      EXPORT_LINK_INTERFACE_LIBRARIES
      )

  if( ${CMAKE_BUILD_TYPE}_ STREQUAL "Debug_" )
    vprint(CMAKE_BUILD_TYPE)
    message("Skipping header installation.")
  else()
    vprint(CMAKE_BUILD_TYPE)
    message("Installing headers")
    install(  FILES   ${SOURCE_PATH}/yojimbo.h 
                      ${SOURCE_PATH}/shared.h 
              DESTINATION 
                      ${CMAKE_INSTALL_PREFIX}/include/yojimbo
    )
  endif()
  
  ## The rest
  install(
      TARGETS
          ${TEST_INSTALL}
          client
          server
          secure_client
          secure_server
          client_server
          loopback
          soak
      RUNTIME  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/yojimbo
      LIBRARY  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
      ARCHIVE  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib     
      INCLUDES DESTINATION ${CMAKE_INSTALL_PREFIX}/include    )

 
  


#### DONE ####



MESSAGE(STATUS "******  Yojimbo -- DONE --")
MESSAGE("*********************************************************************")