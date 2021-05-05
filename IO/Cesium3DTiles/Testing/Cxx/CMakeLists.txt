vtk_add_test_cxx(vtkIOCesium3DTilesCxxTests tests
  TestCesium3DTilesWriter.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkIOCesium3DTilesCxxTests tests)
