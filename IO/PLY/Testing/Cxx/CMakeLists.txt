vtk_add_test_cxx(vtkIOPLYCxxTests tests
  TestPLYReader.cxx
  TestPLYReaderIntensity.cxx
  TestPLYReaderPointCloud.cxx
  TestPLYWriterAlpha.cxx
  TestPLYWriter.cxx,NO_VALID
  TestPLYWriterString.cxx,NO_VALID,NO_OUTPUT
  )
vtk_add_test_cxx(vtkIOPLYCxxTests tests
  TestPLYReaderTextureUVPoints,TestPLYReaderTextureUV.cxx squareTextured.ply
  vtk.png
)
vtk_add_test_cxx(vtkIOPLYCxxTests tests
  TestPLYReaderTextureUVFaces,TestPLYReaderTextureUV.cxx squareTexturedFaces.ply
  two_vtk_logos_stacked.png
)
vtk_test_cxx_executable(vtkIOPLYCxxTests tests)
