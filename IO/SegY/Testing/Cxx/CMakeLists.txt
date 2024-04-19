vtk_add_test_cxx(vtkIOSegYCxxTests tests
  TestSegY2DReader.cxx
  TestSegY2DReaderZoom.cxx
# TestSegY3DReader.cxx #19221
  )
vtk_test_cxx_executable(vtkIOSegYCxxTests tests)
