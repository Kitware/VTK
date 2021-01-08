vtk_module_test_data(
  Data/mbwavelet_ascii.q
  Data/mbwavelet_ascii.xyz
)

message(STATUS "${_vtk_build_TEST_INPUT_DATA_DIRECTORY}")

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/beach.jpg
    Data/earth.ppm)

  add_subdirectory(Python)
endif ()
