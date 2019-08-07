vtk_module_test_data(
  Data/clt.vtm
  Data/clt/,REGEX:.*)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/combq.bin
    Data/combxyz.bin
  )
  add_subdirectory(Python)
endif ()
