vtk_module_test_data(
  Data/ghost_cells.vtk
  Data/invalid_polydata.vtk)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
