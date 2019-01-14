vtk_module_test_data(
  Data/SainteHelens.dem
  Data/vtk.png
  Data/vehicle_data.csv)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
