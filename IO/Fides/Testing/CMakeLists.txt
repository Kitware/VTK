if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/vtk-uns-grid-2.json)
  vtk_module_test_data(
    Data/tris-blocks-time.bp)
  vtk_module_test_data(
    Data/tris-blocks-time.bp.dir/,REGEX:.*)
  add_subdirectory(Python)
endif ()
