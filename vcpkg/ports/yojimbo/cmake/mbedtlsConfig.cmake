if(NOT DEFINED VCPKG_ROOT_DIR)
    message(FATAL_ERROR "VCPKG_ROOT_DIR is not defined. Please set it to the root folder of your vcpkg installation.")
endif()

############################################################
## External imports: mbedtls
############################################################
##
    ## TODO: 
    ##      -> 32 bit ???
    ##      -> Dynamic linking setup ???
  
    # External library paths
  if(WIN32)
    SET(VCPKG_MBEDTLS_BASE   "${VCPKG_ROOT_DIR}/packages/mbedtls_x64-windows-static" )
  elseif(UNIX)
    SET(VCPKG_MBEDTLS_BASE   "${VCPKG_ROOT_DIR}/installed/x64-linux")
  else()
    MESSAGE(FATAL "Unrecognized Operating System. I understand either WIN32 or UNIX to be set to true.")
  endif()
  
  ## External libraries and headers
  
  add_library( mbedtls STATIC IMPORTED )  
  target_include_directories( mbedtls INTERFACE "${VCPKG_MBEDTLS_BASE}/include"  )
    
  add_library( mbedx509 STATIC IMPORTED )  
  target_include_directories( mbedx509 INTERFACE "${VCPKG_MBEDTLS_BASE}/include"  )
  
  add_library( mbedcrypto STATIC IMPORTED )  
  target_include_directories( mbedcrypto INTERFACE "${VCPKG_MBEDTLS_BASE}/include"  )
  
  ## External library binaries
  if(WIN32)
    if(${CMAKE_BUILD_TYPE}_ STREQUAL "Debug_")
        set_target_properties(mbedtls    PROPERTIES IMPORTED_LOCATION ${VCPKG_MBEDTLS_BASE}/debug/lib/mbedtls.lib     )
        set_target_properties(mbedx509   PROPERTIES IMPORTED_LOCATION ${VCPKG_MBEDTLS_BASE}/debug/lib/mbedx509.lib    )
        set_target_properties(mbedcrypto PROPERTIES IMPORTED_LOCATION ${VCPKG_MBEDTLS_BASE}/debug/lib/mbedcrypto.lib  )
    else()
        set_target_properties(mbedtls    PROPERTIES IMPORTED_LOCATION ${VCPKG_MBEDTLS_BASE}/lib/mbedtls.lib     )
        set_target_properties(mbedx509   PROPERTIES IMPORTED_LOCATION ${VCPKG_MBEDTLS_BASE}/lib/mbedx509.lib    )
        set_target_properties(mbedcrypto PROPERTIES IMPORTED_LOCATION ${VCPKG_MBEDTLS_BASE}/lib/mbedcrypto.lib  )
    endif()
  ELSEIF(UNIX)
    if(${CMAKE_BUILD_TYPE}_ STREQUAL "Debug_")
        set_target_properties(mbedtls    PROPERTIES IMPORTED_LOCATION ${VCPKG_MBEDTLS_BASE}/debug/lib/libmbedtls.a    )
        set_target_properties(mbedx509   PROPERTIES IMPORTED_LOCATION ${VCPKG_MBEDTLS_BASE}/debug/lib/libmbedx509.a   )
        set_target_properties(mbedcrypto PROPERTIES IMPORTED_LOCATION ${VCPKG_MBEDTLS_BASE}/debug/lib/libmbedcrypto.a )
    else()
        set_target_properties(mbedtls    PROPERTIES IMPORTED_LOCATION ${VCPKG_MBEDTLS_BASE}/lib/libmbedtls.a    )
        set_target_properties(mbedx509   PROPERTIES IMPORTED_LOCATION ${VCPKG_MBEDTLS_BASE}/lib/libmbedx509.a   )
        set_target_properties(mbedcrypto PROPERTIES IMPORTED_LOCATION ${VCPKG_MBEDTLS_BASE}/lib/libmbedcrypto.a )
    endif()
  ENDIF()
  

