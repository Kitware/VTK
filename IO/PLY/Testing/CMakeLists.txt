vtk_module_test_data(
  Data/Armadillo.ply
  Data/PointCloud.ply
  Data/bunny.ply
  Data/squareTextured.ply
  Data/squareTexturedFaces.ply
  Data/vtk.png
  Data/two_vtk_logos_stacked.png)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
