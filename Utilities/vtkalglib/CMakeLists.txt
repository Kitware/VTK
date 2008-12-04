project( alglib )

cmake_minimum_required( VERSION 2.6 )
if ( NOT LIBRARY_OUTPUT_PATH )
  set( LIBRARY_OUTPUT_PATH ${alglib_BINARY_DIR}/bin CACHE INTERNAL "" FORCE )
endif ( NOT LIBRARY_OUTPUT_PATH )
if ( NOT EXECUTABLE_OUTPUT_PATH )
  set( EXECUTABLE_OUTPUT_PATH ${alglib_BINARY_DIR}/bin CACHE INTERNAL "" FORCE )
endif ( NOT EXECUTABLE_OUTPUT_PATH )

set( ALGLIB_SRCS
  ap.cpp
  bdsvd.cpp
  bidiagonal.cpp
  blas.cpp
  lq.cpp
  qr.cpp
  reflections.cpp
  rotations.cpp
  svd.cpp
)

set( ALGLIB_SHARED_LIB ${BUILD_SHARED_LIBS} CACHE INTERNAL "" FORCE )
configure_file(
  ${alglib_SOURCE_DIR}/alglib/ap.h.in
  ${alglib_BINARY_DIR}/alglib/ap.h
  @ONLY
)

include_directories(
  ${alglib_SOURCE_DIR}
  ${alglib_BINARY_DIR}
)

add_library( vtkalglib
  ${ALGLIB_SRCS}
)

