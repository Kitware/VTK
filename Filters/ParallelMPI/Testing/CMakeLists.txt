vtk_module_test_data(
  Data/voronoiMesh.vtu)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
