# Find POSTGRESQL library and header file
# Sets
#   POSTGRES_FOUND               to 0 or 1 depending on the result
#   POSTGRES_INCLUDE_DIRECTORIES to directories required for using libpq
#   POSTGRES_LIBRARIES           to libpq and any dependent libraries
# If POSTGRES_REQUIRED is defined, then a fatal error message will be generated if libpq is not found

if ( NOT POSTGRES_INCLUDE_DIRECTORIES OR NOT POSTGRES_LIBRARIES OR NOT POSTGRES_FOUND )

  if ( $ENV{POSTGRES_DIR} )
    file( TO_CMAKE_PATH "$ENV{POSTGRES_DIR}" _POSTGRES_DIR )
  endif ( $ENV{POSTGRES_DIR} )

  find_library( POSTGRES_LIBRARIES
    NAMES pq libpq
    PATHS
      ${_POSTGRES_DIR}/lib64
      ${CMAKE_INSTALL_PREFIX}/lib64
      /usr/local/pgsql/lib64
      /usr/local/lib64
      /usr/lib64
      ${_POSTGRES_DIR}
      ${_POSTGRES_DIR}/lib
      ${CMAKE_INSTALL_PREFIX}/bin
      ${CMAKE_INSTALL_PREFIX}/lib
      /usr/local/pgsql/lib
      /usr/local/lib
      /usr/lib
    NO_DEFAULT_PATH
  )

  find_path( POSTGRES_INCLUDE_DIRECTORIES
    NAMES libpq-fe.h
    PATHS
      ${_POSTGRES_DIR}
      ${_POSTGRES_DIR}/include
      ${CMAKE_INSTALL_PREFIX}/include
      /usr/local/pgsql/include
      /usr/local/include
      /usr/include
      /usr/include/postgresql
    NO_DEFAULT_PATH
  )

  if ( NOT POSTGRES_INCLUDE_DIRECTORIES OR NOT POSTGRES_LIBRARIES ) 
    if ( POSTGRES_REQUIRED )
      message( FATAL_ERROR "POSTGRES is required. Set POSTGRES_DIR" )
    endif ( POSTGRES_REQUIRED )
  else ( NOT POSTGRES_INCLUDE_DIRECTORIES OR NOT POSTGRES_LIBRARIES ) 
    set( POSTGRES_FOUND 1 )
    mark_as_advanced( POSTGRES_FOUND )
  endif ( NOT POSTGRES_INCLUDE_DIRECTORIES OR NOT POSTGRES_LIBRARIES )

endif ( NOT POSTGRES_INCLUDE_DIRECTORIES OR NOT POSTGRES_LIBRARIES OR NOT POSTGRES_FOUND )

mark_as_advanced( FORCE POSTGRES_INCLUDE_DIRECTORIES )
mark_as_advanced( FORCE POSTGRES_LIBRARIES )
