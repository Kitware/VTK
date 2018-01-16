vtk_module_test_data(
  Data/SampleStructGrid.vtk
  Data/cow.vtp)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
