vtk_add_test_cxx(vtkIOPLYCxxTests tests
  TestPLYReader.cxx
  TestPLYReaderIntensity.cxx
  TestPLYReaderPointCloud.cxx
  TestPLYWriterAlpha.cxx
  TestPLYWriter.cxx,NO_VALID
  )
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestPLYReaderTextureUVPoints,TestPLYReaderTextureUV.cxx squareTextured.ply
)
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestPLYReaderTextureUVFaces,TestPLYReaderTextureUV.cxx squareTexturedFaces.ply
)
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
