vtk_add_test_cxx(vtkIOCesium3DTilesCxxTests tests
  TestCesium3DTilesWriter.cxx
  TestCesium3DTilesReader.cxx
  TestCesiumB3DMReader.cxx
  )

vtk_test_cxx_executable(vtkIOCesium3DTilesCxxTests tests)
