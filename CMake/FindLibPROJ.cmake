# Find LibPROJ library and header file
# Sets
#   LibPROJ_FOUND       to 0 or 1 depending on the result
#   LibPROJ_INCLUDE_DIR to directories required for using libproj4
#   LibPROJ_LIBRARIES   to libproj4 and any dependent libraries
# If LibPROJ_REQUIRED is defined, then a fatal error message will be generated if libproj4 is not found

if ( NOT LibPROJ_INCLUDE_DIR OR NOT LibPROJ_LIBRARIES OR NOT LibPROJ_FOUND )

  if ( $ENV{LibPROJ_DIR} )
    file( TO_CMAKE_PATH "$ENV{LibPROJ_DIR}" _LibPROJ_DIR )
  endif ()

  set(LibPROJ_LIBRARY_SEARCH_PATHS
    ${_LibPROJ_DIR}
    ${_LibPROJ_DIR}/lib64
    ${_LibPROJ_DIR}/lib
  )

  find_library( LibPROJ_LIBRARY_RELEASE
    NAMES proj
    HINTS
      ${LibPROJ_LIBRARY_SEARCH_PATHS}
  )

  find_library( LibPROJ_LIBRARY_DEBUG
    NAMES projd
    PATHS
      ${LibPROJ_LIBRARY_SEARCH_PATHS}
  )

  find_path( LibPROJ_INCLUDE_DIR
    NAMES proj_api.h
    HINTS
      ${_LibPROJ_DIR}
      ${_LibPROJ_DIR}/include
  )

  include(SelectLibraryConfigurations)
  select_library_configurations(LibPROJ)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibPROJ
                                    REQUIRED_VARS LibPROJ_LIBRARY LibPROJ_INCLUDE_DIR)

  if(LibPROJ_FOUND)
    set(LibPROJ_INCLUDE_DIRS ${LibPROJ_INCLUDE_DIR})

    if(NOT LibPROJ_LIBRARIES)
      set(LibPROJ_LIBRARIES ${LibPROJ_LIBRARY})
    endif()
  endif()
endif ()

mark_as_advanced(LibPROJ_INCLUDE_DIR)
