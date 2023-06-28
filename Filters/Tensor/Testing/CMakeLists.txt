vtk_module_test_data(
  Data/MinimalTensors.vtu)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()
