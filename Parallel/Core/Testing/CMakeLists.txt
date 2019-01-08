vtk_module_test_data(
  Data/RectGrid2.vtk
  Data/blow.vtk
  Data/combq.bin
  Data/combxyz.bin
  Data/earth.ppm
  Data/teapot.g
  )

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
