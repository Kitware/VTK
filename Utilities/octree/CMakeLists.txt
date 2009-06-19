project( ndtree )

  cmake_minimum_required( VERSION 2.6 )

  set( LIBRARY_OUTPUT_PATH    ${ndtree_BINARY_DIR}/bin CACHE "" INTERNAL FORCE )
  set( EXECUTABLE_OUTPUT_PATH ${ndtree_BINARY_DIR}/bin CACHE "" INTERNAL FORCE )
  mark_as_advanced( LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH )

  find_package( VTK REQUIRED )
  include( ${VTK_USE_FILE} )

  include_directories(
    ${ndtree_SOURCE_DIR}
  )

  enable_testing()

  add_executable( Octree
    test_octree.cxx
  )

  find_package( Doxygen )
  configure_file(
    ${ndtree_SOURCE_DIR}/doxyfile.in
    ${ndtree_BINARY_DIR}/doxyfile
    @ONLY
  )
  add_custom_target( docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${ndtree_BINARY_DIR}/doxyfile
    DEPENDS ${ndtree_BINARY_DIR}/doxyfile
  )

