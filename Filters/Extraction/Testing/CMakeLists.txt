vtk_module_test_data(
  Data/cad_cubes.vtp
  Data/can.ex2)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/RectGrid2.vtk
    Data/blow.vtk
    Data/combq.bin
    Data/combxyz.bin)

  add_subdirectory(Python)
endif ()
