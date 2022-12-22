vtk_module_test_data(
  Data/ghost_cells.vtk
  Data/invalid_polydata.vtk)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
