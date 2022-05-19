vtk_module_test_data(
  Data/AMR/htg3d.htg)
add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
