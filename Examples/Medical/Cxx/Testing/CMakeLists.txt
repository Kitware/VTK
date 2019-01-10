set(_vtk_build_test Medical)
set(_vtk_build_TEST_OUTPUT_DATA_DIRECTORY
  "${CMAKE_CURRENT_SOURCE_DIR}")
set(_vtk_build_TEST_OUTPUT_DIRECTORY
  "${CMAKE_BINARY_DIR}/Testing/Temporary")
set_property(GLOBAL
  PROPERTY
    "_vtk_module_${_vtk_build_test}_test_depends"
      ${VTK_LIBRARIES})
add_subdirectory(Cxx)
