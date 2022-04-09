message("****** mbedtlsConfig - START - ******")

############################################################
##

  # Compute the installation prefix relative to this file.
    get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
    get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
    get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
    if(_IMPORT_PREFIX STREQUAL "/")
      set(_IMPORT_PREFIX "")
    endif()
    MESSAGE("_IMPORT_PREFIX = ${_IMPORT_PREFIX}")
  
  ## External libraries and headers
  
  add_library( mbedtls STATIC IMPORTED )  
  target_include_directories( mbedtls INTERFACE "${_IMPORT_PREFIX}/include"  )
    
  add_library( mbedx509 STATIC IMPORTED )  
  target_include_directories( mbedx509 INTERFACE "${_IMPORT_PREFIX}/include"  )
  
  add_library( mbedcrypto STATIC IMPORTED )  
  target_include_directories( mbedcrypto INTERFACE "${_IMPORT_PREFIX}/include"  )
  
  ## External library binaries
  if(WIN32)
    if(${CMAKE_BUILD_TYPE}_ STREQUAL "Debug_")
        set_target_properties(mbedtls    PROPERTIES IMPORTED_LOCATION ${_IMPORT_PREFIX}/debug/lib/mbedtls.lib     )
        set_target_properties(mbedx509   PROPERTIES IMPORTED_LOCATION ${_IMPORT_PREFIX}/debug/lib/mbedx509.lib    )
        set_target_properties(mbedcrypto PROPERTIES IMPORTED_LOCATION ${_IMPORT_PREFIX}/debug/lib/mbedcrypto.lib  )
    else()
        set_target_properties(mbedtls    PROPERTIES IMPORTED_LOCATION ${_IMPORT_PREFIX}/lib/mbedtls.lib     )
        set_target_properties(mbedx509   PROPERTIES IMPORTED_LOCATION ${_IMPORT_PREFIX}/lib/mbedx509.lib    )
        set_target_properties(mbedcrypto PROPERTIES IMPORTED_LOCATION ${_IMPORT_PREFIX}/lib/mbedcrypto.lib  )
    endif()
  ELSEIF(UNIX)
    if(${CMAKE_BUILD_TYPE}_ STREQUAL "Debug_")
        set_target_properties(mbedtls    PROPERTIES IMPORTED_LOCATION ${_IMPORT_PREFIX}/debug/lib/libmbedtls.a    )
        set_target_properties(mbedx509   PROPERTIES IMPORTED_LOCATION ${_IMPORT_PREFIX}/debug/lib/libmbedx509.a   )
        set_target_properties(mbedcrypto PROPERTIES IMPORTED_LOCATION ${_IMPORT_PREFIX}/debug/lib/libmbedcrypto.a )
    else()
        set_target_properties(mbedtls    PROPERTIES IMPORTED_LOCATION ${_IMPORT_PREFIX}/lib/libmbedtls.a    )
        set_target_properties(mbedx509   PROPERTIES IMPORTED_LOCATION ${_IMPORT_PREFIX}/lib/libmbedx509.a   )
        set_target_properties(mbedcrypto PROPERTIES IMPORTED_LOCATION ${_IMPORT_PREFIX}/lib/libmbedcrypto.a )
    endif()
  ENDIF()
  
message("****** mbedtlsConfig - DONE - ******")