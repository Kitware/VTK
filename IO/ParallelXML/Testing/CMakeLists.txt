vtk_module_test_data(
  Data/ChangingBlocksOverTime/wavelet_0.vtm
  Data/ChangingBlocksOverTime/wavelet_1.vtm
  Data/ChangingBlocksOverTime/wavelet_0.vtu
  Data/ChangingBlocksOverTime/wavelet_1.vtu)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
