vtk_module_test_data(
  Data/combq.bin
  Data/combxyz.bin)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
