vtk_module_test_data(
  Data/Armadillo.ply
  Data/FaceTexturedWithReusedTCoords.ply
  Data/PointCloud.ply
  Data/bunny.ply
  Data/squareTextured.ply
  Data/squareTexturedFaces.ply
  Data/vtk.png
  Data/two_vtk_logos_stacked.png)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
