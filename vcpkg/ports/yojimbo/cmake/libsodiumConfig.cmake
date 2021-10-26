if(NOT DEFINED VCPKG_ROOT_DIR)
    message(FATAL_ERROR "VCPKG_ROOT_DIR is not defined. Please set it to the root folder of your vcpkg installation.")
endif()


############################################################
## External imports: libsodium
############################################################
  # External library paths
  if(WIN32)
    SET(VCPKG_LIBSODIUM_BASE "${VCPKG_ROOT_DIR}/packages/libsodium_x64-windows-static" )
  elseif(UNIX)
    SET(VCPKG_LIBSODIUM_BASE "${VCPKG_ROOT_DIR}/installed/x64-linux")
  else()
    MESSAGE(FATAL "Unrecognized Operating System. I understand either WIN32 or UNIX to be set to true.")
  endif()
  
  ## External libraries and headers
  add_library( sodium STATIC IMPORTED)
  target_include_directories( sodium INTERFACE "${VCPKG_LIBSODIUM_BASE}/include"  )
  
  ## External library binaries
  if(WIN32)
      if(${CMAKE_BUILD_TYPE}_ STREQUAL "Debug_")
        set_target_properties(sodium PROPERTIES IMPORTED_LOCATION ${VCPKG_LIBSODIUM_BASE}/debug/lib/libsodium.lib )
      else()
        set_target_properties(sodium PROPERTIES IMPORTED_LOCATION ${VCPKG_LIBSODIUM_BASE}/lib/libsodium.lib )
      endif()
  ELSEIF(UNIX)
    if(${CMAKE_BUILD_TYPE}_ STREQUAL "Debug_")
        set_target_properties(sodium PROPERTIES IMPORTED_LOCATION ${VCPKG_LIBSODIUM_BASE}/debug/lib/libsodium.a   )
    else()
        set_target_properties(sodium PROPERTIES IMPORTED_LOCATION ${VCPKG_LIBSODIUM_BASE}/lib/libsodium.a   )
    endif()
  ENDIF()
  

