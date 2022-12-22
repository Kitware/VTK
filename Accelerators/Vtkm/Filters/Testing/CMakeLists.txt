vtk_module_test_data(
  Data/SampleStructGrid.vtk
  Data/cow.vtp)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
