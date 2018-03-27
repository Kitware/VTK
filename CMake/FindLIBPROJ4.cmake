# Find LIBPROJ4 library and header file
# Sets
#   LIBPROJ4_FOUND       to 0 or 1 depending on the result
#   LIBPROJ4_INCLUDE_DIR to directories required for using libproj4
#   LIBPROJ4_LIBRARIES   to libproj4 and any dependent libraries
# If LIBPROJ4_REQUIRED is defined, then a fatal error message will be generated if libproj4 is not found

if ( NOT LIBPROJ4_INCLUDE_DIR OR NOT LIBPROJ4_LIBRARIES OR NOT LIBPROJ4_FOUND )

  if ( $ENV{LIBPROJ4_DIR} )
    file( TO_CMAKE_PATH "$ENV{LIBPROJ4_DIR}" _LIBPROJ4_DIR )
  endif ()

  set(LIBPROJ4_LIBRARY_SEARCH_PATHS
    ${_LIBPROJ4_DIR}
    ${_LIBPROJ4_DIR}/lib64
    ${_LIBPROJ4_DIR}/lib
  )

  find_library( LIBPROJ4_LIBRARY_RELEASE
    NAMES proj
    HINTS
      ${LIBPROJ4_LIBRARY_SEARCH_PATHS}
  )

  find_library( LIBPROJ4_LIBRARY_DEBUG
    NAMES projd
    PATHS
      ${LIBPROJ4_LIBRARY_SEARCH_PATHS}
  )

  find_path( LIBPROJ4_INCLUDE_DIR
    NAMES proj_api.h
    HINTS
      ${_LIBPROJ4_DIR}
      ${_LIBPROJ4_DIR}/include
  )

  include(SelectLibraryConfigurations)
  select_library_configurations(LIBPROJ4)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LIBPROJ4
                                    REQUIRED_VARS LIBPROJ4_LIBRARY LIBPROJ4_INCLUDE_DIR)

  if(LIBPROJ4_FOUND)
    set(LIBPROJ4_INCLUDE_DIRS ${LIBPROJ4_INCLUDE_DIR})

    if(NOT LIBPROJ4_LIBRARIES)
      set(LIBPROJ4_LIBRARIES ${LIBPROJ4_LIBRARY})
    endif()
  endif()
endif ()

mark_as_advanced(LIBPROJ4_INCLUDE_DIR)
