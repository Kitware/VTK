vtk_module_test_data(
  Data/SainteHelens.dem
  Data/vtk.png
  Data/vehicle_data.csv)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
