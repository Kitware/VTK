project( alglib )

set(vtkalglib_THIRD_PARTY 1)
set(vtkalglib_LIBRARIES vtkalglib)
vtk_module_impl()

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

vtk_add_library( vtkalglib
  ${ALGLIB_SRCS}
)

#
# Install rules
#

# Grab install locations from VTK if we are building as part of VTK
if ( VTK_INSTALL_INCLUDE_DIR_CM24 )
  set( alglib_install_inc_dir ${VTK_INSTALL_INCLUDE_DIR_CM24} )
else ( VTK_INSTALL_INCLUDE_DIR_CM24 )
  set( alglib_install_inc_dir ${CMAKE_INSTALL_PREFIX}/include )
endif ( VTK_INSTALL_INCLUDE_DIR_CM24 )

# Header files:
if ( NOT VTK_INSTALL_NO_DEVELOPMENT )
  install( FILES
    ${alglib_SOURCE_DIR}/alglib/apvt.h
    ${alglib_SOURCE_DIR}/alglib/bdsvd.h
    ${alglib_SOURCE_DIR}/alglib/bidiagonal.h
    ${alglib_SOURCE_DIR}/alglib/blas.h
    ${alglib_SOURCE_DIR}/alglib/lq.h
    ${alglib_SOURCE_DIR}/alglib/qr.h
    ${alglib_SOURCE_DIR}/alglib/reflections.h
    ${alglib_SOURCE_DIR}/alglib/rotations.h
    ${alglib_SOURCE_DIR}/alglib/svd.h
    ${alglib_BINARY_DIR}/alglib/ap.h
    DESTINATION ${alglib_install_inc_dir}/alglib
    COMPONENT Development
  )
endif ( NOT VTK_INSTALL_NO_DEVELOPMENT )
