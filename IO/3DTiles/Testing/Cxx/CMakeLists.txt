vtk_add_test_cxx(vtkIO3DTilesCxxTests tests
  Test3DTilesWriter.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkIO3DTilesCxxTests tests)
