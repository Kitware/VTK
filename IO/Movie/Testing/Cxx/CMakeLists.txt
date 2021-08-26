vtk_add_test_cxx(vtkIOMovieCxxTests tests
  NO_VALID
  TestAVIWriter.cxx
  TestMP4Writer.cxx
  )

vtk_test_cxx_executable(vtkIOMovieCxxTests tests)
