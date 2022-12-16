vtk_module_test_data(
  Data/combq.bin
  Data/combxyz.bin
  Data/teapot.g)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/bore.vtk
    Data/track1.binary.vtk
    Data/track2.binary.vtk
    Data/track3.binary.vtk)

  add_subdirectory(Python)
endif ()
