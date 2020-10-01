vtk_module_test_data(
  Data/ChangingBlocksOverTime/wavelet_0.vtm
  Data/ChangingBlocksOverTime/wavelet_1.vtm
  Data/ChangingBlocksOverTime/wavelet_0.vtu
  Data/ChangingBlocksOverTime/wavelet_1.vtu)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
