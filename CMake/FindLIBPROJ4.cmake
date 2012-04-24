# Find LIBPROJ4 library and header file
# Sets
#   LIBPROJ4_FOUND       to 0 or 1 depending on the result
#   LIBPROJ4_INCLUDE_DIR to directories required for using libproj4
#   LIBPROJ4_LIBRARIES   to libproj4 and any dependent libraries
# If LIBPROJ4_REQUIRED is defined, then a fatal error message will be generated if libproj4 is not found

if ( NOT LIBPROJ4_INCLUDE_DIR OR NOT LIBPROJ4_LIBRARIES OR NOT LIBPROJ4_FOUND )

  if ( $ENV{LIBPROJ4_DIR} )
    file( TO_CMAKE_PATH "$ENV{LIBPROJ4_DIR}" _LIBPROJ4_DIR )
  endif ( $ENV{LIBPROJ4_DIR} )

  find_library( LIBPROJ4_LIBRARIES
    NAMES proj4 libproj4
    PATHS
      ${_LIBPROJ4_DIR}/lib64
      ${CMAKE_INSTALL_PREFIX}/lib64
      /usr/local/lib64
      /usr/lib64
      ${_LIBPROJ4_DIR}
      ${_LIBPROJ4_DIR}/lib
      ${CMAKE_INSTALL_PREFIX}/bin
      ${CMAKE_INSTALL_PREFIX}/lib
      /usr/local/lib
      /usr/lib
    NO_DEFAULT_PATH
  )

  find_path( LIBPROJ4_INCLUDE_DIR
    NAMES lib_proj.h
    PATHS
      ${_LIBPROJ4_DIR}
      ${_LIBPROJ4_DIR}/include
      ${CMAKE_INSTALL_PREFIX}/include
      /usr/local/pgsql/include
      /usr/local/include
      /usr/include
      /usr/include/postgresql
    NO_DEFAULT_PATH
  )

  if ( NOT LIBPROJ4_INCLUDE_DIR OR NOT LIBPROJ4_LIBRARIES )
    if ( LIBPROJ4_REQUIRED )
      message( FATAL_ERROR "LIBPROJ4 is required. Set LIBPROJ4_DIR" )
    endif ( LIBPROJ4_REQUIRED )
  else ( NOT LIBPROJ4_INCLUDE_DIR OR NOT LIBPROJ4_LIBRARIES )
    set( LIBPROJ4_FOUND 1 )
    mark_as_advanced( LIBPROJ4_FOUND )
  endif ( NOT LIBPROJ4_INCLUDE_DIR OR NOT LIBPROJ4_LIBRARIES )

endif ( NOT LIBPROJ4_INCLUDE_DIR OR NOT LIBPROJ4_LIBRARIES OR NOT LIBPROJ4_FOUND )

mark_as_advanced( FORCE LIBPROJ4_INCLUDE_DIR )
mark_as_advanced( FORCE LIBPROJ4_LIBRARIES )
