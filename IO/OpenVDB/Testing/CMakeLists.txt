vtk_module_test_data(
  Data/sphere_points.vdb)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
