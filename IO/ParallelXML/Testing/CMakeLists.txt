vtk_module_test_data(
  Data/ChangingBlocksOverTime/wavelet_0.vtm
  Data/ChangingBlocksOverTime/wavelet_1.vtm
  Data/ChangingBlocksOverTime/wavelet_0.vtu
  Data/ChangingBlocksOverTime/wavelet_1.vtu
  Data/AMR/amrug.vthb
  Data/AMR/amrug/amrug_0.vti
  Data/AMR/amrug/amrug_1.vti
  Data/AMR/amrug/amrug_2.vti)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
